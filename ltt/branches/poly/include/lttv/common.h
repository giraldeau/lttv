#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <ltt/ltt.h>
#include <gtk/gtk.h>

typedef struct _mainWindow mainWindow;
typedef struct _systemView systemView;
typedef struct _tab tab;
typedef struct _TimeInterval TimeInterval;

/* constructor of the viewer */
typedef GtkWidget * (*lttv_constructor)(mainWindow * main_window);
typedef lttv_constructor view_constructor;



#endif // COMMON_H
