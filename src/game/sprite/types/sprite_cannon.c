#include "game/penance.h"

#define FIRE_PERIOD 1.500

struct sprite_cannon {
  struct sprite hdr;
  int heatseek;
  double rotation; // radians clockwise from right, if (heatseek)
  double clock; // Counts up to FIRE_PERIOD
  uint8_t tileid0;
};

#define SPRITE ((struct sprite_cannon*)sprite)

static int _cannon_init(struct sprite *sprite,const uint8_t *def,int defc) {
  struct rom_sprite res={0};
  if (rom_sprite_decode(&res,def,defc)<0) return -1;
  struct rom_command_reader reader={.v=res.cmdv,.c=res.cmdc};
  struct rom_command command;
  while (rom_command_reader_next(&command,&reader)>0) {
    switch (command.opcode) {
      case 0x3f: SPRITE->heatseek=command.argv[1]; break;
    }
  }
  SPRITE->clock=0.200; // Bottom of cycle is the distended frame; skip that the first time.
  SPRITE->tileid0=sprite->tileid;
  return 0;
}

static void cannon_fire(struct sprite *sprite) {

  // Don't fire if there isn't a lock, it feels weird.
  int locked=0,i=COLC*ROWC;
  const uint8_t *p=g.map->v;
  for (;i-->0;p++) {
    if (*p==0x30) { locked=1; break; }
  }
  if (!locked) return;

  const double speed=16.0;
  double nx=1.0;
  double ny=0.0;
  if (SPRITE->heatseek) {
    nx=cos(SPRITE->rotation);
    ny=sin(SPRITE->rotation);
  }
  double x=sprite->x+nx*0.5;
  double y=sprite->y+ny*0.5;
  struct sprite *missile=sprite_spawn_with_type(x,y,&sprite_type_missile,0,0);
  if (!missile) return;
  missile->imageid=sprite->imageid;
  missile->tileid=SPRITE->tileid0+3;
  sprite_group_add(GRP(VISIBLE),missile);
  sprite_group_add(GRP(UPDATE),missile);
  sprite_group_add(GRP(HAZARD),missile);
  sfx(SFX_CANNON_FIRE);
  sprite_missile_launch(missile,nx*speed,ny*speed);
}

static void _cannon_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->clock+=elapsed)>=FIRE_PERIOD) {
    SPRITE->clock-=FIRE_PERIOD;
    cannon_fire(sprite);
  }
  if (SPRITE->heatseek) {
    if (GRP(HERO)->spritec>0) {
      struct sprite *hero=GRP(HERO)->spritev[0];
      double dx=hero->x-sprite->x;
      double dy=hero->y-sprite->y;
      SPRITE->rotation=atan2(dy,dx);
    }
  }
}

static void _cannon_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx; // center
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  uint8_t tileid=SPRITE->tileid0;
  if (SPRITE->clock<0.200) {
    tileid+=2;
  } else if (SPRITE->clock>FIRE_PERIOD-0.200) {
    tileid+=1;
  }
  if (SPRITE->heatseek) {
    int16_t srcx=(tileid&0x0f)*TILESIZE;
    int16_t srcy=(tileid>>4)*TILESIZE;
    graf_draw_mode7(&g.graf,texid,dstx,dsty,srcx,srcy,TILESIZE,TILESIZE,1.0,1.0,SPRITE->rotation,0);
  } else {
    graf_draw_tile(&g.graf,texid,dstx,dsty,tileid,0);
  }
}

const struct sprite_type sprite_type_cannon={
  .name="cannon",
  .objlen=sizeof(struct sprite_cannon),
  .init=_cannon_init,
  .update=_cannon_update,
  .render=_cannon_render,
};
