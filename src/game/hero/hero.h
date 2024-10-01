/* hero.h
 * Everything related to the main sprite.
 */
 
#ifndef HERO_H
#define HERO_H

void sprite_hero_input(struct sprite *sprite,int input,int pvinput);

void hero_draw_overlay(struct sprite *sprite,int16_t addx,int16_t addy);

void hero_map_changed(struct sprite *sprite);

int hero_is_human();

void hero_apply_wind(struct sprite *sprite,double dx);

#endif
