#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/penance.h"

struct sprite_hero {
  struct sprite hdr;
  int8_t indx,indy; // State of dpad.
  uint8_t facedir; // We are only doing horizontal faces, to spare some effort drawing. So will only be DIR_W or DIR_E.
  uint8_t movedir; // Last nonzero motion on dpad. Basically (facedir) but includes vertical.
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_hero*)sprite)

#endif
