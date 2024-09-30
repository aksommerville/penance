#include "game/penance.h"

/* Object definition.
 */
 
struct menu_teleport {
  struct menu hdr;
};

#define MENU ((struct menu_teleport*)menu)

/* Delete.
 */
 
static void _teleport_del(struct menu *menu) {
}

/* Init.
 */
 
static int _teleport_init(struct menu *menu) {
  return 0;
}

/* Input.
 */
 
static void _teleport_input(struct menu *menu,int input,int pvinput) {
}

/* Update.
 */
 
static void _teleport_update(struct menu *menu,double elapsed) {
}

/* Render.
 */
 
static void _teleport_render(struct menu *menu) {
  
}

/* Type definition.
 */
 
const struct menu_type menu_type_teleport={
  .name="teleport",
  .objlen=sizeof(struct menu_teleport),
  .del=_teleport_del,
  .init=_teleport_init,
  .input=_teleport_input,
  .update=_teleport_update,
  .render=_teleport_render,
};
