#include "game/penance.h"

/* Object definition.
 */
 
struct menu_farewell {
  struct menu hdr;
};

#define MENU ((struct menu_farewell*)menu)

/* Delete.
 */
 
static void _farewell_del(struct menu *menu) {
}

/* Init.
 */
 
static int _farewell_init(struct menu *menu) {
  return 0;
}

/* Input.
 */
 
static void _farewell_input(struct menu *menu,int input,int pvinput) {
}

/* Update.
 */
 
static void _farewell_update(struct menu *menu,double elapsed) {
}

/* Render.
 */
 
static void _farewell_render(struct menu *menu) {
}

/* Type definition.
 */
 
const struct menu_type menu_type_farewell={
  .name="farewell",
  .objlen=sizeof(struct menu_farewell),
  .del=_farewell_del,
  .init=_farewell_init,
  .input=_farewell_input,
  .update=_farewell_update,
  .render=_farewell_render,
};
