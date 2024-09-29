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
  if (egg_texture_load_image(g.texid_tiles=egg_texture_new(),RID_image_outdoors)<0) return -1;//XXX
  if (egg_texture_load_image(g.texid_hero=egg_texture_new(),RID_image_hero)<0) return -1;//XXX
  
  if (maps_reset(g.rom,g.romc)<0) return -1;
  if (!(g.map=map_by_id(1))) return -1;
  
  egg_play_song(2,0,1);
  
  srand_auto();
  
  return 0;
}

static void navigate(int dx,int dy) {
  int nx=g.map->x+dx,ny=g.map->y+dy;
  struct map *nmap=map_by_location(nx,ny);
  if (!nmap) return;
  g.map=nmap;
}

void egg_client_update(double elapsed) {
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_LEFT)&&!(g.pvinput&EGG_BTN_LEFT)) navigate(-1,0);
    if ((input&EGG_BTN_RIGHT)&&!(g.pvinput&EGG_BTN_RIGHT)) navigate(1,0);
    if ((input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) navigate(0,-1);
    if ((input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) navigate(0,1);
    g.pvinput=input;
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  egg_draw_clear(1,0x201008ff);
  
  {
    struct egg_draw_tile vtxv[COLC*ROWC];
    struct egg_draw_tile *vtx=vtxv;
    const uint8_t *srcp=g.map->v;
    int i=COLC*ROWC;
    int16_t x=8,y=8;
    for (;i-->0;vtx++,srcp++) {
      vtx->dstx=x;
      vtx->dsty=y;
      vtx->tileid=*srcp;
      vtx->xform=0;
      if ((x+=16)>=320) {
        x=8;
        y+=16;
      }
    }
    graf_flush(&g.graf);
    egg_draw_tile(1,g.texid_tiles,vtxv,COLC*ROWC);
  }
  {
    struct egg_draw_tile vtxv[]={
      {40,40,0x00,0},
    };
    egg_draw_tile(1,g.texid_hero,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  graf_flush(&g.graf);
}
