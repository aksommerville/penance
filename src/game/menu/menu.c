#include "game/penance.h"

/* Delete menu.
 */
 
void menu_del(struct menu *menu) {
  if (!menu) return;
  menu->type->del(menu);
  free(menu);
}

/* New menu.
 */
 
struct menu *menu_new(const struct menu_type *type) {
  if (!type) return 0;
  struct menu *menu=calloc(1,type->objlen);
  if (!menu) return 0;
  menu->type=type;
  if (type->init(menu)<0) {
    menu_del(menu);
    return 0;
  }
  return menu;
}

/* Pop from stack.
 */

void menu_pop(struct menu *menu) {
  if (menu) {
    int i=g.menuc;
    while (i-->0) {
      if (g.menuv[i]!=menu) continue;
      g.menuc--;
      memmove(g.menuv+i,g.menuv+i+1,sizeof(void*)*(g.menuc-i));
      menu_del(menu);
      break;
    }
  } else if (g.menuc) {
    g.menuc--;
    menu_del(g.menuv[g.menuc]);
  }
  // Force input to look fresh at the next poll.
  g.pvinput=0;
}

/* Create and push.
 */
 
struct menu *menu_push(const struct menu_type *type) {
  if (g.menuc>=MENU_STACK_LIMIT) return 0;
  struct menu *menu=menu_new(type);
  if (!menu) return 0;
  g.menuv[g.menuc++]=menu;
  return menu;
}
