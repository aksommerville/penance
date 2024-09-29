#include "../penance.h"

/* Sprite type by id.
 */
 
const struct sprite_type *sprite_type_by_id(int id) {
  switch (id) {
    case 0: return &sprite_type_dummy;
    case 1: return &sprite_type_hero;
    case 2: return &sprite_type_fleshpuppet;
    case 3: return &sprite_type_fireball;
    case 4: return &sprite_type_bonfire;
  }
  return 0;
}
