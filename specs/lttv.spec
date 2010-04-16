#
# Spec file for LTTV
#
Summary: Linux Trace Toolkit Viewer
Name: lttv
Version: 0.8.48
Release: 21062006
License: GPL
Group: Applications/Development
Source: http://ltt.polymtl.ca/packages/LinuxTraceToolkitViewer-%{version}-%{release}.tar.gz
URL: http://ltt.polymtl.ca
Packager: Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# Where do we install the libs
%ifarch x86_64 ppc64 ppc64iseries ia64 
%define libdir /usr/lib64
%else
%define libdir /usr/lib
%endif


%description
LTTV is a modular trace viewer. It can perform analysis on traces of a Linux
 kernel instrumented with LTTng.

%prep
%setup -q -n LinuxTraceToolkitViewer-%{version}-%{release}

%build
# These two commands were added to fix compilation on x86_64 machines
aclocal
automake

./configure --prefix=/usr --libdir=%{libdir}
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%post
echo "Running ldconfig (might take a while)"
ldconfig

%postun
echo "Running ldconfig (might take a while)"
ldconfig

%files
%{libdir}/liblttvtraceread_loader-2.6.so
%{libdir}/liblttvtraceread_loader.so
%{libdir}/liblttvtraceread_loader.la
%{libdir}/liblttvtraceread_loader.a
%{libdir}/liblttvtraceread-2.6.so
%{libdir}/liblttvtraceread.so
%{libdir}/liblttvtraceread.la
%{libdir}/liblttvtraceread.a
%{libdir}/lttv
%{libdir}/lttv/plugins
%{libdir}/lttv/plugins/libtextDump.so.0.0.0
%{libdir}/lttv/plugins/libtextDump.so.0
%{libdir}/lttv/plugins/libtextDump.so
%{libdir}/lttv/plugins/libtextDump.la
%{libdir}/lttv/plugins/libtextDump.a
%{libdir}/lttv/plugins/libbatchAnalysis.so.0.0.0
%{libdir}/lttv/plugins/libbatchAnalysis.so.0
%{libdir}/lttv/plugins/libbatchAnalysis.so
%{libdir}/lttv/plugins/libbatchAnalysis.la
%{libdir}/lttv/plugins/libbatchAnalysis.a
%{libdir}/lttv/plugins/libtextFilter.so.0.0.0
%{libdir}/lttv/plugins/libtextFilter.so.0
%{libdir}/lttv/plugins/libtextFilter.so
%{libdir}/lttv/plugins/libtextFilter.la
%{libdir}/lttv/plugins/libtextFilter.a
%{libdir}/lttv/plugins/libguicontrolflow.so.0.0.0
%{libdir}/lttv/plugins/libguicontrolflow.so.0
%{libdir}/lttv/plugins/libguicontrolflow.so
%{libdir}/lttv/plugins/libguicontrolflow.la
%{libdir}/lttv/plugins/libguicontrolflow.a
%{libdir}/lttv/plugins/libguievents.so.0.0.0
%{libdir}/lttv/plugins/libguievents.so.0
%{libdir}/lttv/plugins/libguievents.so
%{libdir}/lttv/plugins/libguievents.la
%{libdir}/lttv/plugins/libguievents.a
%{libdir}/lttv/plugins/libguistatistics.so.0.0.0
%{libdir}/lttv/plugins/libguistatistics.so.0
%{libdir}/lttv/plugins/libguistatistics.so
%{libdir}/lttv/plugins/libguistatistics.la
%{libdir}/lttv/plugins/libguistatistics.a
%{libdir}/lttv/plugins/libguifilter.so.0.0.0
%{libdir}/lttv/plugins/libguifilter.so.0
%{libdir}/lttv/plugins/libguifilter.so
%{libdir}/lttv/plugins/libguifilter.la
%{libdir}/lttv/plugins/libguifilter.a
%{libdir}/lttv/plugins/libguitracecontrol.so.0.0.0
%{libdir}/lttv/plugins/libguitracecontrol.so.0
%{libdir}/lttv/plugins/libguitracecontrol.so
%{libdir}/lttv/plugins/libguitracecontrol.la
%{libdir}/lttv/plugins/libguitracecontrol.a
%{libdir}/liblttvwindow.so.0.0.0
%{libdir}/liblttvwindow.so.0
%{libdir}/liblttvwindow.so
%{libdir}/liblttvwindow.la
%{libdir}/liblttvwindow.a
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
/usr/bin/lttv.real
/usr/bin/lttv
/usr/bin/lttv-gui
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
