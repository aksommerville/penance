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
  struct rom_sprite res={0};
  if (rom_sprite_decode(&res,def,defc)<0) return -1;
  struct rom_command_reader reader={.v=res.cmdv,.c=res.cmdc};
  struct rom_command command;
  while (rom_command_reader_next(&command,&reader)>0) {
    switch (command.opcode) {
      case 0x3f: SPRITE->candleid=(command.argv[0]<<8)|command.argv[1]; break;
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
    if (alllit&&ENABLE_MUSIC) {
      egg_play_song(RID_song_antechamber,0,1);
    }
  }
  return 1;
}

int sprite_candle_is_lit(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_candle)) return 0;
  return SPRITE->lit;
}
