#ifndef _DRAW_ITEM_H
#define _DRAW_ITEM_H

typedef struct _DrawContext DrawContext;
typedef struct _DrawInfo DrawInfo;
typedef struct _ItemInfo ItemInfo;

typedef struct _DrawOperation DrawOperation;


typedef struct _PropertiesText PropertiesText;
typedef struct _PropertiesIcon PropertiesIcon;
typedef struct _PropertiesLine PropertiesLine;
typedef struct _PropertiesArc PropertiesArc;
typedef struct _PropertiesBG PropertiesBG;


void draw_item(	GdkDrawable *drawable,
		gint x,
		gint y,
		LttvTraceState *ts,
		LttvTracefileState *tfs,
		LttvIAttribute *attributes);

/*
 * The tree of attributes used to store drawing operations goes like this :
 *
 * event_types/
 *   "facility-event_type"
 * cpus/
 *   "cpu name"
 * mode_types/
 *   "execution mode"/
 *     submodes/
 *       "submode"
 * process_states/
 *   "state name"
 * 
 * So if, for example, we want to add a hook to get called each time we
 * receive an event that is in state LTTV_STATE_SYSCALL, we put the
 * pointer to the GArray of DrawOperation in
 * process_states/ "name associated with LTTV_STATE_SYSCALL"
 */

/* 
 * The add_operation has to do a quick sort by priority to keep the operations
 * in the right order.
 */
void add_operation(	LttvIAttribute *attributes,
			gchar *pathname,
			DrawOperation *Operation);

/* 
 * The del_operation seeks the array present at pathname (if any) and
 * removes the DrawOperation if present. It returns 0 on success, -1
 * if it fails.
 */
gint del_operation(	LttvIAttribute *attributes,
			gchar *pathname,
			DrawOperation *Operation);

/* 
 * The clean_operations removes all operations present at a pathname.
 * returns 0 on success, -1 if it fails.
 */
gint clean_operations(	LttvIAttribute *attributes,
			gchar *pathname );


/* 
 * The list_operations gives a pointer to the operation array associated
 * with the pathname. It will be NULL if no operation is present.
 */
void list_operations(	LttvIAttribute *attributes,
			gchar *pathname,
			GArray **Operation);



/*
 * exec_operation executes the operations if present in the attributes, or
 * do nothing if not present.
 */
void exec_operations(	LttvIAttribute *attributes,
			gchar *pathname);


/*
 * Functions to create Properties structures.
 */

PropertiesText *properties_text_create(
	GdkColor	*foreground,
	GdkColor	*background,
	gint		size,
	gchar		*Text,
	RelPos		position);

PropertiesIcon *properties_icon_create(
	gchar		*icon_name,
	gint		width,
	gint		height,
	RelPos		position),

PropertiesLine *properties_line_create(
	GdkColor	*color,
	gint		line_width,
	GdkLineStyle	style,
	RelPos		position),

PropertiesArc *properties_arc_create(
	GdkColor	*color,
	gint		size,
	gboolean	filled,
	RelPos		position),

PropertiesBG *properties_bg_create(
	GdkColor	*color);




/*
 * Here follow the prototypes of the hook functions used to draw the
 * different items.
 */

gboolean draw_text( void *hook_data, void *call_data);
gboolean draw_icon( void *hook_data, void *call_data);
gboolean draw_line( void *hook_data, void *call_data);
gboolean draw_arc( void *hook_data, void *call_data);
gboolean draw_bg( void *hook_data, void *call_data);


#endif // _DRAW_ITEM_H
