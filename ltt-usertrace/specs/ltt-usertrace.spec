#
# Spec file for LTT Usertrace
#
Summary: Linux Trace Toolkit Userspace Tracing Package
Name: ltt-usertrace
Version: 0.13
License: GPL
Release: 1
Group: Applications/Development
Source: http://ltt.polymtl.ca/packages/%{name}-%{version}.tar.gz
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
This packages makes it possible to do userspace tracing with the Linux
Trace Toolkit.

%prep
%setup -q

%build
make libs

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT $RPM_BUILD_ROOT/usr/include $RPM_BUILD_ROOT/%{libdir}
make INCLUDE_DIR=$RPM_BUILD_ROOT/usr/include LIB_DIR=$RPM_BUILD_ROOT/%{libdir} install

%post
echo "Running ldconfig (might take a while)"
ldconfig

%postun
echo "Running ldconfig (might take a while)"
ldconfig

%files
/usr/include/ltt
/usr/include/ltt/atomic-ppc.h
/usr/include/ltt/atomic-ppc64.h
/usr/include/ltt/ltt-facility-custom-user_generic.h
/usr/include/ltt/ltt-facility-id-user_generic.h
/usr/include/ltt/ltt-facility-user_generic.h
/usr/include/ltt/ltt-usertrace-fast.h
/usr/include/ltt/ltt-usertrace-ppc.h
/usr/include/ltt/ltt-usertrace.h
/usr/include/ltt/ppc_asm-ppc.h
/usr/include/ltt/system-ppc.h
/usr/include/ltt/system-ppc64.h
/usr/include/ltt/timex-ppc.h
%{libdir}/libltt-instrument-functions.a
%{libdir}/libltt-instrument-functions.so
%{libdir}/libltt-instrument-functions.so.0
%{libdir}/libltt-loader-user_generic.a
%{libdir}/libltt-loader-user_generic.so
%{libdir}/libltt-loader-user_generic.so.0
%{libdir}/libltt-usertrace-fast.a
%{libdir}/libltt-usertrace-fast.so
%{libdir}/libltt-usertrace-fast.so.0
