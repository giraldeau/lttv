
#include <gtk/gtk.h>
#include <glib.h>
#include "Process_List.h"
#include "Draw_Item.h"

/*****************************************************************************
 *                       Methods to synchronize process list                 *
 *****************************************************************************/

/* Enumeration of the columns */
enum
{
	PROCESS_COLUMN,
	PID_COLUMN,
	BIRTH_S_COLUMN,
	BIRTH_NS_COLUMN,
	N_COLUMNS
};


gint process_sort_func	(	GtkTreeModel *model,
				GtkTreeIter *it_a,
				GtkTreeIter *it_b,
				gpointer user_data)
{
	GValue a, b;

	memset(&a, 0, sizeof(GValue));
	memset(&b, 0, sizeof(GValue));
	
	/* Order by PID */
	gtk_tree_model_get_value(	model,
					it_a,
					PID_COLUMN,
					&a);

	gtk_tree_model_get_value(	model,
					it_b,
					PID_COLUMN,
					&b);

	if(G_VALUE_TYPE(&a) == G_TYPE_UINT
		&& G_VALUE_TYPE(&b) == G_TYPE_UINT )
	{
		if(g_value_get_uint(&a) > g_value_get_uint(&b))
		{
			g_value_unset(&a);
			g_value_unset(&b);
			return 1;
		}
		if(g_value_get_uint(&a) < g_value_get_uint(&b))
		{
			g_value_unset(&a);
			g_value_unset(&b);
			return 0;
		}
	}

	g_value_unset(&a);
	g_value_unset(&b);


	/* Order by birth second */
	gtk_tree_model_get_value(	model,
					it_a,
					BIRTH_S_COLUMN,
					&a);

	gtk_tree_model_get_value(	model,
					it_b,
					BIRTH_S_COLUMN,
					&b);


	if(G_VALUE_TYPE(&a) == G_TYPE_ULONG
		&& G_VALUE_TYPE(&b) == G_TYPE_ULONG )
	{
		if(g_value_get_ulong(&a) > g_value_get_ulong(&b))
		{
			g_value_unset(&a);
			g_value_unset(&b);
			return 1;
		}
		if(g_value_get_ulong(&a) < g_value_get_ulong(&b))
		{
			g_value_unset(&a);
			g_value_unset(&b);
			return 0;
		}

	}

	g_value_unset(&a);
	g_value_unset(&b);

	/* Order by birth nanosecond */
	gtk_tree_model_get_value(	model,
					it_a,
					BIRTH_NS_COLUMN,
					&a);

	gtk_tree_model_get_value(	model,
					it_b,
					BIRTH_NS_COLUMN,
					&b);


	if(G_VALUE_TYPE(&a) == G_TYPE_ULONG
		&& G_VALUE_TYPE(&b) == G_TYPE_ULONG )
	{
		if(g_value_get_ulong(&a) > g_value_get_ulong(&b))
		{
			g_value_unset(&a);
			g_value_unset(&b);
			return 1;
		}
		// Final condition
		//if(g_value_get_ulong(&a) < g_value_get_ulong(&b))
		//{
		//	g_value_unset(&a);
		//	g_value_unset(&b);
		//	return 0;
		//}

	}
	
	g_value_unset(&a);
	g_value_unset(&b);

	return 0;

}

guint hash_fct(gconstpointer key)
{
	return ((ProcessInfo*)key)->pid;
}

gboolean equ_fct(gconstpointer a, gconstpointer b)
{
	if(((ProcessInfo*)a)->pid != ((ProcessInfo*)b)->pid)
		return 0;
//	g_critical("compare %u and %u",((ProcessInfo*)a)->pid,((ProcessInfo*)b)->pid);
	if(((ProcessInfo*)a)->birth.tv_sec != ((ProcessInfo*)b)->birth.tv_sec)
		return 0;
//	g_critical("compare %u and %u",((ProcessInfo*)a)->birth.tv_sec,((ProcessInfo*)b)->birth.tv_sec);

	if(((ProcessInfo*)a)->birth.tv_nsec != ((ProcessInfo*)b)->birth.tv_nsec)
		return 0;
//	g_critical("compare %u and %u",((ProcessInfo*)a)->birth.tv_nsec,((ProcessInfo*)b)->birth.tv_nsec);

	return 1;
}

void destroy_hash_key(gpointer key);

void destroy_hash_data(gpointer data);




ProcessList *processlist_construct(void)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	ProcessList* Process_List = g_new(ProcessList,1);
	
	Process_List->Number_Of_Process = 0;

	/* Create the Process list */
	Process_List->Store_M = gtk_list_store_new ( 	N_COLUMNS,
							G_TYPE_STRING,
							G_TYPE_UINT,
							G_TYPE_ULONG,
							G_TYPE_ULONG);


	Process_List->Process_List_VC = 
		gtk_tree_view_new_with_model
		(GTK_TREE_MODEL (Process_List->Store_M));

	g_object_unref (G_OBJECT (Process_List->Store_M));

	gtk_tree_sortable_set_sort_func(
			GTK_TREE_SORTABLE(Process_List->Store_M),
			PID_COLUMN,
			process_sort_func,
			NULL,
			NULL);
	
	gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(Process_List->Store_M),
			PID_COLUMN,
			GTK_SORT_ASCENDING);
	
	Process_List->Process_Hash = g_hash_table_new_full(
			hash_fct, equ_fct,
			destroy_hash_key, destroy_hash_data
			);
	
	
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

	column = gtk_tree_view_column_new_with_attributes (	"PID",
								renderer,
								"text",
								PID_COLUMN,
								NULL);
	gtk_tree_view_append_column (
		GTK_TREE_VIEW (Process_List->Process_List_VC), column);


	column = gtk_tree_view_column_new_with_attributes (	"Birth sec",
								renderer,
								"text",
								BIRTH_S_COLUMN,
								NULL);
	gtk_tree_view_append_column (
		GTK_TREE_VIEW (Process_List->Process_List_VC), column);

	//gtk_tree_view_column_set_visible(column, 0);
	//
	column = gtk_tree_view_column_new_with_attributes (	"Birth nsec",
								renderer,
								"text",
								BIRTH_NS_COLUMN,
								NULL);
	gtk_tree_view_append_column (
		GTK_TREE_VIEW (Process_List->Process_List_VC), column);

	//gtk_tree_view_column_set_visible(column, 0);
	
	g_object_set_data_full(
			G_OBJECT(Process_List->Process_List_VC),
			"Process_List_Data",
			Process_List,
			(GDestroyNotify)processlist_destroy);

	Process_List->Test_Process_Sent = 0;
	
	return Process_List;
}
void processlist_destroy(ProcessList *Process_List)
{
	g_hash_table_destroy(Process_List->Process_Hash);
	Process_List->Process_Hash = NULL;

	g_free(Process_List);
}

GtkWidget *processlist_get_widget(ProcessList *Process_List)
{
	return Process_List->Process_List_VC;
}



gint get_cell_height(GtkTreeView *TreeView)
{
	gint height;
	GtkTreeViewColumn *Column = gtk_tree_view_get_column(TreeView, 0);
	//GList *Render_List = gtk_tree_view_column_get_cell_renderers(Column);
	//GtkCellRenderer *Renderer = g_list_first(Render_List)->data;
	
	//g_list_free(Render_List);
	gtk_tree_view_column_cell_get_size(Column, NULL, NULL, NULL, NULL, &height);
	//g_critical("cell 0 height : %u",height);
	
	return height;
}

void destroy_hash_key(gpointer key)
{
	g_free(key);
}

void destroy_hash_data(gpointer data)
{
	g_free(data);
}

int processlist_add(	ProcessList *Process_List,
			guint pid,
			LttTime *birth,
			gchar *name,
			guint *height,
			HashedProcessData **pmHashed_Process_Data)
{
	GtkTreeIter iter ;
	ProcessInfo *Process_Info = g_new(ProcessInfo, 1);
	HashedProcessData *Hashed_Process_Data = g_new(HashedProcessData, 1);
	*pmHashed_Process_Data = Hashed_Process_Data;
	
	Process_Info->pid = pid;
	Process_Info->birth = *birth;
	
	Hashed_Process_Data->draw_context = g_new(DrawContext, 1);
	Hashed_Process_Data->draw_context->drawable = NULL;
	Hashed_Process_Data->draw_context->gc = NULL;
	Hashed_Process_Data->draw_context->pango_layout = NULL;
	Hashed_Process_Data->draw_context->Current = g_new(DrawInfo,1);
	Hashed_Process_Data->draw_context->Current->over = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Current->over->x = -1;
	Hashed_Process_Data->draw_context->Current->over->y = -1;
	Hashed_Process_Data->draw_context->Current->middle = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Current->middle->x = -1;
	Hashed_Process_Data->draw_context->Current->middle->y = -1;
	Hashed_Process_Data->draw_context->Current->under = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Current->under->x = -1;
	Hashed_Process_Data->draw_context->Current->under->y = -1;
	Hashed_Process_Data->draw_context->Current->modify_over = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Current->modify_over->x = -1;
	Hashed_Process_Data->draw_context->Current->modify_over->y = -1;
	Hashed_Process_Data->draw_context->Current->modify_middle = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Current->modify_middle->x = -1;
	Hashed_Process_Data->draw_context->Current->modify_middle->y = -1;
	Hashed_Process_Data->draw_context->Current->modify_under = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Current->modify_under->x = -1;
	Hashed_Process_Data->draw_context->Current->modify_under->y = -1;
	Hashed_Process_Data->draw_context->Current->status = LTTV_STATE_UNNAMED;
	Hashed_Process_Data->draw_context->Previous = g_new(DrawInfo,1);
	Hashed_Process_Data->draw_context->Previous->over = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Previous->over->x = -1;
	Hashed_Process_Data->draw_context->Previous->over->y = -1;
	Hashed_Process_Data->draw_context->Previous->middle = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Previous->middle->x = -1;
	Hashed_Process_Data->draw_context->Previous->middle->y = -1;
	Hashed_Process_Data->draw_context->Previous->under = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Previous->under->x = -1;
	Hashed_Process_Data->draw_context->Previous->under->y = -1;
	Hashed_Process_Data->draw_context->Previous->modify_over = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Previous->modify_over->x = -1;
	Hashed_Process_Data->draw_context->Previous->modify_over->y = -1;
	Hashed_Process_Data->draw_context->Previous->modify_middle = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Previous->modify_middle->x = -1;
	Hashed_Process_Data->draw_context->Previous->modify_middle->y = -1;
	Hashed_Process_Data->draw_context->Previous->modify_under = g_new(ItemInfo,1);
	Hashed_Process_Data->draw_context->Previous->modify_under->x = -1;
	Hashed_Process_Data->draw_context->Previous->modify_under->y = -1;
	Hashed_Process_Data->draw_context->Previous->status = LTTV_STATE_UNNAMED;
	
	/* Add a new row to the model */
	gtk_list_store_append (	Process_List->Store_M, &iter);
	//g_critical ( "iter before : %s", gtk_tree_path_to_string (
	//		gtk_tree_model_get_path (
	//				GTK_TREE_MODEL(Process_List->Store_M),
	//				&iter)));
	gtk_list_store_set (	Process_List->Store_M, &iter,
				PROCESS_COLUMN, name,
				PID_COLUMN, pid,
				BIRTH_S_COLUMN, birth->tv_sec,
				BIRTH_NS_COLUMN, birth->tv_nsec,
				-1);
	Hashed_Process_Data->RowRef = gtk_tree_row_reference_new (
			GTK_TREE_MODEL(Process_List->Store_M),
			gtk_tree_model_get_path(
				GTK_TREE_MODEL(Process_List->Store_M),
				&iter));
	g_hash_table_insert(	Process_List->Process_Hash,
				(gpointer)Process_Info,
				(gpointer)Hashed_Process_Data);
	
	//g_critical ( "iter after : %s", gtk_tree_path_to_string (
	//		gtk_tree_model_get_path (
	//				GTK_TREE_MODEL(Process_List->Store_M),
	//				&iter)));
	Process_List->Number_Of_Process++;

	*height = get_cell_height(GTK_TREE_VIEW(Process_List->Process_List_VC))
				* Process_List->Number_Of_Process ;
	
	
	return 0;
	
}

int processlist_remove(	ProcessList *Process_List,
			guint pid,
			LttTime *birth)
{
	ProcessInfo Process_Info;
	gint *path_indices;
	HashedProcessData *Hashed_Process_Data;
	GtkTreeIter iter;
	
	Process_Info.pid = pid;
	Process_Info.birth = *birth;


	if(Hashed_Process_Data = 
		(HashedProcessData*)g_hash_table_lookup(
					Process_List->Process_Hash,
					&Process_Info))
	{
		gtk_tree_model_get_iter (
				GTK_TREE_MODEL(Process_List->Store_M),
				&iter,
				gtk_tree_row_reference_get_path(
					(GtkTreeRowReference*)Hashed_Process_Data->RowRef)
				);

		gtk_list_store_remove (Process_List->Store_M, &iter);
		
		g_free(Hashed_Process_Data->draw_context->Previous->modify_under);
		g_free(Hashed_Process_Data->draw_context->Previous->modify_middle);
		g_free(Hashed_Process_Data->draw_context->Previous->modify_over);
		g_free(Hashed_Process_Data->draw_context->Previous->under);
		g_free(Hashed_Process_Data->draw_context->Previous->middle);
		g_free(Hashed_Process_Data->draw_context->Previous->over);
		g_free(Hashed_Process_Data->draw_context->Previous);
		g_free(Hashed_Process_Data->draw_context->Current->modify_under);
		g_free(Hashed_Process_Data->draw_context->Current->modify_middle);
		g_free(Hashed_Process_Data->draw_context->Current->modify_over);
		g_free(Hashed_Process_Data->draw_context->Current->under);
		g_free(Hashed_Process_Data->draw_context->Current->middle);
		g_free(Hashed_Process_Data->draw_context->Current->over);
		g_free(Hashed_Process_Data->draw_context->Current);
		g_free(Hashed_Process_Data->draw_context);
		g_free(Hashed_Process_Data);

		g_hash_table_remove(Process_List->Process_Hash,
				&Process_Info);
		
		Process_List->Number_Of_Process--;

		return 0;	
	} else {
		return 1;
	}
}


guint processlist_get_height(ProcessList *Process_List)
{
	return get_cell_height(GTK_TREE_VIEW(Process_List->Process_List_VC))
				* Process_List->Number_Of_Process ;
}


gint processlist_get_process_pixels(	ProcessList *Process_List,
					guint pid, LttTime *birth,
					guint *y,
					guint *height,
					HashedProcessData **pmHashed_Process_Data)
{
	ProcessInfo Process_Info;
	gint *path_indices;
	GtkTreePath *tree_path;
	HashedProcessData *Hashed_Process_Data = NULL;

	Process_Info.pid = pid;
	Process_Info.birth = *birth;

	if(Hashed_Process_Data = 
		(HashedProcessData*)g_hash_table_lookup(
					Process_List->Process_Hash,
					&Process_Info))
	{
		tree_path = gtk_tree_row_reference_get_path(
										Hashed_Process_Data->RowRef);
		path_indices =	gtk_tree_path_get_indices (tree_path);

	 	*height = get_cell_height(
				GTK_TREE_VIEW(Process_List->Process_List_VC));
		*y = *height * path_indices[0];
		*pmHashed_Process_Data = Hashed_Process_Data;
		return 0;	
	} else {
		*pmHashed_Process_Data = Hashed_Process_Data;
		return 1;
	}

}


gint processlist_get_pixels_from_data(	ProcessList *Process_List,
					ProcessInfo *process_info,
					HashedProcessData *Hashed_Process_Data,
					guint *y,
					guint *height)
{
	gint *path_indices;
	GtkTreePath *tree_path;

	tree_path = gtk_tree_row_reference_get_path(
									Hashed_Process_Data->RowRef);
	path_indices =	gtk_tree_path_get_indices (tree_path);

	*height = get_cell_height(
			GTK_TREE_VIEW(Process_List->Process_List_VC));
	*y = *height * path_indices[0];

	return 0;	

}
