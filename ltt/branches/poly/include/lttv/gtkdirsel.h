/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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


#ifndef __GTK_DIR_SEL_H__
#define __GTK_DIR_SEL_H__


#include <gdk/gdk.h>
#include <gtk/gtkdialog.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_DIR_SELECTION            (gtk_dir_selection_get_type ())
#define GTK_DIR_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_DIR_SELECTION, GtkDirSelection))
#define GTK_DIR_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_DIR_SELECTION, GtkDirSelectionClass))
#define GTK_IS_DIR_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_DIR_SELECTION))
#define GTK_IS_DIR_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DIR_SELECTION))
#define GTK_DIR_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DIR_SELECTION, GtkDirSelectionClass))


typedef struct _GtkDirSelection       GtkDirSelection;
typedef struct _GtkDirSelectionClass  GtkDirSelectionClass;

struct _GtkDirSelection
{
  GtkDialog parent_instance;

  GtkWidget *dir_list;
  GtkWidget *file_list;
  GtkWidget *selection_entry;
  GtkWidget *selection_text;
  GtkWidget *main_vbox;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkWidget *help_button;
  GtkWidget *history_pulldown;
  GtkWidget *history_menu;
  GList     *history_list;
  GtkWidget *fileop_dialog;
  GtkWidget *fileop_entry;
  gchar     *fileop_file;
  gpointer   cmpl_state;
  
  GtkWidget *fileop_c_dir;
  GtkWidget *fileop_del_file;
  GtkWidget *fileop_ren_file;
  
  GtkWidget *button_area;
  GtkWidget *action_area;

  GPtrArray *selected_names;
  gchar     *last_selected;
};

struct _GtkDirSelectionClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType      gtk_dir_selection_get_type            (void) G_GNUC_CONST;
GtkWidget* gtk_dir_selection_new                 (const gchar      *title);
void       gtk_dir_selection_set_filename        (GtkDirSelection *filesel,
						   const gchar      *filename);
/* This function returns the selected filename in the C runtime's
 * multibyte string encoding, which may or may not be the same as that
 * used by GDK (UTF-8). To convert to UTF-8, call g_filename_to_utf8().
 * The returned string points to a statically allocated buffer and
 * should be copied away.
 */
G_CONST_RETURN gchar* gtk_dir_selection_get_filename        (GtkDirSelection *filesel);

void	   gtk_dir_selection_complete		  (GtkDirSelection *filesel,
						   const gchar	    *pattern);
void       gtk_dir_selection_show_fileop_buttons (GtkDirSelection *filesel);
void       gtk_dir_selection_hide_fileop_buttons (GtkDirSelection *filesel);

gchar**    gtk_dir_selection_get_selections      (GtkDirSelection *filesel);
const gchar *    gtk_dir_selection_get_dir (GtkDirSelection *filesel);
void       gtk_dir_selection_set_select_multiple (GtkDirSelection *filesel,
						   gboolean          select_multiple);
gboolean   gtk_dir_selection_get_select_multiple (GtkDirSelection *filesel);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_DIR_SEL_H__ */
