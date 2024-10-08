#include "hero_internal.h"

/* Enter tree-spell mode.
 */
 
static void hero_begin_tree(struct sprite *sprite,uint8_t tileid) {
  sfx(SFX_ENTER_TREE);
  sprite->y=SPRITE->celly;
  sprite->x=SPRITE->cellx+((tileid==0x2e)?1.0:0.0);
  SPRITE->mode=HERO_MODE_TREE;
  SPRITE->input_blackout=1;
  SPRITE->indx=0;
  SPRITE->indy=0;
  SPRITE->spellc=0;
}

/* Teleport: Enter a modal to selection destination.
 */
 
static void hero_cast_teleport(struct sprite *sprite) {
  menu_push(&menu_type_teleport);
  sprite->y-=1.500; // Stay on the stump. Mode has changed to FREE and that's OK; hero_animate() won't have been called.
  sfx(SFX_TELEPORT_OPEN);
  g.spellusage|=SPELLBIT_TELEPORT;
}

/* The Spell of Opening.
 * It's also the Spell of Closing. Look for 4 specific tiles (0xe0,0xe1,0xf0,0xf1) and toggle them with their neighbors +2.
 * Obviously this is a poor way to do things, but I'm on a tight schedule.
 */
 
static void hero_cast_open(struct sprite *sprite) {
  uint8_t *p=g.map->v;
  int i=COLC*ROWC,effective=0;
  for (;i-->0;p++) {
    switch (*p) {
      case 0xe0: *p=0xe2; effective=1; break;
      case 0xe1: *p=0xe3; effective=1; break;
      case 0xe2: *p=0xe0; effective=1; break;
      case 0xe3: *p=0xe1; effective=1; break;
      case 0xf0: *p=0xf2; effective=1; break;
      case 0xf1: *p=0xf3; effective=1; break;
      case 0xf2: *p=0xf0; effective=1; break;
      case 0xf3: *p=0xf1; effective=1; break;
    }
  }
  if (effective) {
    sfx(SFX_OPENING_ACCEPT);
    g.spellusage|=SPELLBIT_OPEN;
  } else {
    sfx(SFX_OPENING_REJECT);
  }
}

/* Finish tree-spell mode.
 * Stumps must have clearance below.
 */
 
static void hero_end_tree(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_FREE;
  sprite->y+=1.500;
  if (!SPRITE->spellc) return;
  #define SPELLCHECK(tag,...) { \
    const uint8_t spell[]={__VA_ARGS__}; \
    if ((SPRITE->spellc==sizeof(spell))&&!memcmp(SPRITE->spellv,spell,sizeof(spell))) { \
      SPRITE->spellc=0; \
      hero_cast_##tag(sprite); \
      return; \
    } \
  }
  SPELLCHECK(teleport,DIR_S,DIR_S,DIR_S)
  SPELLCHECK(open,DIR_W,DIR_E,DIR_W,DIR_N,DIR_N)
  #undef SPELLCHECK
  SPRITE->spellc=0;
  sfx(SFX_INVALID_SPELL);
}

/* Enter hole-spell mode.
 */
 
static void hero_begin_hole(struct sprite *sprite,uint8_t tileid) {
  sfx(SFX_ENTER_HOLE);
  sprite->y=SPRITE->celly+0.5+(1.0/TILESIZE);
  sprite->x=SPRITE->cellx+((tileid==0x3e)?1.0:0.0);
  SPRITE->mode=HERO_MODE_HOLE;
  SPRITE->input_blackout=1;
  SPRITE->indx=0;
  SPRITE->indy=0;
  SPRITE->spellc=0;
}

/* Turn into animals.
 */
 
static void hero_cast_rabbit(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_RABBIT;
  sfx(SFX_TRANSFORM);
  g.spellusage|=SPELLBIT_RABBIT;
}
 
static void hero_cast_bird(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_BIRD;
  sprite->y-=1.000; // Normally you get pushed off the hole after casting, but for bird no need to.
  sfx(SFX_TRANSFORM);
  g.spellusage|=SPELLBIT_BIRD;
}
 
static void hero_cast_turtle(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_TURTLE;
  sfx(SFX_TRANSFORM);
  g.spellusage|=SPELLBIT_TURTLE;
}
 
static void hero_cast_jammio(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_JAMMIO;
  sfx(SFX_TRANSFORM);
  g.jammio=1; // This one isn't part of (spellusage); it's a separate thing for tax purposes.
}

static void hero_end_transform(struct sprite *sprite) {

  /* If we're transforming out of BIRD, check first that there is at least one vacant cell.
   * There's a screen in our world that's nothing but water and rocks, and it would be disastrous to untransform there.
   * Otherwise, it's fine to unbird over water; physics rectification will find a safe place for you to land.
   */
  if (SPRITE->mode==HERO_MODE_BIRD) {
    const uint8_t *cell=g.map->v;
    int i=COLC*ROWC,ok=0;
    for (;i-->0;cell++) {
      if (g.map->tileprops[*cell]) continue;
      ok=1;
      break;
    }
    if (!ok) {
      sfx(SFX_REJECT_UNTRANSFORM);
      return;
    }
  }

  SPRITE->mode=HERO_MODE_FREE;
  hero_rectify_position(sprite); // We might be transforming out of BIRD over water.
  sfx(SFX_UNTRANSFORM);
  struct sprite *fireworks=sprite_spawn_with_type(sprite->x,sprite->y,&sprite_type_fireworks,0,0);
  
  // Force a recheck of the grid. Stumps are inert to all transformed modes, and birds won't enter holes.
  hero_quantized_motion(sprite,(int)sprite->x,(int)sprite->y);
}

/* Finish hole-spell mode.
 * Holes must have clearance below.
 */
 
static void hero_end_hole(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_FREE;
  sprite->y+=1.000;
  if (!SPRITE->spellc) return;
  #define SPELLCHECK(tag,...) { \
    const uint8_t spell[]={__VA_ARGS__}; \
    if ((SPRITE->spellc==sizeof(spell))&&!memcmp(SPRITE->spellv,spell,sizeof(spell))) { \
      SPRITE->spellc=0; \
      hero_cast_##tag(sprite); \
      return; \
    } \
  }
  SPELLCHECK(rabbit,DIR_E,DIR_E,DIR_S,DIR_E)
  SPELLCHECK(bird,DIR_N,DIR_N,DIR_W,DIR_E,DIR_N)
  SPELLCHECK(turtle,DIR_W,DIR_W,DIR_E,DIR_E,DIR_S)
  SPELLCHECK(jammio,DIR_N,DIR_N,DIR_S,DIR_S,DIR_W,DIR_E,DIR_W,DIR_E)
  #undef SPELLCHECK
  SPRITE->spellc=0;
  sfx(SFX_INVALID_SPELL);
}

/* Throw fireball.
 */
 
static void hero_cast_fireball(struct sprite *sprite,double dx) {
  SPRITE->mode=HERO_MODE_FIREBALL;
  SPRITE->spellclock=0.500;
  struct sprite *fireball=sprite_spawn_with_type(sprite->x+dx,sprite->y,&sprite_type_fireball,0,0);
  if (!fireball) return;
  sfx(SFX_FIREBALL);
  fireball->imageid=sprite->imageid;
  fireball->tileid=0x22;
  sprite_fireball_set_direction(fireball,dx*12.0,0.0);
  sprite_group_add(GRP(UPDATE),fireball);
  sprite_group_add(GRP(VISIBLE),fireball);
  if (dx>0.0) {
    SPRITE->facedir=DIR_E;
  } else {
    SPRITE->facedir=DIR_W;
  }
  SPRITE->respell=HERO_MODE_FIREBALL;
  g.spellusage|=SPELLBIT_FIREBALL;
}
 
static void hero_cast_fireballl(struct sprite *sprite) { hero_cast_fireball(sprite,-1.0); }
static void hero_cast_fireballr(struct sprite *sprite) { hero_cast_fireball(sprite,1.0); }

static void hero_cast_invalid(struct sprite *sprite) {
  // Called for loose UUUU and DDDD. A user might think that those would cause a vertical fireball, but we don't do vertical.
  sfx(SFX_INVALID_SPELL);
  SPRITE->spellc=0;
}

/* Turn into flower.
 */
 
static void hero_cast_flower(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_FLOWER;
  SPRITE->respell=HERO_MODE_FLOWER;
  SPRITE->wind=0;
  SPRITE->animclock=0.0;
  SPRITE->animframe=0;
  sfx(SFX_FLOWER);
  g.spellusage|=SPELLBIT_FLOWER;
}

static void hero_end_flower(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_FREE;
  sfx(SFX_UNFLOWER);
}

/* Turn into ghost.
 */
 
static void hero_cast_disembody(struct sprite *sprite) {
  sfx(SFX_GHOST);
  SPRITE->mode=HERO_MODE_GHOST;
  sprite->layer++;
  struct sprite *fleshpuppet=sprite_spawn_with_type(sprite->x,sprite->y,&sprite_type_fleshpuppet,0,0);
  if (fleshpuppet) {
    sprite_group_add(GRP(UPDATE),fleshpuppet);
    sprite_group_add(GRP(VISIBLE),fleshpuppet);
    fleshpuppet->imageid=sprite->imageid;
    fleshpuppet->tileid=0x12;
  }
  SPRITE->ghost_mapid=g.map->rid;
  SPRITE->ghost_x=sprite->x;
  SPRITE->ghost_y=sprite->y;
  SPRITE->respell=HERO_MODE_GHOST;
  g.spellusage|=SPELLBIT_DISEMBODY;
}

/* End ghost mode (by pressing South).
 */
 
static void hero_end_ghost(struct sprite *sprite) {
  sfx(SFX_UNGHOST);
  SPRITE->mode=HERO_MODE_FREE;
  sprite->x=SPRITE->ghost_x;
  sprite->y=SPRITE->ghost_y;
  sprite->layer--;
  /* If we're on the right map, kill the fleshpuppet.
   * Take the fleshpuppet's position too, if there is one.
   * It might have been moved by wind or something.
   */
  if (SPRITE->ghost_mapid==g.map->rid) {
    int i=GRP(KEEPALIVE)->spritec;
    while (i-->0) {
      struct sprite *fleshpuppet=GRP(KEEPALIVE)->spritev[i];
      if (fleshpuppet->type!=&sprite_type_fleshpuppet) continue;
      sprite->x=fleshpuppet->x;
      sprite->y=fleshpuppet->y;
      sprite_kill_later(fleshpuppet);
      break;
    }
  } else { // Need to change maps.
    g.mapid_load_soon=SPRITE->ghost_mapid;
  }
}

void hero_unghost() {
  if (GRP(HERO)->spritec<1) return;
  hero_end_ghost(GRP(HERO)->spritev[0]);
}

/* Check the tail of (spellv) for foot spells.
 */
 
static void hero_check_free_spell(struct sprite *sprite) {
  if (SPRITE->spellc>SPELL_LIMIT) return;
  if (SPRITE->spellc<1) return;
  #define SPELLCHECK(tag,...) { \
    const uint8_t spell[]={__VA_ARGS__}; \
    if ((SPRITE->spellc>=sizeof(spell))&&!memcmp(SPRITE->spellv+SPRITE->spellc-sizeof(spell),spell,sizeof(spell))) { \
      SPRITE->spellc=0; \
      hero_cast_##tag(sprite); \
      return; \
    } \
  }
  SPELLCHECK(fireballl,DIR_W,DIR_W,DIR_W,DIR_W)
  SPELLCHECK(fireballr,DIR_E,DIR_E,DIR_E,DIR_E)
  SPELLCHECK(flower,DIR_S,DIR_S,DIR_N,DIR_S)
  SPELLCHECK(disembody,DIR_N,DIR_W,DIR_E,DIR_S,DIR_N)
  SPELLCHECK(invalid,DIR_N,DIR_N,DIR_N,DIR_N)
  SPELLCHECK(invalid,DIR_S,DIR_S,DIR_S,DIR_S)
  #undef SPELLCHECK
}

/* Add one word to the spell.
 * In FREE mode, spells cast instantly -- check for completion.
 */
 
static void hero_spell_add(struct sprite *sprite,uint8_t word) {
  if (SPRITE->spellc<SPELL_LIMIT) {
    SPRITE->spellv[SPRITE->spellc]=word;
  } else {
    memmove(SPRITE->spellv,SPRITE->spellv+1,SPELL_LIMIT-1);
    SPRITE->spellv[SPELL_LIMIT-1]=word;
  }
  SPRITE->spellc++;
  SPRITE->spellclock=0.333;
  if (SPRITE->mode==HERO_MODE_FREE) {
    hero_check_free_spell(sprite);
  } else if ((SPRITE->mode==HERO_MODE_TREE)||(SPRITE->mode==HERO_MODE_HOLE)) {
    sfx(SFX_ENCODE_PIP);
  }
}

/* Dpad changed.
 */
 
static void hero_dpad(struct sprite *sprite,int8_t dx,int8_t dy) {

  // In FIREBALL mode, don't update.
  if (SPRITE->mode==HERO_MODE_FIREBALL) {
    SPRITE->indx=dx;
    SPRITE->indy=dy;
    return;
  }

  // Any cardinal state entered from the zero state is a word to add to the spell.
  if (!SPRITE->indx&&!SPRITE->indy) {
    if (dx&&!dy) {
      hero_spell_add(sprite,(dx<0)?DIR_W:DIR_E);
    } else if (!dx&&dy) {
      hero_spell_add(sprite,(dy<0)?DIR_N:DIR_S);
    }
  }

  if ((dx<0)&&(SPRITE->indx>=0)) {
    SPRITE->facedir=SPRITE->movedir=DIR_W;
  } else if ((dx>0)&&(SPRITE->indx<=0)) {
    SPRITE->facedir=SPRITE->movedir=DIR_E;
  } else if ((dy<0)&&(SPRITE->indy>=0)) {
    SPRITE->movedir=DIR_N;
  } else if ((dy>0)&&(SPRITE->indy<=0)) {
    SPRITE->movedir=DIR_S;
  }
  SPRITE->indx=dx;
  SPRITE->indy=dy;
}

/* Actions (south button).
 */
 
static void hero_action_begin(struct sprite *sprite) {
  switch (SPRITE->mode) {
    case HERO_MODE_FREE: switch (SPRITE->respell) {
        case HERO_MODE_FLOWER: hero_cast_flower(sprite); break;
        case HERO_MODE_FIREBALL: hero_cast_fireball(sprite,(SPRITE->facedir==DIR_W)?-1.0:1.0); break;
        case HERO_MODE_GHOST: hero_cast_disembody(sprite); break;
      } break;
    case HERO_MODE_TREE: hero_end_tree(sprite); break;
    case HERO_MODE_HOLE: hero_end_hole(sprite); break;
    case HERO_MODE_GHOST: hero_end_ghost(sprite); break;
    case HERO_MODE_FLOWER: hero_end_flower(sprite); break;
    case HERO_MODE_RABBIT: hero_end_transform(sprite); break;
    case HERO_MODE_BIRD: hero_end_transform(sprite); break;
    case HERO_MODE_TURTLE: hero_end_transform(sprite); break;
    case HERO_MODE_JAMMIO: hero_end_transform(sprite); break;
  }
}
 
static void hero_action_end(struct sprite *sprite) {
}

/* Receive input.
 */
 
void sprite_hero_input(struct sprite *sprite,int input,int pvinput) {
  if (g.gameover) return;

  /* Apply or lift blackout, if warranted.
   */
  if (SPRITE->input_blackout) {
    if (input&(EGG_BTN_LEFT|EGG_BTN_RIGHT|EGG_BTN_UP|EGG_BTN_DOWN)) input=0; // Still waiting.
    else SPRITE->input_blackout=0; // OK proceed.
  }
  
  /* Changes to dpad?
   */
  int8_t ndx=0,ndy=0;
  switch (input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: ndx=-1; break;
    case EGG_BTN_RIGHT: ndx=1; break;
  }
  switch (input&(EGG_BTN_UP|EGG_BTN_DOWN)) {
    case EGG_BTN_UP: ndy=-1; break;
    case EGG_BTN_DOWN: ndy=1; break;
  }
  if ((ndx!=SPRITE->indx)||(ndy!=SPRITE->indy)) {
    hero_dpad(sprite,ndx,ndy);
  }
  
  /* Begin or end action?
   */
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) hero_action_begin(sprite);
  else if (!(input&EGG_BTN_SOUTH)&&(pvinput&EGG_BTN_SOUTH)) hero_action_end(sprite);
}

/* React to quantized motion.
 */
 
void hero_quantized_motion(struct sprite *sprite,int nx,int ny) {
  // I don't expect to need the previous position, but it's available if we want.
  SPRITE->cellx=nx;
  SPRITE->celly=ny;
  
  // Some mode don't use quantized motion at all (and they probably shouldn't even call here).
  switch (SPRITE->mode) {
    case HERO_MODE_BIRD:
    case HERO_MODE_GHOST:
      return;
  }
  
  if ((nx>=0)&&(ny>=0)&&(nx<COLC)&&(ny<ROWC)) {
    uint8_t tileid=g.map->v[ny*COLC+nx];
    // Stump and Hole are in fixed positions on every tilesheet.
    // If we were doing things right, it would be a POI command on the map, but this is a 9-day game jam...
    // TURTLE and RABBIT modes do land here; they mostly behave like FREE. They can go into holes, but not stumps.
    if ((tileid==0x2e)||(tileid==0x2f)) {
      if (SPRITE->mode==HERO_MODE_RABBIT) return;
      if (SPRITE->mode==HERO_MODE_TURTLE) return;
      if (SPRITE->mode==HERO_MODE_JAMMIO) return;
      hero_begin_tree(sprite,tileid);
    } else if ((tileid==0x3e)||(tileid==0x3f)) {
      hero_begin_hole(sprite,tileid);
    }
  }
}

/* React to map navigation.
 */
 
void hero_map_changed(struct sprite *sprite) {
  if (g.gameover) {
    SPRITE->animclock=0.0;
    return;
  }
  if (SPRITE->mode==HERO_MODE_GHOST) {
    if (g.map->rid==SPRITE->ghost_mapid) {
      struct sprite *fleshpuppet=sprite_spawn_with_type(SPRITE->ghost_x,SPRITE->ghost_y,&sprite_type_fleshpuppet,0,0);
      if (fleshpuppet) {
        sprite_group_add(GRP(UPDATE),fleshpuppet);
        sprite_group_add(GRP(VISIBLE),fleshpuppet);
        fleshpuppet->imageid=sprite->imageid;
        fleshpuppet->tileid=0x12;
      }
    }
  }
}
