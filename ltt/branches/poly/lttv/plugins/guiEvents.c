/*! \defgroup guiEvents libguiEvents: The GUI Events display plugin */
/*\@{*/

/*! \file guiEvents.c
 * \brief Graphical plugin for showing events.
 *
 * This plugin lists all the events contained in the current time interval
 * in a list.
 * 
 * This plugin adds a Events Viewer functionnality to Linux TraceToolkit
 * GUI when this plugin is loaded. The init and destroy functions add the
 * viewer's insertion menu item and toolbar icon by calling gtkTraceSet's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the TraceSet window.
 *
 * Author : Karim Yaghmour
 *          Integrated to LTTng by Mathieu Desnoyers, June 2003
 */

#include <glib.h>
#include <gmodule.h>
#include <gtk.h>
#include <gdk.h>

#include <lttv/module.h>

#include "icons/guiEventsInsert.xpm"

/** Array containing instanced objects. Used when module is unloaded */
static GPtrArray *RawTracesArray = NULL;

//! Event Viewer's constructor
GtkWidget *guiEvents(GtkWidget *ParentWindow);

/**
 * plugin's init function
 *
 * This function initializes the Event Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init() {
	g_critical("GUI Event Viewer init()");

	/* Register the toolbar insert button */
	ToolbarItemReg(guiEventsInsert_xpm, "Insert Event Viewer", guiEvent);

	/* Register the menu item insert entry */
	MenuItemReg("/", "Insert Event Viewer", guiEvent);

	RawTracesArray = g_ptr_array_new();
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
	int i;
	
	g_critical("GUI Event Viewer destroy()");

	for(i=0 ; i<RawTracesArray->len ; i++) {
		gtk_widget_destroy((Widget *)g_ptr_array_index(RawTracesArray,i));
	}
	
	g_ptr_array_free(RawTracesArray);

	/* Unregister the toolbar insert button */
	ToolbarItemUnreg(guiEvent);

	/* Unregister the menu item insert entry */
	MenuItemUnreg(guiEvents);
}

/**
 * Event Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
static GtkWidget *
guiEvents(GtkWidget *ParentWindow)
{
	

  /* Create raw trace list and pack it */
  pWindow->RTCList  = gtk_clist_new_with_titles(RTCLIST_NB_COLUMNS, RTCListTitles);
  gtk_clist_set_selection_mode(GTK_CLIST(pWindow->RTCList), GTK_SELECTION_SINGLE);
  gtk_box_pack_start(GTK_BOX(pWindow->RTHBox), pWindow->RTCList, TRUE, TRUE, 0);

  /* Create vertical scrollbar and pack it */
  pWindow->RTVScroll  = gtk_vscrollbar_new(NULL);
  gtk_box_pack_start(GTK_BOX(pWindow->RTHBox), pWindow->RTVScroll, FALSE, TRUE, 0);

  /* Get the vertical scrollbar's adjustment */
  pWindow->RTVAdjust = gtk_range_get_adjustment(GTK_RANGE(pWindow->RTVScroll));

  /* Configure the columns of the list */
  gtk_clist_set_column_justification(GTK_CLIST(pWindow->RTCList), 0, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification(GTK_CLIST(pWindow->RTCList), 1, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_justification(GTK_CLIST(pWindow->RTCList), 2, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(GTK_CLIST(pWindow->RTCList), 3, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(GTK_CLIST(pWindow->RTCList), 4, GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_justification(GTK_CLIST(pWindow->RTCList), 5, GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_width(GTK_CLIST(pWindow->RTCList), 0, 45);
  gtk_clist_set_column_width(GTK_CLIST(pWindow->RTCList), 1, 120);
  gtk_clist_set_column_width(GTK_CLIST(pWindow->RTCList), 2, 120);
  gtk_clist_set_column_width(GTK_CLIST(pWindow->RTCList), 3, 45);
  gtk_clist_set_column_width(GTK_CLIST(pWindow->RTCList), 4, 60);


	
  /*  Raw event trace */
  gtk_widget_show(pmWindow->RTHBox);
  gtk_widget_show(pmWindow->RTCList);
  gtk_widget_show(pmWindow->RTVScroll);


}

static GtkWidget
~guiEvents(GtkWidget *guiEvents)
{
  /*  Clear raw event trace */
  gtk_clist_clear(GTK_CLIST(pSysView->Window->RTCList));
  gtk_widget_queue_resize(pSysView->Window->RTCList);

  /* Reset the CList adjustment */
  pSysView->Window->RTVAdjust->lower          = 0;
  pSysView->Window->RTVAdjust->upper          = 0;
  pSysView->Window->RTVAdjust->step_increment = 0;
  pSysView->Window->RTVAdjust->page_increment = 0;
  pSysView->Window->RTVAdjust->page_size      = 0;
  gtk_adjustment_changed(GTK_ADJUSTMENT(pSysView->Window->RTVAdjust));

}


/* Imported code from LTT 0.9.6pre2 tracevisualizer */


/******************************************************************
 * Function :
 *    WDI_gtk_clist_set_last_row_data_full()
 * Description :
 *    Appends data to the last row of a GtkClist.
 * Parameters :
 * Return values :
 *    NONE.
 * History :
 *    J.H.D., 27/08/99, Initial typing.
 * Note :
 *    Based on gtk_clist_set_row_data_full() version 1.2.3.
 *    Much faster than using gtk_clist_set_row_data_full().
 ******************************************************************/
void WDI_gtk_clist_set_last_row_data_full(GtkCList*         pmClist,
					  gpointer          pmData,
					  GtkDestroyNotify  pmDestroy)
{
  GtkCListRow *pClistRow;

  g_return_if_fail (pmClist != NULL);
  g_return_if_fail (GTK_IS_CLIST (pmClist));
  g_return_if_fail (pmClist->row_list_end != NULL);

  pClistRow = pmClist->row_list_end->data;
  pClistRow->data    = pmData;
  pClistRow->destroy = pmDestroy;
}


/******************************************************************
 * Function :
 *    SHRTEventSelect()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
void SHRTEventSelect(GtkWidget*      pmCList,
		     gint            pmRow,
		     gint            pmColumn,
		     GdkEventButton* pmEvent,
		     gpointer        pmData)
{
  systemView*  pSysView;        /* The system being displayed */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* Store the selected event */
  pSysView->Window->LastSelectedEvent = *(event*) gtk_clist_get_row_data(GTK_CLIST(pmCList), pmRow);
  pSysView->Window->EventSelected = TRUE;
}

/******************************************************************
 * Function :
 *    SHRTEventButtonPress()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
void SHRTEventButtonPress(GtkWidget*      pmCList,
			  GdkEventButton* pmEvent,
			  gpointer        pmData)
{
  systemView*  pSysView;        /* The system being displayed */
  gint         row, column;     /* The clicked row and column */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* if we have a right-click event */
  if(pmEvent->button == 3)
    /* If we clicked on an item, get its row and column values */
    if(gtk_clist_get_selection_info(GTK_CLIST(pmCList), pmEvent->x, pmEvent->y, &row, &column))
      {
      /* Highlight the selected row */
      gtk_clist_select_row(GTK_CLIST(pmCList), row, column);

      /* Store the selected event */
      pSysView->Window->LastSelectedEvent = *(event*) gtk_clist_get_row_data(GTK_CLIST(pmCList), row);
      pSysView->Window->EventSelected = TRUE;

      /* Display the popup menu */
      gtk_menu_popup(GTK_MENU(pSysView->Window->RawEventPopup),
		     NULL, NULL, NULL, NULL,
		     pmEvent->button, GDK_CURRENT_TIME);
      }
}


/******************************************************************
 * Function :
 *    SHRTVAdjustValueChanged()
 * Description :
 * Parameters :
 * Return values :
 * History :
 * Note :
 ******************************************************************/
void SHRTVAdjustValueChanged(GtkAdjustment*  pmVAdjust,
			     gpointer        pmData)
{
  event        lEvent;          /* Event used for searching */
  guint32      lPosition;       /* The position to scroll to */
  systemView*  pSysView;        /* The system being displayed */

  /* Do we have anything meaningfull */
  if((pSysView = (systemView*) pmData) == NULL)
    return;

  /* Is there an event database? */
  if(pSysView->EventDB == NULL)
    return;

  /* Set the pointer to the first event */
  if(pSysView->EventDB->TraceStart == NULL)
    return;

  /* Are we closer to the beginning? */
  if((pmVAdjust->value - (pmVAdjust->upper / 2)) < 0)
    {
    /* Set the navigation pointer to the beginning of the list */
    lEvent =  pSysView->EventDB->FirstEvent;

    /* Calculate distance from beginning */
    lPosition = (guint32) pmVAdjust->value;

    /* Find the event in the event database */
    while(lPosition > 0)
      {
      lPosition--;
      if(DBEventNext(pSysView->EventDB, &lEvent) != TRUE)
	break;
      }
    }
  else
    {
    /* Set the navigation pointer to the end of the list */
    lEvent = pSysView->EventDB->LastEvent;

    /* Calculate distance from end */
    lPosition = (guint32) (pmVAdjust->upper - pmVAdjust->value);

    /* Find the event in the event database */
    while(lPosition > 0)
      {
      lPosition--;
      if(DBEventPrev(pSysView->EventDB, &lEvent) != TRUE)
	break;
      }
    }

  /* Fill the event list according to what was found */
  WDFillEventList(pSysView->Window->RTCList,
		  pSysView->EventDB,
		  pSysView->System,
		  &lEvent,
		  &(pSysView->Window->LastSelectedEvent));
}



/******************************************************************
 * Function :
 *    WDConnectSignals()
 * Description :
 *    Attaches signal handlers to the window items.
 * Parameters :
 *    pmSysView, System view for which signals have to be connected
 * Return values :
 *    NONE
 * History :
 * Note :
 *    This function attaches a pointer to the main window during
 *    the connect. This means that the handlers will get a pointer
 *    to the window in the data argument.
 ******************************************************************/
void WDConnectSignals(systemView* pmSysView)
{
  /* Raw event Popup menu */
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RawGotoProcess),
		     "activate",
		     GTK_SIGNAL_FUNC(SHGotoProcAnalysis),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RawViewEvent),
		     "activate",
		     GTK_SIGNAL_FUNC(SHViewEventInEG),
		     pmSysView);

  /* Set event list callbacks */
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTCList),
		     "select_row",
		     GTK_SIGNAL_FUNC(SHRTEventSelect),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTCList),
		     "button-press-event",
		     GTK_SIGNAL_FUNC(SHRTEventButtonPress),
		     pmSysView);
  gtk_signal_connect(GTK_OBJECT(pmSysView->Window->RTVAdjust),
		     "value-changed",
		     GTK_SIGNAL_FUNC(SHRTVAdjustValueChanged),
		     pmSysView);


}


/******************************************************************
 * Function :
 *    WDFillEventList()
 * Description :
 *    Fills the window's event list using the trace database.
 * Parameters :
 *    pmList, The list to be filled.
 *    pmTraceDB, The database of events.
 *    pmSystem, The system to which this list belongs.
 *    pmEvent, Event from which we start drawing.
 *    pmSelectedEvent, Event selected if any.
 * Return values :
 *    NONE.
 * History :
 *    K.Y., 18/06/99, Initial typing.
 * Note :
 ******************************************************************/
void WDFillEventList(GtkWidget*  pmList,
		     db*         pmTraceDB,
		     systemInfo* pmSystem,
		     event*      pmEvent,
		     event*      pmSelectedEvent)
{
  gint                i = 0;                              /* Generic index */
  event               lEvent;                             /* Generic event */
  gchar               lTimeStr[TIME_STR_LEN];             /* Time of event */
  static gchar*       lString[RTCLIST_NB_COLUMNS]={'\0'}; /* Strings describing event */
  process*            pProcess;                           /* Generic process pointer */
#if SUPP_RTAI
  RTAItask*           pTask = NULL;                       /* Generic task pointer */
#endif /* SUPP_RTAI */
  eventDescription    lEventDesc;                         /* Description of event */

  /* Did we allocate space for strings */
  if(lString[0] == NULL)
    /* Allocate space for strings */
    for (i = 0;  i < RTCLIST_NB_COLUMNS - 1; i++)
      lString[i] = (char*) g_malloc(MW_DEFAULT_STRLEN);

  /* Allocate space for description string */
  lString[RTCLIST_NB_COLUMNS - 1] = (char*) g_malloc(MW_LONG_STRLEN);

  /* If no event was supplied, start at the beginning */
  if(pmEvent == NULL)
    lEvent = pmTraceDB->FirstEvent;
  else
    lEvent = *pmEvent;

  /* Freeze and clear clist */
  gtk_clist_freeze(GTK_CLIST(pmList));
  gtk_clist_clear(GTK_CLIST(pmList));

  /* Reset index */
  i = 0;

  /* Go through the event list */
  do
    {
    /* Get the event description */
    DBEventDescription(pmTraceDB, &lEvent, TRUE, &lEventDesc);

    /* Get the event's process */
    pProcess = DBEventProcess(pmTraceDB, &lEvent, pmSystem, FALSE);

#if SUPP_RTAI
    /* Does this trace contain RTAI information */
    if(pmTraceDB->SystemType == TRACE_SYS_TYPE_RTAI_LINUX)
      /* Get the RTAI task to which this event belongs */
      pTask = RTAIDBEventTask(pmTraceDB, &lEvent, pmSystem, FALSE);
#endif /* SUPP_RTAI */

    /* Set the event's entry in the list of raw events displayed */
    sRawEventsDisplayed[i] = lEvent;

    /* Add text describing the event */
    /*  The CPU ID */
    if(pmTraceDB->LogCPUID == TRUE)
      snprintf(lString[0], MW_DEFAULT_STRLEN, "%d", lEventDesc.CPUID);
    else
      snprintf(lString[0], MW_DEFAULT_STRLEN, "0");

    /*  The event ID */
    snprintf(lString[1], MW_DEFAULT_STRLEN, "%s", pmTraceDB->EventString(pmTraceDB, lEventDesc.ID, &lEvent));

    /*  The event's time of occurence */
    DBFormatTimeInReadableString(lTimeStr,
				 lEventDesc.Time.tv_sec,
				 lEventDesc.Time.tv_usec);    
    snprintf(lString[2], MW_DEFAULT_STRLEN, "%s", lTimeStr);

    /* Is this an RT event */
    if(lEventDesc.ID <= TRACE_MAX)
      {
      /*  The PID of the process to which the event belongs */
      if(pProcess != NULL)
	snprintf(lString[3], MW_DEFAULT_STRLEN, "%d", pProcess->PID);
      else
	snprintf(lString[3], MW_DEFAULT_STRLEN, "N/A");
      }
#if SUPP_RTAI
    else
      {
      /*  The TID of the task to which the event belongs */
      if(pTask != NULL)
	snprintf(lString[3], MW_DEFAULT_STRLEN, "RT:%d", pTask->TID);
      else
	snprintf(lString[3], MW_DEFAULT_STRLEN, "RT:N/A");
      }
#endif /* SUPP_RTAI */

    /*  The size of the entry */
    snprintf(lString[4], MW_DEFAULT_STRLEN, "%d", lEventDesc.Size);

    /*  The string describing the event */
    snprintf(lString[5], MW_LONG_STRLEN, "%s", lEventDesc.String);

    /* Insert the entry into the list */
    gtk_clist_append(GTK_CLIST(pmList), lString);

    /* Set the row's data to point to the current event */
    WDI_gtk_clist_set_last_row_data_full(GTK_CLIST(pmList), (gpointer) &(sRawEventsDisplayed[i]), NULL);

    /* Was this the last selected event */
    if(DBEventsEqual(lEvent, (*pmSelectedEvent)))
      gtk_clist_select_row(GTK_CLIST(pmList), i, 0);

    /* Go to next row */
    i++;
    } while((DBEventNext(pmTraceDB, &lEvent) == TRUE) && (i < RTCLIST_NB_ROWS));

  /* Resize the list's length */
  gtk_widget_queue_resize(pmList);

  /* Thaw the clist */
  gtk_clist_thaw(GTK_CLIST(pmList));
}




/*\@}*/
