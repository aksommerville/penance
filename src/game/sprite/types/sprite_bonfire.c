#include "game/penance.h"

struct sprite_bonfire {
  struct sprite hdr;
  double animclock;
  int animframe;
  double ttl;
};

#define SPRITE ((struct sprite_bonfire*)sprite)

static int _bonfire_init(struct sprite *sprite,const uint8_t *arg,int argc,const uint8_t *def,int defc) {
  SPRITE->ttl=1.000;
  if (sprite_group_add(GRP(VISIBLE),sprite)<0) return -1;
  if (sprite_group_add(GRP(UPDATE),sprite)<0) return -1;
  if (sprite_group_add(GRP(HAZARD),sprite)<0) return -1;
  return 0;
}

static void _bonfire_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite_kill_later(sprite);
    return;
  }
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.150;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    switch (SPRITE->animframe) {
      case 0: sprite->tileid-=3; break;
      default: sprite->tileid++;
    }
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_bonfire={
  .name="bonfire",
  .objlen=sizeof(struct sprite_bonfire),
  .init=_bonfire_init,
  .update=_bonfire_update,
};
