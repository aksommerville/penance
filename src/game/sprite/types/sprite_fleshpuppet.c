#include "game/penance.h"

struct sprite_fleshpuppet {
  struct sprite hdr;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_fleshpuppet*)sprite)

static void _fleshpuppet_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.500;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
    if (SPRITE->animframe) sprite->tileid++;
    else sprite->tileid--;
  }
  // If we strike a hazard, wake up.
  int i=GRP(HAZARD)->spritec;
  while (i-->0) {
    struct sprite *hazard=GRP(HAZARD)->spritev[i];
    double dx=sprite->x-hazard->x;
    double dy=sprite->y-hazard->y;
    double d2=dx*dx+dy*dy;
    if (d2>=0.666) continue;
    sprite_kill_later(sprite);
    hero_unghost();
    return;
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_fleshpuppet={
  .name="fleshpuppet",
  .objlen=sizeof(struct sprite_fleshpuppet),
  .update=_fleshpuppet_update,
};
