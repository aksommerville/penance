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
  SPRITE->cellx=SPRITE->celly=-1;
  if (sprite_group_add(GRP(VISIBLE),sprite)<0) return -1;
  if (sprite_group_add(GRP(UPDATE),sprite)<0) return -1;
  if (sprite_group_add(GRP(HERO),sprite)<0) return -1;
  return 0;
}

/* Advance animation counters and select the appropriate frame.
 */
 
static void hero_animate(struct sprite *sprite,double elapsed) {

  // Ghost.
  if (SPRITE->mode==HERO_MODE_GHOST) {
    sprite->tileid=0x11;
    sprite->xform=0;
    
  // Flower.
  } else if (SPRITE->mode==HERO_MODE_FLOWER) {
    sprite->tileid=0x21;
    sprite->xform=0;
    
  // Fireball.
  } else if (SPRITE->mode==HERO_MODE_FIREBALL) {
    sprite->tileid=0x20;
    sprite->xform=(SPRITE->facedir==DIR_E)?0:EGG_XFORM_XREV;
    
  // Hurt.
  } else if (SPRITE->mode==HERO_MODE_HURT) {
    sprite->tileid=0x10;
    sprite->xform=(SPRITE->facedir==DIR_E)?0:EGG_XFORM_XREV;

  // Hole spells.
  } else if (SPRITE->mode==HERO_MODE_HOLE) {
    sprite->tileid=0x40;
    sprite->xform=0;
    if (SPRITE->indx<0) sprite->tileid+=1;
    else if (SPRITE->indx>0) sprite->tileid+=2;
    else if (SPRITE->indy<0) sprite->tileid+=3;
    else if (SPRITE->indy>0) sprite->tileid+=4;
    
  // Tree spells.
  } else if (SPRITE->mode==HERO_MODE_TREE) {
    sprite->tileid=0x30;
    sprite->xform=0;
    if (SPRITE->indx<0) sprite->tileid+=1;
    else if (SPRITE->indx>0) sprite->tileid+=2;
    else if (SPRITE->indy<0) sprite->tileid+=3;
    else if (SPRITE->indy>0) sprite->tileid+=4;

  // Walking.
  } else if (SPRITE->indx||SPRITE->indy) {
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

/* Update, FREE mode.
 * Walking and spellclock.
 */
 
static void hero_update_FREE(struct sprite *sprite,double elapsed) {
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
    
    // Quantize position and react if changed.
    int cellx=(int)sprite->x; if (sprite->x<0.0) cellx--;
    int celly=(int)sprite->y; if (sprite->y<0.0) celly--;
    if ((cellx!=SPRITE->cellx)||(celly!=SPRITE->celly)) {
      hero_quantized_motion(sprite,cellx,celly);
    }
  }
  
  if ((SPRITE->spellclock-=elapsed)<=0.0) {
    SPRITE->spellc=0;
    SPRITE->spellclock=0.0;
  }
}

/* Update, GHOST mode.
 * Almost the same as FREE. Physics will apply a little different, and you can't cast foot spells.
 * (because you don't have feet).
 * And we don't update (cellx,celly) -- those belong to the corporeal body you left behind.
 */
 
static void hero_update_GHOST(struct sprite *sprite,double elapsed) {
  if (SPRITE->indx||SPRITE->indy) {
    const double walkspeed=8.0; // tile/sec
    double dx=SPRITE->indx*elapsed*walkspeed;
    double dy=SPRITE->indy*elapsed*walkspeed;
    SPRITE->pvx=sprite->x;
    SPRITE->pvy=sprite->y;
    sprite->x+=dx;
    sprite->y+=dy;
  }
}

/* Update, TREE or HOLE mode.
 * Maybe nothing to do here? In a spell mode, everything is event-driven.
 */
 
static void hero_update_spells(struct sprite *sprite,double elapsed) {
}

/* Update, FIREBALL mode.
 */
 
static void hero_update_FIREBALL(struct sprite *sprite,double elapsed) {
  if ((SPRITE->spellclock-=elapsed)<=0.0) {
    SPRITE->mode=HERO_MODE_FREE;
    // (indx) might have reversed while we were posing. Update facedir.
    if (SPRITE->indx<0) SPRITE->facedir=SPRITE->movedir=DIR_W;
    else if (SPRITE->indx>0) SPRITE->facedir=SPRITE->movedir=DIR_E;
  }
}

/* Update, HURT mode.
 */
 
static void hero_update_HURT(struct sprite *sprite,double elapsed) {
  if ((SPRITE->hurtclock-=elapsed)<=0.0) {
    SPRITE->mode=HERO_MODE_FREE;
    if (SPRITE->indx<0) SPRITE->facedir=SPRITE->movedir=DIR_W;
    else if (SPRITE->indx>0) SPRITE->facedir=SPRITE->movedir=DIR_E;
    return;
  }
  sprite->x+=SPRITE->hurtdx*elapsed;
  sprite->y+=SPRITE->hurtdy*elapsed;
  const double decay=12.0; // tile/sec. Not exponential decay as you'd expect.
  if (SPRITE->hurtdx<0.0) {
    if ((SPRITE->hurtdx+=decay*elapsed)>0.0) SPRITE->hurtdx=0.0;
  } else {
    if ((SPRITE->hurtdx-=decay*elapsed)<0.0) SPRITE->hurtdx=0.0;
  }
  if (SPRITE->hurtdy<0.0) {
    if ((SPRITE->hurtdy+=decay*elapsed)>0.0) SPRITE->hurtdy=0.0;
  } else {
    if ((SPRITE->hurtdy-=decay*elapsed)<0.0) SPRITE->hurtdy=0.0;
  }
  // And the motion stuff:
  hero_rectify_position(sprite);
  int cellx=(int)sprite->x; if (sprite->x<0.0) cellx--;
  int celly=(int)sprite->y; if (sprite->y<0.0) celly--;
  if ((cellx!=SPRITE->cellx)||(celly!=SPRITE->celly)) {
    hero_quantized_motion(sprite,cellx,celly);
  }
}

/* Is this mode subject to damage from hazard sprites?
 */
 
static int hero_mode_vulnerable(int mode) {
  switch (mode) {
    case HERO_MODE_GHOST:
    case HERO_MODE_FLOWER:
      return 0;
  }
  return 1;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  // Check hazards.
  if (GRP(HAZARD)->spritec&&hero_mode_vulnerable(SPRITE->mode)) {
    const double radius=0.666; // All hazard sprites have the same radius. I... think we can get away with that?
    const double radius2=radius*radius;
    int i=GRP(HAZARD)->spritec;
    while (i-->0) {
      struct sprite *hazard=GRP(HAZARD)->spritev[i];
      double dx=sprite->x-hazard->x;
      double dy=sprite->y-hazard->y;
      if (dx*dx+dy*dy>radius2) continue;
      //TODO sound effect
      double distance=sqrt(dx*dx+dy*dy);
      double nx=dx/distance,ny=dy/distance;
      SPRITE->hurtdx=nx*16.0;
      SPRITE->hurtdy=ny*16.0;
      SPRITE->hurtclock=0.200;
      SPRITE->mode=HERO_MODE_HURT;
      SPRITE->spellc=0;
      break;
    }
  }

  switch (SPRITE->mode) {
    case HERO_MODE_FREE: hero_update_FREE(sprite,elapsed); break;
    case HERO_MODE_TREE:
    case HERO_MODE_HOLE: hero_update_spells(sprite,elapsed); break;
    case HERO_MODE_GHOST: hero_update_GHOST(sprite,elapsed); break;
    case HERO_MODE_FIREBALL: hero_update_FIREBALL(sprite,elapsed); break;
    case HERO_MODE_HURT: hero_update_HURT(sprite,elapsed); break;
  }
  hero_animate(sprite,elapsed);
}

/* Render. XXX Trying not to need this.
 */
 
static void _hero_render(struct sprite *sprite,int16_t addx,int16_t addy) {
}

/* Render overlay.
 * After all sprites have rendered.
 */
 
static uint8_t spell_tile(uint8_t dir) {
  switch (dir) {
    case DIR_W: return 0x52;
    case DIR_E: return 0x53;
    case DIR_N: return 0x54;
    case DIR_S: return 0x55;
  }
  return 0x56;
}
 
void hero_draw_overlay(struct sprite *sprite,int16_t addx,int16_t addy) {
  // Spell?
  if (SPRITE->spellc&&((SPRITE->mode==HERO_MODE_TREE)||(SPRITE->mode==HERO_MODE_HOLE))) {
    int wordc=SPRITE->spellc,overflow=0;
    if (wordc>SPELL_LIMIT) {
      overflow=1;
      wordc=SPELL_LIMIT;
    }
    int16_t dsty=(int)((sprite->y-1.0)*TILESIZE)+addy;
    int16_t dstw=14+7*wordc;
    int16_t dstx=(int)(sprite->x*TILESIZE)+addx-(dstw>>1);
    if (dstx<0) dstx=0; else if (dstx>g.fbw-dstw) dstx=g.fbw-dstw;
    dstx+=3;
    int texid=texcache_get_image(&g.texcache,sprite->imageid);
    graf_draw_tile(&g.graf,texid,dstx,dsty,0x50,0); dstx+=7;
    graf_draw_tile(&g.graf,texid,dstx,dsty,overflow?0x56:spell_tile(SPRITE->spellv[0]),0); dstx+=7;
    int i=1; for (;i<wordc;i++,dstx+=7) {
      graf_draw_tile(&g.graf,texid,dstx,dsty,spell_tile(SPRITE->spellv[i]),0);
    }
    graf_draw_tile(&g.graf,texid,dstx,dsty,0x51,0);
  }
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
