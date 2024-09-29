#include "hero_internal.h"

/* Delete.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 * The hero will not have a resource or any other generic config.
 * We do it all here.
 */
 
static int _hero_init(struct sprite *sprite,const uint8_t *arg,int argc,const uint8_t *def,int defc) {
  sprite->imageid=RID_image_hero;
  sprite->tileid=0x00;
  sprite->xform=0;
  SPRITE->facedir=DIR_E;
  SPRITE->movedir=DIR_E;
  if (sprite_group_add(GRP(VISIBLE),sprite)<0) return -1;
  if (sprite_group_add(GRP(UPDATE),sprite)<0) return -1;
  if (sprite_group_add(GRP(HERO),sprite)<0) return -1;
  return 0;
}

/* Advance animation counters and select the appropriate frame.
 */
 
static void hero_animate(struct sprite *sprite,double elapsed) {

  //XXX Trying out some other faces.
  if (g.pvinput&EGG_BTN_SOUTH) {
    if (0) { // hurt.
      sprite->tileid=0x10;
      sprite->xform=(SPRITE->facedir==DIR_W)?EGG_XFORM_XREV:0;
      return;
    }
    if (0) { // encoding tree spell
      sprite->tileid=0x30;
      sprite->xform=0;
      if (SPRITE->indx<0) sprite->tileid+=0x01;
      else if (SPRITE->indx>0) sprite->tileid+=0x02;
      else if (SPRITE->indy<0) sprite->tileid+=0x03;
      else if (SPRITE->indy>0) sprite->tileid+=0x04;
      return;
    }
  }

  // Walking.
  if (SPRITE->indx||SPRITE->indy) {
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.200;
      if ((SPRITE->animframe+=1)>=3) SPRITE->animframe=0;
    }
    sprite->tileid=0x00+SPRITE->animframe;
    sprite->xform=(SPRITE->facedir==DIR_W)?EGG_XFORM_XREV:0;
    
  // Idle.
  } else {
    SPRITE->animclock=0.0;
    sprite->tileid=0;
    sprite->xform=(SPRITE->facedir==DIR_W)?EGG_XFORM_XREV:0;
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  if (SPRITE->indx||SPRITE->indy) {
    const double walkspeed=6.0; // tile/sec
    double dx=SPRITE->indx*elapsed*walkspeed;
    double dy=SPRITE->indy*elapsed*walkspeed;
    SPRITE->pvx=sprite->x;
    SPRITE->pvy=sprite->y;
    sprite->x+=dx;
    sprite->y+=dy;
    /* Currently, the only things relevant to physics are this motion right here, and the map.
     * No other sprites participate, and the hero only moves right here.
     * That's a pretty delicate balance to maintain, I'm not sure we'll be able to, but for now physics can be real easy.
     */
    hero_rectify_position(sprite);
  }

  hero_animate(sprite,elapsed);
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int16_t addx,int16_t addy) {
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  //.render=_hero_render,
};
