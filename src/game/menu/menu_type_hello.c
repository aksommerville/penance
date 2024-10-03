#include "game/penance.h"

#define STAGE_FADE_IN_TITLE 0
#define STAGE_FADE_OUT_TITLE 1
#define STAGE_SPOOKY_SILHOUETTE 2
#define STAGE_PLEADY_WOLF 3
#define STAGE_MENACE 4
#define STAGE_AWAKE 5
#define STAGE_ENLIST 6

#define FOR_EACH_STAGE \
  _(FADE_IN_TITLE) \
  _(FADE_OUT_TITLE) \
  _(SPOOKY_SILHOUETTE) \
  _(PLEADY_WOLF) \
  _(MENACE) \
  _(AWAKE) \
  _(ENLIST)

/* Object definition.
 */
 
struct menu_hello {
  struct menu hdr;
  int stage;
  double stageclock;
};

#define MENU ((struct menu_hello*)menu)

/* Delete.
 */
 
static void _hello_del(struct menu *menu) {
}

/* FADE_IN_TITLE: Start with light background and border, then title slide fades in and holds.
 */
 
static void hello_begin_FADE_IN_TITLE(struct menu *menu) {
  MENU->stage=STAGE_FADE_IN_TITLE;
  MENU->stageclock=10.0;
}

static void hello_render_FADE_IN_TITLE(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x887890ff);
  int texid_hello=texcache_get_image(&g.texcache,RID_image_hello);
  int hellow=0,helloh=0;
  egg_texture_get_status(&hellow,&helloh,texid_hello);
  graf_set_tint(&g.graf,0x201008ff);
  int alpha=(int)((9.5-MENU->stageclock)*255.0);
  if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
  graf_set_alpha(&g.graf,alpha);
  graf_draw_decal(&g.graf,texid_hello,44,36,0,0,hellow,helloh,0);
  graf_set_tint(&g.graf,0);
  graf_set_alpha(&g.graf,0xff);
}

/* FADE_OUT_TITLE: Keep title in place as with FADE_IN_TITLE, and slide the background color to match it.
 */
 
static void hello_begin_FADE_OUT_TITLE(struct menu *menu) {
  MENU->stage=STAGE_FADE_OUT_TITLE;
  MENU->stageclock=1.0;
}

static void hello_render_FADE_OUT_TITLE(struct menu *menu) {

  // Background color: 887890 => 201008
  double fr=MENU->stageclock/1.0;
  double to=1.0-fr;
  int r=(int)(fr*0x88+to*0x20);
  int G=(int)(fr*0x78+to*0x10);
  int b=(int)(fr*0x90+to*0x08);
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,(r<<24)|(G<<16)|(b<<8)|0xff);

  int texid_hello=texcache_get_image(&g.texcache,RID_image_hello);
  int hellow=0,helloh=0;
  egg_texture_get_status(&hellow,&helloh,texid_hello);
  graf_set_tint(&g.graf,0x201008ff);
  graf_draw_decal(&g.graf,texid_hello,44,36,0,0,hellow,helloh,0);
  graf_set_tint(&g.graf,0);
}

/* SPOOKY_SILHOUETTE: The full moon rises and we see a silhouette of the evil witch against it.
 */
 
static void hello_begin_SPOOKY_SILHOUETTE(struct menu *menu) {
  MENU->stage=STAGE_SPOOKY_SILHOUETTE;
  MENU->stageclock=10.0;
}

static void hello_render_SPOOKY_SILHOUETTE(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x201008ff);
  int texid_bits=texcache_get_image(&g.texcache,RID_image_hello_bits);
  double norm=(10.0-MENU->stageclock)/10.0; // 0 => 1
  
  { // Moon.
    int16_t dsty=10;
    if (norm<0.75) dsty+=(int16_t)((0.75-norm)*200.0);
    graf_set_tint(&g.graf,0xc0c0e0ff);
    graf_draw_decal(&g.graf,texid_bits,(g.fbw>>1)-64,dsty,0,0,128,128,0);
    graf_set_tint(&g.graf,0);
  }
  
  { // Silhouette.
    graf_set_tint(&g.graf,0x201008ff);
    graf_draw_decal(&g.graf,texid_bits,(g.fbw>>1)-64,g.fbh-256,128,0,128,256,0);
    if (norm>=0.85) {
      graf_draw_decal(&g.graf,texid_bits,150,70,128,0,80,40,0);
    }
    graf_set_tint(&g.graf,0);
  }
}

/* PLEADY_WOLF: The poor werewolf begs for mercy.
 */
 
static void hello_begin_PLEADY_WOLF(struct menu *menu) {
  MENU->stage=STAGE_PLEADY_WOLF;
  MENU->stageclock=5.0;
}

static void hello_render_PLEADY_WOLF(struct menu *menu) {
  int texid_bits=texcache_get_image(&g.texcache,RID_image_hello_bits);
  int animframe=((int)(MENU->stageclock*2.0))&1;
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x201008ff);
  graf_draw_decal(&g.graf,texid_bits,150,100,256+animframe*64,160,64,48,0);
  graf_draw_decal(&g.graf,texid_bits,48,24,256,0,256,160,0);
  graf_draw_decal(&g.graf,texid_bits,185,110,256+animframe*64,208,64,48,0);
}

/* MENACE: Silhouette resolves into evil Dot.
 */
 
static void hello_begin_MENACE(struct menu *menu) {
  MENU->stage=STAGE_MENACE;
  MENU->stageclock=5.0;
}

static void hello_render_MENACE(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x201008ff);
  int texid_bits=texcache_get_image(&g.texcache,RID_image_hello_bits);
  
  { // Moon.
    int16_t dsty=10;
    graf_set_tint(&g.graf,0xc0c0e0ff);
    graf_draw_decal(&g.graf,texid_bits,(g.fbw>>1)-64,dsty,0,0,128,128,0);
    graf_set_tint(&g.graf,0);
  }
  
  { // Silhouette.
    int alpha=(MENU->stageclock/5.0)*255.0;
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
    graf_set_tint(&g.graf,0x20100800|alpha);
    graf_draw_decal(&g.graf,texid_bits,(g.fbw>>1)-64,g.fbh-256,128,0,128,256,0);
    graf_set_tint(&g.graf,0);
  }
}

/* AWAKE: Restore light bg color, Dot wakes up with a start, then looks at her hands.
 */
 
static void hello_begin_AWAKE(struct menu *menu) {
  MENU->stage=STAGE_AWAKE;
  MENU->stageclock=6.0;
}

static void hello_render_AWAKE(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x887890ff);
  int texid_bits=texcache_get_image(&g.texcache,RID_image_hello_bits);
  double norm=(8.0-MENU->stageclock)/8.0; // 0 => 1
  
  { // Main pic, first frame. Second frame overlays it.
    graf_draw_decal(&g.graf,texid_bits,0,0,0,256,256,176,0);
  }
  
  if (norm>=0.500) {
    graf_draw_decal(&g.graf,texid_bits,128,0,256,256,128,128,0);
  }
}

/* ENLIST: Cut to the convent interior. Dot walks in the left door, talks to Mother Superior, and exits the right door as a nun.
 */
 
static void hello_begin_ENLIST(struct menu *menu) {
  MENU->stage=STAGE_ENLIST;
  MENU->stageclock=10.0;
}

static void hello_render_ENLIST(struct menu *menu) {
  int texid_bits=texcache_get_image(&g.texcache,RID_image_hello_bits);
  double norm=(10.0-MENU->stageclock)/10.0; // 0 => 1
  graf_draw_decal(&g.graf,texid_bits,0,0,0,432,320,176,0); // Floor is 112 pixels down.
  
  { // Mother Superior.
    uint8_t xform=(norm<0.700)?0:EGG_XFORM_XREV; // Facing left naturally.
    graf_draw_decal(&g.graf,texid_bits,180,80,0,152,16,32,xform);
  }
  
  { // Dot.
    int16_t dstx=60;
    int16_t dsty=88;
    int16_t srcx=0;
    int16_t srcy=128;
    if (norm<0.250) { // Walking in.
      dstx+=(int16_t)((norm/0.250)*100.0);
      if (((int)(MENU->stageclock*4.0))&1) srcx+=16;
    } else if (norm<0.400) {
      dstx=160;
    } else if (norm<0.600) {
      dstx=160;
      srcx=32;
    } else {
      dstx=160;
      srcx=32;
      if (((int)(MENU->stageclock*4.0))&1) srcx+=16;
      dstx+=(int16_t)(((norm-0.600)/0.400)*92.0);
    }
    graf_draw_decal(&g.graf,texid_bits,dstx,dsty,srcx,srcy,16,24,0);
  }
  
  { // And as we approach the end, fade back to the background color.
    if (norm>=0.900) {
      int alpha=(int)((norm-0.900)*2550.0);
      if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
      graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x88789000|alpha);
    }
  }
}

/* Init.
 */
 
static int _hello_init(struct menu *menu) {
  menu->opaque=1;
  egg_play_song(RID_song_penance_prima,0,1);
  hello_begin_FADE_IN_TITLE(menu);
  //hello_begin_ENLIST(menu);//XXX
  return 0;
}

/* Input.
 */
 
static void _hello_input(struct menu *menu,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    g.gameover=0;
    memset(g.candlev,0,sizeof(g.candlev));
    sprite_group_kill(GRP(HERO));
    maps_reset(g.rom,g.romc);
    g.playtime=0.0;
    penance_load_map(1,TRANSITION_CUT);
    egg_play_song(RID_song_doors_without_walls,0,1);
    menu_pop(menu);
    return;
  }
}

/* Update.
 */
 
static void _hello_update(struct menu *menu,double elapsed) {
  if ((MENU->stageclock-=elapsed)<=0.0) {
    MENU->stage++;
    switch (MENU->stage) {
      #define _(tag) case STAGE_##tag: hello_begin_##tag(menu); break;
      FOR_EACH_STAGE
      #undef _
      default: hello_begin_FADE_IN_TITLE(menu); break;
    }
  }
}

/* Render.
 */
 
static void _hello_render(struct menu *menu) {
  /*XXX
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
  */
  switch (MENU->stage) {
    #define _(tag) case STAGE_##tag: hello_render_##tag(menu); break;
    FOR_EACH_STAGE
    #undef _
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
