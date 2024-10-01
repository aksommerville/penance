#include "game/penance.h"

struct sprite_fireball {
  struct sprite hdr;
  double animclock;
  int animframe;
  double dx,dy;
};

#define SPRITE ((struct sprite_fireball*)sprite)

static void _fireball_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
    if (SPRITE->animframe) sprite->tileid++;
    else sprite->tileid--;
  }
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  if ((sprite->x<-1.0)||(sprite->x>COLC+1.0)||(sprite->y<-1.0)||(sprite->y>ROWC+1.0)) {
    sprite_kill_later(sprite);
    return;
  }
  
  // If we cross a tile of physics 3 "flammable", replace it with zero, kill me, and produce a time-limited bonfire.
  // Don't allow burning on the outermost column or row. Physics doesn't read across map gaps, so that would lead to flicker problems
  // where you could walk into a neighbor map, directly into a wall.
  int x=(int)sprite->x,y=(int)sprite->y;
  if ((x>=1)&&(y>=1)&&(x<COLC-1)&&(y<ROWC-1)) {
    uint8_t tileid=g.map->v[y*COLC+x];
    uint8_t prop=g.map->tileprops[tileid];
    if (prop==3) {
      g.map->v[y*COLC+x]=0;
      sprite_kill_later(sprite);
      struct sprite *bonfire=sprite_spawn_with_type(x+0.5,y+0.5,&sprite_type_bonfire,0,0);
      if (bonfire) {
        bonfire->imageid=sprite->imageid;
        bonfire->tileid=0x24;
        sprite_bonfire_set_ttl(bonfire,1.000);
      }
      //TODO sound effect
      return;
    }
  }
  
  // If we touch a candle sprite, light it and stop.
  int i=GRP(VISIBLE)->spritec;
  while (i-->0) {
    struct sprite *candle=GRP(VISIBLE)->spritev[i];
    if (candle->type!=&sprite_type_candle) continue;
    double dx=sprite->x-candle->x; if (dx>1.0) continue;
    double dy=sprite->y-candle->y; if (dy>1.0) continue;
    if (dx*dx+dy*dy>1.0) continue;
    if (!sprite_candle_light(candle)) continue;
    sprite_kill_later(sprite);
    return;
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_fireball={
  .name="fireball",
  .objlen=sizeof(struct sprite_fireball),
  .update=_fireball_update,
};

/* Set direction.
 */
 
void sprite_fireball_set_direction(struct sprite *sprite,double dx,double dy) {
  if (!sprite||(sprite->type!=&sprite_type_fireball)) return;
  SPRITE->dx=dx;
  SPRITE->dy=dy;
  if (dx>0.0) sprite->xform=0;
  else if (dx<0.0) sprite->xform=EGG_XFORM_XREV;
  else if (dy>0.0) sprite->xform=EGG_XFORM_SWAP;
  else if (dy<0.0) sprite->xform=EGG_XFORM_SWAP|EGG_XFORM_XREV;
}
