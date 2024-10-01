#include "game/penance.h"

/* Say one pixel per tile in the map?
 * Make an effort to preserve the real 20x11 aspect, or close to.
 * Must be at least 5 on each axis.
 */
#define CELLW 20
#define CELLH 11

/* Object definition.
 */
 
struct menu_teleport {
  struct menu hdr;
  struct egg_draw_tile *vtxv;
  int vtxc,vtxa;
  int wx,wy,ww,wh;
  int cx,cy; // Cursor position in world coords.
  double animclock;
  int animframe;
};

#define MENU ((struct menu_teleport*)menu)

/* Delete.
 */
 
static void _teleport_del(struct menu *menu) {
  if (MENU->vtxv) free(MENU->vtxv);
}

/* Add vertex.
 */
 
static struct egg_draw_tile *teleport_add_vertex(struct menu *menu) {
  if (MENU->vtxc>=MENU->vtxa) {
    int na=MENU->vtxa+16;
    if (na>INT_MAX/sizeof(struct egg_draw_tile)) return 0;
    void *nv=realloc(MENU->vtxv,sizeof(struct egg_draw_tile)*na);
    if (!nv) return 0;
    MENU->vtxv=nv;
    MENU->vtxa=na;
  }
  return MENU->vtxv+MENU->vtxc++;
}

/* Fill (vtxv) with the map, stumps, and an extra slot at the end for the cursor.
 */
 
static int teleport_generate_vertices(struct menu *menu) {
  int wx,wy,ww,wh;
  maps_get_world_bounds(&wx,&wy,&ww,&wh);
  int dstw=ww*TILESIZE;
  int dsth=wh*TILESIZE;
  int dstx=(g.fbw>>1)-(dstw>>1)+(TILESIZE>>1);
  int dsty=(g.fbh>>1)-(dsth>>1)+(TILESIZE>>1);
  MENU->wx=wx;
  MENU->wy=wy;
  MENU->ww=ww;
  MENU->wh=wh;
  
  // First, a background image for each map that exists.
  int x=wx+ww; while (x-->wx) {
    int y=wy+wh; while (y-->wy) {
      const struct map *map=map_by_location(x,y);
      if (!map) continue;
      struct egg_draw_tile *vtx=teleport_add_vertex(menu);
      if (!vtx) return -1;
      vtx->dstx=dstx+(x-wx)*TILESIZE;
      vtx->dsty=dsty+(y-wy)*TILESIZE;
      vtx->xform=0;
      // I carefully arranged the tiles to make this possible, check it out:
      vtx->tileid=0x90;
      if (map_by_location(x,y+1)) vtx->tileid+=1;
      if (map_by_location(x+1,y)) vtx->tileid+=2;
      if (map_by_location(x-1,y)) vtx->tileid+=4;
      if (map_by_location(x,y-1)) vtx->tileid+=8;
    }
  }

  // Next, a little stump on each of the world's stumps. Different tile for the current stump.
  int y,p=0;
  for (;;p++) {
    if (!maps_get_stump(&x,&y,p)) break;
    struct egg_draw_tile *vtx=teleport_add_vertex(menu);
    if (!vtx) return -1;
    vtx->dstx=dstx+(x-wx)*TILESIZE;
    vtx->dsty=dsty+(y-wy)*TILESIZE;
    vtx->xform=0;
    vtx->tileid=((x==g.map->x)&&(y==g.map->y))?0xa0:0xa1;
  }
  
  // And finally, a vertex for the cursor.
  struct egg_draw_tile *vtx=teleport_add_vertex(menu);
  if (!vtx) return -1;
  vtx->dstx=dstx+(g.map->x-wx)*TILESIZE;
  vtx->dsty=dsty+(g.map->y-wy)*TILESIZE;
  vtx->tileid=0xa2;
  vtx->xform=0;
  MENU->cx=g.map->x;
  MENU->cy=g.map->y;
  
  return 0;
}

/* Init.
 */
 
static int _teleport_init(struct menu *menu) {
  if (teleport_generate_vertices(menu)<0) return -1;
  return 0;
}

/* Move cursor.
 */
 
static void teleport_move_cursor(struct menu *menu,int dx,int dy) {
  int nx=MENU->cx+dx,ny=MENU->cy+dy;
  struct map *map=map_by_location(nx,ny);
  if (!map) return;
  MENU->cx=nx;
  MENU->cy=ny;
  // Cursor tile is always last in the list.
  struct egg_draw_tile *vtx=MENU->vtxv+MENU->vtxc-1;
  vtx->dstx+=dx*TILESIZE;
  vtx->dsty+=dy*TILESIZE;
  MENU->animclock=0.0; // force a fresh frame
}

/* Cancel teleport, stay here.
 * Does not pop the menu.
 */
 
static void teleport_dismiss(struct menu *menu) {
  if (GRP(HERO)->spritec>=1) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    hero->y+=1.500; // Move her off the stump. She's already out of TREE mode.
  }
}

/* Apply teleport.
 */
 
static void teleport_warp(struct menu *menu,const struct map *map) {
  g.mapid_load_soon=map->rid;
  if (GRP(HERO)->spritec>=1) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    const uint8_t *cell=map->v;
    int y=0; for (;y<ROWC;y++) {
      int x=0; for (;x<COLC;x++,cell++) {
        if (*cell==0x2e) {
          hero->x=(double)(x+1);
          hero->y=(double)y+1.500;
          return;
        }
      }
    }
  }
}

/* Input.
 */
 
static void _teleport_input(struct menu *menu,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    //TODO sound effects, fireworks
    if ((MENU->cx==g.map->x)&&(MENU->cy==g.map->y)) {
      teleport_dismiss(menu);
    } else {
      const struct map *map=map_by_location(MENU->cx,MENU->cy);
      if (!map||!map->stump) teleport_dismiss(menu);
      else teleport_warp(menu,map);
    }
    menu_pop(menu);
    return;
  }
       if ((input&EGG_BTN_LEFT)&&!(pvinput&EGG_BTN_LEFT)) teleport_move_cursor(menu,-1,0);
  else if ((input&EGG_BTN_RIGHT)&&!(pvinput&EGG_BTN_RIGHT)) teleport_move_cursor(menu,1,0);
  else if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) teleport_move_cursor(menu,0,-1);
  else if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) teleport_move_cursor(menu,0,1);
}

/* Update.
 */
 
static void _teleport_update(struct menu *menu,double elapsed) {
  if ((MENU->animclock-=elapsed)<=0.0) {
    MENU->animclock+=0.500;
    if (++(MENU->animframe)>=2) MENU->animframe=0;
    struct egg_draw_tile *vtx=MENU->vtxv+MENU->vtxc-1;
    if (MENU->animframe) { // Frame 1: dot-on-stump (noop), X, or Check.
      if ((MENU->cx==g.map->x)&&(MENU->cy==g.map->y)) {
        vtx->tileid=0xa0;
      } else {
        const struct map *map=map_by_location(MENU->cx,MENU->cy);
        if (!map||!map->stump) vtx->tileid=0xa3;
        else vtx->tileid=0xa4;
      }
    } else { // Frame 0: Always mini-dot
      vtx->tileid=0xa2;
    }
  }
}

/* Render.
 */
 
static void _teleport_render(struct menu *menu) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x00102080);
  graf_flush(&g.graf);
  egg_draw_tile(1,texcache_get_image(&g.texcache,RID_image_hero),MENU->vtxv,MENU->vtxc);
}

/* Type definition.
 */
 
const struct menu_type menu_type_teleport={
  .name="teleport",
  .objlen=sizeof(struct menu_teleport),
  .del=_teleport_del,
  .init=_teleport_init,
  .input=_teleport_input,
  .update=_teleport_update,
  .render=_teleport_render,
};
