/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers
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


#ifndef _DRAW_ITEM_H
#define _DRAW_ITEM_H

#include <lttv/state.h>

typedef struct _DrawContext DrawContext;
typedef struct _DrawInfo DrawInfo;
typedef struct _ItemInfo ItemInfo;

typedef struct _IconStruct IconStruct;

typedef struct _DrawOperation DrawOperation;


typedef struct _PropertiesText PropertiesText;
typedef struct _PropertiesIcon PropertiesIcon;
typedef struct _PropertiesLine PropertiesLine;
typedef struct _PropertiesArc PropertiesArc;
typedef struct _PropertiesBG PropertiesBG;

typedef enum _DrawableItems DrawableItems;
enum _DrawableItems {
    ITEM_TEXT, ITEM_ICON, ITEM_LINE, ITEM_POINT, ITEM_BACKGROUND
};

typedef enum _RelPosX {
  POS_START, POS_END
} RelPosX;

typedef enum _RelPosY {
  OVER, MIDDLE, UNDER
} RelPosY;


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
 * The modify_* positions are altered by the draw item functions.
 *
 */


struct _DrawContext {
  GdkDrawable *drawable;
  GdkGC   *gc;
  PangoLayout *pango_layout;

  struct {
    struct {
      gint x;
      struct {
        gint over;
        gint middle;
        gint under;
      } offset;
    } start;

    struct {
      gint x;
      struct {
        gint over;
        gint middle;
        gint under;
      } offset;
    } end;

    struct {
      gint over;
      gint middle;
      gint under;
    } y;

  } drawinfo;
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
  DrawableItems item;
  LttvHooks *hook;
};
#if 0
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
  50, /* ITEM_TEXT */
  40, /* ITEM_ICON */
  20, /* ITEM_LINE */
  30, /* ITEM_POINT */
  10  /* ITEM_BACKGROUND */
};
#endif //0

/*
 * Here are the different structures describing each item type that can be
 * drawn. They contain the information necessary to draw the item : not the
 * position (this is provided by the DrawContext), but the text, icon name,
 * line width, color; all the properties of the specific items.
 */

struct _PropertiesText {
  GdkColor  *foreground;
  GdkColor  *background;
  gint       size;
  gchar     *text;
  struct {
    RelPosX    x;
    RelPosY    y;
  } position;
};


struct _PropertiesIcon {
  gchar   *icon_name;
  gint    width;
  gint    height;
  struct {
    RelPosX    x;
    RelPosY    y;
  } position;
};

struct _PropertiesLine {
  GdkColor  color;
  gint    line_width;
  GdkLineStyle  style;
  RelPosY    y;
};

struct _PropertiesArc {
  GdkColor  *color;
  gint    size; /* We force circle by width = height */
  gboolean  filled;
  struct {
    RelPosX    x;
    RelPosY    y;
  } position;
};

struct _PropertiesBG {
  GdkColor  *color;
};



void draw_item( GdkDrawable *drawable,
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


#if 0
/* 
 * The add_operation has to do a quick sort by priority to keep the operations
 * in the right order.
 */
void add_operation( LttvIAttribute *attributes,
      gchar *pathname,
      DrawOperation *operation);

/* 
 * The del_operation seeks the array present at pathname (if any) and
 * removes the DrawOperation if present. It returns 0 on success, -1
 * if it fails.
 */
gint del_operation( LttvIAttribute *attributes,
      gchar *pathname,
      DrawOperation *operation);

/* 
 * The clean_operations removes all operations present at a pathname.
 * returns 0 on success, -1 if it fails.
 */
gint clean_operations(  LttvIAttribute *attributes,
      gchar *pathname );


/* 
 * The list_operations gives a pointer to the operation array associated
 * with the pathname. It will be NULL if no operation is present.
 */
void list_operations( LttvIAttribute *attributes,
      gchar *pathname,
      GArray **operation);



/*
 * exec_operation executes the operations if present in the attributes, or
 * do nothing if not present.
 */
void exec_operations( LttvIAttribute *attributes,
      gchar *pathname);
#endif //0

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
