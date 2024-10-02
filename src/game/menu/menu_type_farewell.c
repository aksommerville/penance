#include "game/penance.h"

/* Object definition.
 */
 
#define FADE_COLOR 0x10080400
 
#define FAREWELL_STAGE_FADEOUT1 1 /* Fade to black-ish, with underlying game intact. */
#define FAREWELL_STAGE_FLYAWAY  2 /* Dot leaves the temple. */
#define FAREWELL_STAGE_FLYHOME  3 /* Dot arrives at the convent. */
#define FAREWELL_STAGE_INSIDE   4 /* At the convent, return habit and depart. */
#define FAREWELL_STAGE_FINAL    5

// Text appears at the bottom of the screen on its own timing, regardless of stage.
// A well-behaved Egg game would store the text in strings resources, but meh.
// Aim for about 40 seconds' worth of subtitles, that's when the visual content ends.
static const struct farewell_subtitle {
  double duration;
  const char *src;
} farewell_subtitlev[]={
  { 5.0,""},
  { 7.0,"Starring\n  Dot Vine"},
  { 2.0,""},
  { 7.0,"Code, graphics, music\n  AK Sommerville"},
  { 2.0,""},
  {10.0,"Written in 9 days for\nGDEX Game Jam 2024"},
  { 2.0,""},
  { 7.0,"Thanks for playing!\n-AK and Dot"},
};
 
struct menu_farewell {
  struct menu hdr;
  int stage;
  double stageclock; // Counts down.
  int subtitlep;
  double subtitleclock;
  int subtitle_texid;
  int subtitlew,subtitleh;
};

#define MENU ((struct menu_farewell*)menu)

/* Delete.
 */
 
static void _farewell_del(struct menu *menu) {
  egg_texture_del(MENU->subtitle_texid);
}

/* Init.
 */
 
static int _farewell_init(struct menu *menu) {
  MENU->stage=FAREWELL_STAGE_FADEOUT1;
  MENU->stageclock=0.5;
  MENU->subtitlep=-1; // Let it advance to zero right away.
  return 0;
}

/* Input.
 */
 
static void _farewell_input(struct menu *menu,int input,int pvinput) {
  // If you make it to the FINAL stage, you're allowed to quit.
  if (MENU->stage==FAREWELL_STAGE_FINAL) {
    if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
      menu_push(&menu_type_hello);
      menu_pop(menu);
      return;
    }
  }
}

/* Advance to next stage.
 */
 
static void farewell_advance(struct menu *menu) {
  MENU->stage++;
  switch (MENU->stage) {
    // At the current timing (10,10,20, and 10 from sprite_hero), we enter FINAL with about 8 seconds left in the song. Feels good.
    case FAREWELL_STAGE_FLYAWAY: {
        MENU->stageclock=10.0;
        menu->opaque=1; // After FADEOUT1, we don't need the game rendered anymore.
      } break;
    case FAREWELL_STAGE_FLYHOME: {
        MENU->stageclock=10.0;
      } break;
    case FAREWELL_STAGE_INSIDE: {
        MENU->stageclock=20.0;
      } break;
    case FAREWELL_STAGE_FINAL: {
        MENU->stageclock=60.0;
      } break;
    default: {
        MENU->stageclock=50.0; // If we repeat, don't fade back in, cheat the clock down a little.
      }
  }
}

/* Update.
 */
 
static void _farewell_update(struct menu *menu,double elapsed) {
  if ((MENU->stageclock-=elapsed)<=0.0) farewell_advance(menu);
  if ((MENU->subtitleclock-=elapsed)<=0.0) {
    egg_texture_del(MENU->subtitle_texid);
    MENU->subtitle_texid=0;
    MENU->subtitlep++;
    if ((MENU->subtitlep>=0)&&(MENU->subtitlep<sizeof(farewell_subtitlev)/sizeof(farewell_subtitlev[0]))) {
      const struct farewell_subtitle *st=farewell_subtitlev+MENU->subtitlep;
      MENU->subtitleclock+=st->duration;
      if (st->src&&st->src[0]) {
        MENU->subtitle_texid=font_tex_multiline(g.font,st->src,-1,g.fbw,g.fbh-136,0xd0c8c0ff);
        egg_texture_get_status(&MENU->subtitlew,&MENU->subtitleh,MENU->subtitle_texid);
      }
    } else {
      MENU->subtitleclock=999.0;
    }
  }
}

/* FADEOUT1
 */
 
static void farewell_render_FADEOUT1(struct menu *menu) {
  int alpha=(int)(((0.5-MENU->stageclock)/0.5)*255.0);
  if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|alpha);
}

/* FLYAWAY
 */
 
static void farewell_render_FLYAWAY(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|0xff);
  int texid=texcache_get_image(&g.texcache,RID_image_farewell);
  graf_draw_decal(&g.graf,texid,0,8,0,0,320,128,0);
  
  // Dot flying.
  int16_t doty=28;
  int16_t dotx=(int16_t)((MENU->stageclock*256.0)/10.0);
  uint8_t dottile=0x0c+((int)((fmod(MENU->stageclock,0.750)/0.750)*4.0)&3);
  graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_hero),dotx,doty,dottile,0);
  
  // Fade out fore and aft.
  int fadealpha=0;
  if (MENU->stageclock>9.5) fadealpha=(MENU->stageclock-9.5)*512.0;
  else if (MENU->stageclock<0.5) fadealpha=(0.5-MENU->stageclock)*512.0;
  if (fadealpha>0xff) fadealpha=0xff;
  if (fadealpha>0) graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|fadealpha);
}

/* FLYHOME
 */
 
static void farewell_render_FLYHOME(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|0xff);
  int texid=texcache_get_image(&g.texcache,RID_image_farewell);
  graf_draw_decal(&g.graf,texid,0,8,0,128,320,128,0);
  
  // Dot flying.
  int16_t doty=28;
  int16_t dotx=64+(int16_t)((MENU->stageclock*256.0)/10.0);
  if (dotx<130) doty+=130-dotx;
  uint8_t dottile=0x0c+((int)((fmod(MENU->stageclock,0.750)/0.750)*4.0)&3);
  graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_hero),dotx,doty,dottile,0);
  
  // Fade out fore and aft.
  int fadealpha=0;
  if (MENU->stageclock>9.5) fadealpha=(MENU->stageclock-9.5)*512.0;
  else if (MENU->stageclock<0.5) fadealpha=(0.5-MENU->stageclock)*512.0;
  if (fadealpha>0xff) fadealpha=0xff;
  if (fadealpha>0) graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|fadealpha);
}

/* INSIDE
 * Timing:
 *   20 Start
 *   19.5 Fade in complete
 *   17.0 Transfer habit
 *   16.0 Bow
 *   14.0 Bow complete
 *   12.0 Begin walking. Dot faces left
 *    0.5 Fade out
 */
 
static void farewell_render_INSIDE(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|0xff);
  int texid=texcache_get_image(&g.texcache,RID_image_farewell);
  graf_draw_decal(&g.graf,texid,0,8,0,256,320,128,0);
  
  // Mother Superior is 16x32 and has 3 frames.
  {
    int msx=0; // 0=plain, 16=holding habit, 32=bowing
    if (MENU->stageclock<14.0) msx=16;
    else if (MENU->stageclock<16.0) msx=32;
    else if (MENU->stageclock<17.0) msx=16;
    graf_draw_decal(&g.graf,texid,200,72,msx,416,16,32,0);
  }
  
  // Dot to her left, and walks away after some time.
  {
    int dotx=180; // End around 60.
    int srcx=0; // 0=holding habit, 16=neutral (also walk), 32=bow, 48=walk
    int xform=0; // Right is natural.
    if (MENU->stageclock<12.0) { // Walking.
      if (((int)(MENU->stageclock*5.0))&1) srcx=48;
      else srcx=16;
      xform=EGG_XFORM_XREV;
      dotx-=(int)(((12.0-MENU->stageclock)*120.0)/12.0);
    } else if (MENU->stageclock<14.0) { // Done bowing, standing around awkwardly.
      srcx=16;
    } else if (MENU->stageclock<16.0) { // Bowing
      srcx=32;
    } else if (MENU->stageclock<17.0) { // Habit transferred.
      srcx=16;
    }
    graf_draw_decal(&g.graf,texid,dotx,72,srcx,384,16,32,xform);
  }
  
  // Fade out fore and aft.
  int fadealpha=0;
  if (MENU->stageclock>19.5) fadealpha=(MENU->stageclock-9.5)*512.0;
  else if (MENU->stageclock<0.5) fadealpha=(0.5-MENU->stageclock)*512.0;
  if (fadealpha>0xff) fadealpha=0xff;
  if (fadealpha>0) graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|fadealpha);
}

/* FINAL
 */
 
static void farewell_render_FINAL(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|0xff);
  int texid=texcache_get_image(&g.texcache,RID_image_farewell);
  graf_draw_decal(&g.graf,texid,(g.fbw>>1)-24,(g.fbh>>1)-32,64,384,48,64,0);
  
  // Fade out fore only.
  int fadealpha=0;
  if (MENU->stageclock>59.5) fadealpha=(MENU->stageclock-9.5)*512.0;
  if (fadealpha>0xff) fadealpha=0xff;
  if (fadealpha>0) graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,FADE_COLOR|fadealpha);
}

/* Render, dispatch.
 */
 
static void _farewell_render(struct menu *menu) {
  switch (MENU->stage) {
    case FAREWELL_STAGE_FADEOUT1: farewell_render_FADEOUT1(menu); break;
    case FAREWELL_STAGE_FLYAWAY: farewell_render_FLYAWAY(menu); break;
    case FAREWELL_STAGE_FLYHOME: farewell_render_FLYHOME(menu); break;
    case FAREWELL_STAGE_INSIDE: farewell_render_INSIDE(menu); break;
    case FAREWELL_STAGE_FINAL: farewell_render_FINAL(menu); break;
  }
  
  // Subtitle may appear during any stage, and it kind of does its own thing.
  if (MENU->subtitle_texid>0) {
    int dstx=(g.fbw>>1)-(MENU->subtitlew>>1);
    int dsty=136+((g.fbh-136)>>1)-(MENU->subtitleh>>1);
    graf_draw_decal(&g.graf,MENU->subtitle_texid,dstx,dsty,0,0,MENU->subtitlew,MENU->subtitleh,0);
  }
}

/* Type definition.
 */
 
const struct menu_type menu_type_farewell={
  .name="farewell",
  .objlen=sizeof(struct menu_farewell),
  .del=_farewell_del,
  .init=_farewell_init,
  .input=_farewell_input,
  .update=_farewell_update,
  .render=_farewell_render,
};
