#include "game/penance.h"

struct sprite_dummy {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_dummy*)sprite)

//static void _dummy_del(struct sprite *sprite)
//static int _dummy_init(struct sprite *sprite,const uint8_t *def,int defc)
//static void _dummy_update(struct sprite *sprite,double elapsed)
//static void _dummy_render(struct sprite *sprite,int16_t addx,int16_t addy)

/* Type definition.
 */
 
const struct sprite_type sprite_type_dummy={
  .name="dummy",
  .objlen=sizeof(struct sprite_dummy),
  //.del=_dummy_del,
  //.init=_dummy_init,
  //.update=_dummy_update,
  //.render=_dummy_render,
};
