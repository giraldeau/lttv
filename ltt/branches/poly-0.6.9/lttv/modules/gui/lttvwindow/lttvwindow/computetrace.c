/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* This file does not even compile yet. It is a starting point to compute
   some values in the background. This is why process_trace was split in
   three. However, process_trace_middle as it is currently is would not work.
   It needs to reinitialize its trace event positions each time since,
   in between background calls to process middle, other foreground calls to 
   process_middle can happen. */

#include <lttvwindow/idleprocesstrace.h>

/* The calling function has checked that the needed information has not
   been or is not being computed yet, has prepared the trace, and now all
   that is needed is to queue it for processing.

   CHECK remove the work_queue global variable, have an automatic adjustment
   of the number of events to process by iteration. */

static gboolean inserted = false;

static GList *work_queue = NULL;

typedef struct _WorkPiece WorkPiece;

struct _WorkPiece {
  LttvTracesetContext *self;
  LttTime end;
  unsigned nb_events;
  LttvHook f;
  void *hook_data;
  unsigned nb_done;
}

guint lttv_process_traceset_piece(gpointer data)
{
  GList *first = g_list_first(work_queue);
  
  guint nb_done, nb_asked;

  if(first == NULL) {
    inserted = false;
    return false;
  }

  WorkPiece *work_piece = (WorkPiece *)first->data;
  nb_asked = work_piece->nb_events - work_piece->nb_done;
  nb_asked = min(nb_asked, 10000); 
  nb_done = lttv_process_trace_middle(work_piece->self,work_piece->end,
      nb_asked);
  work_piece->nb_done += nb_done;
  if(nb_done < nb_asked) {
    lttv_process_trace_end(work_piece->self);
    work_queue = g_list_delete(work_queue, first);
  }
}


void lttv_process_traceset_when_idle(LttvTracesetContext *self, LttTime end,
    unsigned nb_events, LttvHook f, void *hook_data)
{
  WorkPiece *work_piece = g_new(WorkPiece);
  work_piece->self = self;
  work_piece->end = end;
  work_piece->nb_events = nb_events;
  work_piece->f = f;
  work_piece->hook_data = hook_data;
  eork_piece->nb_done = 0;

  lttv_process_traceset_begin(self);
  work_queue = g_list_append(work_queue, work_piece);
  if(!inserted) g_idle_add(lttv_process_traceset_piece, work_queue);
}


