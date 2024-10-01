#include "../penance.h"

struct sprite_group sprite_groupv[32]={0};

/* Used by sprite_group_update() to stash the set of updatable sprites,
 * so if group membership changes during the update, we're safe.
 * Use (spritec) as an in-use flag, and be sure to zero it when done.
 */
static struct sprite_group scratch={0};

/* Init globals.
 */
 
int sprite_groups_init() {

  // Straight zeroes is appropriate for almost everything.
  memset(sprite_groupv,0,sizeof(sprite_groupv));
  
  // Groups with a non-default mode need that set here.
  GRP(VISIBLE)->mode=SPRITE_GROUP_MODE_RENDER;
  GRP(HERO)->mode=SPRITE_GROUP_MODE_SINGLE;
  
  return 0;
}

/* Delete.
 */

void sprite_group_del(struct sprite_group *group) {
  if (!group||!group->refc) return;
  if (group->refc-->1) return;
  if (group->spritev) {
    if (group->spritec) fprintf(stderr,"%s:%d: Deleting group %p with spritec==%d\n",__FILE__,__LINE__,group,group->spritec);
    free(group->spritev);
  }
  free(group);
}

/* New.
 */
 
struct sprite_group *sprite_group_new() {
  struct sprite_group *group=calloc(1,sizeof(struct sprite_group));
  if (!group) return 0;
  group->refc=1;
  return group;
}

/* Retain.
 */
 
int sprite_group_ref(struct sprite_group *group) {
  if (!group) return -1;
  if (!group->refc) return 0; // Immortal, so retain and delete are noop.
  if (group->refc<1) return -1;
  if (group->refc>=INT_MAX) return -1;
  group->refc++;
  return 0;
}

/* Compare sprites for render sort.
 */
 
static int sprite_render_cmp(const struct sprite *a,const struct sprite *b) {
  if (a->layer<b->layer) return -1;
  if (a->layer>b->layer) return 1;
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  if (a<b) return -1;
  if (a>b) return 1;
  return 0;
}

/* Search sprites in group.
 */
 
static int sprite_group_search(const struct sprite_group *group,const struct sprite *sprite) {
  switch (group->mode) {
    case SPRITE_GROUP_MODE_ADDR: {
        int lo=0,hi=group->spritec;
        while (lo<hi) {
          int ck=(lo+hi)>>1;
          const struct sprite *q=group->spritev[ck];
               if (sprite<q) hi=ck;
          else if (sprite>q) lo=ck+1;
          else return ck;
        }
        return -lo-1;
      }
    case SPRITE_GROUP_MODE_RENDER: {
        int insp=0;
        int i=0; for (;i<group->spritec;i++) {
          const struct sprite *q=group->spritev[i];
          if (q==sprite) return i;
          if (sprite_render_cmp(sprite,q)>0) insp=i+1;
        }
        return -insp-1;
      }
    default: {
        int i=0; for (;i<group->spritec;i++) {
          if (group->spritev[i]==sprite) return i;
        }
        return -group->spritec-1;
      }
  }
  return -1;
}

/* Search groups in sprite.
 */
 
static int sprite_search(const struct sprite *sprite,const struct sprite_group *group) {
  int lo=0,hi=sprite->groupc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprite_group *q=sprite->groupv[ck];
         if (group<q) hi=ck;
    else if (group>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* List primitives.
 */
 
static int sprite_group_spritev_add(struct sprite_group *group,int p,struct sprite *sprite) {
  if (group->spritec>=group->spritea) {
    int na=group->spritea+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(group->spritev,sizeof(void*)*na);
    if (!nv) return -1;
    group->spritev=nv;
    group->spritea=na;
  }
  if (sprite_ref(sprite)<0) return -1;
  memmove(group->spritev+p+1,group->spritev+p,sizeof(void*)*(group->spritec-p));
  group->spritec++;
  group->spritev[p]=sprite;
  return 0;
}

static int sprite_groupv_add(struct sprite *sprite,int p,struct sprite_group *group) {
  if (sprite->groupc>=sprite->groupa) {
    int na=sprite->groupa+4;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(sprite->groupv,sizeof(void*)*na);
    if (!nv) return -1;
    sprite->groupv=nv;
    sprite->groupa=na;
  }
  if (sprite_group_ref(group)<0) return -1;
  memmove(sprite->groupv+p+1,sprite->groupv+p,sizeof(void*)*(sprite->groupc-p));
  sprite->groupv[p]=group;
  sprite->groupc++;
  return 0;
}

static void sprite_group_spritev_remove(struct sprite_group *group,int p) {
  struct sprite *sprite=group->spritev[p];
  group->spritec--;
  memmove(group->spritev+p,group->spritev+p+1,sizeof(void*)*(group->spritec-p));
  sprite_del(sprite);
}

static void sprite_groupv_remove(struct sprite *sprite,int p) {
  struct sprite_group *group=sprite->groupv[p];
  sprite->groupc--;
  memmove(sprite->groupv+p,sprite->groupv+p+1,sizeof(void*)*(sprite->groupc-p));
  sprite_group_del(group);
}

/* Test membership, public.
 */

int sprite_group_has(const struct sprite_group *group,const struct sprite *sprite) {
  if (!group||!sprite) return 0;
  return (sprite_search(sprite,group)>=0)?1:0;
}

/* Add sprite to group and vice-versa, public.
 */
 
int sprite_group_add(struct sprite_group *group,struct sprite *sprite) {
  if (!group||!sprite) return -1;
  
  // For SINGLE mode, there's a special gotcha. If some other sprite is already in (group), remove it.
  if (group->mode==SPRITE_GROUP_MODE_SINGLE) {
    if (group->spritec) {
      if (group->spritev[0]==sprite) return 0;
      sprite_group_clear(group);
    }
  }
  
  int sprp=sprite_group_search(group,sprite);
  if (sprp>=0) return 0; // TODO If we add an "explicit order" mode, would need to shuffle to rear in this case.
  int grpp=sprite_search(sprite,group);
  if (grpp>=0) return -1; // Broken mutual links!
  sprp=-sprp-1;
  grpp=-grpp-1;
  if (sprite_group_spritev_add(group,sprp,sprite)<0) return -1;
  if (sprite_groupv_add(sprite,grpp,group)<0) {
    sprite_group_spritev_remove(group,sprp);
    return -1;
  }
  return 1;
}

/* Remove sprite from group and vice-versa, public.
 */
 
int sprite_group_remove(struct sprite_group *group,struct sprite *sprite) {
  if (!group||!sprite) return 0;
  int sprp=sprite_group_search(group,sprite);
  if (sprp<0) return 0;
  int grpp=sprite_search(sprite,group);
  if (grpp<0) return -1; // Broken mutual link!
  if (sprite_ref(sprite)<0) return -1;
  sprite_group_spritev_remove(group,sprp);
  sprite_groupv_remove(sprite,grpp);
  sprite_del(sprite);
  return 1;
}

/* Remove all sprites from group.
 */

void sprite_group_clear(struct sprite_group *group) {
  if (!group) return;
  if (group->spritec<1) return;
  if (sprite_group_ref(group)<0) return;
  while (group->spritec>0) {
    group->spritec--;
    struct sprite *sprite=group->spritev[group->spritec];
    int grpp=sprite_search(sprite,group);
    if (grpp>=0) sprite_groupv_remove(sprite,grpp);
    sprite_del(sprite);
  }
  sprite_group_del(group);
}

/* Remove all groups from all sprites in group.
 */
 
void sprite_group_kill(struct sprite_group *group) {
  if (!group) return;
  if (group->spritec<1) return;
  if (sprite_group_ref(group)<0) return;
  while (group->spritec>0) {
    group->spritec--;
    struct sprite *sprite=group->spritev[group->spritec];
    int grpp=sprite_search(sprite,group);
    if (grpp>=0) sprite_groupv_remove(sprite,grpp);
    sprite_kill_now(sprite);
    sprite_del(sprite);
  }
  sprite_group_del(group);
}

/* Remove all groups for one sprite.
 */
 
void sprite_kill_now(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->groupc<1) return;
  if (sprite_ref(sprite)<0) return;
  while (sprite->groupc>0) {
    sprite->groupc--;
    struct sprite_group *group=sprite->groupv[sprite->groupc];
    int sprp=sprite_group_search(group,sprite);
    if (sprp>=0) sprite_group_spritev_remove(group,sprp);
    sprite_group_del(group);
  }
  sprite_del(sprite);
}

/* Convenience: Add to death row.
 */
 
void sprite_kill_later(struct sprite *sprite) {
  sprite_group_add(GRP(DEATHROW),sprite);
}

/* Update all sprites in group.
 */

void sprite_group_update(struct sprite_group *group,double elapsed) {
  if (!group||!group->spritec) return;
  if (scratch.spritec) return; // Someone is using scratch, oh no!
  // Copy (group)'s members into (scratch) so they can't change during the update.
  if (group->spritec>scratch.spritea) {
    int na=group->spritec+8;
    void *nv=realloc(scratch.spritev,sizeof(void*)*na);
    if (!nv) return;
    scratch.spritev=nv;
    scratch.spritea=na;
  }
  scratch.spritec=group->spritec;
  memcpy(scratch.spritev,group->spritev,sizeof(void*)*group->spritec);
  // Now we update from (scratch) and we know the list won't move around on us.
  // Mind that we haven't retained them. It would be disastrous if a sprite in UPDATE gets deleted before we reach it.
  // That shouldn't happen because we're not allowed to delete sprites during an update (by convention).
  int i=0; for (;i<scratch.spritec;i++) {
    struct sprite *sprite=scratch.spritev[i];
    if (!sprite->type->update) continue; // grr what the hell is it doing in this group?
    sprite->type->update(sprite,elapsed);
  }
  scratch.spritec=0;
}

/* Sort one pass, then render all sprites in group.
 */
 
void sprite_group_render(struct sprite_group *group,int addx,int addy) {
  if (!group||!group->spritec) return;
  
  /* If it uses the RENDER mode, perform one pass of a cocktail sort.
   * This mode allows the group to become unsorted temporarily.
   * If one sprite is out of place, it is guaranteed to land in the correct place within 2 updates.
   * And in all cases, the operation runs in linear time: Once thru the list.
   */
  if (group->mode==SPRITE_GROUP_MODE_RENDER) {
    int first,last,i,d;
    if (group->sortdir>0) {
      group->sortdir=-1;
      first=0;
      last=group->spritec-1;
      d=1;
    } else {
      group->sortdir=1;
      first=group->spritec-1;
      last=0;
      d=-1;
    }
    for (i=first;i!=last;i+=d) {
      struct sprite *a=group->spritev[i];
      struct sprite *b=group->spritev[i+d];
      if (sprite_render_cmp(a,b)==d) {
        group->spritev[i]=b;
        group->spritev[i+d]=a;
      }
    }
  }
  
  int i=0;
  for (;i<group->spritec;i++) {
    struct sprite *sprite=group->spritev[i];
    if (sprite->type->render) {
      sprite->type->render(sprite,addx,addy);
    } else if (!sprite->imageid) {
      // Obviously not going to work, skip it.
    } else {
      //TODO Consecutive (imageid) are extremely likely to be identical. Skip texcache_get_image() when unchanged.
      graf_draw_tile(&g.graf,
        texcache_get_image(&g.texcache,sprite->imageid),
        (int)(sprite->x*TILESIZE)+addx,
        (int)(sprite->y*TILESIZE)+addy,
        sprite->tileid,sprite->xform
      );
    }
  }
}
