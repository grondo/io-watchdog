
Name: io-watchdog
Version: 
Release:

Summary: IO Watchdog for user applications.

License: GPL
Group: System Environment/Base
Source: 
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: bison flex slurm-devel

%package devel
Requires: %{name} = %{version}-%{release}
Summary:  IO Watchdog client API headers and libraries
Group:    Development/Libraries

%package libs
Requires: %{name} = %{version}-%{release}
Summary:  IO Watchdog client API and interposer libraries
Group:    Development/Libraries

%package slurm
Requires: %{name} = %{version}-%{release}
Summary:  SLURM spank plugin for IO Watchdog
Group:    System Environment/Base

##############################################################################

%description
io-watchdog is a facility for monitoring user applications and parallel
jobs for "hangs" which typically have a side effect of ceasing all IO
in a cyclic application (i.e. one that writes something to a log or data
file during each cycle of computation). The io-watchdog attempts to
watch all IO coming from an application and triggers a set of user-defined
actions when IO has stopped for a configurable timeout period.

%description devel
A header file and static library for compiling against the IO Watchdog
client API.

%description libs
IO Watchdog shared libraries including the interposer library used to
intercept write calls in applications, and the io-watchdog client API
used to modify and query IO watchdog parameter at runtime from within
the monitored application.

%description slurm
IO Watchdog plugin for SLURM which adds a --io-watchdog option to srun(1).

##############################################################################

%prep
%setup 
##############################################################################

%build
%configure --program-prefix=%{?_program_prefix:%{_program_prefix}}
make %{_smp_mflags} CFLAGS="$RPM_OPT_FLAGS"
make check TEST_ITERATIONS=256
##############################################################################

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
DESTDIR="$RPM_BUILD_ROOT" make install
# 
#  Remove all module .a's as they are not needed on any known RPM platform.
rm -f $RPM_BUILD_ROOT/%{_libdir}/*/*.{a,la}

#
#  Remove .la's which aren't currently required anywhere we support.
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.la

#
#  Remove interposer .a
#
rm -f $RPM_BUILD_ROOT/%{_libdir}/io-watchdog-interposer.a

PLUGIN=$RPM_BUILD_ROOT/%{_libdir}/*/io-watchdog.so
if [ -f $PLUGIN ]; then
   SLURM_LIBDIR=$RPM_BUILD_ROOT/%{_libdir}/slurm
   mkdir -p $SLURM_LIBDIR
   mv $PLUGIN $SLURM_LIBDIR
fi

##############################################################################

%clean
rm -rf "$RPM_BUILD_ROOT"
##############################################################################

%files
%defattr(-,root,root,0755)
%doc AUTHORS
%doc NEWS
%doc COPYING
%doc ChangeLog
%doc README
%doc doc/*
%{_bindir}/*
%{_mandir}/*[^3]/*

%files devel
%defattr(-,root,root,0755)
%{_includedir}/io-watchdog.h
%{_libdir}/libio-watchdog.a
%{_libdir}/libio-watchdog.so
%{_mandir}/*3/*

%files libs
%defattr(-,root,root,0755)
%{_libdir}/*.so.*
%{_libdir}/io-watchdog-interposer.so

%files slurm
%defattr(-,root,root,0755)
%{_libdir}/slurm/*.so


##############################################################################

%changelog
* Wed Nov 11 2009 Mark Grondona <mgrondona@llnl.gov>
- Separate package into devel, libs, and slurm subpackages
- Run make check in build section

* Wed May 16 2007 Mark Grondona <mgrondona@llnl.gov>
- fix %files section to avoid grabbing debug info.

* Mon Jan  8 2007 Mark Grondona <mgrondona@llnl.gov>
-Initial version.
