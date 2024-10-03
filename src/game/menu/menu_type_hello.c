#include "game/penance.h"

/* Object definition.
 */
 
struct menu_hello {
  struct menu hdr;
  double alpha;
  int dismissed;
};

#define MENU ((struct menu_hello*)menu)

/* Delete.
 */
 
static void _hello_del(struct menu *menu) {
}

/* Init.
 */
 
static int _hello_init(struct menu *menu) {
  menu->opaque=1;
  MENU->alpha=0.0;
  egg_play_song(RID_song_penance_prima,0,1);
  return 0;
}

/* Input.
 */
 
static void _hello_input(struct menu *menu,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    if (MENU->dismissed) return;
    MENU->dismissed=1;
    MENU->alpha=1.0;
    menu->opaque=0;
    g.gameover=0;
    memset(g.candlev,0,sizeof(g.candlev));
    sprite_group_kill(GRP(HERO));
    maps_reset(g.rom,g.romc);
    g.playtime=0.0;
    penance_load_map(1,TRANSITION_CUT); // We do the transition.
    egg_play_song(RID_song_doors_without_walls,0,1);
  }
}

/* Update.
 */
 
static void _hello_update(struct menu *menu,double elapsed) {
  if (MENU->dismissed) {
    if ((MENU->alpha-=2.000*elapsed)<=0.0) {
      menu_pop(menu);
      return;
    }
  } else {
    if (MENU->alpha<1.0) {
      if ((MENU->alpha+=0.333*elapsed)>=1.0) {
        MENU->alpha=1.0;
      }
    }
  }
}

/* Render.
 */
 
static void _hello_render(struct menu *menu) {
  if (MENU->dismissed) {
    int alpha=(int)(MENU->alpha*255.0);
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
    graf_set_alpha(&g.graf,alpha);
    graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x887890ff);
  } else {
    egg_draw_clear(1,0x887890ff);
  }
  int texid_hello=texcache_get_image(&g.texcache,RID_image_hello);
  int hellow=0,helloh=0;
  egg_texture_get_status(&hellow,&helloh,texid_hello);
  graf_set_tint(&g.graf,0x201008ff);
  if (!MENU->dismissed) {
    int alpha=(int)(MENU->alpha*255.0);
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
    graf_set_alpha(&g.graf,alpha);
  }
  graf_draw_decal(&g.graf,texid_hello,44,36,0,0,hellow,helloh,0);
  graf_set_tint(&g.graf,0);
  if (!MENU->dismissed) {
    graf_set_alpha(&g.graf,0xff);
  }
  graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,RID_image_border),0,0,0,0,g.fbw,g.fbh,0);
}

/* Type definition.
 */
 
const struct menu_type menu_type_hello={
  .name="hello",
  .objlen=sizeof(struct menu_hello),
  .del=_hello_del,
  .init=_hello_init,
  .input=_hello_input,
  .update=_hello_update,
  .render=_hello_render,
};
