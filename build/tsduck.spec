Name:           tsduck
Version:        %{version}
Release:        %{commit}%{distro}
Summary:        MPEG transport stream toolkit

Group:          Applications/Multimedia
License:        BSD
Source0:        tsduck-%{version}-%{commit}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires:       pcsc-lite
Requires:       libcurl
BuildRequires:  gcc-c++
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  binutils
BuildRequires:  pcsc-lite-devel
BuildRequires:  libcurl-devel

%description
TSDuck, the MPEG Transport Stream Toolkit, provides some simple utilities to
process MPEG Transport Streams (TS), either as recorded files or live streams.

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
%setup -q -n %{name}-%{version}-%{commit}

%build
make NOTEST=true %{?_smp_mflags} %{?mflags}

%install
rm -rf $RPM_BUILD_ROOT
make NOTEST=true install SYSROOT=$RPM_BUILD_ROOT
make NOTEST=true install-devel SYSROOT=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/ts*
/lib/udev/rules.d/80-tsduck.rules
%{_sysconfdir}/security/console.perms.d/80-tsduck.perms
%doc CHANGELOG.txt LICENSE.txt doc/tsduck.pdf

%files devel
%defattr(-,root,root,-)
%{_libdir}/libtsduck.a
%{_includedir}/tsduck
%doc LICENSE.txt
