
#ifndef __GTK_CUSTOM_H__
#define __GTK_CUSTOM_H__


#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>
#include <lttv/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_CUSTOM            (gtk_custom_get_type ())
#define GTK_CUSTOM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CUSTOM, GtkCustom))
#define GTK_CUSTOM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CUSTOM, GtkCustomClass))
#define GTK_IS_CUSTOM(obj   )      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CUSTOM))
#define GTK_IS_CUSTOM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CUSTOM))
#define GTK_CUSTOM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CUSTOM, GtkCustomClass))


typedef struct _GtkCustom	 GtkCustom;
typedef struct _GtkCustomClass   GtkCustomClass;

struct _GtkCustom
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

struct _GtkCustomClass
{
  GtkPanedClass parent_class;
};


GType	   gtk_custom_get_type	       (void) G_GNUC_CONST;
GtkWidget* gtk_custom_new (void);

void gtk_custom_set_focus (GtkWidget * widget, gpointer user_data);     
void gtk_custom_widget_add(GtkCustom * custom, GtkWidget * widget1);
void gtk_custom_widget_delete(GtkCustom * custom);
void gtk_custom_widget_move_up(GtkCustom * custom);
void gtk_custom_widget_move_down(GtkCustom * custom);
void gtk_custom_set_adjust(GtkCustom * custom, gboolean first_time);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_CUSTOM_H__ */
