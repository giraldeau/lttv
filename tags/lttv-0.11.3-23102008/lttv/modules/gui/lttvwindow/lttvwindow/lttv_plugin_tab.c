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


#include "lttv_plugin_tab.h"
#include <lttvwindow/lttvwindow.h>

/* 
 * forward definitions
 */

/*
 * Implementation
 */

static void tab_update_filter(LttvPlugin *parent, LttvFilter *filter)
{
  LttvPluginTab *self = LTTV_PLUGIN_TAB(parent);
  g_message("In tab update filter.");
  lttv_filter_destroy(self->tab->filter);
  self->tab->filter = filter;
  lttvwindow_report_filter(self->tab, filter);
}


static void
lttv_plugin_tab_class_init (LttvPluginTabClass *klass)
{
  LttvPluginClass *parent_klass;
  parent_klass = &klass->parent;
  parent_klass->update_filter = tab_update_filter;
  g_type_class_add_private (klass, sizeof (Tab));
}


static void
lttv_plugin_tab_init (GTypeInstance *instance, gpointer g_class)
{
  LttvPluginTab *self = LTTV_PLUGIN_TAB (instance);
  self->tab = G_TYPE_INSTANCE_GET_PRIVATE (self,
      LTTV_TYPE_PLUGIN_TAB, Tab);
}


__EXPORT GType
lttv_plugin_tab_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (LttvPluginTabClass),
      NULL,   /* base_init */
      NULL,   /* base_finalize */
      lttv_plugin_tab_class_init,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      sizeof (LttvPluginTab),
      0,      /* n_preallocs */
      lttv_plugin_tab_init    /* instance_init */
      };
      type = g_type_register_static (G_TYPE_OBJECT,
                                     "LttvPluginTabType",
                                     &info, 0);
    }
    return type;
}


