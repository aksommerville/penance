#include "penance.h"

/* Load a map.
 */
 
static int penance_load_map_object(struct map *map) {
  if (!map) return -1;
  g.map=map;
  struct rom_command_reader reader={.v=map->serial+COLC*ROWC,.c=map->serialc-COLC*ROWC};
  struct rom_command command;
  while (rom_command_reader_next(&command,&reader)>0) {
    switch (command.opcode) {
      case 0x20: { // image
          int imageid=(command.argv[0]<<8)|command.argv[1];
          if (imageid==g.map_imageid) continue;
          if (egg_texture_load_image(g.texid_tiles,imageid)<0) {
            fprintf(stderr,"Failed to load image:%d for map:%d\n",imageid,map->rid);
            return -2;
          }
        } break;
      case 0x21: { // hero
          if (GRP(HERO)->spritec) break;
          double x=(double)command.argv[0]+0.5;
          double y=(double)command.argv[1]+0.5;
          struct sprite *sprite=sprite_spawn_with_type(x,y,&sprite_type_hero,0,0);
          if (!sprite) return -1;
        } break;
      case 0x40: { // sprite
          double x=(double)command.argv[0]+0.5;
          double y=(double)command.argv[1]+0.5;
          int rid=(command.argv[2]<<8)|command.argv[3];
          const uint8_t *serial=0;
          int serialc=maps_get_sprite(&serial,rid);
          struct sprite *sprite=sprite_spawn_for_map(x,y,serial,serialc);
          if (!sprite) break; // It's ok, sprites are allowed to fail to abort construction.
        } break;
    }
  }
  spawn_bugs();
  return 0;
}
 
int penance_load_map(int mapid,int transition) {
  struct map *map=map_by_id(mapid);
  if (!map) return -1;
  
  struct sprite *hero=0;
  if (GRP(HERO)->spritec>=1) {
    hero=GRP(HERO)->spritev[0];
    if (sprite_ref(hero)<0) return -1;
    sprite_group_remove(GRP(KEEPALIVE),hero);
    sprite_group_remove(GRP(VISIBLE),hero);
  }

  if (transition) {
    if (g.transition_pvbits<=0) {
      g.transition_pvbits=egg_texture_new();
      if (egg_texture_load_raw(g.transition_pvbits,g.fbw,g.fbh,g.fbw<<2,0,0)<0) return -1;
    }
    penance_render_game_to(g.transition_pvbits);
  }
  
  sprite_group_kill(GRP(KEEPALIVE));
  sprite_group_add(GRP(KEEPALIVE),hero);
  sprite_group_add(GRP(VISIBLE),hero);
  sprite_del(hero);
  
  if (penance_load_map_object(map)<0) return -1;
  
  if (hero) hero_map_changed(hero);
  
  g.transition_clock=0.0;
  g.transition=transition;
  
  return 0;
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
    sprite_group_remove(GRP(KEEPALIVE),hero);
    sprite_group_remove(GRP(VISIBLE),hero);
    hero->x-=dx*COLC;
    hero->y-=dy*ROWC;
  }
  
  // Capture the scene for transition's "from" bits.
  if (g.transition_pvbits<=0) {
    g.transition_pvbits=egg_texture_new();
    if (egg_texture_load_raw(g.transition_pvbits,g.fbw,g.fbh,g.fbw<<2,0,0)<0) return -1;
  }
  penance_render_game_to(g.transition_pvbits);
  
  // Drop all sprites. Except the hero if she exists.
  sprite_group_kill(GRP(KEEPALIVE));
  sprite_group_add(GRP(KEEPALIVE),hero);
  sprite_group_add(GRP(VISIBLE),hero);
  sprite_del(hero);
  
  // Run all map commands and record it globally.
  if (penance_load_map_object(map)<0) return -1;
  
  // Hero might have extra things to do when the map changes.
  if (hero) hero_map_changed(hero);
  
  // Prepare transition.
  g.transition_clock=0;
  if (dx<0) g.transition=TRANSITION_PAN_LEFT;
  else if (dx>0) g.transition=TRANSITION_PAN_RIGHT;
  else if (dy<0) g.transition=TRANSITION_PAN_UP;
  else g.transition=TRANSITION_PAN_DOWN;
  
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
    return penance_load_map(mapid,TRANSITION_FADE);
  }
  
  // Forbid navigation during a transition.
  if (g.transition) return 0;

  if (GRP(HERO)->spritec<1) return 0;
  struct sprite *hero=GRP(HERO)->spritev[0];
  
  /* If the hero's midpoint goes offscreen, and a map exists over there, do it.
   */
  if ((hero->x<0.0)&&neighbor_map_exists(-1,0)) return penance_navigate(-1,0);
  if ((hero->y<0.0)&&neighbor_map_exists(0,-1)) return penance_navigate(0,-1);
  if ((hero->x>COLC)&&neighbor_map_exists(1,0)) return penance_navigate(1,0);
  if ((hero->y>ROWC)&&neighbor_map_exists(0,1)) return penance_navigate(0,1);
  
  return 0;
}

/* End game (success).
 */
 
void penance_gameover() {
  g.gameover=1;
  if (g.playtime<g.besttime) {
    g.besttime=g.playtime;
    penance_save_hiscore();
  }
  if (ENABLE_MUSIC) egg_play_song(RID_song_take_wing,0,0);
  if (GRP(HERO)->spritec>=1) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    hero_map_changed(hero);
  }
}
