#include <gtk/gtk.h>

/* internal functions */

void createNewWindow(GtkWidget* widget, gpointer user_data, gboolean clone);


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
on_close_tab_activate                  (GtkMenuItem     *menuitem,
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
on_load_module_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_module_search_path_activate     (GtkMenuItem     *menuitem,
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
on_button_open_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_add_trace_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_remove_trace_clicked         (GtkButton       *button,
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
on_MWindow_destroy                     (GtkObject       *object,
                                        gpointer         user_data);


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
