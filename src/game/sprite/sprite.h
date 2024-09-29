/* sprite.h
 * Generic sprites.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;
struct sprite_group;

struct sprite {
  const struct sprite_type *type; // REQUIRED
  int refc;
  double x,y; // In tiles, fractional.
  int imageid;
  uint8_t tileid;
  uint8_t xform;
  int layer;
  struct sprite_group **groupv;
  int groupc,groupa;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  
  /* (arg) from the spawn point, and (def) from the resource.
   * Generic fields from (def), and location, are already applied when this gets called.
   */
  int (*init)(struct sprite *sprite,const uint8_t *arg,int argc,const uint8_t *def,int defc);
  
  void (*update)(struct sprite *sprite,double elapsed);
  
  // If you implement (render) we do not use (imageid,tileid,xform) generically.
  void (*render)(struct sprite *sprite,int16_t addx,int16_t addy);
};

struct sprite_group {
  int refc; // 0=immortal
  struct sprite **spritev;
  int spritec,spritea;
  int mode;
  int sortdir; // For RENDER only.
};

/* Generic sprite.
 *****************************************************************************/

/* You don't normally use these directly.
 * "spawn" is preferred for creating a sprite, and "kill_later" for deleting (deferred).
 * DO NOT delete a sprite during update, if there's a chance it is participating in the update.
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(const struct sprite_type *type);
int sprite_ref(struct sprite *sprite);

/* Create and initialize a sprite, return a WEAK reference.
 */
struct sprite *sprite_spawn_for_map(
  int x,int y,
  const uint8_t *arg,int argc
);
struct sprite *sprite_spawn_with_type(
  double x,double y,
  const struct sprite_type *type,
  const uint8_t *arg,int argc,
  const uint8_t *def,int defc
);

/* Groups.
 ***********************************************************************/

#define SPRITE_GROUP_MODE_ADDR   0 /* Default, sort by address. */
#define SPRITE_GROUP_MODE_RENDER 1 /* Sort by (layer,y). */
#define SPRITE_GROUP_MODE_SINGLE 2 /* Adding one drops any existing. Must be immortal or have some external strong reference. */
 
#define SPRITE_GROUP_KEEPALIVE 0 /* Special, contains all active sprites. */
#define SPRITE_GROUP_DEATHROW  1 /* Special, all these sprites get killed at the end of the update cycle. */
#define SPRITE_GROUP_UPDATE    2
#define SPRITE_GROUP_VISIBLE   3
#define SPRITE_GROUP_HERO      4
#define SPRITE_GROUP_HAZARD    5
#define SPRITE_GROUP_SOLID     6

extern struct sprite_group sprite_groupv[32];

#define GRP(tag) (sprite_groupv+SPRITE_GROUP_##tag)

int sprite_groups_init();

void sprite_group_del(struct sprite_group *group);
struct sprite_group *sprite_group_new();
int sprite_group_ref(struct sprite_group *group);

int sprite_group_has(const struct sprite_group *group,const struct sprite *sprite);
int sprite_group_add(struct sprite_group *group,struct sprite *sprite);
int sprite_group_remove(struct sprite_group *group,struct sprite *sprite);

void sprite_group_clear(struct sprite_group *group);
void sprite_group_kill(struct sprite_group *group);
void sprite_kill_now(struct sprite *sprite); // Remove from all groups immediately, typically deletes the sprite.
void sprite_kill_later(struct sprite *sprite); // Convenience, just adds to DEATHROW.

void sprite_group_update(struct sprite_group *group,double elapsed);
void sprite_group_render(struct sprite_group *group,int addx,int addy);

/* Types.
 *********************************************************************/
 
extern const struct sprite_type sprite_type_dummy; // 0
extern const struct sprite_type sprite_type_hero; // 1

const struct sprite_type *sprite_type_by_id(int id);

#endif
