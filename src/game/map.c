#include "penance.h"
#include "shared_symbols.h"

/* Globals.
 */
 
#define STUMP_LIMIT 8
 
static const uint8_t tileprops_default[256]={0};

static uint8_t tilesheet_storage[256*TILESHEET_LIMIT];
 
static struct {
  const uint8_t *tilesheetv[TILESHEET_LIMIT]; // sparse, indexed by rid
  struct map *mapv[LONG_LIMIT*LAT_LIMIT]; // sparse
  int x,y,w,h; // World extent in (mapv).
  struct stump {
    int x,y; // Map position in world. We don't care about the stump's position in map.
  } stumpv[STUMP_LIMIT];
  int stumpc;
  struct rom_res *spritev;
  int spritec,spritea;
} maps={0};

/* Add resource, during reset.
 */
 
static int maps_add_resource(const uint8_t *src,int srcc,int rid) {

  struct rom_map rmap={0};
  if (rom_map_decode(&rmap,src,srcc)<0) {
    fprintf(stderr,"map:%d failed to decode\n",rid);
    return -1;
  }
  if ((rmap.w!=COLC)||(rmap.h!=ROWC)) {
    fprintf(stderr,"map:%d has size %d,%d but must be %d,%d\n",rid,rmap.w,rmap.h,COLC,ROWC);
    return -1;
  }

  struct map *map=calloc(1,sizeof(struct map));
  if (!map) return -1;
  map->rid=rid;
  map->serial=src+8; // strip the signature and dimensions; pre-egg-maps code assumed a slightly different format.
  map->serialc=srcc-8;
  map->tileprops=tileprops_default;
  memcpy(map->v,rmap.v,COLC*ROWC);
  
  // Read commands. Only thing we actually care about at this time is 0x22 location.
  int x=255,y=255;
  struct rom_command_reader reader={.v=map->serial+COLC*ROWC,.c=map->serialc-COLC*ROWC};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case 0x22: { // location
          x=cmd.argv[0];
          y=cmd.argv[1];
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
  
  // Extend global bounds if needed.
  if (maps.w) {
    if (x<maps.x) {
      maps.w+=maps.x-x;
      maps.x=x;
    } else if (x>=maps.x+maps.w) {
      maps.w=x+1-maps.x;
    }
    if (y<maps.y) {
      maps.h+=maps.y-y;
      maps.y=y;
    } else if (y>=maps.y+maps.h) {
      maps.h=y+1-maps.y;
    }
  } else {
    maps.x=x;
    maps.y=y;
    maps.w=1;
    maps.h=1;
  }
  
  // Search for stumps and note if we get one.
  // The stump is 0x2e,0x2f regardless of tilesheet.
  if (maps.stumpc<STUMP_LIMIT) {
    const uint8_t *p=map->v;
    int i=COLC*ROWC;
    for (;i-->0;p++) {
      if (*p==0x2e) {
        struct stump *stump=maps.stumpv+maps.stumpc++;
        stump->x=x;
        stump->y=y;
        map->stump=1;
        break;
      }
    }
  }
  
  return 0;
}

/* Link map.
 * In particular, ensure it has a valid tilesheet.
 */
 
static int map_link(struct map *map) {
  struct rom_command_reader reader={.v=map->serial+COLC*ROWC,.c=map->serialc-COLC*ROWC};
  struct rom_command command;
  while (rom_command_reader_next(&command,&reader)>0) {
    switch (command.opcode) {
      case 0x20: { // image (parallel to tilesheet)
          int tsid=(command.argv[0]<<8)|command.argv[1];
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
          if ((res->rid<0)||(res->rid>=TILESHEET_LIMIT)) {
            fprintf(stderr,"Invalid tilesheet id %d\n",res->rid);
            return -2;
          }
          uint8_t *storage=tilesheet_storage+256*res->rid;
          struct rom_tilesheet_reader tsr;
          if (rom_tilesheet_reader_init(&tsr,res->v,res->c)<0) return -1;
          struct rom_tilesheet_entry ts;
          while (rom_tilesheet_reader_next(&ts,&tsr)>0) {
            if (ts.tableid!=NS_tilesheet_physics) continue;
            memcpy(storage+ts.tileid,ts.v,ts.c);
          }
          maps.tilesheetv[res->rid]=storage;
        } break;
      case EGG_TID_sprite: {
          if (maps.spritec>=maps.spritea) {
            int na=maps.spritea+32;
            if (na>INT_MAX/sizeof(struct rom_res)) return -1;
            void *nv=realloc(maps.spritev,sizeof(struct rom_res)*na);
            if (!nv) return -1;
            maps.spritev=nv;
            maps.spritea=na;
          }
          maps.spritev[maps.spritec++]=*res;
        } break;
    }
  }
  
  if (!maps.w) {
    fprintf(stderr,"No maps!\n");
    return -2;
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

/* Fetch resources from store.
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

int maps_get_sprite(void *dstpp,int rid) {
  int lo=0,hi=maps.spritec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_res *q=maps.spritev+ck;
         if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else {
      if (dstpp) *(const void**)dstpp=q->v;
      return q->c;
    }
  }
  return 0;
}

/* Misc state.
 */
 
void maps_get_world_bounds(int *x,int *y,int *w,int *h) {
  *x=maps.x;
  *y=maps.y;
  *w=maps.w;
  *h=maps.h;
}

int maps_get_stump(int *x,int *y,int p) {
  if (p<0) return 0;
  if (p>=maps.stumpc) return 0;
  const struct stump *stump=maps.stumpv+p;
  *x=stump->x;
  *y=stump->y;
  return 1;
}
