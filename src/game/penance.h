/* penance.h
 * Main internal header.
 */
 
#ifndef PENANCE_H
#define PENANCE_H

#define COLC 20
#define ROWC 11
#define TILESIZE 16

#include <egg/egg.h>
#include <opt/stdlib/egg-stdlib.h>
#include <opt/text/text.h>
#include <opt/graf/graf.h>
#include <opt/rom/rom.h>
#include "map.h"
#include "egg_rom_toc.h"

extern struct globals {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int fbw,fbh;
  int texid_tiles;
  int texid_hero;//XXX
  int pvinput;
  struct map *map;
} g;

#endif
