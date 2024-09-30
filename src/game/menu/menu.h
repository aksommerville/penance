/* menu.h
 * The global menu system.
 * Actual storage is in penance.h:g.
 * The game itself is not a menu; it happens when the menu stack is empty.
 */
 
#ifndef MENU_H
#define MENU_H

#define MENU_STACK_LIMIT 8

struct menu;
struct menu_type;

struct menu {
  const struct menu_type *type;
  int opaque; // If zero, content below me will be rendered.
};

struct menu_type {
  const char *name;
  int objlen;
// Required hooks:
  void (*del)(struct menu *menu);
  int (*init)(struct menu *menu);
  void (*input)(struct menu *menu,int input,int pvinput);
  void (*update)(struct menu *menu,double elapsed);
  void (*render)(struct menu *menu);
};

void menu_del(struct menu *menu);
struct menu *menu_new(const struct menu_type *type);

/* Add or remove on the global stack.
 * menu_pop() accepts null to pop whatever's on top.
 * Otherwise, it removes only the instance you supply, wherever it is.
 * Popping deletes the menu object.
 * Pushing returns WEAK.
 */
void menu_pop(struct menu *menu);
struct menu *menu_push(const struct menu_type *type);

extern const struct menu_type menu_type_hello;
extern const struct menu_type menu_type_teleport;
extern const struct menu_type menu_type_farewell;

#endif
