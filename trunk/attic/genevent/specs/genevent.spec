#
# Spec file for genevent
#
Summary: Genevent package
Name: genevent
Version: 0.22
License: GPL
Release: 1
Group: Applications/Development
Source: http://ltt.polymtl.ca/packages/%{name}-%{version}.tar.gz
URL: http://ltt.polymtl.ca
Packager: Martin Bisson <bissonm@discreet.com>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)


%description
This packages contains the facility code generator.

%prep
%setup -q

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT $RPM_BUILD_ROOT/usr/bin
cp genevent $RPM_BUILD_ROOT/usr/bin/genevent

%files
/usr/bin/genevent
