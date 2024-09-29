#include "penance.h"

struct globals g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  egg_texture_get_status(&g.fbw,&g.fbh,1);
  if ((g.fbw!=COLC*TILESIZE)||(g.fbh!=ROWC*TILESIZE)) {
    fprintf(stderr,"Invalid framebuffer size (%d,%d) for (%d,%d) cells of %dx%d pixels.\n",g.fbw,g.fbh,COLC,ROWC,TILESIZE,TILESIZE);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_witchy_0020)<0) return -1;
  if ((g.texid_tiles=egg_texture_new())<1) return -1;
  
  if (sprite_groups_init()<0) return -1;
  if (maps_reset(g.rom,g.romc)<0) return -1;
  if (penance_load_map(1)<0) return -1;
  
  srand_auto();
  
  return 0;
}

void egg_client_update(double elapsed) {

  /* Poll for input and dispatch to modal or hero.
   */
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    //TODO If there's a modal, send input to it instead of the hero.
    int i=GRP(HERO)->spritec;
    while (i-->0) sprite_hero_input(GRP(HERO)->spritev[i],input,g.pvinput);
    g.pvinput=input;
  }
  
  sprite_group_update(GRP(UPDATE),elapsed);
  sprite_group_kill(GRP(DEATHROW));
  penance_check_navigation();
}

void egg_client_render() {
  graf_reset(&g.graf);
  egg_draw_clear(1,0x201008ff);
  graf_draw_tile_buffer(&g.graf,g.texid_tiles,8,8,g.map->v,COLC,ROWC,COLC);
  sprite_group_render(GRP(VISIBLE),0,0);
  graf_flush(&g.graf);
}
