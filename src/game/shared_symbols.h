/* shared_symbols.h
 * Symbols both we and the editor can enjoy.
 */
 
#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define NS_sys_tilesize 16
#define NS_sys_mapw 20
#define NS_sys_maph 11

#define CMD_map_image     0x20 /* u16:imageid */
#define CMD_map_hero      0x21 /* u16:pos */
#define CMD_map_location  0x22 /* u8:long u8:lat */
#define CMD_map_song      0x23 /* u16:songid */
#define CMD_map_sprite    0x40 /* u16:pos u16:spriteid */

#define CMD_sprite_type   0x20 /* u16:type */
#define CMD_sprite_image  0x21 /* u16:imageid */
#define CMD_sprite_tileid 0x22 /* u8:tileid u8:reserved */
#define CMD_sprite_layer  0x23 /* s16 */
#define CMD_sprite_groups 0x40 /* u32:bits */

#define NS_tilesheet_physics 1
#define NS_tilesheet_neighbors 0
#define NS_tilesheet_family 0
#define NS_tilesheet_weight 0

#define NS_sprtype_dummy            0
#define NS_sprtype_hero             1
#define NS_sprtype_fleshpuppet      2
#define NS_sprtype_fireball         3
#define NS_sprtype_bonfire          4
#define NS_sprtype_candle           5
#define NS_sprtype_terminator       6
#define NS_sprtype_wind             7
#define NS_sprtype_dualephant       8
#define NS_sprtype_missile          9
#define NS_sprtype_treadle         10
#define NS_sprtype_lock            11
#define NS_sprtype_scoreboard      12
#define NS_sprtype_fireworks       13
#define NS_sprtype_neon            14
#define NS_sprtype_raccoon         15
#define NS_sprtype_bug             16
#define NS_sprtype_target          17
#define NS_sprtype_cannon          18
#define NS_sprtype_werewolf        19

#endif
