/******************************************************************************
 * Draw_Item.c
 *
 * This file contains methods responsible for drawing a generic type of data
 * in a drawable. Doing this generically will permit user defined drawing
 * behavior in a later time.
 *
 * We keep each data type in a hash table, as this container suits the
 * best the information we receive (GQuark).
 * (A hash table for facilities, pointing to an array per facility, containing
 * event_number events.)
 * (hash tables for cpus, for process state, for execution mode and submode).
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
 * - point (color, size)
 * - background color (color)
 *
 * Each item has an array of hooks (hook list). Each hook represents an
 * operation to perform. We seek the array each time we want to
 * draw an item. We execute each operation in order.
 *
 * The array has to be sorted by priority each time we add a task in it.
 * A priority is associated with each hook. It permits
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
 * Author : Mathieu Desnoyers, October 2003
 */

#include <glib.h>
#include <lttv/hook.h>


