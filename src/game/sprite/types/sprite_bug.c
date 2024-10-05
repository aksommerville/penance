#include "game/penance.h"

#define BUG_STYLE_BUTTERFLY 0
#define BUG_STYLE_BEE 1
#define BUG_STYLE_COUNT 2

struct sprite_bug {
  struct sprite hdr;
  int style;
  double animclock;
  int animframe;
  int animframec;
  double moveclock;
  double dx,dy;
  double speed; // m/s
};

#define SPRITE ((struct sprite_bug*)sprite)

static int _bug_init(struct sprite *sprite,const uint8_t *def,int defc) {
  sprite->imageid=RID_image_hero;
  SPRITE->style=rand()%BUG_STYLE_COUNT;
  switch (SPRITE->style) {
    case BUG_STYLE_BUTTERFLY: {
        sprite->tileid=0x4b;
        SPRITE->animframec=4;
        SPRITE->speed=1.5;
      } break;
    case BUG_STYLE_BEE: {
        sprite->tileid=0x4e;
        SPRITE->animframec=2;
        SPRITE->speed=2.0;
      } break;
    default: return -1;
  }
  if (sprite_group_add(GRP(VISIBLE),sprite)<0) return -1;
  if (sprite_group_add(GRP(UPDATE),sprite)<0) return -1;
  return 0;
}

static void _bug_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=SPRITE->animframec) {
      SPRITE->animframe=0;
    }
    switch (SPRITE->style) {
      case BUG_STYLE_BUTTERFLY: switch (SPRITE->animframe) {
          case 0: sprite->tileid=0x4b; break;
          case 1: case 3: sprite->tileid=0x4c; break;
          case 2: sprite->tileid=0x4d; break;
        } break;
      case BUG_STYLE_BEE: sprite->tileid=0x4e +SPRITE->animframe; break;
    }
  }
  if ((SPRITE->moveclock-=elapsed)<=0.0) {
    SPRITE->moveclock=0.5+(rand()%1000)/1000.0;
    double t=(rand()%6283)/1000.0;
    SPRITE->dx=cos(t)*SPRITE->speed;
    SPRITE->dy=sin(t)*SPRITE->speed;
    // Force to move toward the center, if we're in the far quarter, per axis.
    if (sprite->x<COLC*0.25) {
      if (SPRITE->dx<0.0) SPRITE->dx=-SPRITE->dx;
    } else if (sprite->x>COLC*0.75) {
      if (SPRITE->dx>0.0) SPRITE->dx=-SPRITE->dx;
    }
    if (sprite->y<ROWC*0.25) {
      if (SPRITE->dy<0.0) SPRITE->dy=-SPRITE->dy;
    } else if (sprite->y>ROWC*0.75) {
      if (SPRITE->dy>0.0) SPRITE->dy=-SPRITE->dy;
    }
  }
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  if (sprite->x<0.0) sprite->x=0.0; else if (sprite->x>COLC) sprite->x=COLC;
  if (sprite->y<0.0) sprite->y=0.0; else if (sprite->y>ROWC) sprite->y=ROWC;
}

const struct sprite_type sprite_type_bug={
  .name="bug",
  .objlen=sizeof(struct sprite_bug),
  .init=_bug_init,
  .update=_bug_update,
};

/* Spawn a bunch of bugs in appropriate places.
 */
 
void spawn_bugs() {
  /* Quick and dirty.
   * The entire top row of our tilesheet are equivalent grass-like tiles.
   * Those are candidates for bugs.
   * Impose a target ratio, so many bugs per grass tile, and then a global limit per screen.
   * Zero is fine (eg in the temple, or screens with only blockages and water).
   * Since our screens are 20x11==220 cells, we can index cells with one byte.
   */
  const int bug_limit=6;
  const int bug_proportion=25; // m**2/bug, also the minimum amount of grass for the first bug.
  uint8_t candidatev[COLC*ROWC]; // tile index.
  int candidatec=0;
  const uint8_t *p=g.map->v;
  int i=0; for (;i<COLC*ROWC;i++,p++) {
    if (*p>=0x10) continue; // not grass
    candidatev[candidatec++]=i;
  }
  int bugc=candidatec/bug_proportion;
  if (bugc>bug_limit) bugc=bug_limit;
  while (bugc-->0) {
    int cp=rand()%candidatec;
    double x=(double)(candidatev[cp]%COLC)+0.5;
    double y=(double)(candidatev[cp]/COLC)+0.5;
    struct sprite *sprite=sprite_spawn_with_type(x,y,&sprite_type_bug,0,0);
    if (!sprite) return;
  }
}
