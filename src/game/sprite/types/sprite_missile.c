#include "game/penance.h"

struct sprite_missile {
  struct sprite hdr;
  double dx,dy;
  int deflected;
};

#define SPRITE ((struct sprite_missile*)sprite)

static int _missile_init(struct sprite *sprite,const uint8_t *def,int defc) {
  return 0;
}

static void _missile_update(struct sprite *sprite,double elapsed) {
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  if ((sprite->x<-1.0)||(sprite->y<-1.0)||(sprite->x>COLC+1.0)||(sprite->y>ROWC+1.0)) {
    sprite_kill_later(sprite);
    return;
  }
  if (!SPRITE->deflected&&(GRP(HERO)->spritec>=1)) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    if (hero_is_turtle(hero)) {
      double dx=sprite->x-hero->x;
      double dy=sprite->y-hero->y;
      double d2=dx*dx+dy*dy;
      if (d2<=0.80) {
        //TODO sound effect
        SPRITE->deflected=1;
        double d=sqrt(d2);
        double v=sqrt(SPRITE->dx*SPRITE->dx+SPRITE->dy*SPRITE->dy);
        hero_nudge(hero,-dx/d,-dy/d);
        SPRITE->dx=(dx*v)/d;
        SPRITE->dy=(dy*v)/d;
      }
    }
  }
}

const struct sprite_type sprite_type_missile={
  .name="missile",
  .objlen=sizeof(struct sprite_missile),
  .init=_missile_init,
  .update=_missile_update,
};

void sprite_missile_target_hero(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_missile)) return;
  if (GRP(HERO)->spritec<1) return;
  struct sprite *hero=GRP(HERO)->spritev[0];
  double dx=hero->x-sprite->x;
  double dy=hero->y-sprite->y;
  double distance=sqrt(dx*dx+dy*dy);
  const double speed=10.0;
  SPRITE->dx=(dx*speed)/distance;
  SPRITE->dy=(dy*speed)/distance;
}

void sprite_missile_launch(struct sprite *sprite,double dx,double dy) {
  if (!sprite||(sprite->type!=&sprite_type_missile)) return;
  SPRITE->dx=dx;
  SPRITE->dy=dy;
}
