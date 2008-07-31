/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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

#ifndef _MAIN_WINDOW_
#define _MAIN_WINDOW_

#include <glib.h>
#include <ltt/time.h>

typedef struct _MainWindow MainWindow;
typedef struct _TimeWindow TimeWindow;
typedef struct _Tab Tab;
typedef struct _TracesetInfo TracesetInfo;

struct _TimeWindow {
	LttTime start_time;
	LttTime time_width;
  double time_width_double;
	LttTime end_time;
}; 



#endif /* _MAIN_WINDOW_ */


