#include <lttv/menu.h>


inline LttvMenus *lttv_menus_new() {
  return g_array_new(FALSE, FALSE, sizeof(lttv_menu_closure));
}

/* MD: delete elements of the array also, but don't free pointed addresses
 * (functions).
 */
inline void lttv_menus_destroy(LttvMenus *h) {
  g_critical("lttv_menus_destroy()");
  g_array_free(h, TRUE);
}

inline void lttv_menus_add(LttvMenus *h, lttv_constructor f, char* menuPath, char* menuText)
{
  lttv_menu_closure c;

  /* if h is null, do nothing, or popup a warning message */
  if(h == NULL)return;

  c.con = f;
  c.menuPath = menuPath;
  c.menuText = menuText;
  g_array_append_val(h,c);
}

gboolean lttv_menus_remove(LttvMenus *h, lttv_constructor f)
{
  lttv_menu_closure * tmp;
  gint i;
  for(i=0;i<h->len;i++){
    tmp = & g_array_index(h, lttv_menu_closure, i);
    if(tmp->con == f)break;
  }
  if(i<h->len){
    g_array_remove_index(h, i);
    return TRUE;
  }else return FALSE;
  
}

unsigned lttv_menus_number(LttvMenus *h)
{
  return h->len;
}


