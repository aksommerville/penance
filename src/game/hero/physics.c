#include "hero_internal.h"

/* Rectify position in place.
 */
 
void hero_rectify_position(struct sprite *sprite) {

  /* Bounds must not exceed 1.0 on either axis -- we'll assume that no more than 4 cells are in play.
   * And also, that would be very unfriendly to the user, if she can't walk thru a single-cell gap!
   */
  #define LRADIUS 0.375
  #define RRADIUS 0.375
  #define URADIUS 0.125
  #define DRADIUS 0.450
  double y0=sprite->y-URADIUS;
  double y1=sprite->y+DRADIUS;
  double x0=sprite->x-LRADIUS;
  double x1=sprite->x+RRADIUS;
  
  /* Determine which cells are in play.
   * (z) are inclusive.
   * Check against all edges. You can walk all the way offscreen, and we'll read it as clamped to the edge.
   * (as opposed to empty).
   */
  int cola=(int)x0; if (cola<0) cola=0; else if (cola>=COLC) cola=COLC-1;
  int colz=(int)x1; if (colz<0) colz=0; else if (colz>=COLC) colz=COLC-1;
  int rowa=(int)y0; if (rowa<0) rowa=0; else if (rowa>=ROWC) rowa=ROWC-1;
  int rowz=(int)y1; if (rowz<0) rowz=0; else if (rowz>=ROWC) rowz=ROWC-1;
  
  /* Check each cell. Is anything impassable?
   * Record collisions as 4 bits, since the hero is smaller than a cell there can only be 4 cells involved (or 1 or 2).
   */
  int collision=0; // 1,2,4,8=NW,NE,SW,SE. There can be no more than 4 participating cells.
  const uint8_t *rowp=g.map->v+rowa*COLC+cola;
  int row=rowa; for (;row<=rowz;row++,rowp+=COLC) {
    const uint8_t *colp=rowp;
    int col=cola; for (;col<=colz;col++,colp++) {
      uint8_t prop=g.map->tileprops[*colp];
      switch (SPRITE->mode) { // Ghosts and birds can pass over holes.
        case HERO_MODE_GHOST: {
            if (prop==2) continue;
          } break;
      }
      if (prop) { // So far, all nonzero physics are impassable. That's likely to change (eg flying over holes).
        if (row==rowa) {
          if (col==cola) collision|=1;
          else collision|=2;
        } else if (col==cola) collision|=4;
        else collision|=8;
      }
    }
  }
  if (!collision) return;
  
  /* If all 4 bits are set, panic. Restore the last known position.
   * If there's exactly one bit clear, snuggle into the corner, obviously.
   */
  switch (collision) {
    case 0x0f: {
        fprintf(stderr,"%s:%d: Collision panic\n",__FILE__,__LINE__);
        sprite->x=SPRITE->pvx;
        sprite->y=SPRITE->pvy;
      } return;
    case 0x0e: { // NW
        sprite->x=(double)(cola+1)-RRADIUS;
        sprite->y=(double)(rowa+1)-DRADIUS;
      } return;
    case 0x0d: { // NE
        sprite->x=(double)(cola+1)+LRADIUS;
        sprite->y=(double)(rowa+1)-DRADIUS;
      } return;
    case 0x0b: { // SW
        sprite->x=(double)(cola+1)-RRADIUS;
        sprite->y=(double)(rowa+1)+URADIUS;
      } return;
    case 0x07: { // SE
        sprite->x=(double)(cola+1)+LRADIUS;
        sprite->y=(double)(rowa+1)-DRADIUS;
      } break;
  }
  
  /* In the two double-corner cases, snap to whichever corner position is closer.
   */
  int iscorner=0;
  double xca,xcb,yca,ycb; // Two candidate points.
  if ((collision&9)==9) { // NW+SE
    iscorner=1;
    double midx=(double)(cola+1);
    double midy=(double)(rowa+1);
    xca=midx+LRADIUS;
    yca=midy-DRADIUS;
    xcb=midx-RRADIUS;
    ycb=midy+URADIUS;
  } else if ((collision&6)==6) { // NE+SW
    iscorner=1;
    double midx=(double)(cola+1);
    double midy=(double)(rowa+1);
    xca=midx-RRADIUS;
    yca=midy-DRADIUS;
    xcb=midx+LRADIUS;
    ycb=midy+URADIUS;
  }
  if (iscorner) {
    double dxa=sprite->x-xca; if (dxa<0.0) dxa=-dxa;
    double dya=sprite->y-yca; if (dya<0.0) dya=-dya;
    double dxb=sprite->x-xcb; if (dxb<0.0) dxb=-dxb;
    double dyb=sprite->y-ycb; if (dyb<0.0) dyb=-dyb;
    if (dxa+dya<=dxb+dyb) {
      sprite->x=xca;
      sprite->y=yca;
    } else {
      sprite->x=xcb;
      sprite->y=ycb;
    }
    return;
  }
  
  /* All other cases form a simple rectangle.
   * Determine the 4 cardinal escapements and apply the shortest.
   */
  double bx,by,bw,bh;
  if (collision&3) { // north row
    by=(double)rowa;
    bh=(collision&12)?2.0:1.0;
  } else {
    by=(double)(rowa+1);
    bh=1.0;
  }
  if (collision&5) { // west column
    bx=(double)cola;
    bw=(collision&10)?2.0:1.0;
  } else {
    bx=(double)(cola+1);
    bw=1.0;
  }
  double el=x1-bx;
  double er=bx+bw-x0;
  double eu=y1-by;
  double ed=by+bh-y0;
  if ((el<=er)&&(el<=eu)&&(el<=ed)) sprite->x-=el;
  else if ((er<=eu)&&(er<=ed)) sprite->x+=er;
  else if (eu<=ed) sprite->y-=eu;
  else sprite->y+=ed;
}