#
# Spec file for ltt-control
#
Summary: LTT Control
Name: ltt-control
Version: 0.39
Release: 25042007
License: GPL
Group: Applications/Development
Source: http://ltt.polymtl.ca/lttng/ltt-control-%{version}-%{release}.tar.gz
URL: http://ltt.polymtl.ca
Packager: Martin Bisson <bissonm@discreet.com>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# Where do we install the libs
%ifarch x86_64 ppc64 ppc64iseries ia64 
%define libdir /usr/lib64
%else
%define libdir /usr/lib
%endif


%description

ltt-control is the part of the Linux Trace Toolkit Next Generation
that allows a machine to be traced.  It holds the daemon with which
the kernel will communicate and the application that starts and
terminate tracing.

%prep
%setup -q -n ltt-control-%{version}-%{release}

%build
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
%{libdir}/liblttctl.so.0.0.0
%{libdir}/liblttctl.so.0
%{libdir}/liblttctl.so
%{libdir}/liblttctl.la
%{libdir}/liblttctl.a
/usr/bin/lttctl
/usr/bin/lttd
/usr/include/liblttctl
/usr/include/liblttctl/lttctl.h
/usr/share/ltt-control
/usr/share/ltt-control/facilities
/usr/share/ltt-control/facilities/compact.xml
/usr/share/ltt-control/facilities/core.xml
/usr/share/ltt-control/facilities/fs.xml
/usr/share/ltt-control/facilities/kernel.xml
/usr/share/ltt-control/facilities/kernel_arch_arm.xml
/usr/share/ltt-control/facilities/kernel_arch_c2.xml
/usr/share/ltt-control/facilities/kernel_arch_i386.xml
/usr/share/ltt-control/facilities/kernel_arch_mips.xml
/usr/share/ltt-control/facilities/kernel_arch_powerpc.xml
/usr/share/ltt-control/facilities/kernel_arch_ppc.xml
/usr/share/ltt-control/facilities/kernel_arch_x86_64.xml
/usr/share/ltt-control/facilities/list.xml
/usr/share/ltt-control/facilities/locking.xml
/usr/share/ltt-control/facilities/mm.xml
/usr/share/ltt-control/facilities/net.xml
/usr/share/ltt-control/facilities/stack_arch_i386.xml
/usr/share/ltt-control/facilities/stack_arch_x86_64.xml
/usr/share/ltt-control/facilities/user_generic.xml
/usr/share/ltt-control/facilities/xen.xml
/usr/share/ltt-control/facilities/
