/* map.h
 * All of the game's map resources are instantiated at boot, and organized geographically.
 * Map instances are mutable per session.
 */
 
#ifndef MAP_H
#define MAP_H

// Maximum size of whole world, in screens. Must agree with src/editor/js/MapRes.js.
#define LONG_LIMIT 32
#define LAT_LIMIT 32

// Maximum ID of a tilesheet resource, so we don't need to grow the list dynamically.
#define TILESHEET_LIMIT 32

struct map {
  uint8_t v[COLC*ROWC];
  const uint8_t *serial;
  int serialc; // >=COLC*ROWC
  const uint8_t *tileprops;
  uint16_t rid;
  uint8_t x,y;
  int stump; // True if there's a stump record for it. (ie false if there is a stump but our buffer filled)
};

int maps_reset(const void *rom,int romc);

struct map *map_by_id(int rid);
struct map *map_by_location(int x,int y);

/* Since we're in there anyway for maps and tilesheets, we also cache all the sprite resources at boot.
 */
int maps_get_sprite(void *dstpp,int rid);

/* World bounds are in maps.
 * Stumps each have a position in global map coords, and index from zero.
 */
void maps_get_world_bounds(int *x,int *y,int *w,int *h);
int maps_get_stump(int *x,int *y,int p);

#endif
