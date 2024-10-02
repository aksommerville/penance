#include "penance.h"

struct globals g={0};

void egg_client_quit(int status) {
}

/* Init.
 */

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
  
  penance_load_hiscore();
  if (sprite_groups_init()<0) return -1;
  if (maps_reset(g.rom,g.romc)<0) return -1;
  
  //if (penance_load_map(1)<0) return -1;
  if (!menu_push(&menu_type_hello)) return -1;
  
  srand_auto();
  
  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {

  /* Poll for input and dispatch to menu or hero.
   */
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if (g.menuc) {
      struct menu *menu=g.menuv[g.menuc-1];
      menu->type->input(menu,input,g.pvinput);
    } else {
      int i=GRP(HERO)->spritec;
      while (i-->0) sprite_hero_input(GRP(HERO)->spritev[i],input,g.pvinput);
    }
    g.pvinput=input;
  }
  
  /* Only one thing gets polled for general update.
   * Either the top menu, or the game.
   */
  if (g.menuc) {
    struct menu *menu=g.menuv[g.menuc-1];
    menu->type->update(menu,elapsed);
  } else {
    if (!g.gameover) g.playtime+=elapsed;
    sprite_group_update(GRP(UPDATE),elapsed);
    sprite_group_kill(GRP(DEATHROW));
    penance_check_navigation();
  }
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  
  // If there's an opaque menu, note it.
  int opaquep=-1;
  int i=g.menuc;
  while (i-->0) {
    if (g.menuv[i]->opaque) {
      opaquep=i;
      break;
    }
  }
  
  // No opaque menu, draw the game. There is always a game, even during hello and farewell.
  if (opaquep<0) {
    graf_draw_tile_buffer(&g.graf,g.texid_tiles,8,8,g.map->v,COLC,ROWC,COLC);
    sprite_group_render(GRP(VISIBLE),0,0);
    if (GRP(HERO)->spritec>=1) hero_draw_overlay(GRP(HERO)->spritev[0],0,0);
  }
  
  // Draw each menu from (opaquep) up.
  if (opaquep<0) opaquep=0;
  for (;opaquep<g.menuc;opaquep++) {
    struct menu *menu=g.menuv[opaquep];
    menu->type->render(menu);
  }
  
  graf_flush(&g.graf);
}

/* High score.
 */
 
void penance_load_hiscore() {
  g.besttime=999999.999;
  char tmp[9]; // MM:SS.mmm
  int tmpc=egg_store_get(tmp,sizeof(tmp),"besttime",8);
  if (tmpc==9) {
    if (
      ((tmp[0]>='0')&&(tmp[0]<='9'))&&
      ((tmp[1]>='0')&&(tmp[1]<='9'))&&
      (tmp[2]==':')&&
      ((tmp[3]>='0')&&(tmp[3]<='9'))&&
      ((tmp[4]>='0')&&(tmp[4]<='9'))&&
      (tmp[5]=='.')&&
      ((tmp[6]>='0')&&(tmp[6]<='9'))&&
      ((tmp[7]>='0')&&(tmp[7]<='9'))&&
      ((tmp[8]>='0')&&(tmp[8]<='9'))
    ) {
      int min=((tmp[0]-'0')*10)+tmp[1]-'0';
      int sec=((tmp[3]-'0')*10)+tmp[4]-'0';
      int mil=((tmp[6]-'0')*100)+((tmp[7]-'0')*10)+tmp[8]-'0';
      g.besttime=min*60+sec+(mil/1000.0);
    }
  }
}

void penance_save_hiscore() {
  int mil=(int)(g.besttime*1000.0);
  int sec=mil/1000; mil%=1000;
  int min=sec/60; sec%=60;
  if (min>99) min=sec=99;
  char tmp[9]={
    '0'+min/10,
    '0'+min%10,
    ':',
    '0'+sec/10,
    '0'+sec%10,
    '.',
    '0'+mil/100,
    '0'+(mil/10)%10,
    '0'+mil%10,
  };
  egg_store_set("besttime",8,tmp,9);
}
