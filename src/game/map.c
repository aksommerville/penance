#include "penance.h"

/* Globals.
 */
 
static struct {
  int TODO;
} maps={0};

/* Add resource, during reset.
 */
 
static int maps_add_resource(const uint8_t *src,int srcc,int rid) {
  fprintf(stderr,"%s srcc=%d rid=%d\n",__func__,srcc,rid);
  if (srcc<COLC*ROWC) {
    fprintf(stderr,"map:%d: Invalid size %d, must be at least %d*%d==%d\n",rid,srcc,COLC,ROWC,COLC*ROWC);
    return -1;
  }
  return 0;
}

/* Reset.
 */
 
int maps_reset(const void *rom,int romc) {
  fprintf(stderr,"%s romc=%d...\n",__func__,romc);
  //TODO wipe
  struct rom_reader reader={0};
  if (rom_reader_init(&reader,rom,romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid<EGG_TID_map) continue;
    if (res->tid>EGG_TID_map) break;
    if (maps_add_resource(res->v,res->c,res->rid)<0) return -1;
  }
  //TODO validate
  return 0;
}
