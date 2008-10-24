/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
 *               2005 Mathieu Desnoyers
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

/* print.c
 *
 * Event printing routines header.
 *
 * Use these functions to print textually event fields. 
 */



void lttv_print_field(LttEvent *e, struct marker_field *f, GString *s,
                      gboolean field_names, LttvTracefileState *tfs);

void lttv_event_to_string(LttEvent *e, GString *s,
    gboolean mandatory_fields, gboolean field_names, LttvTracefileState *tfs);

