#include "game/penance.h"

struct sprite_bonfire {
  struct sprite hdr;
  double animclock;
  int animframe;
  double ttl;
  int blow;
  int tileid0;
};

#define SPRITE ((struct sprite_bonfire*)sprite)

static int _bonfire_init(struct sprite *sprite,const uint8_t *def,int defc) {
  SPRITE->tileid0=sprite->tileid;
  if (sprite_group_add(GRP(VISIBLE),sprite)<0) return -1;
  if (sprite_group_add(GRP(UPDATE),sprite)<0) return -1;
  if (sprite_group_add(GRP(HAZARD),sprite)<0) return -1;
  return 0;
}

static void _bonfire_update(struct sprite *sprite,double elapsed) {
  if (!SPRITE->tileid0) SPRITE->tileid0=sprite->tileid;
  if (SPRITE->ttl>0.0) {
    if ((SPRITE->ttl-=elapsed)<=0.0) {
      sprite_kill_later(sprite);
      return;
    }
  }
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.150;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
  if (SPRITE->blow<0) {
    SPRITE->blow++;
    sprite->tileid=0x3b+SPRITE->animframe;
    sprite->xform=0;
  } else if (SPRITE->blow>0) {
    SPRITE->blow--;
    sprite->tileid=0x3b+SPRITE->animframe;
    sprite->xform=EGG_XFORM_XREV;
  } else {
    sprite->tileid=SPRITE->tileid0+SPRITE->animframe;
    sprite->xform=0;
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

/* Public accessors.
 */
 
void sprite_bonfire_set_ttl(struct sprite *sprite,double ttl) {
  if (!sprite||(sprite->type!=&sprite_type_bonfire)) return;
  SPRITE->ttl=ttl;
}

void sprite_bonfire_blow(struct sprite *sprite,int dx) {
  if (!sprite||(sprite->type!=&sprite_type_bonfire)) return;
  SPRITE->blow=dx*2;
}
