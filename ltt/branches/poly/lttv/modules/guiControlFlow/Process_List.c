
#include "Process_List.h"

/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/

typedef struct _ProcessList {
	
	GtkWidget *Process_List_VC;
	GtkListStore *Store_M;
	
	guint Number_Of_Process;

} ProcessList ;


/* Enumeration of the columns */
enum
{
	PROCESS_COLUMN,
	N_COLUMNS
};


ProcessList *ProcessList(void)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	ProcessList* Process_List = g_new(ProcessList,1);
	
	/* Create the Process list */
	Process_List->Store_M = gtk_list_store_new ( 	N_COLUMNS,
							G_TYPE_STRING);


	Process_List->Process_List_VC = 
		gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (Process_List->Store_M));

	g_object_unref (G_OBJECT (Process_List->Store_M));

	gtk_tree_view_set_headers_visible(
		GTK_TREE_VIEW(Process_List->Process_List_VC), FALSE);

	/* Create a column, associating the "text" attribute of the
	 * cell_renderer to the first column of the model */
	/* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (	"Process",
								renderer,
								"text",
								PROCESS_COLUMN,
								NULL);
	gtk_tree_view_column_set_alignment (column, 0.0);
	gtk_tree_view_column_set_fixed_width (column, 45);
	gtk_tree_view_append_column (
		GTK_TREE_VIEW (Process_List->Process_List_VC), column);
	
	g_object_set_data_full(
			G_OBJECT(Process_List->Process_List_VC),
			"Process_List_Data",
			Process_List,
			ProcessList_destroy);
			
	
	return Process_List;
}

void ProcessList_destroy(ProcessList *Process_List)
{
	g_object_unref (G_OBJECT (Process_List->Process_List_VC));

	g_free(Process_List);
}

GtkWidget *ProcessList_getWidget(ProcessList *Process_List)
{
	return Process_List->Process_List_VC;
}



gint get_cell_height(GtkTreeView *TreeView)
{
	gint height, width;
	GtkTreeViewColumn *Column = gtk_tree_view_get_column(TreeView, 0);
	GList *Render_List = gtk_tree_view_column_get_cell_renderers(Column);
	GtkCellRenderer *Renderer = g_list_first(Render_List)->data;
	
	gtk_tree_view_column_cell_get_size(Column, NULL, NULL, NULL, NULL, &height);
	g_critical("cell 0 height : %u",height);
	
	return height;
}


int ProcessList_add(Process *myproc, ProcessList *Process_List, guint *height)
{
	// add proc to container

	height = get_cell_height(GTK_TREE_VIEW(Process_List->Process_List_VC))
				* Process_List->Number_Of_Process ;
	
	return 0;
	
}

int ProcessList_remove(Process *myproc, ProcessList *Process_List)
{
	// remove proc from container
	
	get_cell_height(GTK_TREE_VIEW(Process_List->Process_List_VC))
				* Process_List->Number_Of_Process ;
	
	return 0;
}
