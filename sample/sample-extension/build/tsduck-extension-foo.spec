Name:           tsduck-extension-foo
Version:        %{version}
Release:        %{commit}%{distro}
Summary:        TSDuck Foo extension

Group:          Applications/Multimedia
License:        BSD
Source0:        tsduck-extension-foo-%{version}-%{commit}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:       tsduck
BuildRequires:  gcc-c++
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  binutils
BuildRequires:  tsduck-devel

%description
TSDuck is the MPEG Transport Stream Toolkit which provides some simple
utilities to process MPEG Transport Streams (TS). This package provides
the sample Foo extension to TSDuck.

# Disable debuginfo package.
%global debug_package %{nil}

%prep
%setup -q -n %{name}-%{version}-%{commit}

%build
make %{?_smp_mflags} %{?mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/footool
%{_libdir}/tslibext_foo.so
%{_libdir}/tsduck/tsplugin_foot.so
%{_datadir}/tsduck/tslibext_foo.xml
%{_datadir}/tsduck/tslibext_foo.names
%doc doc/tsduck-extension-foo.pdf
