#include "game/penance.h"

struct sprite_scoreboard {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_scoreboard*)sprite)

static void scoreboard_repr(struct egg_draw_tile *vtxv/*9*/,double time,int16_t dstx,int16_t dsty) {
  if (time<0.0) time=0.0;
  int mil=(int)(time*1000.0);
  int sec=mil/1000; mil%=1000;
  int min=sec/60; sec%=60;
  if (min>99) min=sec=99;
  vtxv[0].tileid=0xa5+min/10;
  vtxv[1].tileid=0xa5+min%10;
  vtxv[2].tileid=0xaf;
  vtxv[3].tileid=0xa5+sec/10;
  vtxv[4].tileid=0xa5+sec%10;
  vtxv[5].tileid=0xaf; // There isn't a dot, so use two colons.
  vtxv[6].tileid=0xa5+mil/100;
  vtxv[7].tileid=0xa5+(mil/10)%10;
  vtxv[8].tileid=0xa5+mil%10;
  struct egg_draw_tile *vtx=vtxv;
  int i=9; for (;i-->0;vtx++,dstx+=7) {
    vtx->dstx=dstx;
    vtx->dsty=dsty;
    vtx->xform=0;
  }
}

static void _scoreboard_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  graf_flush(&g.graf);
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx-8;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy+2;
  struct egg_draw_tile vtxv[18];
  scoreboard_repr(vtxv,g.playtime,dstx,dsty);
  scoreboard_repr(vtxv+9,g.besttime,dstx,dsty+12);
  egg_draw_globals(0xe0c080ff,0xff);
  egg_draw_tile(1,texcache_get_image(&g.texcache,sprite->imageid),vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  egg_draw_globals(0,0xff);
}

const struct sprite_type sprite_type_scoreboard={
  .name="scoreboard",
  .objlen=sizeof(struct sprite_scoreboard),
  .render=_scoreboard_render,
};
