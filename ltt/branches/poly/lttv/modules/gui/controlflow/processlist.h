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



#ifndef _PROCESS_LIST_H
#define _PROCESS_LIST_H

#include <gtk/gtk.h>
#include <lttv/state.h>
#include <ltt/ltt.h>

#include "drawitem.h"

/* The process list
 *
 * Tasks :
 * Create a process list
 * contains the data for the process list
 * tells the height of the process list widget
 * provides methods to add/remove process from the list
 *  note : the sync with drawing is left to the caller.
 * provides helper function to convert a process unique identifier to
 *  pixels (in height).
 *
 * //FIXME : connect the scrolled window adjustment with the list.
 */

typedef struct _ProcessInfo {
  
  guint pid;
  GQuark cpu; /* only for PID 0 */
  guint ppid;
  LttTime birth;
  guint trace_num;

} ProcessInfo;

typedef struct _HashedProcessData {
  
  GtkTreeRowReference *row_ref;
 // DrawContext *draw_context;
  /* Information on current drawing */
  struct {
    guint over;
    guint middle;
    guint under;
  } x; /* last x position saved by after state update */

  // FIXME : add info on last event ?

} HashedProcessData;
  
struct _ProcessList {
  
  GtkWidget *process_list_widget;
  GtkListStore *list_store;
  GtkWidget *button; /* one button of the tree view */

  /* A hash table by PID to speed up process position find in the list */
  GHashTable *process_hash;
  
  guint number_of_process;
};


typedef struct _ProcessList ProcessList;

ProcessList *processlist_construct(void);
void processlist_destroy(ProcessList *process_list);
GtkWidget *processlist_get_widget(ProcessList *process_list);

void processlist_clear(ProcessList *process_list);

// out : success (0) and height
/* CPU num is only used for PID 0 */
int processlist_add(ProcessList *process_list, guint pid, guint cpu, guint ppid,
    LttTime *birth, guint trace_num, const gchar *name, guint *height,
    HashedProcessData **hashed_process_data);
// out : success (0) and height
int processlist_remove(ProcessList *process_list, guint pid, guint cpu, 
    LttTime *birth, guint trace_num);

guint processlist_get_height(ProcessList *process_list);

// Returns 0 on success
gint processlist_get_process_pixels(ProcessList *process_list,
        guint pid, guint cpu, LttTime *birth, guint trace_num,
        guint *y, guint *height,
        HashedProcessData **hashed_process_data);

gint processlist_get_pixels_from_data(  ProcessList *process_list,
          ProcessInfo *process_info,
          HashedProcessData *hashed_process_data,
          guint *y,
          guint *height);

#endif // _PROCESS_LIST_H
