/******************************************************************************
 * Draw_Item.c
 *
 * This file contains methods responsible for drawing a generic type of data
 * in a drawable. Doing this generically will permit user defined drawing
 * behavior in a later time.
 *
 * This file provides an API which is meant to be reusable for all viewers that
 * need to show information in line, icon, text, background or point form in
 * a drawable area having time for x axis. The y axis, in the control flow
 * viewer case, is corresponding to the different processes, but it can be
 * reused integrally for cpu, and eventually locks, buffers, network
 * interfaces... What will differ between the viewers is the precise
 * information which interests us. We may think that the most useful
 * information for control flow are some specific events, like schedule
 * change, and processes'states. It may differ for a cpu viewer : the
 * interesting information could be more the execution mode of each cpu.
 * This API in meant to make viewer's writers life easier : it will become
 * a simple choice of icons and line types for the precise information
 * the viewer has to provide (agremented with keeping supplementary records
 * and modifying slightly the DrawContext to suit the needs.)
 *
 * We keep each data type in attributes, keys to specific information
 * being formed from the GQuark corresponding to the information received.
 * (facilities / facility_name / events / eventname.)
 * (cpus/cpu_name, process_states/ps_name,
 * execution_modes/em_name, execution_submodes/es_name).
 * The goal is then to provide a generic way to print information on the
 * screen for all this different information.
 *
 * Information can be printed as
 *
 * - text (text + color + size + position (over or under line)
 * - icon (icon filename, corresponding to a loaded icon, accessible through
 *   a GQuark. Icons are loaded statically at the guiControlFlow level during
 *   module initialization and can be added on the fly if not present in the
 *   GQuark.) The habitual place for xpm icons is in
 *   ${prefix}/share/LinuxTraceToolkit.) + position (over or under line)
 * - line (color, width, style)
 * - Arc (big points) (color, size)
 * - background color (color)
 *
 * An item is a leaf of the attributes tree. It is, in that case, including
 * all kind of events categories we can have. It then associates each category
 * with one or more actions (drawing something) or nothing.
 * 
 * Each item has an array of hooks (hook list). Each hook represents an
 * operation to perform. We seek the array each time we want to
 * draw an item. We execute each operation in order. An operation type
 * is associated with each hook to permit user listing and modification
 * of these operations. The operation type is also used to find the
 * corresponding priority for the sorting. Operation type and priorities
 * are enum and a static int table.
 *
 * The array has to be sorted by priority each time we add a task in it.
 * A priority is associated with each operation type. It permits
 * to perform background color selection before line or text drawing. We also
 * draw lines before text, so the text appears over the lines.
 *
 * Executing all the arrays of operations for a specific event (which
 * implies information for state, event, cpu, execution mode and submode)
 * has to be done in a same DrawContext. The goal there is to keep the offset
 * of the text and icons over and under the middle line, so a specific
 * event could be printed as (  R Si 0 for running, scheduled in, cpu 0  ),
 * text being easy to replace with icons. The DrawContext is passed as
 * call_data for the operation hooks.
 *
 * We use the lttv global attributes to keep track of the loaded icons.
 * If we need an icon, we look for it in the icons / icon name pathname.
 * If found, we use the pointer to it. If not, we load the pixmap in
 * memory and set the pointer to the GdkPixmap in the attributes. The
 * structure pointed to contains the pixmap and the mask bitmap.
 * 
 * Author : Mathieu Desnoyers, October 2003
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <lttv/hook.h>
#include <lttv/attribute.h>
#include <lttv/iattribute.h>
#include <string.h>

#include <lttv/processTrace.h>
#include <lttv/state.h>

#include "Draw_Item.h"


#define MAX_PATH_LEN 256

/* The DrawContext keeps information about the current drawing position and
 * the previous one, so we can use both to draw lines.
 *
 * over : position for drawing over the middle line.
 * middle : middle line position.
 * under : position for drawing under the middle line.
 *
 * the modify_* are used to take into account that we should go forward
 * when we draw a text, an arc or an icon, while it's unneeded when we
 * draw a line or background.
 *
 */
struct _DrawContext {
	GdkDrawable	*drawable;
	GdkGC		*gc;
	

	DrawInfo	*Current;
	DrawInfo	*Previous;
};

struct _DrawInfo {
	ItemInfo	*over;
	ItemInfo	*middle;
	ItemInfo	*under;
	
	ItemInfo	*modify_over;
	ItemInfo	*modify_middle;
	ItemInfo	*modify_under;
};

/* LttvExecutionState is accessible through the LttvTracefileState. Is has
 * a pointer to the LttvProcessState which points to the top of stack
 * execution state : LttvExecutionState *state.
 *
 * LttvExecutionState contains (useful here):
 * LttvExecutionMode t,
 * LttvExecutionSubmode n,
 * LttvProcessStatus s
 * 
 *
 * LttvTraceState will be used in the case we need the string of the
 * different processes, eventtype_names, syscall_names, trap_names, irq_names.
 *
 * LttvTracefileState also gives the cpu_name and, as it herits from
 * LttvTracefileContext, it gives the LttEvent structure, which is needed
 * to get facility name and event name.
 */
struct _ItemInfo {
	gint	x, y;
	LttvTraceState		*ts;
	LttvTracefileState	*tfs;
};

/*
 * Structure used to keep information about icons.
 */
struct _IconStruct {
	GdkPixmap *pixmap;
	GdkBitmap *mask;
};


/*
 * The Item element is only used so the DrawOperation is modifiable by users.
 * During drawing, only the Hook is needed.
 */
struct _DrawOperation {
	DrawableItems	Item;
	LttvHooks	*Hook;
};

/*
 * We define here each items that can be drawn, together with their
 * associated priority. Many item types can have the same priority,
 * it's only used for quicksorting the operations when we add a new one
 * to the array of operations to perform. Lower priorities are executed
 * first. So, for example, we may want to give background color a value
 * of 10 while a line would have 20, so the background color, which
 * is in fact a rectangle, does not hide the line.
 */

static int Items_Priorities[] = {
	50,	/* ITEM_TEXT */
	40,	/* ITEM_ICON */
	20,	/* ITEM_LINE */
	30,	/* ITEM_POINT */
	10	/* ITEM_BACKGROUND */
};

/*
 * Here are the different structures describing each item type that can be
 * drawn. They contain the information necessary to draw the item : not the
 * position (this is provided by the DrawContext), but the text, icon name,
 * line width, color; all the properties of the specific items.
 */

struct _PropertiesText {
	GdkColor	*foreground;
	GdkColor	*background;
	gint		size;
	gchar		*Text;
	RelPos		position;
};


struct _PropertiesIcon {
	gchar		*icon_name;
	gint		width;
	gint		height;
	RelPos		position;
};

struct _PropertiesLine {
	GdkColor	*color;
	gint		line_width;
	GdkLineStyle	style;
	RelPos		position;
};

struct _PropertiesArc {
	GdkColor	*color;
	gint		size;	/* We force circle by width = height */
	gboolean	filled;
	RelPos		position;
};

struct _PropertiesBG {
	GdkColor	*color;
};


/* Drawing hook functions */
gboolean draw_text( void *hook_data, void *call_data)
{
	PropertiesText *Properties = (PropertiesText*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

	PangoContext *context;
	PangoLayout *layout;
	PangoFontDescription *FontDesc;// = pango_font_description_new();
	gint Font_Size;
	PangoRectangle ink_rect;
		
	gdk_gc_set_foreground(Draw_Context->gc, Properties->foreground);
	gdk_gc_set_background(Draw_Context->gc, Properties->background);

	layout = gtk_widget_create_pango_layout(GTK_WIDGET(Draw_Context->drawable), NULL);
	context = pango_layout_get_context(layout);
	FontDesc = pango_context_get_font_description(context);
	Font_Size = pango_font_description_get_size(FontDesc);
	pango_font_description_set_size(FontDesc, Properties->size*PANGO_SCALE);
	
	
	pango_layout_set_text(layout, Properties->Text, -1);
	pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
	switch(Properties->position) {
		case OVER:
							gdk_draw_layout(Draw_Context->drawable, Draw_Context->gc,
								Draw_Context->Current->modify_over->x,
								Draw_Context->Current->modify_over->y,
								layout);
							Draw_Context->Current->modify_over->x += ink_rect.width;

			break;
		case MIDDLE:
							gdk_draw_layout(Draw_Context->drawable, Draw_Context->gc,
								Draw_Context->Current->modify_middle->x,
								Draw_Context->Current->modify_middle->y,
								layout);
							Draw_Context->Current->modify_middle->x += ink_rect.width;
			break;
		case UNDER:
							gdk_draw_layout(Draw_Context->drawable, Draw_Context->gc,
								Draw_Context->Current->modify_under->x,
								Draw_Context->Current->modify_under->y,
								layout);
							Draw_Context->Current->modify_under->x += ink_rect.width;
			break;
	}


	pango_font_description_set_size(FontDesc, Font_Size);
	g_free(layout);
	
	return 0;
}


/* To speed up the process, search in already loaded icons list first. Only
 * load it if not present.
 */
gboolean draw_icon( void *hook_data, void *call_data)
{
	PropertiesIcon *Properties = (PropertiesIcon*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
	LttvAttributeValue value;
	gchar icon_name[MAX_PATH_LEN] = "icons/";
	IconStruct *icon_info;
	
	strcat(icon_name, Properties->icon_name);
	
  g_assert(lttv_iattribute_find_by_path(attributes, icon_name,
      LTTV_POINTER, &value));
	if(*(value.v_pointer) == NULL)
	{
  	*(value.v_pointer) = icon_info = g_new(IconStruct,1);
		
		icon_info->pixmap = gdk_pixmap_create_from_xpm(Draw_Context->drawable,
													&icon_info->mask, NULL, Properties->icon_name);
	}
	else
	{
		icon_info = *(value.v_pointer);
	}
	
	gdk_gc_set_clip_mask(Draw_Context->gc, icon_info->mask);
	
	switch(Properties->position) {
		case OVER:
							gdk_gc_set_clip_origin(
									Draw_Context->gc,
									Draw_Context->Current->modify_over->x,
									Draw_Context->Current->modify_over->y);
							gdk_draw_drawable(Draw_Context->drawable, 
									Draw_Context->gc,
									icon_info->pixmap,
									0, 0,
									Draw_Context->Current->modify_over->x,
									Draw_Context->Current->modify_over->y,
									Properties->width, Properties->height);

							Draw_Context->Current->modify_over->x += Properties->width;

			break;
		case MIDDLE:
							gdk_gc_set_clip_origin(
									Draw_Context->gc,
									Draw_Context->Current->modify_middle->x,
									Draw_Context->Current->modify_middle->y);
							gdk_draw_drawable(Draw_Context->drawable, 
									Draw_Context->gc,
									icon_info->pixmap,
									0, 0,
									Draw_Context->Current->modify_middle->x,
									Draw_Context->Current->modify_middle->y,
									Properties->width, Properties->height);

							Draw_Context->Current->modify_middle->x += Properties->width;
			break;
		case UNDER:
							gdk_gc_set_clip_origin(
									Draw_Context->gc,
									Draw_Context->Current->modify_under->x,
									Draw_Context->Current->modify_under->y);
							gdk_draw_drawable(Draw_Context->drawable, 
									Draw_Context->gc,
									icon_info->pixmap,
									0, 0,
									Draw_Context->Current->modify_under->x,
									Draw_Context->Current->modify_under->y,
									Properties->width, Properties->height);

							Draw_Context->Current->modify_under->x += Properties->width;
			break;
	}

	gdk_gc_set_clip_origin(Draw_Context->gc, 0, 0);
	gdk_gc_set_clip_mask(Draw_Context->gc, NULL);
	
	return 0;
}

gboolean draw_line( void *hook_data, void *call_data)
{
	PropertiesLine *Properties = (PropertiesLine*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

	gdk_gc_set_foreground(Draw_Context->gc, Properties->color);
	gdk_gc_set_line_attributes(	Draw_Context->gc,
															Properties->line_width,
															Properties->style,
															GDK_CAP_BUTT,
															GDK_JOIN_MITER);

	switch(Properties->position) {
		case OVER:
							drawing_draw_line(
								NULL, Draw_Context->drawable,
								Draw_Context->Previous->over->x,
								Draw_Context->Previous->over->y,
								Draw_Context->Current->over->x,
								Draw_Context->Current->over->y,
								Draw_Context->gc);
			break;
		case MIDDLE:
							drawing_draw_line(
								NULL, Draw_Context->drawable,
								Draw_Context->Previous->middle->x,
								Draw_Context->Previous->middle->y,
								Draw_Context->Current->middle->x,
								Draw_Context->Current->middle->y,
								Draw_Context->gc);
			break;
		case UNDER:
							drawing_draw_line(
								NULL, Draw_Context->drawable,
								Draw_Context->Previous->under->x,
								Draw_Context->Previous->under->y,
								Draw_Context->Current->under->x,
								Draw_Context->Current->under->y,
								Draw_Context->gc);

			break;
	}

	return 0;
}

gboolean draw_arc( void *hook_data, void *call_data)
{
	PropertiesArc *Properties = (PropertiesArc*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

	gdk_gc_set_foreground(Draw_Context->gc, Properties->color);

	switch(Properties->position) {
		case OVER:
			gdk_draw_arc(Draw_Context->drawable, Draw_Context->gc,
							Properties->filled,
							Draw_Context->Current->modify_over->x,
							Draw_Context->Current->modify_over->y,
							Properties->size, Properties->size, 0, 360*64);
			Draw_Context->Current->modify_over->x += Properties->size;
		
			break;
		case MIDDLE:
			gdk_draw_arc(Draw_Context->drawable, Draw_Context->gc,
							Properties->filled,
							Draw_Context->Current->modify_middle->x,
							Draw_Context->Current->modify_middle->y,
							Properties->size, Properties->size, 0, 360*64);
			Draw_Context->Current->modify_middle->x += Properties->size;
			
			break;
		case UNDER:
			gdk_draw_arc(Draw_Context->drawable, Draw_Context->gc,
							Properties->filled,
							Draw_Context->Current->modify_under->x,
							Draw_Context->Current->modify_under->y,
							Properties->size, Properties->size, 0, 360*64);
			Draw_Context->Current->modify_under->x += Properties->size;
	
			break;
	}

	
	return 0;
}

gboolean draw_bg( void *hook_data, void *call_data)
{
	PropertiesBG *Properties = (PropertiesBG*)hook_data;
	DrawContext *Draw_Context = (DrawContext*)call_data;

	gdk_gc_set_foreground(Draw_Context->gc, Properties->color);


	gdk_draw_rectangle(Draw_Context->drawable, Draw_Context->gc,
					TRUE,
					Draw_Context->Previous->over->x,
					Draw_Context->Previous->over->y,
					Draw_Context->Current->over->x - Draw_Context->Previous->over->x,
					Draw_Context->Previous->under->y);

	return 0;
}


