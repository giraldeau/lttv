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


#include "lttv_plugin_xfv.h"
#include <lttvwindow/lttvwindow.h>
#include "xenoltt_drawing.h"

/* 
 * forward definitions
 */

/*
 * Implementation
 */

static void xfv_update_filter(LttvPlugin *parent, LttvFilter *filter)
{
  LttvPluginXFV *self = LTTV_PLUGIN_XFV(parent);
  g_message("In XFV update filter.");
  lttv_filter_destroy(self->xfd->filter);
  self->xfd->filter = filter;
  redraw_notify(self->xfd, NULL);
}


static void
lttv_plugin_xfv_class_init (LttvPluginXFVClass *klass)
{
  LttvPluginClass *parent_klass;
  parent_klass = &klass->parent;
  parent_klass->update_filter = xfv_update_filter;
  g_type_class_add_private (klass, sizeof (XenoLTTData));
}


static void
lttv_plugin_xfv_init (GTypeInstance *instance, gpointer g_class)
{
  LttvPluginXFV *self = LTTV_PLUGIN_XFV (instance);
  self->xfd = G_TYPE_INSTANCE_GET_PRIVATE (self,
      LTTV_TYPE_PLUGIN_XFV, XenoLTTData);
}


GType
lttv_plugin_xfv_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvPluginXFVClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      lttv_plugin_xfv_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvPluginXFV),
      0,      /* n_preallocs */
      lttv_plugin_xfv_init    /* instance_init */
      };
      type = g_type_register_static (G_TYPE_OBJECT,
                                     "LttvPluginXFVType",
                                     &info, 0);
    }
    return type;
}


