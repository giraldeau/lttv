/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers
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



#ifndef _XFV_H
#define _XFV_H

#include <gtk/gtk.h>
#include <lttvwindow/mainwindow.h>
#include <lttv/filter.h>
#include "xenoltt_threadlist.h"
#include <lttvwindow/lttv_plugin_tab.h>

extern GQuark LTT_NAME_CPU;

#ifndef TYPE_XENOLTT_DRAWING_T_DEFINED
#define TYPE_XENOLTT_DRAWING_T_DEFINED
typedef struct _XenoLtt_Drawing_t XenoLtt_Drawing_t;
#endif //TYPE_XENOLTT_DRAWING_T_DEFINED

#ifndef TYPE_XENOLTTDATA_DEFINED
#define TYPE_XENOLTTDATA_DEFINED
typedef struct _XenoLTTData XenoLTTData;
#endif //TYPE_XENOLTTDATA_DEFINED

struct _XenoLTTData {

  GtkWidget *top_widget;
  Tab *tab;
  LttvPluginTab *ptab;
  
  GtkWidget *hbox;
  GtkWidget *toolbar; /* Vbox that contains the viewer's toolbar */
  GtkToolItem *button_prop; /* Properties button. */
  GtkToolItem *button_filter; /* Properties button. */
  GtkWidget *box; /* box that contains the hpaned. necessary for it to work */
  GtkWidget *h_paned;

  ThreadList *thread_list;

  XenoLtt_Drawing_t *drawing;
  GtkAdjustment *v_adjust ;
  
  guint number_of_thread;
  guint background_info_waiting; /* Number of background requests waited for
                                    in order to have all the info ready. */

  LttvFilter *filter;
} ;

/* XenoLTT Data constructor */
XenoLTTData *guixenoltt(LttvPluginTab *ptab);
void guixenoltt_destructor_full(gpointer data);
void guixenoltt_destructor(gpointer data);

static inline GtkWidget *guixenoltt_get_widget(XenoLTTData *xenoltt_data){
    return xenoltt_data->top_widget ;
}

static inline ThreadList *guixenoltt_get_thread_list(XenoLTTData *xenoltt_data){
    return xenoltt_data->thread_list ;
}



#endif // _XFV_H
