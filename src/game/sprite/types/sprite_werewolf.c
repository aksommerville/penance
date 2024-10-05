#include "game/penance.h"

struct sprite_werewolf {
  struct sprite hdr;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_werewolf*)sprite)

void _werewolf_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.333;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
    // Check for rescue only at animation ticks, just so we're not spamming it every frame.
    if (!g.rescued) {
      int havelock=0,i=COLC*ROWC;
      const uint8_t *p=g.map->v;
      for (;i-->0;p++) {
        if (*p==0x30) {
          havelock=1;
          break;
        }
      }
      if (!havelock) {
        g.rescued=1;
      }
    }
  }
}

void _werewolf_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  if (g.rescued) {
    graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid+2,0);
    if (SPRITE->animframe) {
      graf_draw_tile(&g.graf,texid,dstx+(TILESIZE>>1),dsty-TILESIZE,sprite->tileid+3,0);
    } else {
      graf_draw_tile(&g.graf,texid,dstx-(TILESIZE>>1),dsty-TILESIZE,sprite->tileid+3,EGG_XFORM_XREV);
    }
  } else {
    graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid+SPRITE->animframe,0);
  }
}

const struct sprite_type sprite_type_werewolf={
  .name="werewolf",
  .objlen=sizeof(struct sprite_werewolf),
  .update=_werewolf_update,
  .render=_werewolf_render,
};
