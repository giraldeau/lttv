
#ifndef __GTK_MULTI_VPANED_H__
#define __GTK_MULTI_VPANED_H__


#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>
#include <lttv/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_MULTI_VPANED            (gtk_multi_vpaned_get_type ())
#define GTK_MULTI_VPANED(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_MULTI_VPANED, GtkMultiVPaned))
#define GTK_MULTI_VPANED_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_MULTI_VPANED, GtkMultiVPanedClass))
#define GTK_IS_MULTI_VPANED(obj   )      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_MULTI_VPANED))
#define GTK_IS_MULTI_VPANED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MULTI_VPANED))
#define GTK_MULTI_VPANED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_MULTI_VPANED, GtkMultiVPanedClass))


typedef struct _GtkMultiVPaned	 GtkMultiVPaned;
typedef struct _GtkMultiVPanedClass   GtkMultiVPanedClass;

struct _GtkMultiVPaned
{
  GtkPaned container;

  /*< public >*/
  GtkPaned * first_pane;
  GtkPaned * last_pane;
  GtkPaned * focused_pane;
  guint num_children;

  GtkWidget * vbox;
  //  GtkWidget * scrollWindow;
  //  GtkWidget * viewport;
  GtkWidget * hscrollbar;  
  GtkAdjustment *hadjust;
  MainWindow * mw;
};

struct _GtkMultiVPanedClass
{
  GtkPanedClass parent_class;
};


GType	   gtk_multi_vpaned_get_type (void) G_GNUC_CONST;
GtkWidget* gtk_multi_vpaned_new (void);

void gtk_multi_vpaned_set_focus (GtkWidget * widget, gpointer user_data);     
void gtk_multi_vpaned_widget_add(GtkMultiVPaned * multi_vpaned, GtkWidget * widget1);
void gtk_multi_vpaned_widget_delete(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_widget_move_up(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_widget_move_down(GtkMultiVPaned * multi_vpaned);
void gtk_multi_vpaned_set_adjust(GtkMultiVPaned * multi_vpaned, gboolean first_time);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MULTI_VPANED_H__ */
