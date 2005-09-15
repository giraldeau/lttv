/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Mathieu Desnoyers
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/hook.h>

#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>

#include "hTraceControlInsert.xpm"
#include "TraceControlStart.xpm"
#include "TraceControlPause.xpm"
#include "TraceControlStop.xpm"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pty.h>
#include <utmp.h>
#include <sys/wait.h>
#include <sys/select.h>

#define MAX_ARGS_LEN PATH_MAX * 10

GSList *g_control_list = NULL ;

/*! \file lttv/modules/gui/tracecontrol/tracecontrol.c
 *  \brief Graphic trace start/stop control interface.
 *
 * This plugin interacts with lttctl to start/stop tracing. It needs to take the
 * root password to be able to interact with lttctl.
 *
 */

typedef struct _ControlData ControlData;

/*
 * Prototypes
 */
GtkWidget *guicontrol_get_widget(ControlData *tcd);
ControlData *gui_control(Tab *tab);
void gui_control_destructor(ControlData *tcd);
GtkWidget* h_guicontrol(Tab *tab);
void control_destroy_walk(gpointer data, gpointer user_data);
  
/*
 * Callback functions
 */

static void start_clicked (GtkButton *button, gpointer user_data);
static void pause_clicked (GtkButton *button, gpointer user_data);
static void stop_clicked (GtkButton *button, gpointer user_data);


/**
 *  @struct _ControlData
 *  
 *  @brief Main structure of gui control
 */
struct _ControlData {
  Tab *tab;                             /**< current tab of module */

  GtkWidget *window;                  /**< window */
  
  GtkWidget *main_box;                /**< main container */
  GtkWidget *start_button;
  GtkWidget *pause_button;
  GtkWidget *stop_button;
  GtkWidget *username_label;
  GtkWidget *username_entry;
  GtkWidget *password_label;
  GtkWidget *password_entry;
  GtkWidget *channel_dir_label;
  GtkWidget *channel_dir_entry;
  GtkWidget *trace_dir_label;
  GtkWidget *trace_dir_entry;
  GtkWidget *trace_name_label;
  GtkWidget *trace_name_entry;
  GtkWidget *trace_mode_label;
  GtkWidget *trace_mode_combo;
  GtkWidget *start_daemon_label;
  GtkWidget *start_daemon_check;
  GtkWidget *optional_label;
  GtkWidget *subbuf_size_label;
  GtkWidget *subbuf_size_entry;
  GtkWidget *subbuf_num_label;
  GtkWidget *subbuf_num_entry;
  GtkWidget *lttctl_path_label;
  GtkWidget *lttctl_path_entry;
  GtkWidget *lttd_path_label;
  GtkWidget *lttd_path_entry;
  GtkWidget *fac_path_label;
  GtkWidget *fac_path_entry;
};

/**
 *  @fn GtkWidget* guicontrol_get_widget(ControlData*)
 * 
 *  This function returns the current main widget 
 *  used by this module
 *  @param tcd the module struct
 *  @return The main widget
 */
GtkWidget*
guicontrol_get_widget(ControlData *tcd)
{
  return tcd->window;
}

/**
 *  @fn ControlData* gui_control(Tab*)
 * 
 *  Constructor is used to create ControlData data structure.
 *  @param tab The tab structure used by the widget
 *  @return The Filter viewer data created.
 */
ControlData*
gui_control(Tab *tab)
{
  g_debug("filter::gui_control()");

  unsigned i;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  ControlData* tcd = g_new(ControlData,1);

  tcd->tab  = tab;

  tcd->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(tcd->window), "LTTng Trace Control");
  /* 
   * Initiating GtkTable layout 
   * starts with 2 rows and 5 columns and 
   * expands when expressions added
   */
  tcd->main_box = gtk_table_new(14,7,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(tcd->main_box),5);
  gtk_table_set_col_spacings(GTK_TABLE(tcd->main_box),5);
  
  gtk_container_add(GTK_CONTAINER(tcd->window), GTK_WIDGET(tcd->main_box));
  
  GList *focus_chain = NULL;
  
  /*
   * start/pause/stop buttons
   */
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlStart_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  tcd->start_button = gtk_button_new_with_label("start");
  gtk_button_set_image(GTK_BUTTON(tcd->start_button), image);
  gtk_button_set_alignment(GTK_BUTTON(tcd->start_button), 0.0, 0.0);
  gtk_widget_show (tcd->start_button);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->start_button,6,7,0,1,GTK_FILL,GTK_FILL,2,2);
  
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlPause_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  tcd->pause_button = gtk_button_new_with_label("pause");
  gtk_button_set_image(GTK_BUTTON(tcd->pause_button), image);
  gtk_button_set_alignment(GTK_BUTTON(tcd->pause_button), 0.0, 0.0);
  gtk_widget_show (tcd->pause_button);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->pause_button,6,7,1,2,GTK_FILL,GTK_FILL,2,2);

  pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)TraceControlStop_xpm);
  image = gtk_image_new_from_pixbuf(pixbuf);
  tcd->stop_button = gtk_button_new_with_label("stop");
  gtk_button_set_image(GTK_BUTTON(tcd->stop_button), image);
  gtk_button_set_alignment(GTK_BUTTON(tcd->stop_button), 0.0, 0.0);
  gtk_widget_show (tcd->stop_button);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->stop_button,6,7,2,3,GTK_FILL,GTK_FILL,2,2);
  
  /*
   *  First half of the filter window
   *  - textual entry of filter expression
   *  - processing button
   */
  tcd->username_label = gtk_label_new("Username:");
  gtk_widget_show (tcd->username_label);
  tcd->username_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->username_entry),"root");
  gtk_widget_show (tcd->username_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->username_label,0,2,0,1,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->username_entry,2,6,0,1,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);



  tcd->password_label = gtk_label_new("Password:");
  gtk_widget_show (tcd->password_label);
  tcd->password_entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(tcd->password_entry), FALSE);
  gtk_widget_show (tcd->password_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->password_label,0,2,1,2,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->password_entry,2,6,1,2,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);


  tcd->channel_dir_label = gtk_label_new("Channel directory:");
  gtk_widget_show (tcd->channel_dir_label);
  tcd->channel_dir_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->channel_dir_entry),"/mnt/relayfs/ltt");
  gtk_widget_show (tcd->channel_dir_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->channel_dir_label,0,2,2,3,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->channel_dir_entry,2,6,2,3,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  tcd->trace_dir_label = gtk_label_new("Trace directory:");
  gtk_widget_show (tcd->trace_dir_label);
  tcd->trace_dir_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->trace_dir_entry),"/tmp/trace1");
  gtk_widget_show (tcd->trace_dir_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->trace_dir_label,0,2,3,4,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->trace_dir_entry,2,6,3,4,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  tcd->trace_name_label = gtk_label_new("Trace name:");
  gtk_widget_show (tcd->trace_name_label);
  tcd->trace_name_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->trace_name_entry),"trace");
  gtk_widget_show (tcd->trace_name_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->trace_name_label,0,2,4,5,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->trace_name_entry,2,6,4,5,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  tcd->trace_mode_label = gtk_label_new("Trace mode ");
  gtk_widget_show (tcd->trace_mode_label);
  tcd->trace_mode_combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(tcd->trace_mode_combo), 
      "normal");
  gtk_combo_box_append_text(GTK_COMBO_BOX(tcd->trace_mode_combo), 
      "flight recorder");
  gtk_combo_box_set_active(GTK_COMBO_BOX(tcd->trace_mode_combo), 0);
  gtk_widget_show (tcd->trace_mode_combo);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->trace_mode_label,0,2,5,6,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->trace_mode_combo,2,6,5,6,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  tcd->start_daemon_label = gtk_label_new("Start daemon ");
  gtk_widget_show (tcd->start_daemon_label);
  tcd->start_daemon_check = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tcd->start_daemon_check), TRUE);
  gtk_widget_show (tcd->start_daemon_check);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->start_daemon_label,0,2,6,7,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->start_daemon_check,2,6,6,7,GTK_FILL,GTK_FILL,0,0);

  tcd->optional_label = gtk_label_new("Optional fields ");
  gtk_widget_show (tcd->optional_label);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->optional_label,0,6,7,8,GTK_FILL,GTK_FILL,2,2);

  tcd->subbuf_size_label = gtk_label_new("Subbuffer size:");
  gtk_widget_show (tcd->subbuf_size_label);
  tcd->subbuf_size_entry = gtk_entry_new();
  gtk_widget_show (tcd->subbuf_size_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->subbuf_size_label,0,2,8,9,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->subbuf_size_entry,2,6,8,9,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  tcd->subbuf_num_label = gtk_label_new("Number of subbuffers:");
  gtk_widget_show (tcd->subbuf_num_label);
  tcd->subbuf_num_entry = gtk_entry_new();
  gtk_widget_show (tcd->subbuf_num_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->subbuf_num_label,0,2,9,10,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->subbuf_num_entry,2,6,9,10,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  tcd->lttctl_path_label = gtk_label_new("path to lttctl:");
  gtk_widget_show (tcd->lttctl_path_label);
  tcd->lttctl_path_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->lttctl_path_entry),PACKAGE_BIN_DIR "/lttctl");
  gtk_widget_show (tcd->lttctl_path_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->lttctl_path_label,0,2,10,11,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->lttctl_path_entry,2,6,10,11,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);


  tcd->lttd_path_label = gtk_label_new("path to lttd:");
  gtk_widget_show (tcd->lttd_path_label);
  tcd->lttd_path_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->lttd_path_entry),PACKAGE_BIN_DIR "/lttd");
  gtk_widget_show (tcd->lttd_path_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->lttd_path_label,0,2,11,12,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->lttd_path_entry,2,6,11,12,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);

  
  tcd->fac_path_label = gtk_label_new("path to facilities:");
  gtk_widget_show (tcd->fac_path_label);
  tcd->fac_path_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(tcd->fac_path_entry),PACKAGE_DATA_DIR "/facilities");
  gtk_widget_set_size_request(tcd->fac_path_entry, 250, -1);
  gtk_widget_show (tcd->fac_path_entry);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->fac_path_label,0,2,12,13,GTK_FILL,GTK_FILL,2,2);
  gtk_table_attach( GTK_TABLE(tcd->main_box),tcd->fac_path_entry,2,6,12,13,GTK_FILL|GTK_EXPAND|GTK_SHRINK,GTK_FILL,0,0);


  GtkWidget *start_button;
  GtkWidget *pause_button;
  GtkWidget *stop_button;

  GtkWidget *username_entry;
  GtkWidget *password_entry;
  GtkWidget *channel_dir_entry;
  GtkWidget *trace_dir_entry;
  GtkWidget *trace_name_entry;
  GtkWidget *trace_mode_combo;
  GtkWidget *start_daemon_check;
  GtkWidget *subbuf_size_entry;
  GtkWidget *subbuf_num_entry;
  GtkWidget *lttctl_path_entry;
  GtkWidget *lttd_path_entry;
  GtkWidget *fac_path_entry;

  focus_chain = g_list_append (focus_chain, tcd->username_entry);
  focus_chain = g_list_append (focus_chain, tcd->password_entry);
  focus_chain = g_list_append (focus_chain, tcd->start_button);
  focus_chain = g_list_append (focus_chain, tcd->pause_button);
  focus_chain = g_list_append (focus_chain, tcd->stop_button);
  focus_chain = g_list_append (focus_chain, tcd->channel_dir_entry);
  focus_chain = g_list_append (focus_chain, tcd->trace_dir_entry);
  focus_chain = g_list_append (focus_chain, tcd->trace_name_entry);
  focus_chain = g_list_append (focus_chain, tcd->trace_mode_combo);
  focus_chain = g_list_append (focus_chain, tcd->start_daemon_check);
  focus_chain = g_list_append (focus_chain, tcd->subbuf_size_entry);
  focus_chain = g_list_append (focus_chain, tcd->subbuf_num_entry);
  focus_chain = g_list_append (focus_chain, tcd->lttctl_path_entry);
  focus_chain = g_list_append (focus_chain, tcd->lttd_path_entry);
  focus_chain = g_list_append (focus_chain, tcd->fac_path_entry);

  gtk_container_set_focus_chain(GTK_CONTAINER(tcd->main_box), focus_chain);

  g_signal_connect(G_OBJECT(tcd->start_button), "clicked",
      (GCallback)start_clicked, tcd);
  g_signal_connect(G_OBJECT(tcd->pause_button), "clicked", 
      (GCallback)pause_clicked, tcd);
  g_signal_connect(G_OBJECT(tcd->stop_button), "clicked", 
      (GCallback)stop_clicked, tcd);

  /* 
   * show main container 
   */
  gtk_widget_show(tcd->main_box);
  gtk_widget_show(tcd->window);
  
  
  g_object_set_data_full(
      G_OBJECT(guicontrol_get_widget(tcd)),
      "control_viewer_data",
      tcd,
      (GDestroyNotify)gui_control_destructor);

  g_control_list = g_slist_append(
      g_control_list,
      tcd);
  
  return tcd;
}


/**
 *  @fn void gui_control_destructor(ControlData*)
 * 
 *  Destructor for the filter gui module
 *  @param tcd The module structure
 */
void
gui_control_destructor(ControlData *tcd)
{
  Tab *tab = tcd->tab;

  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(guicontrol_get_widget(tcd))){
    g_info("widget still exists");
  }
//  if(tab != NULL) {
//    lttvwindow_unregister_traceset_notify(tcd->tab,
//                                          filter_traceset_changed,
//                                          filter_viewer_data);
//  }
  lttvwindowtraces_background_notify_remove(tcd);
  
  g_control_list = g_slist_remove(g_control_list, tcd);
 
  g_free(tcd);
}

/* Callbacks */

void start_clicked (GtkButton *button, gpointer user_data)
{
  ControlData *tcd = (ControlData*)user_data;

  const gchar *username = gtk_entry_get_text(GTK_ENTRY(tcd->username_entry));
  const gchar *password = gtk_entry_get_text(GTK_ENTRY(tcd->password_entry));
  const gchar *channel_dir =
    gtk_entry_get_text(GTK_ENTRY(tcd->channel_dir_entry));
  const gchar *trace_dir = gtk_entry_get_text(GTK_ENTRY(tcd->trace_dir_entry));
  const gchar *trace_name =
    gtk_entry_get_text(GTK_ENTRY(tcd->trace_name_entry));
  
  const gchar *trace_mode_sel =
    gtk_combo_box_get_active_text(GTK_COMBO_BOX(tcd->trace_mode_combo));
  const gchar *trace_mode;
  if(strcmp(trace_mode_sel, "normal") == 0)
    trace_mode = "normal";
  else
    trace_mode = "flight";
  
  gboolean start_daemon =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tcd->start_daemon_check));
  
  const gchar *subbuf_size =
    gtk_entry_get_text(GTK_ENTRY(tcd->subbuf_size_entry));
  const gchar *subbuf_num =
    gtk_entry_get_text(GTK_ENTRY(tcd->subbuf_num_entry));
  const gchar *lttctl_path =
    gtk_entry_get_text(GTK_ENTRY(tcd->lttctl_path_entry));
  const gchar *lttd_path = gtk_entry_get_text(GTK_ENTRY(tcd->lttd_path_entry));
  const gchar *fac_path = gtk_entry_get_text(GTK_ENTRY(tcd->fac_path_entry));

  //pid_t pid = fork();
  pid_t pid;
  int fdpty;
  pid = forkpty(&fdpty, NULL, NULL, NULL);

  if(pid > 0) {
    /* parent */
    gchar buf[256];
    int status;
    ssize_t count;
    /* discuss with su */
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int nbdes;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fdpty, &readfds);

    nbdes = select(fdpty+1, &readfds, NULL, NULL, &timeout);

    if(nbdes > 0) {
      do {
        count = read (fdpty, buf, 256);
        if(count > 0) {
          buf[count] = '\0';
          printf("%s", buf);
        }
      } while(select(fdpty+1, &readfds, NULL, NULL, &timeout) > 0);

    } else if(nbdes == -1) {
      perror("Timeout occured when waiting for su password prompt");
      return;
    } else {
      g_warning("No data within 2 seconds when waiting for su prompt");
      return;
    }

    g_info("Got su prompt, now writing password...");
    sleep(1);
    int ret;
    ret = write(fdpty, password, strlen(password));
    if(ret < 0) perror("Error in write");
    ret = write(fdpty, "\n", 1);
    if(ret < 0) perror("Error in write");
    fsync(fdpty);

    FD_ZERO(&readfds);
    FD_SET(fdpty, &readfds);
    do {
      if (select(fdpty+1, &readfds, NULL, NULL, &timeout) < 0) {
        g_warning("Cannot read from child pipe");
        return;
      }
      if(FD_ISSET(fdpty, &readfds)) {
        count = read(fdpty, buf, 256);
        buf[count] = '\0';
        printf("%s", buf);
      } else FD_SET(fdpty, &readfds);
      usleep(200);
    } while(!(ret = waitpid(pid, &status, WNOHANG)));

   if(WIFEXITED(ret))
     if(WEXITSTATUS(ret) != 0)
      g_warning("An error occured in the su command, exit code : %hhu",
          WEXITSTATUS(ret));


  } else if(pid == 0) {
    /* child */
    gchar args[MAX_ARGS_LEN];
    gint args_left = MAX_ARGS_LEN - 1; /* for \0 */

    /* Setup environment variables */
    if(strcmp(lttd_path, "") != 0)
      setenv("LTT_DAEMON", lttd_path, 1);
    if(strcmp(fac_path, "") != 0)
      setenv("LTT_FACILITIES", fac_path, 1);
   
    /* Setup arguments to su */
    strncpy(args, "\"", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    
    if(strcmp(lttctl_path, "") == 0)
      lttctl_path = "lttctl";

    /* channel dir */
    strncat(args, "-l ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    strncat(args, channel_dir, args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* space */
    strncat(args, " ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* trace dir */
    strncat(args, "-t ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    strncat(args, trace_dir, args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* space */
    strncat(args, " ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    
    /* name */
    strncat(args, "-n ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    strncat(args, trace_name, args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* space */
    strncat(args, " ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* trace mode */
    strncat(args, "-m ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    strncat(args, trace_mode, args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* space */
    strncat(args, " ", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;

    /* Start daemon ? */
    if(start_daemon) {
      strncat(args, "-d", args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;
    } else {
      /* Simply create the channel and then start tracing */
      strncat(args, "-b", args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;
    }
    
    /* optional arguments */
    /* subbuffer size */
    if(strcmp(subbuf_size, "") != 0) {
      /* space */
      strncat(args, " ", args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;

      strncat(args, "-z ", args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;
      strncat(args, subbuf_size, args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;
    }

    /* number of subbuffers */
    if(strcmp(subbuf_num, "") != 0) {
      /* space */
      strncat(args, " ", args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;

      strncat(args, "-x ", args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;
      strncat(args, subbuf_num, args_left);
      args_left = MAX_ARGS_LEN - strlen(args) - 1;
    }
    
    strncat(args, "\"", args_left);
    args_left = MAX_ARGS_LEN - strlen(args) - 1;
    
    //printf("Executing (as %s) : %s %s\n", username, lttctl_path, args);
    
    execlp("su", "su", "-p", "-c", lttctl_path, username, args, NULL);
    exit(-1); /* not supposed to happen! */
    //system(args);
    //system("echo blah");
    //exit(0);
    
    //gint ret = execvp();
  
  } else {
    /* error */
    g_warning("Error happened when forking for su");
  }
  
}


void pause_clicked (GtkButton *button, gpointer user_data)
{
  ControlData *tcd = (ControlData*)user_data;


}

void stop_clicked (GtkButton *button, gpointer user_data)
{
  ControlData *tcd = (ControlData*)user_data;


}


/**
 *  @fn GtkWidget* h_guicontrol(Tab*)
 * 
 *  Control Module's constructor hook
 *
 *  This constructor is given as a parameter to the menuitem and toolbar button
 *  registration. It creates the list.
 *  @param tab A pointer to the parent window.
 *  @return The widget created.
 */
GtkWidget *
h_guicontrol(Tab *tab)
{
  ControlData* f = gui_control(tab) ;

  return NULL;
}

/**
 *  @fn static void init()
 * 
 *  This function initializes the Filter Viewer functionnality through the
 *  gtkTraceSet API.
 */
static void init() {

  lttvwindow_register_constructor("guicontrol",
                                  "/",
                                  "Insert Tracing Control Module",
                                  hTraceControlInsert_xpm,
                                  "Insert Tracing Control Module",
                                  h_guicontrol);
}

/**
 *  @fn void control_destroy_walk(gpointer,gpointer)
 * 
 *  Initiate the destruction of the current gui module
 *  on the GTK Interface
 */
void 
control_destroy_walk(gpointer data, gpointer user_data)
{
  ControlData *tcd = (ControlData*)data;

  g_debug("traceontrol.c : control_destroy_walk, %p", tcd);

  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guicontrol_get_widget(tcd)))
    gtk_widget_destroy(guicontrol_get_widget(tcd));
}

/**
 *  @fn static void destroy()
 *  @brief plugin's destroy function
 *
 *  This function releases the memory reserved by the module and unregisters
 *  everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
 
  g_slist_foreach(g_control_list, control_destroy_walk, NULL );
  
  lttvwindow_unregister_constructor(h_guicontrol);
  
}


LTTV_MODULE("guitracecontrol", "Trace Control Window", \
    "Graphical module that let user control kernel tracing", \
    init, destroy, "lttvwindow")

