#include "game/penance.h"

#define WIND_STAGE_IDLE 0
#define WIND_STAGE_INHALE 1
#define WIND_STAGE_EXHALE 2

#define PARTICLEC 40

struct sprite_wind {
  struct sprite hdr;
  int stage;
  double stageclock;
  struct particle {
    int16_t x,y;
  } particlev[PARTICLEC];
};

#define SPRITE ((struct sprite_wind*)sprite)

/* Be careful with these parameters!
 */

static void wind_begin_IDLE(struct sprite *sprite) {
  SPRITE->stage=WIND_STAGE_IDLE;
  SPRITE->stageclock=0.850;
}

static void wind_begin_INHALE(struct sprite *sprite) {
  SPRITE->stage=WIND_STAGE_INHALE;
  SPRITE->stageclock=0.250;
}

static void wind_begin_EXHALE(struct sprite *sprite) {
  SPRITE->stage=WIND_STAGE_EXHALE;
  SPRITE->stageclock=1.250;
  int16_t ylo=(int16_t)((sprite->y-1.5)*TILESIZE);
  int16_t xhi=(int16_t)((sprite->x-1.0)*TILESIZE);
  struct particle *particle=SPRITE->particlev;
  int i=PARTICLEC;
  for (;i-->0;particle++) {
    particle->x=rand()%xhi;
    particle->y=ylo+rand()%(TILESIZE<<1);
  }
}

static int _wind_init(struct sprite *sprite,const uint8_t *def,int defc) {
  sprite->y+=1.0;
  wind_begin_IDLE(sprite);
  return 0;
}

static void wind_blow(struct sprite *sprite,double elapsed) {
  int i=GRP(VISIBLE)->spritec;
  while (i-->0) {
    struct sprite *victim=GRP(VISIBLE)->spritev[i];
    if (victim->x>=sprite->x-0.5) continue;
    if (victim->y<sprite->y-1.5) continue;
    if (victim->y>sprite->y+0.5) continue; // My (x,y) is 1/4 from top left, I'm 2x2 tiles.
    if (victim->type==&sprite_type_hero) ;
    else if (victim->type==&sprite_type_fleshpuppet) ;
    else if (victim->type==&sprite_type_fireball) {
      sprite_fireball_blow_out(victim);
      continue;
    } else {
      // All others, no impact.
      continue;
    }
    double distance=victim->x/sprite->x; // Normalized, always <1
    const double maximpact=40.0;
    double impact=sqrt(distance)*maximpact;
    if (victim->type==&sprite_type_hero) {
      hero_apply_wind(victim,-impact*elapsed);
    } else {
      victim->x-=impact*elapsed;
    }
  }
}

static void _wind_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->stageclock-=elapsed)<=0.0) {
    switch (SPRITE->stage) {
      case WIND_STAGE_IDLE: wind_begin_INHALE(sprite); break;
      case WIND_STAGE_INHALE: wind_begin_EXHALE(sprite); break;
      default: wind_begin_IDLE(sprite); break;
    }
  }
  if (SPRITE->stage==WIND_STAGE_EXHALE) wind_blow(sprite,elapsed);
}

static void _wind_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)((sprite->x-0.5)*TILESIZE);
  int16_t dsty=(int16_t)((sprite->y-1.5)*TILESIZE);
  int16_t srcx=(sprite->tileid&0x0f)*TILESIZE;
  int16_t srcy=(sprite->tileid>>4)*TILESIZE;
  srcx+=SPRITE->stage*(TILESIZE<<1);
  graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,sprite->imageid),dstx,dsty,srcx,srcy,TILESIZE<<1,TILESIZE<<1,0);
  if (SPRITE->stage==WIND_STAGE_EXHALE) {
    struct particle *particle=SPRITE->particlev;
    int i=PARTICLEC;
    for (;i-->0;particle++) {
      particle->x-=6;
      if (particle->x<0) {
        particle->x=rand()%dstx;
      }
      graf_draw_rect(&g.graf,particle->x,particle->y,4,1,0xf0f0ff80);
    }
  }
}

const struct sprite_type sprite_type_wind={
  .name="wind",
  .objlen=sizeof(struct sprite_wind),
  .init=_wind_init,
  .update=_wind_update,
  .render=_wind_render,
};
