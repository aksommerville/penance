#include "game/penance.h"

#define INHALE_TIME 0.5
#define SHOOT_TIME 0.8
#define RESET_TIME 1.25

struct sprite_dualephant {
  struct sprite hdr;
  double clock;
};

#define SPRITE ((struct sprite_dualephant*)sprite)

static int _dualephant_init(struct sprite *sprite,const uint8_t *def,int defc) {
  sprite->y+=1.0;
  return 0;
}

static int dualephant_hero_in_range(struct sprite *sprite) {
  if (GRP(HERO)->spritec<1) return 0;
  const struct sprite *hero=GRP(HERO)->spritev[0];
  if (hero->x<3.0) return 0;
  return 1;
}

static void dualephant_init_missile(struct sprite *sprite,struct sprite *missile) {
  missile->imageid=sprite->imageid;
  missile->tileid=0x5e;
  sprite_group_add(GRP(VISIBLE),missile);
  sprite_group_add(GRP(UPDATE),missile);
  sprite_group_add(GRP(HAZARD),missile);
  sprite_missile_launch(missile,-8.0,0.0);
}

static void dualephant_fire(struct sprite *sprite) {
  struct sprite *missile;
  sfx(SFX_DUALEPHANT_FIRE);
  if (missile=sprite_spawn_with_type(sprite->x-1.0,sprite->y-1.0,&sprite_type_missile,0,0)) {
    dualephant_init_missile(sprite,missile);
  }
  if (missile=sprite_spawn_with_type(sprite->x-1.0,sprite->y,&sprite_type_missile,0,0)) {
    dualephant_init_missile(sprite,missile);
  }
}

static void _dualephant_update(struct sprite *sprite,double elapsed) {
  if (!dualephant_hero_in_range(sprite)) {
    SPRITE->clock=0.0;
    return;
  }
  int unfired=(SPRITE->clock<SHOOT_TIME);
  SPRITE->clock+=elapsed;
  if (unfired&&(SPRITE->clock>=SHOOT_TIME)) {
    dualephant_fire(sprite);
  }
  if (SPRITE->clock>=RESET_TIME) SPRITE->clock-=RESET_TIME;
}

static void _dualephant_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)((sprite->x-0.5)*TILESIZE)+addx;
  int16_t dsty=(int16_t)((sprite->y-1.5)*TILESIZE)+addy;
  int16_t srcx=112,srcy=80,w=TILESIZE<<1,h=TILESIZE<<1;
  if (SPRITE->clock>=SHOOT_TIME) { srcx+=TILESIZE<<2; w+=TILESIZE; dstx-=TILESIZE; }
  else if (SPRITE->clock>=INHALE_TIME) srcx+=TILESIZE<<1;
  graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,RID_image_hero),dstx,dsty,srcx,srcy,w,h,0);
}

const struct sprite_type sprite_type_dualephant={
  .name="dualephant",
  .objlen=sizeof(struct sprite_dualephant),
  .init=_dualephant_init,
  .update=_dualephant_update,
  .render=_dualephant_render,
};
