#include "penance.h"

static void *rom=0;
static int romc=0;
static struct graf graf={0};
static struct font *font=0;
static int fbw=0,fbh=0;
static int texid_tiles=0;
static int texid_hero=0;

//XXX very temp
static uint8_t map[COLC*ROWC];

void egg_client_quit(int status) {
  // Flush save state, log performance... Usually noop.
  // Full cleanup really isn't necessary, but if you like:
  if (rom) free(rom);
  font_del(font);
  fprintf(stderr,"%s\n",__func__);
}

int egg_client_init() {
  fprintf(stderr,"%s:%d:%s: My game is starting up...\n",__FILE__,__LINE__,__func__);
  
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=COLC*TILESIZE)||(fbh!=ROWC*TILESIZE)) {
    fprintf(stderr,"Invalid framebuffer size (%d,%d) for (%d,%d) cells of %dx%d pixels.\n",fbw,fbh,COLC,ROWC,TILESIZE,TILESIZE);
    return -1;
  }
  
  if ((romc=egg_get_rom(0,0))<=0) return -1;
  if (!(rom=malloc(romc))) return -1;
  if (egg_get_rom(rom,romc)!=romc) return -1;
  strings_set_rom(rom,romc);
  
  if (!(font=font_new())) return -1;
  if (font_add_image_resource(font,0x0020,RID_image_witchy_0020)<0) return -1;
  //if (font_add_image_resource(font,0x00a1,RID_image_font9_00a1)<0) return -1;
  //if (font_add_image_resource(font,0x0400,RID_image_font9_0400)<0) return -1;
  // Also supplied by default: font9_0020, font6_0020, cursive_0020, witchy_0020
  if (egg_texture_load_image(texid_tiles=egg_texture_new(),RID_image_outdoors)<0) return -1;
  if (egg_texture_load_image(texid_hero=egg_texture_new(),RID_image_hero)<0) return -1;
  
  if (maps_reset(rom,romc)<0) return -1;
  
  srand_auto();
  
  //XXX TEMP
  uint8_t *p=map;
  int i=COLC*ROWC;
  for (;i-->0;p++) {
    *p=(rand()&31);
    if ((*p==0x10)||(*p==0x11)||(*p==0x16)||(*p==0x17)) *p=0x00;
    else if (*p>=0x1c) (*p)=0x22+(*p)-0x1c;
    else if (*p>=0x18) (*p)=0x32+(*p)-0x18;
  }
  p=map+6*COLC+5;
  p[ 0]=0x16;
  p[ 1]=0x17;
  p[ 2]=0x17;
  p[ 3]=0x17;
  p[ 4]=0x18;
  p[20]=0x26;
  p[21]=0x27;
  p[22]=0x27;
  p[23]=0x27;
  p[24]=0x28;
  p[40]=0x36;
  p[41]=0x37;
  p[42]=0x37;
  p[43]=0x37;
  p[44]=0x38;
  
  return 0;
}

void egg_client_update(double elapsed) {
  // TODO
}

void egg_client_render() {
  graf_reset(&graf);
  egg_draw_clear(1,0x201008ff);
  
  {
    struct egg_draw_tile vtxv[COLC*ROWC];
    struct egg_draw_tile *vtx=vtxv;
    const uint8_t *srcp=map;
    int i=COLC*ROWC;
    int16_t x=8,y=8;
    for (;i-->0;vtx++,srcp++) {
      vtx->dstx=x;
      vtx->dsty=y;
      vtx->tileid=*srcp;
      vtx->xform=0;
      if ((x+=16)>=320) {
        x=8;
        y+=16;
      }
    }
    graf_flush(&graf);
    egg_draw_tile(1,texid_tiles,vtxv,COLC*ROWC);
  }
  {
    struct egg_draw_tile vtxv[]={
      {40,40,0x00,0},
    };
    egg_draw_tile(1,texid_hero,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  graf_flush(&graf);
}
