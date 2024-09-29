#include "opt/fs/fs.h"
#include "opt/serial/serial.h"
#include "egg_rom_toc.h"
#include <egg/egg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

// Must agree with editor and runtime.
#define COLC 20
#define ROWC 11

/* Resource TOC from prepared header.
 * We're lazy, we load it the first time we need it.
 */
 
static const char *tocpath=0;
static int toc_status=0;
static struct toc_entry { // unsorted
  int tid,rid;
  char *name;
  int namec;
} *tocv=0;
static int tocc=0,toca=0;

static int tid_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>=0)&&(v<=0xff)) return v;
  // We don't need to read the TOC file. It's built for this use case, we can include it at compile time.
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return EGG_TID_##tag;
  EGG_TID_FOR_EACH
  EGG_TID_FOR_EACH_CUSTOM
  #undef _
  return -1;
}

static int toc_add(int tid,int rid,const char *name,int namec) {
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (tocc>=toca) {
    int na=toca+256;
    if (na>INT_MAX/sizeof(struct toc_entry)) return -1;
    void *nv=realloc(tocv,sizeof(struct toc_entry)*na);
    if (!nv) return -1;
    tocv=nv;
    toca=na;
  }
  struct toc_entry *entry=tocv+tocc++;
  if (!(entry->name=malloc(namec+1))) return -1;
  memcpy(entry->name,name,namec);
  entry->name[namec]=0;
  entry->namec=namec;
  entry->tid=tid;
  entry->rid=rid;
  return 0;
}

static int toc_require() {
  if (toc_status) return toc_status;
  if (!tocpath) return toc_status=-1;
  // #define RID_image_font9_0020 1
  char *src=0;
  int srcc=file_read(&src,tocpath);
  if (srcc<0) {
    fprintf(stderr,"%s:WARNING: Failed to read TOC file\n",tocpath);
    return toc_status=-1;
  }
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec;
  while ((linec=sr_decode_line(&line,&decoder))>0) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if ((linec<7)||memcmp(line,"#define",7)) continue;
    int linep=7;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *macro=line+linep;
    int macroc=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) macroc++;
    if ((macroc<4)||memcmp(macro,"RID_",4)) continue;
    macro+=4; // Now it's "TYPE_NAME"
    macroc-=4;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *value=line+linep;
    int valuec=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) valuec++;
    int rid;
    if ((sr_int_eval(&rid,value,valuec)<2)||(rid<0)||(rid>0xffff)) continue;
    const char *tname=macro;
    int tnamec=0;
    while ((tnamec<macroc)&&(macro[tnamec]!='_')) tnamec++;
    if (tnamec>=macroc) continue; // Missing underscore.
    int tid=tid_eval(tname,tnamec);
    if ((tid>0)&&(tid<=0xff)) {
      const char *name=macro+tnamec+1;
      int namec=macroc-tnamec-1;
      toc_add(tid,rid,name,namec);
    }
  }
  free(src);
  return toc_status=1;
}

static int rid_eval(int tid,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int rid;
  if ((sr_int_eval(&rid,src,srcc)>=2)&&(rid>=0)&&(rid<=0xffff)) return rid;
  if (toc_require()<0) return -1;
  const struct toc_entry *entry=tocv;
  int i=tocc;
  for (;i-->0;entry++) {
    if (entry->tid!=tid) continue;
    if (entry->namec!=srcc) continue;
    if (memcmp(entry->name,src,srcc)) continue;
    return entry->rid;
  }
  return -1;
}

/* Compile map: bits.
 */
 
static int map_opcode_eval(const char *kw,int kwc) {
  // Canonical list is at src/editor/js/CommandModal.js
  if ((kwc==5)&&!memcmp(kw,"image",5)) return 0x20;
  if ((kwc==4)&&!memcmp(kw,"hero",4)) return 0x21;
  if ((kwc==8)&&!memcmp(kw,"location",8)) return 0x22;
  if ((kwc==4)&&!memcmp(kw,"song",4)) return 0x23;
  return -1;
}

// 0xe0..0xef read from the next byte, and 0xf0..0xff are reserved. We return -1 in both cases.
static int map_cmdlen(uint8_t opcode) {
  switch (opcode&0xe0) {
    case 0x00: return 0;
    case 0x20: return 2;
    case 0x40: return 4;
    case 0x60: return 6;
    case 0x80: return 8;
    case 0xa0: return 12;
    case 0xc0: return 16;
  }
  return -1;
}

static int map_tid_for_arg(uint8_t opcode,int position) {
  switch (opcode) {
    case 0x20: switch (position) { // image
        case 0: return EGG_TID_image;
      } break;
    case 0x23: switch (position) { // song
        case 0: return EGG_TID_song;
      } break;
  }
  return -1;
}

/* Compile single map argument.
 */
 
static int map_arg_compile(struct sr_encoder *dst,uint8_t opcode,int position,const char *src,int srcc,const char *srcpath,int lineno) {
  
  /* Plain integers, optional explicit length.
   */
  int explicitlen=0;
       if ((srcc>=3)&&!memcmp(src+srcc-3,"u16",3)) { srcc-=3; explicitlen=2; }
  else if ((srcc>=3)&&!memcmp(src+srcc-3,"u24",3)) { srcc-=3; explicitlen=3; }
  else if ((srcc>=3)&&!memcmp(src+srcc-3,"u32",3)) { srcc-=3; explicitlen=4; }
  int v;
  if (sr_int_eval(&v,src,srcc)>=2) {
    if (!explicitlen) explicitlen=1;
    return sr_encode_intbe(dst,v,explicitlen);
  }
  if (explicitlen) srcc+=3; // Expect failure, but some non-integer thing ending eg "u16" isn't necessarily an error.
  
  /* "@X,Y" and "@X,Y,W,H" produce 2 and 4 bytes respectively.
   * We'll do it generically and actually accept any count.
   */
  if ((srcc>=1)&&(src[0]=='@')) {
    int srcp=1;
    while (srcp<srcc) {
      const char *token=src+srcp;
      int tokenc=0;
      while ((srcp<srcc)&&(src[srcp++]!=',')) tokenc++;
      if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xff)) {
        fprintf(stderr,"%s:%d: Expected 0..255 for '@' token, found '%.*s'. Full arg '%.*s'\n",srcpath,lineno,tokenc,token,srcc,src);
        return -2;
      }
      if (sr_encode_u8(dst,v)<0) return -1;
    }
    return 0;
  }
  
  /* Quoted strings evaluate and emit verbatim.
   */
  if ((srcc>=2)&&(src[0]=='"')&&(src[srcc-1]=='"')) {
    for (;;) {
      int err=sr_string_eval((char*)dst->v+dst->c,dst->a-dst->c,src,srcc);
      if (err<0) {
        fprintf(stderr,"%s:%d: Failed to evaluate string literal.\n",srcpath,lineno);
        return -2;
      }
      if (dst->c<=dst->a-err) {
        dst->c+=err;
        return 0;
      }
      if (sr_encoder_require(dst,err)<0) return -1;
    }
  }
  
  /* "TYPE:NAME" for a 2-byte resource id.
   */
  int i=0;
  for (;i<srcc;i++) {
    if (src[i]==':') {
      int tid=tid_eval(src,i);
      if ((tid<0)||(tid>0xff)) break;
      int rid=rid_eval(tid,src+i+1,srcc-i-1);
      if ((rid<0)||(rid>0xffff)) break;
      return sr_encode_intbe(dst,rid,2);
    }
  }
  
  /* Finally, if there is a known resource type for this opcode+position, try that.
   */
  int tid=map_tid_for_arg(opcode,position);
  if (tid>0) {
    int rid=rid_eval(tid,src,srcc);
    if ((rid>=0)&&(rid<=0xffff)) {
      return sr_encode_intbe(dst,rid,2);
    }
  }
  
  /* Additional rules from Arrautza not implemented yet:
- `FLD_*` as defined in arrautza.h.
- `item:NAME` emits item ID in one byte.
  */
  
  fprintf(stderr,"%s:%d: Failed to evaluate argument '%.*s' to command 0x%02x\n",srcpath,lineno,srcc,src,opcode);
  return -2;
}

/* Compile map.
 */
 
static int digest_map(struct sr_encoder *dst,const char *src,int srcc,const char *dstpath,const char *srcpath) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec,lineno=1,y=0;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    int i=0; for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) linec--;
    if (!linec) continue;
    
    // Starts with hex dump image.
    if (y<ROWC) {
      if (linec!=COLC<<1) {
        fprintf(stderr,"%s:%d: Expected %d bytes for row %d of map image, found %d.\n",srcpath,lineno,COLC<<1,y,linec);
        return -2;
      }
      for (i=0;i<linec;i+=2) {
        int hi=sr_digit_eval(line[i]);
        int lo=sr_digit_eval(line[i+1]);
        if ((hi<0)||(hi>0xf)||(lo<0)||(lo>0xf)) {
          fprintf(stderr,"%s:%d: Illegal hex byte '%.2s' in map image\n",srcpath,lineno,line+i);
          return -2;
        }
        if (sr_encode_u8(dst,(hi<<4)|lo)<0) return -1;
      }
      y++;
      continue;
    }
    
    // Followed by loose commands.
    i=0;
    const char *kw=line+i;
    int kwc=0;
    while ((i<linec)&&((unsigned char)line[i++]>0x20)) kwc++;
    while ((i<linec)&&((unsigned char)line[i]<=0x20)) i++;
    int opcode=map_opcode_eval(kw,kwc);
    if (opcode<0) {
      fprintf(stderr,"%s:%d: Unknown map command '%.*s'\n",srcpath,lineno,kwc,kw);
      return -2;
    }
    if (sr_encode_u8(dst,opcode)<0) return -1;
    int lenp=-1;
    if ((opcode>=0xe0)&&(opcode<0xf0)) { // explicit length; emit placeholder.
      lenp=dst->c;
      if (sr_encode_u8(dst,0)<0) return -1;
    }
    int argp=dst->c,position=0;
    while (i<linec) {
      const char *token=line+i;
      int tokenc=0;
      while ((i<linec)&&((unsigned char)line[i++]>0x20)) tokenc++;
      while ((i<linec)&&((unsigned char)line[i]<=0x20)) i++;
      int err=map_arg_compile(dst,opcode,position++,token,tokenc,srcpath,lineno);
      if (err<0) return err;
    }
    int cmdlen=dst->c-argp;
    if ((opcode>=0xe0)&&(opcode<0xf0)) {
      if (cmdlen>0xff) {
        fprintf(stderr,"%s:%d: Invalid command length %d\n",srcpath,lineno,cmdlen);
        return -2;
      }
      ((uint8_t*)dst->v)[lenp]=cmdlen;
    } else {
      int expect=map_cmdlen(opcode);
      if (cmdlen!=expect) {
        fprintf(stderr,"%s:%d: Map command 0x%02x (%.*s) expects %d bytes parameter, found %d\n",srcpath,lineno,opcode,kwc,kw,expect,cmdlen);
        return -2;
      }
    }
  }
  return 0;
}

/* Compile tilesheet.
 */
 
static int digest_tilesheet(struct sr_encoder *dst,const char *src,int srcc,const char *dstpath,const char *srcpath) {
  /* Each line is either blank, 32 hex digits, or the name of the next sheet.
   * Hex dumps should be 16 lines long but we'll truncate and pad as needed.
   * We only emit the "physics" sheet, and we emit exactly 256 bytes.
   */
  uint8_t tmp[256]={0};
  int tmpc=0;
  int copying=0;
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec,lineno=1;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    if (linec==32) { // We're not going to allow 32-digit table names.
      if (!copying) continue;
      int linep=0;
      for (;linep<32;linep+=2) {
        int hi=sr_digit_eval(line[linep]);
        int lo=sr_digit_eval(line[linep+1]);
        if ((hi<0)||(hi>0xf)||(lo<0)||(lo>0xf)) {
          fprintf(stderr,"%s:%d: Invalid hex byte '%.2s' in tilesheet image\n",srcpath,lineno,line+linep);
          return -2;
        }
        if (tmpc<256) tmp[tmpc++]=(hi<<4)|lo;
      }
    } else {
      if ((linec==7)&&!memcmp(src,"physics",7)) {
        copying=1;
      } else if (copying) break;
    }
  }
  return sr_encode_raw(dst,tmp,sizeof(tmp));
}

/* Process file in memory, dispatch.
 */
 
static int digest(struct sr_encoder *dst,const char *src,int srcc,const char *dstpath,const char *srcpath) {
  // Determine what kind of thing we're doing, by examining components of (dstpath).
  int p=0;
  while (dstpath[p]) {
    if (dstpath[p]=='/') { p++; continue; }
    const char *name=dstpath+p;
    int namec=0;
    while (dstpath[p]&&(dstpath[p++]!='/')) namec++;
    
    if ((namec==3)&&!memcmp(name,"map",3)) return digest_map(dst,src,srcc,dstpath,srcpath);
    if ((namec==9)&&!memcmp(name,"tilesheet",9)) return digest_tilesheet(dst,src,srcc,dstpath,srcpath);
    
  }
  fprintf(stderr,"%s: Unable to determine operation based on dstpath='%s'\n",srcpath,dstpath);
  return -2;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  const char *exename="tool";
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) exename=argv[0];
  const char *dstpath=0;
  const char *srcpath=0;
  int argi=1; while (argi<argc) {
    const char *arg=argv[argi++];
    if ((arg[0]=='-')&&(arg[1]=='o')) {
      if (dstpath) {
        fprintf(stderr,"%s: Multiple output paths\n",exename);
        return 1;
      }
      dstpath=arg+2;
    } else if (!memcmp(arg,"--toc=",6)) {
      if (tocpath) {
        fprintf(stderr,"%s: Multiple TOC paths\n",exename);
        return 1;
      }
      tocpath=arg+6;
    } else if (arg[0]=='-') {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",exename,arg);
      return 1;
    } else if (srcpath) {
      fprintf(stderr,"%s: Multiple input paths\n",exename);
      return 1;
    } else {
      srcpath=arg;
    }
  }
  if (!dstpath||!srcpath) {
    fprintf(stderr,"Usage: %s -oOUTPUT INPUT [--toc=PATH]\n",exename);
    return 1;
  }
  
  char *src=0;
  int srcc=file_read(&src,srcpath);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file\n",srcpath);
    return 1;
  }
  
  struct sr_encoder dst={0};
  int err=digest(&dst,src,srcc,dstpath,srcpath);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error processing %d-byte file for output to '%s'\n",srcpath,srcc,dstpath);
    return 1;
  }
  
  if (file_write(dstpath,dst.v,dst.c)<0) {
    fprintf(stderr,"%s: Failed to write file, %d bytes\n",dstpath,dst.c);
    return 1;
  }
  return 0;
}
