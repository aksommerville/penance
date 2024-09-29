/* penance.h
 * Main internal header.
 */
 
#ifndef PENANCE_H
#define PENANCE_H

#define COLC 20
#define ROWC 11
#define TILESIZE 16

#define DIR_N 0x40
#define DIR_W 0x10
#define DIR_E 0x08
#define DIR_S 0x02

// Spells are cast into a circular buffer this long.
#define SPELL_LIMIT 8

/* SPELLS
Foot:
  Fireball   LRLRL or RLRLR
  Flower     DDUD
  Disembody  UULRDU
Hole:
  Rabbit
  Bird
  Turtle
Tree:
  Teleport
  Crow
  Open
*/

#include <egg/egg.h>
#include <opt/stdlib/egg-stdlib.h>
#include <opt/text/text.h>
#include <opt/graf/graf.h>
#include <opt/rom/rom.h>
#include "map.h"
#include "sprite/sprite.h"
#include "hero/hero.h"
#include "egg_rom_toc.h"

extern struct globals {
  void *rom;
  int romc;
  struct texcache texcache;
  struct graf graf;
  struct font *font;
  int fbw,fbh;
  int texid_tiles;
  int pvinput;
  struct map *map;
  int map_imageid;
  int mapid_load_soon;
} g;

int penance_load_map(int mapid);
int penance_navigate(int dx,int dy);

/* If a door or neighbor needs entered, make it happen.
 * This should be done well outside the usual update cycle, since a lot of things can change.
 */
int penance_check_navigation();

#endif
