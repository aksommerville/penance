#include "game/penance.h"

struct sprite_treadle {
  struct sprite hdr;
  int lampix;
  int pressed;
  int lit;
  double clock;
  uint8_t tileid0;
};

#define SPRITE ((struct sprite_treadle*)sprite)

static int _treadle_init(struct sprite *sprite,const uint8_t *def,int defc) {
  SPRITE->tileid0=sprite->tileid;
  // Choose an index by counting existing treadles. We should already be in VISIBLE, check for it.
  int i=GRP(VISIBLE)->spritec;
  while (i-->0) {
    const struct sprite *other=GRP(VISIBLE)->spritev[i];
    if (other==sprite) continue;
    if (other->type!=&sprite_type_treadle) continue;
    SPRITE->lampix++;
  }
  return 0;
}

static int treadle_check_hero(const struct sprite *sprite) {
  if (GRP(HERO)->spritec<1) return 0;
  const struct sprite *hero=GRP(HERO)->spritev[0];
  double dx=sprite->x-hero->x;
  double dy=sprite->y-hero->y;
  const double thresh=0.75;
  if ((dx<-thresh)||(dx>thresh)) return 0;
  if ((dy<-thresh)||(dy>thresh)) return 0;
  return 1;
}

static void treadle_notify_lock(struct sprite *sprite) {
  int i=GRP(VISIBLE)->spritec;
  while (i-->0) {
    struct sprite *lock=GRP(VISIBLE)->spritev[i];
    if (lock->type!=&sprite_type_lock) continue;
    sprite_lock_set_lamp(lock,SPRITE->lampix,SPRITE->lit);
  }
}

static void _treadle_update(struct sprite *sprite,double elapsed) {
  int npressed=treadle_check_hero(sprite);
  if (npressed!=SPRITE->pressed) {
    SPRITE->pressed=npressed;
    if (SPRITE->pressed) {
      //TODO Sound effect
      sprite->tileid=SPRITE->tileid0+1;
      SPRITE->lit=1;
      treadle_notify_lock(sprite);
    } else {
      SPRITE->clock=2.250;
    }
  } else if (SPRITE->lit&&!SPRITE->pressed) {
    if ((SPRITE->clock-=elapsed)<=0.0) {
      //TODO Sound effect
      sprite->tileid=SPRITE->tileid0;
      SPRITE->lit=0;
      treadle_notify_lock(sprite);
    }
  }
}

const struct sprite_type sprite_type_treadle={
  .name="treadle",
  .objlen=sizeof(struct sprite_treadle),
  .init=_treadle_init,
  .update=_treadle_update,
};
