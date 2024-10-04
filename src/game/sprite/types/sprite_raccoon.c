#include "game/penance.h"

#define MASKC 7

#define STAGE_WAIT 0 /* Stand at the focus point, where we start. */
#define STAGE_RETURN 1 /* Walk to the toy chest. */
#define STAGE_RUMMAGE 2 /* Dig around for a mask. */
#define STAGE_ENTER 3 /* Walk from toy chest to focus point. */
#define STAGE_CONGRATULATE 4 /* React to a single match. */

struct sprite_raccoon {
  struct sprite hdr;
  uint8_t tileid0;
  uint8_t maskv[MASKC]; // tileid
  int maskp;
  int stage;
  double stageclock;
  double focusx,focusy; // Where I start.
  double toysx,toysy; // Where I should be when rummaging. Must be straight left of focus. Pops to (toysy) only during the rummage animation.
  double animclock;
  int animframe;
  int16_t bonex,boney; // Center of first bone.
  uint8_t scorev[MASKC]; // 0|1
  int won;
};

#define SPRITE ((struct sprite_raccoon*)sprite)

/* Init.
 */
 
static int _raccoon_init(struct sprite *sprite,const uint8_t *def,int defc) {
  SPRITE->tileid0=sprite->tileid;
  SPRITE->maskp=-1;
  SPRITE->stage=STAGE_WAIT;
  SPRITE->stageclock=4.0;
  SPRITE->focusx=sprite->x;
  SPRITE->focusy=sprite->y;
  
  /* Locate the toy chest.
   * It's the first physics==1 cell left of our starting point.
   */
  int col=(int)sprite->x,row=(int)sprite->y;
  while (col-->0) {
    uint8_t tileid=g.map->v[row*COLC+col];
    uint8_t props=g.map->tileprops[tileid];
    if (props==1) {
      SPRITE->toysx=col+1.0;
      SPRITE->toysy=row+0.3125;
      break;
    }
  }
  if (SPRITE->toysx<=0.0) {
    fprintf(stderr,"ERROR: raccoon requires a toy chest to his left\n");
    return -1;
  }
  
  /* Select a random order for the masks.
   * It's important that we cycle through the same order every time (at least as long as this sprite is alive).
   * Otherwise it could be possible to win the game without having matched every mask.
   * But with this same-order policy, "seven in a row" is the same thing as "all seven".
   */
  int optionc=MASKC;
  uint8_t optionv[MASKC];
  int i=MASKC;
  while (i-->0) optionv[i]=i;
  for (i=0;i<MASKC;i++) {
    int choice=rand()%optionc;
    SPRITE->maskv[i]=optionv[choice];
    optionc--;
    memmove(optionv+choice,optionv+choice+1,optionc-choice);
  }
  
  /* Bones go in the second row, centered on me.
   * These are not different sprites; I render them.
   */
  SPRITE->bonex=(int16_t)((sprite->x-3.0)*TILESIZE);
  SPRITE->boney=TILESIZE+(TILESIZE>>1);
  
  return 0;
}

/* Locate the hero and return the "mask" she's "wearing", if she's in my line of sight.
 * Valid masks are: 0..6=nun,ghost,flower,jammio,bird,rabbit,turtle, or OOB if OOB.
 */
 
static int raccoon_read_hero(struct sprite *sprite) {
  if (GRP(HERO)->spritec<1) return 0xff;
  const struct sprite *hero=GRP(HERO)->spritev[0];
  if (hero->x<=sprite->x) return 0xff;
  if (hero->x>sprite->x+3.0) return 0xff;
  if (hero->y<sprite->y-1.0) return 0xff;
  if (hero->y>sprite->y+1.0) return 0xff;
  return hero_get_form_raccoonwise(hero);
}

/* Matched a mask.
 */
 
static void raccoon_match(struct sprite *sprite) {
  memmove(SPRITE->scorev,SPRITE->scorev+1,sizeof(SPRITE->scorev)-1);
  SPRITE->scorev[MASKC-1]=1;
  SPRITE->stage=STAGE_CONGRATULATE;
  SPRITE->stageclock=3.0;
  if (!memcmp(SPRITE->scorev,"\1\1\1\1\1\1\1",MASKC)) {
    SPRITE->won=1;
    g.bonus=1;
  }
}

/* WAIT stage expired with no match: Record a fault.
 */
 
static void raccoon_miss(struct sprite *sprite) {
  memmove(SPRITE->scorev,SPRITE->scorev+1,sizeof(SPRITE->scorev)-1);
  SPRITE->scorev[MASKC-1]=0;
}

/* Advance stage.
 */
 
static void raccoon_advance_stage(struct sprite *sprite) {
  switch (SPRITE->stage) {
    case STAGE_WAIT: {
        if (SPRITE->won) {
          SPRITE->maskp=-1;
        } else {
          SPRITE->stage=STAGE_RETURN;
        }
        SPRITE->stageclock=999.999;
        if ((SPRITE->maskp>=0)&&(SPRITE->maskp<MASKC)) raccoon_miss(sprite);
      } break;
    case STAGE_RETURN: {
        SPRITE->stage=STAGE_RUMMAGE;
        SPRITE->stageclock=2.0;
      } break;
    case STAGE_RUMMAGE: {
        SPRITE->maskp++;
        if ((SPRITE->maskp<0)||(SPRITE->maskp>=MASKC)) SPRITE->maskp=0;
        SPRITE->stage=STAGE_ENTER;
        SPRITE->stageclock=999.999;
        sprite->y=SPRITE->focusy;
      } break;
    case STAGE_ENTER: {
        SPRITE->stage=STAGE_WAIT;
        SPRITE->stageclock=10.0; // This is the important time. How long does Dot have to match it?
      } break;
    case STAGE_CONGRATULATE: {
        if (SPRITE->won) {
          SPRITE->stage=STAGE_WAIT;
          SPRITE->maskp=-1;
        } else {
          SPRITE->stage=STAGE_RETURN;
        }
        SPRITE->stageclock=999.999;
      } break;
  }
}

/* Update.
 */
 
static void _raccoon_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) { // Animate at the same rate regardless of stage, don't overthink it.
    SPRITE->animclock+=0.250;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0; // Every animation we do is two frames.
  }
  if ((SPRITE->stageclock-=elapsed)<=0.0) {
    raccoon_advance_stage(sprite);
  }
  switch (SPRITE->stage) {
    case STAGE_WAIT: {
        if ((SPRITE->maskp>=0)&&(SPRITE->maskp<MASKC)) {
          int heromask=raccoon_read_hero(sprite);
          if (heromask==SPRITE->maskv[SPRITE->maskp]) {
            raccoon_match(sprite);
          }
        }
      } break;
    case STAGE_RETURN: {
        sprite->x-=elapsed*3.0;
        if (sprite->x<=SPRITE->toysx) {
          sprite->x=SPRITE->toysx;
          sprite->y=SPRITE->toysy;
          raccoon_advance_stage(sprite);
        }
      } break;
    case STAGE_RUMMAGE: {
        sprite->y=SPRITE->toysy;
      } break;
    case STAGE_ENTER: {
        sprite->x+=elapsed*3.0;
        if (sprite->x>=SPRITE->focusx) {
          sprite->x=SPRITE->focusx;
          sprite->y=SPRITE->focusy;
          raccoon_advance_stage(sprite);
        }
      } break;
    case STAGE_CONGRATULATE: break;
    default: sprite_kill_later(sprite);
  }
}

/* Render.
 */
 
static void _raccoon_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  uint8_t xform=0,tileid=SPRITE->tileid0;
  switch (SPRITE->stage) {
    case STAGE_WAIT: break;
    case STAGE_RETURN: {
        xform=EGG_XFORM_XREV;
        tileid+=1+SPRITE->animframe;
      } break;
    case STAGE_RUMMAGE: {
        tileid+=3+SPRITE->animframe;
      } break;
    case STAGE_ENTER: {
        tileid+=1+SPRITE->animframe;
      } break;
    case STAGE_CONGRATULATE: {
        graf_draw_tile(&g.graf,texid,dstx,dsty-TILESIZE,SPRITE->tileid0+12,0);
        if (SPRITE->animframe) dsty--;
      } break;
  }
  graf_draw_tile(&g.graf,texid,dstx,dsty,tileid,xform);
  if ((SPRITE->maskp>=0)&&(SPRITE->maskp<MASKC)&&(SPRITE->stage!=STAGE_RUMMAGE)) {
    graf_draw_tile(&g.graf,texid,dstx,dsty,SPRITE->tileid0+5+SPRITE->maskv[SPRITE->maskp],xform);
  }
  // Bones...
  dstx=SPRITE->bonex;
  dsty=SPRITE->boney;
  const uint8_t *p=SPRITE->scorev;
  int i=MASKC;
  for (;i-->0;p++,dstx+=TILESIZE) {
    if (*p) graf_draw_tile(&g.graf,texid,dstx,dsty,SPRITE->tileid0-14,0);
    else graf_draw_tile(&g.graf,texid,dstx,dsty,SPRITE->tileid0-15,0);
  }
}

/* Type.
 */
 
const struct sprite_type sprite_type_raccoon={
  .name="raccoon",
  .objlen=sizeof(struct sprite_raccoon),
  .init=_raccoon_init,
  .update=_raccoon_update,
  .render=_raccoon_render,
};
