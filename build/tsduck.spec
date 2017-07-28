Name:           tsduck
Version:        %{version}
Release:        %{release}
Summary:        MPEG transport stream toolkit

Group:          Applications/Multimedia
License:        BSD
Source0:        tsduck-%{version}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:       pcsc-lite
BuildRequires:  gcc-c++
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  binutils
BuildRequires:  pcsc-lite-devel

%description
The MPEG Transport Stream Toolkit provides some simple utilities to process
MPEG Transport Streams (TS), either as recorded files or live streams. The
structure of an MPEG TS is defined in ISO 13818-1.

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       pcsc-lite-devel
Requires:       %{name} = %{version}-%{release}

%description    devel
The %{name}-devel package contains the static library and header files for
developing applications that use %{name}.

# Disable debuginfo package.
%global debug_package %{nil}

%prep
%setup -q

%build
make %{?_smp_mflags} %{?mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
make install-devel DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/ts*
%{_sysconfdir}/udev/rules.d/80-tsduck.rules
%{_sysconfdir}/security/console.perms.d/80-tsduck.perms
%doc CHANGELOG.txt LICENSE.txt doc/tsduck.pdf

%files devel
%defattr(-,root,root,-)
%{_libdir}/libtsduck.a
%{_includedir}/tsduck
