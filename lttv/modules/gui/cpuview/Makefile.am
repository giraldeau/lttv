# This file is part of the Linux Trace Toolkit viewer
# Copyright (C) 2003-2004 Mathieu Desnoyers
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License Version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
# MA 02111-1307, USA.



#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

include ../common.am

lib_LTLIBRARIES = libguicpuview.la
libguicpuview_la_SOURCES = cpumodule.c cpueventhooks.c cpucfv.c \
			cpubuttonwidget.c cpudrawing.c cpudrawitem.c

noinst_HEADERS = cpueventhooks.h cpucfv.h \
		cpubuttonwidget.h cpudrawing.h cpudrawitem.h

EXTRA_DIST = \
		hCpuUsage.xpm stock_zoom_in_24.xpm stock_zoom_out_24.xpm \
		stock_zoom_fit_24.xpm