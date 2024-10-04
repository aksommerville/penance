/* penance.h
 * Main internal header.
 */
 
#ifndef PENANCE_H
#define PENANCE_H

#define COLC 20
#define ROWC 11
#define TILESIZE 16

// I need a music killswitch while testing sound effects. Users don't get this privilege, sorry.
#define ENABLE_MUSIC 1

#define DIR_N 0x40
#define DIR_W 0x10
#define DIR_E 0x08
#define DIR_S 0x02

#define CANDLE_COUNT 3

// Spells are cast into a circular buffer this long.
#define SPELL_LIMIT 8

/* SPELLS
Foot:
  Flower     DDUD
  Fireball   LLLL or RRRR
  Disembody  ULRDU
Hole:
  Rabbit     RRDR
  Bird       UULRU
  Turtle     LLRRD
Tree:
  Teleport   DDD
  Open       LRLUU
*/
#define SPELLBIT_FLOWER    0x01
#define SPELLBIT_FIREBALL  0x02
#define SPELLBIT_DISEMBODY 0x04
#define SPELLBIT_RABBIT    0x08
#define SPELLBIT_BIRD      0x10
#define SPELLBIT_TURTLE    0x20
#define SPELLBIT_TELEPORT  0x40
#define SPELLBIT_OPEN      0x80

#define TRANSITION_CUT 0
#define TRANSITION_PAN_LEFT 1
#define TRANSITION_PAN_RIGHT 2
#define TRANSITION_PAN_UP 3
#define TRANSITION_PAN_DOWN 4
#define TRANSITION_FADE 5

#define TRANSITION_TIME 0.500

// We track sound effects and decline to play the same one within s/10 or so of itself.
#define SFXTRACK_LIMIT 8

#include <egg/egg.h>
#include <opt/stdlib/egg-stdlib.h>
#include <opt/text/text.h>
#include <opt/graf/graf.h>
#include <opt/rom/rom.h>
#include "map.h"
#include "sprite/sprite.h"
#include "hero/hero.h"
#include "menu/menu.h"
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
  struct menu *menuv[MENU_STACK_LIMIT];
  int menuc;
  uint8_t candlev[CANDLE_COUNT]; // Not going to generalize global state, there's so little of it.
  int gameover; // Nonzero after we enter the triangle with all candles lit. Outtro animation.
  double playtime;
  double besttime;
  int bonus,jammio,spellusage;
  int transition;
  double transition_clock; // 0=>TRANSITION_TIME
  int transition_pvbits; // texid, capture of the last frame of the "from" scene
  struct sfxtrack {
    double when; // egg_time_real()
    int id;
  } sfxtrackv[SFXTRACK_LIMIT];
  int sfxtrackc;
} g;

int penance_load_map(int mapid,int transition);
int penance_navigate(int dx,int dy);

/* If a door or neighbor needs entered, make it happen.
 * This should be done well outside the usual update cycle, since a lot of things can change.
 */
int penance_check_navigation();

void penance_gameover();

void penance_load_hiscore();
void penance_save_hiscore();

void penance_render_game_to(int texid);

/* Sound effects.
 */
#define SFX_BURN_TREE 1
#define SFX_LIGHT_CANDLE 2
#define SFX_EMERGENCY_MANEUVER 3
#define SFX_TREADLE_ON 4
#define SFX_TREADLE_OFF 5
#define SFX_UNLOCK 6
#define SFX_DEFLECT_MISSILE 7
#define SFX_TRANSFORM 8
#define SFX_UNTRANSFORM 9
#define SFX_REJECT_UNTRANSFORM 10
#define SFX_INVALID_SPELL 11
#define SFX_FIREBALL 12
#define SFX_FLOWER 13
#define SFX_UNFLOWER 14
#define SFX_GHOST 15
#define SFX_UNGHOST 16
#define SFX_ENTER_HOLE 17
#define SFX_ENTER_TREE 18
#define SFX_HURT 19
#define SFX_TELEPORT_NOOP 20
#define SFX_TELEPORT_REJECT 21
#define SFX_TELEPORT_ACCEPT 22
#define SFX_TELEPORT_OPEN 23
#define SFX_OPENING_ACCEPT 24
#define SFX_OPENING_REJECT 25
#define SFX_ENCODE_PIP 26
#define SFX_DUALEPHANT_FIRE 27
#define SFX_WIND_BLOW 28
#define SFX_TICK 29
//#define sfx(id) egg_play_sound(2,id)
#define sfx(id) _penance_sfx(id)
void _penance_sfx(int id); // Don't call directly; use the macro.

#endif
