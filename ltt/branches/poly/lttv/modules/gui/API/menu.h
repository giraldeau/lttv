#ifndef MENU_H
#define MENU_H

#include "common.h"

/* constructor of the viewer */
//typedef GtkWidget* (*lttv_constructor)(void * main_window);


typedef GArray LttvMenus;

typedef struct _lttv_menu_closure {
  lttv_constructor con;
  char * menuPath;
  char * menuText;
} lttv_menu_closure;


LttvMenus *lttv_menus_new();

void lttv_menus_destroy(LttvMenus *h);

void lttv_menus_add(LttvMenus *h, lttv_constructor f, char* menuPath, char * menuText);

gboolean lttv_menus_remove(LttvMenus *h, lttv_constructor f);

unsigned lttv_menus_number(LttvMenus *h);

#endif // MENU_H
