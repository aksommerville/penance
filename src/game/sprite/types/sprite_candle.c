#include "game/penance.h"

struct sprite_candle {
  struct sprite hdr;
  double animclock;
  int animframe;
  uint8_t tileid0;
  int lit;
  int candleid;
};

#define SPRITE ((struct sprite_candle*)sprite)

static int _candle_init(struct sprite *sprite,const uint8_t *def,int defc) {
  SPRITE->candleid=-1;
  struct map_command_reader reader;
  map_command_reader_init_serial(&reader,def,defc);
  const uint8_t *arg;
  int argc,opcode;
  while ((argc=map_command_reader_next(&arg,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x3f: SPRITE->candleid=(arg[0]<<8)|arg[1]; break;
    }
  }
  if ((SPRITE->candleid>=0)&&(SPRITE->candleid<CANDLE_COUNT)) SPRITE->lit=g.candlev[SPRITE->candleid];
  return 0;
}

static void _candle_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.150;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    if (!SPRITE->tileid0) SPRITE->tileid0=sprite->tileid;
    if (SPRITE->lit) sprite->tileid=SPRITE->tileid0+1+SPRITE->animframe;
    else sprite->tileid=SPRITE->tileid0;
  }
}

const struct sprite_type sprite_type_candle={
  .name="candle",
  .objlen=sizeof(struct sprite_candle),
  .init=_candle_init,
  .update=_candle_update,
};

int sprite_candle_light(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_candle)) return 0;
  if (SPRITE->lit) return 0;
  SPRITE->lit=1;
  SPRITE->animclock=0.0;
  if ((SPRITE->candleid>=0)&&(SPRITE->candleid<CANDLE_COUNT)) {
    g.candlev[SPRITE->candleid]=1;
    int alllit=1,i=CANDLE_COUNT;
    while (i-->0) if (!g.candlev[i]) { alllit=0; break; }
    if (alllit) {
      egg_play_song(RID_song_antechamber,0,1);
    }
  }
  return 1;
}

int sprite_candle_is_lit(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_candle)) return 0;
  return SPRITE->lit;
}
