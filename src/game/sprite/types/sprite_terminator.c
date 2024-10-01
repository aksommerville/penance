/* sprite_terminator.c
 * Triangle on the floor that glows when all the candles are lit, and when the hero steps on it the game ends.
 */
 
#include "game/penance.h"

#define TERMINATOR_PERIOD 2.0

struct sprite_terminator {
  struct sprite hdr;
  int active;
  int complete;
  double animclock;
  int16_t dstx,dsty;
  int16_t srcx,srcy;
};

#define SPRITE ((struct sprite_terminator*)sprite)

static int _terminator_init(struct sprite *sprite,const uint8_t *def,int defc) {
  sprite->layer=-1;
  SPRITE->active=1;
  const uint8_t *p=g.candlev;
  int i=CANDLE_COUNT;
  for (;i-->0;p++) if (!*p) { SPRITE->active=0; break; }
  if (!SPRITE->active) return 0;
  int col=(int)sprite->x; // Bottom point of triangle, 3x2 tiles total.
  int row=(int)sprite->y; // And our image is 2x2, with (tileid) the top left.
  SPRITE->dstx=col*TILESIZE-(TILESIZE>>1);
  SPRITE->dsty=(row-1)*TILESIZE;
  SPRITE->srcx=(sprite->tileid&0x0f)*TILESIZE;
  SPRITE->srcy=(sprite->tileid>>4)*TILESIZE;
  sprite->y-=0.5; // My position only matters for comparing to hero. Put it dead center in the triangle.
  return 0;
}

static void _terminator_update(struct sprite *sprite,double elapsed) {
  if (!SPRITE->active) return;
  if (SPRITE->complete) return;
  if ((SPRITE->animclock+=elapsed)>=TERMINATOR_PERIOD) {
    SPRITE->animclock-=TERMINATOR_PERIOD;
  }
  if (!hero_is_human()) return;
  if (GRP(HERO)->spritec<1) return;
  struct sprite *hero=GRP(HERO)->spritev[0];
  const double radius=1.0;
  double dx=hero->x-sprite->x; if ((dx<-radius)||(dx>radius)) return;
  double dy=hero->y-sprite->y; if ((dy<-radius)||(dy>radius)) return;
  if (dx*dx+dy*dy>radius*radius) return;
  SPRITE->complete=1;
  hero->x=sprite->x;
  hero->y=sprite->y-(5.0/TILESIZE);
  penance_gameover();
}

static void _terminator_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  if (!SPRITE->active) return;
  int a=0xff;
  if (!SPRITE->complete) {
    const double halfperiod=TERMINATOR_PERIOD*0.5;
    double v=halfperiod-SPRITE->animclock;
    if (v<0.0) v=-v;
    v*=0.5;
    a=(int)(v*255.0);
    if (a<0) a=0; else if (a>0xff) a=0xff;
  }
  graf_set_tint(&g.graf,0xffff00ff);
  graf_set_alpha(&g.graf,a);
  graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,sprite->imageid),SPRITE->dstx,SPRITE->dsty,SPRITE->srcx,SPRITE->srcy,TILESIZE<<1,TILESIZE<<1,0);
  graf_set_tint(&g.graf,0);
  graf_set_alpha(&g.graf,0xff);
}

const struct sprite_type sprite_type_terminator={
  .name="terminator",
  .objlen=sizeof(struct sprite_terminator),
  .init=_terminator_init,
  .update=_terminator_update,
  .render=_terminator_render,
};
