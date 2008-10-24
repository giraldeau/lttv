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

#include <gtk/gtk.h>
#include <lttvwindow/mainwindow.h>

/* internal functions */

void create_new_window(GtkWidget* widget, gpointer user_data, gboolean clone);
void insert_menu_toolbar_item(MainWindow * mw, gpointer user_data);
MainWindow *construct_main_window(MainWindow * parent);
void main_window_free(MainWindow * mw);
void main_window_destructor(MainWindow * mw);

void create_main_window_with_trace_list(GSList *traces);

void insert_viewer_wrap(GtkWidget *menuitem, gpointer user_data);
gboolean execute_events_requests(Tab *tab);


/* callback functions*/

void
on_empty_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clone_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tab_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_close_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_close_tab_X_clicked                 (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_close_tab_activate                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_add_trace_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove_trace_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_as_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_zoom_in_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_zoom_out_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_zoom_extended_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_go_to_time_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_show_time_frame_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_move_viewer_up_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_move_viewer_down_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove_viewer_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trace_filter_activate              (GtkMenuItem     *menuitem,
				       gpointer         user_data);

void
on_trace_facility_activate              (GtkMenuItem     *menuitem,
				       gpointer         user_data);

void
on_load_library_activate                (GtkMenuItem     *menuitem,
                                         gpointer         user_data);

void
on_unload_library_activate                (GtkMenuItem     *menuitem,
                                         gpointer         user_data);


void
on_load_module_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_library_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
void
on_remove_library_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_color_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_filter_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_configuration_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_content_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_button_new_clicked                  (GtkButton       *button,
                                        gpointer         user_data);
void
on_button_new_tab_clicked              (GtkButton       *button,
                                        gpointer         user_data);
void
on_button_open_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_add_trace_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_remove_trace_clicked         (GtkButton       *button,
                                        gpointer         user_data);
void
on_button_redraw_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_continue_processing_clicked  (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_stop_processing_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_save_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_save_as_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_zoom_in_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_zoom_out_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_zoom_extended_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_go_to_time_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_show_time_frame_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_move_up_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_move_down_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_delete_viewer_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_MWindow_destroy                     (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean    
on_MWindow_configure                   (GtkWidget         *widget,
                                        GdkEventConfigure *event,
                                        gpointer           user_data);

gboolean    
on_MWindow_expose                   (GtkWidget         *widget,
                                        GdkEventExpose *event,
                                        gpointer           user_data);
gboolean    
on_MWindow_after                   (GtkWidget         *widget,
                                        GdkEvent *event,
                                        gpointer           user_data);



void
on_insert_viewer_test_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
insertViewTest(GtkMenuItem *menuitem, gpointer user_data);

void
on_MNotebook_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data);


void
on_MEntry1_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data);
void
on_MEntry2_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data);
void
on_MEntry3_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data);
void
on_MEntry4_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data);
void
on_MEntry5_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data);
void
on_MEntry6_value_changed               (GtkSpinButton *spinbutton,
                                        gpointer user_data);


void time_change_manager               (Tab *tab,
                                        TimeWindow new_time_window);

void current_time_change_manager       (Tab *tab,
                                        LttTime new_current_time);

gboolean execute_time_requests(MainWindow * mw);
