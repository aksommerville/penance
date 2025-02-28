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

struct sprite *sprite_spawn_for_map(int x,int y,const uint8_t *def,int defc) {
  if ((defc<7)||(def[4]!=0x20)) {
    fprintf(stderr,"ERROR: Sprite must begin with 'type' command (0x20)\n");
    return 0;
  }
  int sprtid=(def[5]<<8)|def[6];
  const struct sprite_type *type=sprite_type_by_id(sprtid);
  if (!type) {
    fprintf(stderr,"ERROR: Invalid sprite type %d\n",sprtid);
    return 0;
  }
  return sprite_spawn_with_type(
    (double)x+0.5,(double)y+0.5,
    type,def,defc
  );
}

struct sprite *sprite_spawn_with_type(
  double x,double y,
  const struct sprite_type *type,
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
  // Sprite resources are framed exactly the same way as map commands (tho the opcodes are completely different).
  if (defc) {
    struct rom_sprite res={0};
    if (rom_sprite_decode(&res,def,defc)<0) { sprite_kill_now(sprite); return 0; }
    struct rom_command_reader reader={.v=res.cmdv,.c=res.cmdc};
    struct rom_command command;
    while (rom_command_reader_next(&command,&reader)>0) {
      switch (command.opcode) {
    
        case 0x20: break; // type -- must have been processed already by our caller.
        case 0x21: { // image
            sprite->imageid=(command.argv[0]<<8)|command.argv[1];
          } break;
        case 0x22: { // tileid
            sprite->tileid=command.argv[0];
          } break;
        case 0x23: { // layer
            sprite->layer=(int16_t)((command.argv[0]<<8)|command.argv[1]);
          } break;
        case 0x40: { // grpmask
            int grpmask=(command.argv[0]<<24)|(command.argv[1]<<16)|(command.argv[2]<<8)|command.argv[3];
            int i=0,bit=1; for (;i<32;i++,bit<<=1) {
              if (grpmask&bit) {
                if (sprite_group_add(sprite_groupv+i,sprite)<0) {
                  sprite_kill_now(sprite);
                  return 0;
                }
              }
            }
          } break;
      }
    }
  }
  
  if (type->init&&(type->init(sprite,def,defc)<0)) {
    sprite_kill_now(sprite);
    return 0;
  }
  
  return sprite;
}
