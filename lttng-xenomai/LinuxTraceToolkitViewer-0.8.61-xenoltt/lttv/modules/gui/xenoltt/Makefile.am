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
# Created by Mathieu Desnoyers on September 27, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L${top_builddir}/lttv/modules/gui/lttvwindow/lttvwindow -llttvwindow

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguixenoltt.la
libguixenoltt_la_LDFLAGS = -module
libguixenoltt_la_SOURCES = 	module.c xenoltt_eventhooks.c xfv.c xenoltt_threadlist.c\
				xenoltt_drawing.c xenoltt_drawitem.c lttv_plugin_xfv.c
noinst_HEADERS = 	xenoltt_eventhooks.h xfv.h xenoltt_threadlist.h\
				xenoltt_drawing.h xenoltt_drawitem.h lttv_plugin_xfv.h

EXTRA_DIST = \
		hGuiControlFlowInsert.xpm hLegendInsert.xpm\
		xenoltt-color-list.png FlagThreadDelete.xpm FlagThreadInit.xpm

