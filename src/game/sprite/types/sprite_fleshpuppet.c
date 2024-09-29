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
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_fleshpuppet={
  .name="fleshpuppet",
  .objlen=sizeof(struct sprite_fleshpuppet),
  .update=_fleshpuppet_update,
};
