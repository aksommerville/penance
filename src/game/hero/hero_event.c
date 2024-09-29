#include "hero_internal.h"

/* Dpad changed.
 */
 
static void hero_dpad(struct sprite *sprite,int8_t dx,int8_t dy) {
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
  fprintf(stderr,"%s\n",__func__);
}
 
static void hero_action_end(struct sprite *sprite) {
  fprintf(stderr,"%s\n",__func__);
}

/* Receive input.
 */
 
void sprite_hero_input(struct sprite *sprite,int input,int pvinput) {
  
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
