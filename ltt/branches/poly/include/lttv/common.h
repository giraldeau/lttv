#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <ltt/ltt.h>
#include <gtk/gtk.h>

typedef struct _MainWindow MainWindow;
//typedef struct _systemView systemView;
typedef struct _Tab Tab;

/* constructor of the viewer */
typedef GtkWidget * (*lttv_constructor)(MainWindow * main_window);
typedef lttv_constructor view_constructor;

typedef struct _TimeWindow {
	LttTime start_time;
	LttTime time_width;
} TimeWindow;

#endif // COMMON_H
