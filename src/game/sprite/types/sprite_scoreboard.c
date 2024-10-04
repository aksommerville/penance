#include "game/penance.h"

struct sprite_scoreboard {
  struct sprite hdr;
  double animphase; // 0..1
};

#define SPRITE ((struct sprite_scoreboard*)sprite)

static void _scoreboard_update(struct sprite *sprite,double elapsed) {
  // Currently exactly 1 hz. Divide (elapsed) by desired rate if you don't like 1.
  if ((SPRITE->animphase+=elapsed)>=1.0) SPRITE->animphase-=1.0;
}

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

static uint8_t scoreboard_moon_tile(uint8_t spellusage) {
  if (spellusage==0xff) return 0x7d; // FULL!
  if (spellusage==0x00) return 0x7a; // New Moon. Not actually possible; you have to cast Open or Disembody to see the scoreboard.
  int popc=0;
  for (;spellusage;spellusage>>=1) if (spellusage&1) popc++;
  if (popc>=4) return 0x7c; // Substantially full.
  return 0x7b; // Narrow crescent.
}

static int scoreboard_is_full_clear() {
  if (!g.bonus) return 0;
  if (!g.jammio) return 0;
  if (g.spellusage!=0xff) return 0;
  int i=CANDLE_COUNT; while (i-->0) if (!g.candlev[i]) return 0;
  return 1;
}

static void _scoreboard_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  graf_flush(&g.graf);
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  
  /* Play time and best time.
   */
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx-8;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy+2;
  struct egg_draw_tile vtxv[18];
  scoreboard_repr(vtxv,g.playtime,dstx,dsty);
  scoreboard_repr(vtxv+9,g.besttime,dstx,dsty+12);
  egg_draw_globals(0xe0c080ff,0xff);
  egg_draw_tile(1,texid,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  egg_draw_globals(0,0xff);
  
  /* Badges blow the time section.
   * Five tiles, aligned with six grid cells: Bone, Jammio, Spell Usage, (blank), 100% (*2)
   */
  dstx=(int16_t)((sprite->x-2.0)*TILESIZE)+addx;
  dsty=(int16_t)((sprite->y+2.0)*TILESIZE)+addy;
  int vtxc=0;
  uint8_t qmark=(SPRITE->animphase>=0.500)?0x77:0x76;
  #define VTX(tile) { \
    struct egg_draw_tile *vtx=vtxv+vtxc++; \
    vtx->dstx=dstx; \
    vtx->dsty=dsty; \
    vtx->tileid=tile; \
    vtx->xform=0; \
    dstx+=TILESIZE; \
  }
  VTX(g.bonus?0x78:qmark) // Bone
  VTX(g.jammio?0x79:qmark) // Jammio
  VTX(scoreboard_moon_tile(g.spellusage)) // Moon: 0x7a,0x7b,0x7c,0x7d
  dstx+=TILESIZE;
  if ((SPRITE->animphase<0.750)&&scoreboard_is_full_clear()) {
    VTX(0x7e) // "100%"
    VTX(0x7f) // ''
  }
  #undef VTX
  egg_draw_tile(1,texid,vtxv,vtxc);
}

const struct sprite_type sprite_type_scoreboard={
  .name="scoreboard",
  .objlen=sizeof(struct sprite_scoreboard),
  .update=_scoreboard_update,
  .render=_scoreboard_render,
};
