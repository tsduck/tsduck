Name:           tsduck
Version:        %{version}
Release:        %{commit}%{distro}
Summary:        MPEG transport stream toolkit

Group:          Applications/Multimedia
License:        BSD
Source0:        tsduck-%{version}-%{commit}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  gcc-c++
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  binutils
%if 0%{!?noeditline:1}
Requires:       libedit
BuildRequires:  libedit-devel
%endif
%if 0%{!?nocurl:1}
Requires:       libcurl
BuildRequires:  libcurl-devel
%endif
%if 0%{!?nopcsc:1}
Requires:       pcsc-lite
BuildRequires:  pcsc-lite-devel
%endif
%if 0%{!?nosrt:1}
Requires:       srt-libs
BuildRequires:  srt-devel
%endif
%if 0%{!?norist:1}
Requires:       librist
BuildRequires:  librist-devel
%endif
%if 0%{!?novatek:1}
%if 0%{?fedora}
Requires:       libusb1
BuildRequires:  libusb1-devel
%else
Requires:       libusbx
BuildRequires:  libusbx-devel
%endif
%endif

%description
TSDuck, the MPEG Transport Stream Toolkit, provides some simple utilities to
process MPEG Transport Streams (TS), either as recorded files or live streams.

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}
%if 0%{!?noeditline:1}
Requires:       libedit-devel
%endif
%if 0%{!?nopcsc:1}
Requires:       pcsc-lite-devel
%endif
%if 0%{!?nocurl:1}
Requires:       libcurl-devel
%endif
%if 0%{!?nosrt:1}
Requires:       srt-devel
%endif
%if 0%{!?norist:1}
Requires:       librist-devel
%endif

%description    devel
The %{name}-devel package contains the static library and header files for
developing applications that use %{name}.

# Disable debuginfo package.
%global debug_package %{nil}

# Propagate component exclusions.
%define makeflags NOTEST=1 %{?nocurl:NOCURL=1} %{?nopcsc:NOPCSC=1} %{?nosrt:NOSRT=1} %{?mflags}

%prep
%setup -q -n %{name}-%{version}-%{commit}

%build
make %{?_smp_mflags} %{makeflags}

%install
rm -rf $RPM_BUILD_ROOT
make %{makeflags} install SYSROOT=$RPM_BUILD_ROOT
# Weird note: libtsduck.so needs to be executable, otherwise rpm does not consider it as a valid dependency.
chmod 0755 $RPM_BUILD_ROOT/usr/lib*/libtsduck.so

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/ts*
%{_libdir}/libtsduck.so
%{_libdir}/tsduck
%{_datadir}/tsduck
%{_datadir}/bash-completion/completions/ts*
%{_datadir}/bash-completion/completions/_tsduck
/lib/udev/rules.d/80-tsduck.rules
%{_sysconfdir}/security/console.perms.d/80-tsduck.perms
%doc CHANGELOG.txt LICENSE.txt OTHERS.txt doc/tsduck.pdf

%files devel
%defattr(-,root,root,-)
%{_libdir}/libtsduck.a
%{_includedir}/tsduck
%{_datadir}/pkgconfig/tsduck.pc
%doc LICENSE.txt
