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

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  switch (SPRITE->mode) {
    case HERO_MODE_FREE: hero_update_FREE(sprite,elapsed); break;
    case HERO_MODE_TREE:
    case HERO_MODE_HOLE: hero_update_spells(sprite,elapsed); break;
    case HERO_MODE_GHOST: hero_update_GHOST(sprite,elapsed); break;
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
