#include "hero_internal.h"

/* Enter tree-spell mode.
 */
 
static void hero_begin_tree(struct sprite *sprite,uint8_t tileid) {
  sprite->y=SPRITE->celly;
  sprite->x=SPRITE->cellx+((tileid==0x2e)?1.0:0.0);
  SPRITE->mode=HERO_MODE_TREE;
  SPRITE->input_blackout=1;
  SPRITE->indx=0;
  SPRITE->indy=0;
  SPRITE->spellc=0;
}

/* Finish tree-spell mode.
 * Stumps must have clearance below.
 */
 
static void hero_end_tree(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_FREE;
  sprite->y+=1.500;
  if (SPRITE->spellc>SPELL_LIMIT) {
    fprintf(stderr,"Invalid tree spell, too long.\n");//TODO sound effect
  } else {
    char tmp[8];
    int i=0; for (;i<SPRITE->spellc;i++) switch (SPRITE->spellv[i]) {
      case DIR_W: tmp[i]='L'; break;
      case DIR_E: tmp[i]='R'; break;
      case DIR_N: tmp[i]='U'; break;
      case DIR_S: tmp[i]='D'; break;
      default: tmp[i]='?';
    }
    fprintf(stderr,"tree spell: %.*s\n",SPRITE->spellc,tmp);
    //TODO Cast spell.
  }
}

/* Enter hole-spell mode.
 */
 
static void hero_begin_hole(struct sprite *sprite,uint8_t tileid) {
  sprite->y=SPRITE->celly+0.5+(1.0/TILESIZE);
  sprite->x=SPRITE->cellx+((tileid==0x3e)?1.0:0.0);
  SPRITE->mode=HERO_MODE_HOLE;
  SPRITE->input_blackout=1;
  SPRITE->indx=0;
  SPRITE->indy=0;
  SPRITE->spellc=0;
}

/* Finish hole-spell mode.
 * Holes must have clearance below.
 */
 
static void hero_end_hole(struct sprite *sprite) {
  SPRITE->mode=HERO_MODE_FREE;
  sprite->y+=1.000;
  if (SPRITE->spellc>SPELL_LIMIT) {
    fprintf(stderr,"Invalid hole spell, too long.\n");//TODO sound effect
  } else {
    char tmp[8];
    int i=0; for (;i<SPRITE->spellc;i++) switch (SPRITE->spellv[i]) {
      case DIR_W: tmp[i]='L'; break;
      case DIR_E: tmp[i]='R'; break;
      case DIR_N: tmp[i]='U'; break;
      case DIR_S: tmp[i]='D'; break;
      default: tmp[i]='?';
    }
    fprintf(stderr,"hole spell: %.*s\n",SPRITE->spellc,tmp);
    //TODO Cast spell.
  }
}

/* Add one word to the spell.
 */
 
static void hero_spell_add(struct sprite *sprite,uint8_t word) {
  if (SPRITE->spellc<SPELL_LIMIT) {
    SPRITE->spellv[SPRITE->spellc]=word;
  } else {
    memmove(SPRITE->spellv,SPRITE->spellv+1,SPELL_LIMIT-1);
    SPRITE->spellv[SPELL_LIMIT-1]=word;
  }
  SPRITE->spellc++;
}

/* Dpad changed.
 */
 
static void hero_dpad(struct sprite *sprite,int8_t dx,int8_t dy) {

  // When casting spells, any cardinal state entered from the zero state is a word to add.
  if ((SPRITE->mode==HERO_MODE_TREE)||(SPRITE->mode==HERO_MODE_HOLE)) {
    if (!SPRITE->indx&&!SPRITE->indy) {
      if (dx&&!dy) {
        hero_spell_add(sprite,(dx<0)?DIR_W:DIR_E);
      } else if (!dx&&dy) {
        hero_spell_add(sprite,(dy<0)?DIR_N:DIR_S);
      }
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
    case HERO_MODE_FREE: break; // Do whatever the button does normally... Nothing? There aren't items or anything.
    case HERO_MODE_TREE: hero_end_tree(sprite); break;
    case HERO_MODE_HOLE: hero_end_hole(sprite); break;
  }
}
 
static void hero_action_end(struct sprite *sprite) {
}

/* Receive input.
 */
 
void sprite_hero_input(struct sprite *sprite,int input,int pvinput) {

  /* Apply or lift blackout, if warranted.
   */
  if (SPRITE->input_blackout) {
    if (input&~EGG_BTN_CD) input=0; // Still waiting.
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
  if ((nx>=0)&&(ny>=0)&&(nx<COLC)&&(ny<ROWC)) {
    uint8_t tileid=g.map->v[ny*COLC+nx];
    // Stump and Hole are in fixed positions on every tilesheet.
    // If we were doing things right, it would be a POI command on the map, but this is a 9-day game jam...
    if ((tileid==0x2e)||(tileid==0x2f)) {
      hero_begin_tree(sprite,tileid);
    } else if ((tileid==0x3e)||(tileid==0x3f)) {
      hero_begin_hole(sprite,tileid);
    }
  }
}
