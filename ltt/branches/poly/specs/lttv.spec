#
# Spec file for LTTV
#
Summary: Linux Trace Toolkit Viewer
Name: lttv
Version: 0.6.9
Release: 10102005
License: GPL
Group: Applications/Development
Source: http://ltt.polymtl.ca/packages/LinuxTraceToolkitViewer-%{version}-%{release}.tar.gz
URL: http://ltt.polymtl.ca
Packager: Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
LTTV is a modular trace viewer. It can perform analysis on traces of a Linux
 kernel instrumented with LTTng.

%prep
%setup -q -n LinuxTraceToolkitViewer-%{version}-%{release}
%patch0 -p1

%build
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%files
/usr/lib/liblttctl.so.0.0.0
/usr/lib/liblttctl.so.0
/usr/lib/liblttctl.so
/usr/lib/liblttctl.la
/usr/lib/liblttctl.a
/usr/lib/liblttvtraceread.so.0.0.0
/usr/lib/liblttvtraceread.so.0
/usr/lib/liblttvtraceread.so
/usr/lib/liblttvtraceread.la
/usr/lib/liblttvtraceread.a
/usr/lib/lttv
/usr/lib/lttv/plugins
/usr/lib/lttv/plugins/libtextDump.so.0.0.0
/usr/lib/lttv/plugins/libtextDump.so.0
/usr/lib/lttv/plugins/libtextDump.so
/usr/lib/lttv/plugins/libtextDump.la
/usr/lib/lttv/plugins/libtextDump.a
/usr/lib/lttv/plugins/libbatchAnalysis.so.0.0.0
/usr/lib/lttv/plugins/libbatchAnalysis.so.0
/usr/lib/lttv/plugins/libbatchAnalysis.so
/usr/lib/lttv/plugins/libbatchAnalysis.la
/usr/lib/lttv/plugins/libbatchAnalysis.a
/usr/lib/lttv/plugins/libtextFilter.so.0.0.0
/usr/lib/lttv/plugins/libtextFilter.so.0
/usr/lib/lttv/plugins/libtextFilter.so
/usr/lib/lttv/plugins/libtextFilter.la
/usr/lib/lttv/plugins/libtextFilter.a
/usr/lib/lttv/plugins/libguicontrolflow.so.0.0.0
/usr/lib/lttv/plugins/libguicontrolflow.so.0
/usr/lib/lttv/plugins/libguicontrolflow.so
/usr/lib/lttv/plugins/libguicontrolflow.la
/usr/lib/lttv/plugins/libguicontrolflow.a
/usr/lib/lttv/plugins/libguievents.so.0.0.0
/usr/lib/lttv/plugins/libguievents.so.0
/usr/lib/lttv/plugins/libguievents.so
/usr/lib/lttv/plugins/libguievents.la
/usr/lib/lttv/plugins/libguievents.a
/usr/lib/lttv/plugins/libguistatistics.so.0.0.0
/usr/lib/lttv/plugins/libguistatistics.so.0
/usr/lib/lttv/plugins/libguistatistics.so
/usr/lib/lttv/plugins/libguistatistics.la
/usr/lib/lttv/plugins/libguistatistics.a
/usr/lib/lttv/plugins/libguifilter.so.0.0.0
/usr/lib/lttv/plugins/libguifilter.so.0
/usr/lib/lttv/plugins/libguifilter.so
/usr/lib/lttv/plugins/libguifilter.la
/usr/lib/lttv/plugins/libguifilter.a
/usr/lib/lttv/plugins/libguitracecontrol.so.0.0.0
/usr/lib/lttv/plugins/libguitracecontrol.so.0
/usr/lib/lttv/plugins/libguitracecontrol.so
/usr/lib/lttv/plugins/libguitracecontrol.la
/usr/lib/lttv/plugins/libguitracecontrol.a
/usr/lib/liblttvwindow.so.0.0.0
/usr/lib/liblttvwindow.so.0
/usr/lib/liblttvwindow.so
/usr/lib/liblttvwindow.la
/usr/lib/liblttvwindow.a
/usr/include/liblttctl
/usr/include/liblttctl/lttctl.h
/usr/include/ltt
/usr/include/ltt/compiler.h
/usr/include/ltt/event.h
/usr/include/ltt/facility.h
/usr/include/ltt/ltt.h
/usr/include/ltt/time.h
/usr/include/ltt/trace.h
/usr/include/ltt/type.h
/usr/include/ltt/ltt-types.h
/usr/include/lttv
/usr/include/lttv/attribute.h
/usr/include/lttv/hook.h
/usr/include/lttv/iattribute.h
/usr/include/lttv/lttv.h
/usr/include/lttv/module.h
/usr/include/lttv/option.h
/usr/include/lttv/state.h
/usr/include/lttv/stats.h
/usr/include/lttv/tracecontext.h
/usr/include/lttv/traceset.h
/usr/include/lttv/filter.h
/usr/include/lttv/print.h
/usr/include/lttvwindow
/usr/include/lttvwindow/lttvwindow.h
/usr/include/lttvwindow/lttvwindowtraces.h
/usr/include/lttvwindow/mainwindow.h
/usr/include/lttvwindow/menu.h
/usr/include/lttvwindow/toolbar.h
/usr/bin/lttctl
/usr/bin/lttv.real
/usr/bin/lttv
/usr/bin/lttv-gui
/usr/bin/lttd
/usr/share/LinuxTraceToolkitViewer
/usr/share/LinuxTraceToolkitViewer/pixmaps
/usr/share/LinuxTraceToolkitViewer/pixmaps/1downarrow.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/1uparrow.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/Makefile
/usr/share/LinuxTraceToolkitViewer/pixmaps/Makefile.am
/usr/share/LinuxTraceToolkitViewer/pixmaps/Makefile.in
/usr/share/LinuxTraceToolkitViewer/pixmaps/close.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/edit_add_22.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/edit_remove_22.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/filenew.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/fileopen.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/filesave.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/filesaveas.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/gtk-add.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/gtk-jump-to.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/lttv-color-list.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/mini-display.xpm
/usr/share/LinuxTraceToolkitViewer/pixmaps/move_message.xpm
/usr/share/LinuxTraceToolkitViewer/pixmaps/remove.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/remove1.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_jump_to_24.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_redo_24.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_refresh_24.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_stop_24.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_zoom_fit_24.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_zoom_in_24.png
/usr/share/LinuxTraceToolkitViewer/pixmaps/stock_zoom_out_24.png
/usr/share/LinuxTraceToolkitViewer/facilities
/usr/share/LinuxTraceToolkitViewer/facilities/core.xml
/usr/share/LinuxTraceToolkitViewer/facilities/fs.xml
/usr/share/LinuxTraceToolkitViewer/facilities/ipc.xml
/usr/share/LinuxTraceToolkitViewer/facilities/kernel.xml
/usr/share/LinuxTraceToolkitViewer/facilities/memory.xml
/usr/share/LinuxTraceToolkitViewer/facilities/network.xml
/usr/share/LinuxTraceToolkitViewer/facilities/process.xml
/usr/share/LinuxTraceToolkitViewer/facilities/s390_kernel.xml
/usr/share/LinuxTraceToolkitViewer/facilities/socket.xml
/usr/share/LinuxTraceToolkitViewer/facilities/timer.xml
