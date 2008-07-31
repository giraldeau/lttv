/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2006 Mathieu Desnoyers
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


#include "lttv_plugin_evd.h"
#include <lttvwindow/lttvwindow.h>
#include "events.h"

/* 
 * forward definitions
 */

/*
 * Implementation
 */

static void evd_update_filter(LttvPlugin *parent, LttvFilter *filter)
{
  LttvPluginEVD *self = LTTV_PLUGIN_EVD(parent);
  g_message("In EVD update filter.");
  lttv_filter_destroy(self->evd->filter);
  self->evd->filter = filter;
  evd_redraw_notify(self->evd, NULL);
}


static void
lttv_plugin_evd_class_init (LttvPluginEVDClass *klass)
{
  LttvPluginClass *parent_klass;
  parent_klass = &klass->parent;
  parent_klass->update_filter = evd_update_filter;
  g_type_class_add_private (klass, sizeof (EventViewerData));
}


static void
lttv_plugin_evd_init (GTypeInstance *instance, gpointer g_class)
{
  LttvPluginEVD *self = LTTV_PLUGIN_EVD (instance);
  self->evd = G_TYPE_INSTANCE_GET_PRIVATE (self,
      LTTV_TYPE_PLUGIN_EVD, EventViewerData);
}


GType
lttv_plugin_evd_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvPluginEVDClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      lttv_plugin_evd_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvPluginEVD),
      0,      /* n_preallocs */
      lttv_plugin_evd_init    /* instance_init */
      };
      type = g_type_register_static (G_TYPE_OBJECT,
                                     "LttvPluginEVDType",
                                     &info, 0);
    }
    return type;
}


