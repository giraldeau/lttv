#ifndef _CFV_PRIVATE_H
#define _CFV_PRIVATE_H



struct _ControlFlowData {

	GtkWidget *Scrolled_Window_VC;
	MainWindow *Parent_Window;

	ProcessList *Process_List;
	Drawing_t *Drawing;

	//GtkWidget *HBox_V;
	//GtkWidget *Inside_HBox_V;
	GtkWidget *HPaned;
		
	GtkAdjustment *VAdjust_C ;
	
	/* Trace information */
	//TraceSet *Trace_Set;
	//TraceStatistics *Trace_Statistics;
	
	/* Shown events information */
	guint First_Event, Last_Event;
	TimeWindow Time_Window;
	LttTime Current_Time;
	
	
	/* TEST DATA, TO BE READ FROM THE TRACE */
	gint Number_Of_Events ;
	guint Currently_Selected_Event  ;
	gboolean Selected_Event ;
	guint Number_Of_Process;

} ;


#endif //_CFV_PRIVATE_H
