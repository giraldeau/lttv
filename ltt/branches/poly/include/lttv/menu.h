/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifndef MENU_H
#define MENU_H

#include <lttv/common.h>

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
