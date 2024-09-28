/* map.h
 * All of the game's map resources are instantiated at boot, and organized geographically.
 * Map instances are mutable per session.
 */
 
#ifndef MAP_H
#define MAP_H

int maps_reset(const void *rom,int romc);

#endif
