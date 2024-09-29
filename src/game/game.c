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
          if (GRP(HERO)->spritec) break;
          double x=(double)arg[0]+0.5;
          double y=(double)arg[1]+0.5;
          struct sprite *sprite=sprite_spawn_with_type(x,y,&sprite_type_hero,0,0,0,0);
          if (!sprite) return -1;
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
  
  // If there's a hero, shuffle by a screen's width or height.
  // And retain her; we are about to kill every sprite.
  struct sprite *hero=0;
  if (GRP(HERO)->spritec>=1) {
    hero=GRP(HERO)->spritev[0];
    if (sprite_ref(hero)<0) return -1;
    hero->x-=dx*COLC;
    hero->y-=dy*ROWC;
  }
  
  // Drop all sprites. Except the hero if she exists.
  sprite_group_remove(GRP(KEEPALIVE),hero);
  sprite_group_kill(GRP(KEEPALIVE));
  sprite_group_add(GRP(KEEPALIVE),hero);
  sprite_del(hero);
  
  // Run all map commands and record it globally.
  if (penance_load_map_object(map)<0) return -1;
  
  // Hero might have extra things to do when the map changes.
  if (hero) hero_map_changed(hero);
  
  return 0;
}

/* Real quick, does a neighbor map exist?
 */
 
static inline int neighbor_map_exists(int dx,int dy) {
  return map_by_location(g.map->x+dx,g.map->y+dy)?1:0;
}

/* Check navigation. Called every update.
 */
 
int penance_check_navigation() {

  if (g.mapid_load_soon) {
    int mapid=g.mapid_load_soon;
    g.mapid_load_soon=0;
    return penance_load_map(mapid);
  }

  if (GRP(HERO)->spritec<1) return 0;
  struct sprite *hero=GRP(HERO)->spritev[0];
  
  /* If the hero's midpoint goes offscreen, and a map exists over there, do it.
   */
  if ((hero->x<0.0)&&neighbor_map_exists(-1,0)) return penance_navigate(-1,0);
  if ((hero->y<0.0)&&neighbor_map_exists(0,-1)) return penance_navigate(0,-1);
  if ((hero->x>COLC)&&neighbor_map_exists(1,0)) return penance_navigate(1,0);
  if ((hero->y>ROWC)&&neighbor_map_exists(0,1)) return penance_navigate(0,1);
  
  //TODO doors. i want to see if we can get by without
  return 0;
}
