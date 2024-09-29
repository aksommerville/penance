#include "penance.h"

/* Load a map.
 */
 
static int penance_load_map_object(struct map *map) {
  if (!map) return -1;
  g.map=map;
  struct map_command_reader reader;
  map_command_reader_init_map(&reader,map);
  const uint8_t *arg;
  int argc,opcode;
  while ((argc=map_command_reader_next(&arg,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x20: { // image
          int imageid=(arg[0]<<8)|arg[1];
          if (imageid==g.map_imageid) continue;
          if (egg_texture_load_image(g.texid_tiles,imageid)<0) {
            fprintf(stderr,"Failed to load image:%d for map:%d\n",imageid,map->rid);
            return -2;
          }
        } break;
      case 0x21: { // hero
          if (GRP(HERO)->spritec) {
            fprintf(stderr,"Loading map:%d, ignoring hero spawn point because hero already exists.\n",map->rid);
          } else {
            double x=(double)arg[0]+0.5;
            double y=(double)arg[1]+0.5;
            struct sprite *sprite=sprite_spawn_with_type(x,y,&sprite_type_hero,0,0,0,0);
            if (!sprite) return -1;
          }
        } break;
      case 0x23: { // song
          int songid=(arg[0]<<8)|arg[1];
          egg_play_song(songid,0,1);
        } break;
    }
  }
  return 0;
}
 
int penance_load_map(int mapid) {
  struct map *map=map_by_id(mapid);
  if (!map) return -1;
  return penance_load_map_object(map);
}

int penance_navigate(int dx,int dy) {
  int nx=g.map->x+dx,ny=g.map->y+dy;
  struct map *map=map_by_location(nx,ny);
  if (!map) return -1;
  return penance_load_map_object(map);
}
