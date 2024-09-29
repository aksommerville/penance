#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/penance.h"

#define HERO_MODE_FREE 0
#define HERO_MODE_TREE 1
#define HERO_MODE_HOLE 2
#define HERO_MODE_GHOST 3

struct sprite_hero {
  struct sprite hdr;
  int mode; // HERO_MODE_*
  int8_t indx,indy; // State of dpad.
  uint8_t facedir; // We are only doing horizontal faces, to spare some effort drawing. So will only be DIR_W or DIR_E.
  uint8_t movedir; // Last nonzero motion on dpad. Basically (facedir) but includes vertical.
  double animclock;
  int animframe;
  double pvx,pvy; // Set to prior position before invoking rectification.
  int cellx,celly; // Most recent quantized position.
  int input_blackout; // Nonzero to wait for input to go zero before receiving any more.
  uint8_t spellv[SPELL_LIMIT]; // DIR_*
  int spellc; // May exceed SPELL_LIMIT, in which case (spellv) contains only the tail, and it's definitely not valid.
  double spellclock; // In FREE mode, the spell clears whenever this expires.
  int ghost_mapid; // So we can return, and also for creating the fleshpuppet as needed.
  double ghost_x,ghost_y;
};

#define SPRITE ((struct sprite_hero*)sprite)

void hero_rectify_position(struct sprite *sprite);
void hero_quantized_motion(struct sprite *sprite,int nx,int ny);

#endif
