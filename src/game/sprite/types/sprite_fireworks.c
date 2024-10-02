#include "game/penance.h"

#define TTL 3.0

struct sprite_fireworks {
  struct sprite hdr;
  double clock; // Counts up.
};

#define SPRITE ((struct sprite_fireworks*)sprite)

static int _fireworks_init(struct sprite *sprite,const uint8_t *def,int defc) {
  if (!sprite->imageid) {
    sprite->imageid=RID_image_hero;
    sprite->tileid=0x63;
  }
  sprite_group_add(GRP(VISIBLE),sprite);
  sprite_group_add(GRP(UPDATE),sprite);
  return 0;
}

static void _fireworks_update(struct sprite *sprite,double elapsed) {
  SPRITE->clock+=elapsed;
  if (SPRITE->clock>=TTL) {
    sprite_kill_later(sprite);
    return;
  }
}

static void _fireworks_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  #define PARTICLEC 7
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  int i=PARTICLEC;
  double t=0.0;
  double dt=(M_PI*2.0)/PARTICLEC;
  double radius=SPRITE->clock*8.0;
  uint8_t tileid=sprite->tileid;
  switch (((int)(SPRITE->clock*10.0))%6) {
    case 0: tileid+=0; break;
    case 1: tileid+=1; break;
    case 2: tileid+=2; break;
    case 3: tileid+=3; break;
    case 4: tileid+=2; break;
    case 5: tileid+=1; break;
  }
  graf_set_alpha(&g.graf,((TTL-SPRITE->clock)*255.0)/TTL);
  for (;i-->0;t+=dt) {
    double x=(sprite->x+cos(t)*radius)*TILESIZE;
    double y=(sprite->y+sin(t)*radius)*TILESIZE;
    graf_draw_tile(&g.graf,texid,(int16_t)x+addx,(int16_t)y+addy,tileid,0);
  }
  graf_set_alpha(&g.graf,0xff);
  #undef PARTICLEC
}

const struct sprite_type sprite_type_fireworks={
  .name="fireworks",
  .objlen=sizeof(struct sprite_fireworks),
  .init=_fireworks_init,
  .update=_fireworks_update,
  .render=_fireworks_render,
};
