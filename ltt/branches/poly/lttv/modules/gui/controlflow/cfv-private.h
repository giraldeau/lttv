#ifndef _CFV_PRIVATE_H
#define _CFV_PRIVATE_H



struct _ControlFlowData {

  GtkWidget *scrolled_window;
  MainWindow *mw;

  ProcessList *process_list;
  Drawing_t *drawing;

  GtkWidget *h_paned;
    
  GtkAdjustment *v_adjust ;
  
  /* Shown events information */
  TimeWindow time_window;
  LttTime current_time;
  
  //guint currently_Selected_Event  ;
  guint number_of_process;

} ;


#endif //_CFV_PRIVATE_H
