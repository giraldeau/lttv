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


#include "lttv_plugin.h"

/* 
 * forward definitions
 */



static void
lttv_plugin_class_init (LttvPluginClass *klass)
{
  klass->update_filter = NULL; /* Pure Virtual */
}

static void
lttv_plugin_instance_init (GTypeInstance *instance, gpointer g_class)
{
  LttvPlugin *self;
  self->top_widget = NULL;
}

GType
lttv_plugin_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvPluginClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      lttv_plugin_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvPlugin),
      0,      /* n_preallocs */
      NULL    /* instance_init */
      };
      type = g_type_register_static (G_TYPE_OBJECT,
                                     "LttvPluginType",
                                     &info, 0);
    }
    return type;
}


/* implementation in the source file */
__EXPORT void lttv_plugin_update_filter (LttvPlugin *self, LttvFilter *filter)
{
  LTTV_PLUGIN_GET_CLASS (self)->update_filter (self, filter);
}




