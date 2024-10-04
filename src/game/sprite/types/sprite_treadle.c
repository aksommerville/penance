#include "game/penance.h"

/* (g.playtime) at the last time we played the "tick" sound.
 * First enabled treadle to notice it expired plays the sound and updates.
 */
static double treadle_tick_time=0.0;

struct sprite_treadle {
  struct sprite hdr;
  int lampix;
  int pressed;
  int lit,fakelit;
  int lightable;
  double clock;
  uint8_t tileid0;
};

#define SPRITE ((struct sprite_treadle*)sprite)

static int _treadle_init(struct sprite *sprite,const uint8_t *def,int defc) {
  if (treadle_tick_time>g.playtime) { // Must have restarted. Reset our tick clock.
    treadle_tick_time=0.0;
  }
  SPRITE->tileid0=sprite->tileid;
  SPRITE->lightable=1;
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
  if (!hero_feet_touch_ground(hero)) return 0;
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

static void treadle_update_tick(struct sprite *sprite) {
  if (!SPRITE->lit) return; // Inactive
  
  // Also, check whether a lock exists and is incomplete.
  // Once the lock is satisfied, the treadles' sticky behavior should stop.
  int i=GRP(VISIBLE)->spritec,locked=0;
  while (i-->0) {
    struct sprite *lock=GRP(VISIBLE)->spritev[i];
    if (lock->type!=&sprite_type_lock) continue;
    if (sprite_lock_is_locked(lock)) { locked=1; break; }
  }
  if (!locked) {
    SPRITE->lit=0;
    SPRITE->fakelit=0;
    SPRITE->lightable=0;
    if (!SPRITE->pressed) {
      sprite->tileid=SPRITE->tileid0;
    }
    // Don't do SFX_TREADLE_OFF here, since there could be several treadles, and we're not the main event.
    return;
  }
  
  if (SPRITE->pressed) return; // Clock isn't running until you step off.
  double elapsed=g.playtime-treadle_tick_time;
  if (elapsed<0.500) return;
  treadle_tick_time=g.playtime;
  sfx(SFX_TICK);
}

static void _treadle_update(struct sprite *sprite,double elapsed) {
  int npressed=treadle_check_hero(sprite);
  if (npressed!=SPRITE->pressed) {
    SPRITE->pressed=npressed;
    if (SPRITE->pressed) {
      if (!SPRITE->fakelit) {
        sfx(SFX_TREADLE_ON);
        sprite->tileid=SPRITE->tileid0+1;
        SPRITE->fakelit=1;
        if (SPRITE->lightable) {
          SPRITE->lit=1;
          treadle_notify_lock(sprite);
        }
      }
    } else {
      if (SPRITE->lightable) {
        SPRITE->clock=2.250;
      } else {
        SPRITE->fakelit=0;
        sprite->tileid=SPRITE->tileid0;
        sfx(SFX_TREADLE_OFF);
      }
    }
  } else if (SPRITE->fakelit&&!SPRITE->pressed) {
    if ((SPRITE->clock-=elapsed)<=0.0) {
      sfx(SFX_TREADLE_OFF);
      sprite->tileid=SPRITE->tileid0;
      SPRITE->lit=0;
      SPRITE->fakelit=0;
      if (SPRITE->lightable) {
        treadle_notify_lock(sprite);
      }
    }
  }
  treadle_update_tick(sprite);
}

const struct sprite_type sprite_type_treadle={
  .name="treadle",
  .objlen=sizeof(struct sprite_treadle),
  .init=_treadle_init,
  .update=_treadle_update,
};
