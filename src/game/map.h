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
};

int maps_reset(const void *rom,int romc);

struct map *map_by_id(int rid);
struct map *map_by_location(int x,int y);

struct map_command_reader {
  const uint8_t *v;
  int p,c;
};
void map_command_reader_init_serial(struct map_command_reader *reader,const void *src,int srcc); // commands only
void map_command_reader_init_res(struct map_command_reader *reader,const void *src,int srcc); // full resource
void map_command_reader_init_map(struct map_command_reader *reader,const struct map *map);
int map_command_reader_next(void *dstpp,int *opcode,struct map_command_reader *reader); // <0 when complete, 0 is legal

#endif
