/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
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

#include <lttv/lttv.h>
#include <lttv/toolbar.h>


inline LttvToolbars *lttv_toolbars_new() {
  return g_array_new(FALSE, FALSE, sizeof(lttv_toolbar_closure));
}

/* MD: delete elements of the array also, but don't free pointed addresses
 * (functions).
 */
inline void lttv_toolbars_destroy(LttvToolbars *h) {
  g_debug("lttv_toolbars_destroy");
  g_array_free(h, TRUE);
}

inline void lttv_toolbars_add(LttvToolbars *h, lttv_constructor f, char* tooltip, char ** pixmap)
{
  lttv_toolbar_closure c;

  /* if h is null, do nothing, or popup a warning message */
  if(h == NULL)return;

  c.con = f;
  c.tooltip = tooltip;
  c.pixmap = pixmap;
  g_array_append_val(h,c);
}

gboolean lttv_toolbars_remove(LttvToolbars *h, lttv_constructor f)
{
  lttv_toolbar_closure * tmp;
  gint i;
  for(i=0;i<h->len;i++){
    tmp = & g_array_index(h, lttv_toolbar_closure, i);
    if(tmp->con == f)break;
  }
  if(i<h->len){
    g_array_remove_index(h, i);
    return TRUE;
  }else return FALSE;
}

unsigned lttv_toolbars_number(LttvToolbars *h)
{
  return h->len;
}


