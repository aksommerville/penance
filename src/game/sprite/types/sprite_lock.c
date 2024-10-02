#include "game/penance.h"

struct sprite_lock {
  struct sprite hdr;
  uint8_t lamps; // 1,2,4
};

#define SPRITE ((struct sprite_lock*)sprite)

static int _lock_init(struct sprite *sprite,const uint8_t *def,int defc) {
  int x=(int)sprite->x,y=(int)sprite->y;
  if ((x<0)||(y<0)||(x>=COLC)||(y>=ROWC)) return -1;
  if (!g.map->tileprops[g.map->v[y*COLC+x]]) return -1; // Tile under me is vacant -- we're unlocked already.
  return 0;
}

static void _lock_update(struct sprite *sprite,double elapsed) {
}

static void _lock_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid,0);
  if (SPRITE->lamps&1) graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid+1,0);
  if (SPRITE->lamps&2) graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid+2,0);
  if (SPRITE->lamps&4) graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid+3,0);
}

const struct sprite_type sprite_type_lock={
  .name="lock",
  .objlen=sizeof(struct sprite_lock),
  .init=_lock_init,
  .update=_lock_update,
  .render=_lock_render,
};

void sprite_lock_set_lamp(struct sprite *sprite,int index,int value) {
  if (!sprite||(sprite->type!=&sprite_type_lock)) return;
  if ((index<0)||(index>7)) return;
  if (value) SPRITE->lamps|=1<<index;
  else SPRITE->lamps&=~(1<<index);
  if (SPRITE->lamps==0x07) {
    int x=(int)sprite->x,y=(int)sprite->y;
    if ((x>=0)&&(y>=0)&&(x<COLC)&&(y<ROWC)) {
      // Clearing the map cell manages the physics of unlocking, and also the persistence.
      g.map->v[y*COLC+x]=0;
    }
    //TODO Sound effect
    //TODO Fireworks?
    sprite_kill_later(sprite);
  }
}
