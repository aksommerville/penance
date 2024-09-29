#include "../penance.h"

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->refc-->1) return;
  if (sprite->type->del) sprite->type->del(sprite);
  if (sprite->groupv) {
    if (sprite->groupc) fprintf(stderr,"%s:%d: sprite->groupc==%d at del\n",__FILE__,__LINE__,sprite->groupc);
    free(sprite->groupv);
  }
  free(sprite);
}

/* New.
 */
 
struct sprite *sprite_new(const struct sprite_type *type) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  sprite->type=type;
  sprite->refc=1;
  return sprite;
}

/* Retain.
 */
 
int sprite_ref(struct sprite *sprite) {
  if (!sprite) return -1;
  if (sprite->refc<1) return -1;
  if (sprite->refc>=INT_MAX) return -1;
  sprite->refc++;
  return 0;
}

/* Spawn.
 */

struct sprite *sprite_spawn_for_map(int x,int y,const uint8_t *arg,int argc) {
  fprintf(stderr,"%s (%d,%d) argc=%d\n",__func__,x,y,argc);
  //TODO How are spawn points structured?
  const struct sprite_type *type=0;
  const uint8_t *def=0;
  int defc=0;
  return sprite_spawn_with_type(
    (double)x+0.5,(double)y+0.5,
    type,arg,argc,def,defc
  );
}

struct sprite *sprite_spawn_with_type(
  double x,double y,
  const struct sprite_type *type,
  const uint8_t *arg,int argc,
  const uint8_t *def,int defc
) {
  struct sprite *sprite=sprite_new(type);
  if (!sprite) return 0;
  
  // First, add to KEEPALIVE, then drop our reference.
  if (sprite_group_add(GRP(KEEPALIVE),sprite)<0) {
    sprite_del(sprite);
    return 0;
  }
  sprite_del(sprite);
  
  sprite->x=x;
  sprite->y=y;
  
  // Iterate (def) and apply all generic fields.
  //TODO
  
  if (type->init&&(type->init(sprite,arg,argc,def,defc)<0)) {
    sprite_kill_now(sprite);
    return 0;
  }
  
  return sprite;
}
