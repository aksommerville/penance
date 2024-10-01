#include "../penance.h"

/* Sprite type by id.
 */
 
static const struct sprite_type *sprite_typev[]={
#define _(tag) &sprite_type_##tag,
FOR_EACH_SPRITE_TYPE
#undef _
};
 
const struct sprite_type *sprite_type_by_id(int id) {
  if (id<0) return 0;
  if (id>=sizeof(sprite_typev)/sizeof(sprite_typev[0])) return 0;
  return sprite_typev[id];
}
