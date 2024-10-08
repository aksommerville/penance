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
  
  /* Generic fields from (def), and location, are already applied when this gets called.
   * I'm not providing a means to pass data via spawn points. Make a separate sprite resource for everything.
   */
  int (*init)(struct sprite *sprite,const uint8_t *def,int defc);
  
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
  const uint8_t *def,int defc
);
struct sprite *sprite_spawn_with_type(
  double x,double y,
  const struct sprite_type *type,
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

#define FOR_EACH_SPRITE_GROUP \
  _(UPDATE) \
  _(VISIBLE) \
  _(HERO) \
  _(HAZARD) \
  _(SOLID)

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
 
extern const struct sprite_type sprite_type_dummy;
extern const struct sprite_type sprite_type_hero;
extern const struct sprite_type sprite_type_fleshpuppet;
extern const struct sprite_type sprite_type_fireball;
extern const struct sprite_type sprite_type_bonfire;
extern const struct sprite_type sprite_type_candle;
extern const struct sprite_type sprite_type_terminator;
extern const struct sprite_type sprite_type_wind;
extern const struct sprite_type sprite_type_dualephant;
extern const struct sprite_type sprite_type_missile;
extern const struct sprite_type sprite_type_treadle;
extern const struct sprite_type sprite_type_lock;
extern const struct sprite_type sprite_type_scoreboard;
extern const struct sprite_type sprite_type_fireworks;
extern const struct sprite_type sprite_type_neon;
extern const struct sprite_type sprite_type_raccoon;
extern const struct sprite_type sprite_type_bug;
extern const struct sprite_type sprite_type_target;
extern const struct sprite_type sprite_type_cannon;
extern const struct sprite_type sprite_type_werewolf;

// ids get assigned in this order. Use the same as the ^ declarations above.
#define FOR_EACH_SPRITE_TYPE \
  _(dummy) \
  _(hero) \
  _(fleshpuppet) \
  _(fireball) \
  _(bonfire) \
  _(candle) \
  _(terminator) \
  _(wind) \
  _(dualephant) \
  _(missile) \
  _(treadle) \
  _(lock) \
  _(scoreboard) \
  _(fireworks) \
  _(neon) \
  _(raccoon) \
  _(bug) \
  _(target) \
  _(cannon) \
  _(werewolf)

const struct sprite_type *sprite_type_by_id(int id);

void sprite_fireball_set_direction(struct sprite *sprite,double dx,double dy);
void sprite_fireball_blow_out(struct sprite *sprite);
void sprite_bonfire_set_ttl(struct sprite *sprite,double ttl);
void sprite_bonfire_blow(struct sprite *sprite,int dx); // Must call every frame.
int sprite_candle_light(struct sprite *sprite); // 0 if invalid or already lit
int sprite_candle_is_lit(const struct sprite *sprite);
void sprite_missile_target_hero(struct sprite *sprite);
void sprite_missile_launch(struct sprite *sprite,double dx,double dy);
void sprite_lock_set_lamp(struct sprite *sprite,int index,int value);
int sprite_lock_is_locked(const struct sprite *sprite);
void sprite_lock_unlock_all();
void spawn_bugs();

#endif
