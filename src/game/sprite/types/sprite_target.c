#include "game/penance.h"

struct sprite_target {
  struct sprite hdr;
  int complete;
};

#define SPRITE ((struct sprite_target*)sprite)

static int _target_init(struct sprite *sprite,const uint8_t *def,int defc) {
  
  /* Stupid and hacky: If there's no tile 0x30 (the lock's backing tile), assume we're already complete.
   * This way, targets don't need their own persistence mechanism, but they'll stay green after leaving and returning.
   */
  SPRITE->complete=1;
  const uint8_t *p=g.map->v;
  int i=COLC*ROWC;
  for (;i-->0;p++) if (*p==0x30) { SPRITE->complete=0; break; }
  if (SPRITE->complete) sprite->tileid++;
  
  return 0;
}

static void _target_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->complete) return;
  // fireball and missile can complete us.
  const double radius=0.800;
  const double radius2=radius*radius;
  int i=GRP(VISIBLE)->spritec;
  while (i-->0) {
    struct sprite *missile=GRP(VISIBLE)->spritev[i];
    if (
      (missile->type==&sprite_type_fireball)||
      (missile->type==&sprite_type_missile)
    ) {
      double dx=missile->x-sprite->x; if ((dx<-radius)||(dx>radius)) continue;
      double dy=missile->y-sprite->y; if ((dy<-radius)||(dy>radius)) continue;
      double d2=dx*dx+dy*dy;
      if (d2>radius2) continue;
      SPRITE->complete=1;
      sprite->tileid++;
      sprite_kill_later(missile);
      sprite_lock_unlock_all();
      // No need for a sound effect; the lock makes one.
      return;
    }
  }
}

const struct sprite_type sprite_type_target={
  .name="target",
  .objlen=sizeof(struct sprite_target),
  .init=_target_init,
  .update=_target_update,
};
