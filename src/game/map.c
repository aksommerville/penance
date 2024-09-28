#include "penance.h"

/* Globals.
 */
 
static const uint8_t tileprops_default[256]={0};
 
static struct {
  const uint8_t *tilesheetv[TILESHEET_LIMIT]; // sparse, indexed by rid
  struct map *mapv[LONG_LIMIT*LAT_LIMIT]; // sparse
} maps={0};

/* Add resource, during reset.
 */
 
static int maps_add_resource(const uint8_t *src,int srcc,int rid) {
  if (srcc<COLC*ROWC) {
    fprintf(stderr,"map:%d: Invalid size %d, must be at least %d*%d==%d\n",rid,srcc,COLC,ROWC,COLC*ROWC);
    return -1;
  }
  struct map *map=calloc(1,sizeof(struct map));
  if (!map) return -1;
  map->rid=rid;
  map->serial=src;
  map->serialc=srcc;
  map->tileprops=tileprops_default;
  memcpy(map->v,src,COLC*ROWC);
  
  struct map_command_reader reader;
  map_command_reader_init_map(&reader,map);
  const uint8_t *arg;
  int opcode,argc;
  int x=255,y=255;
  while ((argc=map_command_reader_next(&arg,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x22: { // location
          x=arg[0];
          y=arg[1];
        } break;
    }
  }
  if ((x>=LONG_LIMIT)||(y>=LAT_LIMIT)) {
    fprintf(stderr,"Invalid location %d,%d for map:%d\n",x,y,rid);
    return -2;
  } else if (maps.mapv[y*LONG_LIMIT+x]) {
    fprintf(stderr,"Map location %d,%d claimed by both map:%d and map:%d\n",x,y,maps.mapv[y*LONG_LIMIT+x]->rid,rid);
    return -2;
  }
  map->x=x;
  map->y=y;
  maps.mapv[y*LONG_LIMIT+x]=map;
  return 0;
}

/* Link map.
 * In particular, ensure it has a valid tilesheet.
 */
 
static int map_link(struct map *map) {
  struct map_command_reader reader;
  map_command_reader_init_map(&reader,map);
  const uint8_t *arg;
  int opcode,argc;
  while ((argc=map_command_reader_next(&arg,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x20: { // image (parallel to tilesheet)
          int tsid=(arg[0]<<8)|arg[1];
          if ((tsid<TILESHEET_LIMIT)&&maps.tilesheetv[tsid]) map->tileprops=maps.tilesheetv[tsid];
          else map->tileprops=tileprops_default;
        } break;
    }
  }
  return 0;
}

/* Reset.
 */
 
int maps_reset(const void *rom,int romc) {
  memset(&maps,0,sizeof(maps));
  
  // Collect map and tilesheet resources.
  struct rom_reader reader={0};
  if (rom_reader_init(&reader,rom,romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    switch (res->tid) {
      case EGG_TID_map: if (maps_add_resource(res->v,res->c,res->rid)<0) return -1; break;
      case EGG_TID_tilesheet: {
          if ((res->rid>=0)&&(res->rid<TILESHEET_LIMIT)&&(res->c>=256)) {
            maps.tilesheetv[res->rid]=res->v;
          } else {
            fprintf(stderr,"Invalid tilesheet:%d, c=%d\n",res->rid,res->c);
            return -2;
          }
        } break;
    }
  }
  
  // Link and validate.
  struct map **p=maps.mapv;
  int i=LONG_LIMIT*LAT_LIMIT;
  for (;i-->0;p++) {
    if (!*p) continue;
    int err=map_link(*p);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"map:%d: Unspecified error linking.\n",(*p)->rid);
      return -2;
    }
  }

  return 0;
}

/* Fetch map from store.
 */
 
struct map *map_by_id(int rid) {
  // This is inefficient, potentially checking over 1000 slots for it.
  // But I'm pretty sure this only has to happen once, at startup.
  struct map **p=maps.mapv;
  int i=LONG_LIMIT*LAT_LIMIT;
  for (;i-->0;p++) {
    if (!*p) continue;
    if ((*p)->rid==rid) return *p;
  }
  return 0;
}

struct map *map_by_location(int x,int y) {
  if ((x<0)||(y<0)||(x>=LONG_LIMIT)||(y>=LAT_LIMIT)) return 0;
  return maps.mapv[y*LONG_LIMIT+x];
}

/* Command reader.
 */
 
void map_command_reader_init_serial(struct map_command_reader *reader,const void *src,int srcc) {
  reader->v=src;
  reader->c=srcc;
  reader->p=0;
}

void map_command_reader_init_res(struct map_command_reader *reader,const void *src,int srcc) {
  if (srcc>=COLC*ROWC) {
    reader->v=(uint8_t*)src+COLC*ROWC;
    reader->c=srcc-COLC*ROWC;
  } else {
    reader->c=0;
  }
  reader->p=0;
}

void map_command_reader_init_map(struct map_command_reader *reader,const struct map *map) {
  if (map) {
    reader->v=map->serial+COLC*ROWC;
    reader->c=map->serialc-COLC*ROWC;
  } else {
    reader->c=0;
  }
  reader->p=0;
}

int map_command_reader_next(void *dstpp,int *opcode,struct map_command_reader *reader) {
  if (reader->p>=reader->c) return -1;
  *opcode=reader->v[reader->p++];
  if (!*opcode) { reader->c=0; return -1; } // 0 is explicit EOF.
  int dstc=0;
  switch ((*opcode)&0xe0) {
    case 0x00: dstc=0; break;
    case 0x20: dstc=2; break;
    case 0x40: dstc=4; break;
    case 0x60: dstc=6; break;
    case 0x80: dstc=8; break;
    case 0xa0: dstc=12; break;
    case 0xc0: dstc=16; break;
    case 0xe0: {
        if (*opcode>=0xf0) return -1;
        dstc=reader->v[reader->p++];
      } break;
  }
  if (reader->p>reader->c-dstc) { reader->c=0; return -1; }
  *(const void**)dstpp=reader->v+reader->p;
  reader->p+=dstc;
  return dstc;
}
