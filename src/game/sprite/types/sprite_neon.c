#include "game/penance.h"

struct sprite_neon {
  struct sprite hdr;
  double clock;
};

#define SPRITE ((struct sprite_neon*)sprite)

static void _neon_update(struct sprite *sprite,double elapsed) {
  SPRITE->clock+=elapsed;
}

static void _neon_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  // The sign is 3x2 tiles, and we should be placed at its top-left tile. Mind that (sprite->x,y) is the middle of the tile.
  int16_t dstx=(int)(sprite->x*TILESIZE)-(TILESIZE>>1)+addx;
  int16_t dsty=(int)(sprite->y*TILESIZE)-(TILESIZE>>1)+addy;
  
  // Quantize time to s/6
  int tp=(int)(SPRITE->clock*6.0);
  
  // Top lightbulbs begin at +15,2, each is 2x2, 6 of them spaced 5 pixels horizontally.
  int topv[6]={0,0,0,0,0,0};
  int t=tp%30;
  if (t<2) {
  } else if (t<4) {
    memset(topv,0xff,sizeof(topv));
  } else if (t<6) {
  } else {
    int lp=5-t%6;
    topv[lp]=1;
  }
  int16_t x=dstx+15,i=6;
  const int *p=topv;
  for (;i-->0;p++,x+=5) if (*p) graf_draw_rect(&g.graf,x,dsty+2,2,2,0xffff00ff);
  
  // Bottom lightbulbs begin at +19,16, each is 2x1, 5 of them spaced 5 pixels horizontally.
  int btmv[5]={0,0,0,0,0};
  t=tp%23;
  if (t<3) {
  } else if (t<5) {
    memset(btmv,0xff,sizeof(btmv));
  } else {
    int tp=4-t%5;
    btmv[tp]=1;
  }
  for (x=dstx+19,i=5,p=btmv;i-->0;p++,x+=5) if (*p) graf_draw_rect(&g.graf,x,dsty+16,2,1,0x00ff00ff);
}

const struct sprite_type sprite_type_neon={
  .name="neon",
  .objlen=sizeof(struct sprite_neon),
  .update=_neon_update,
  .render=_neon_render,
};
