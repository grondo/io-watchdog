
Name: chaos-dchroot
Version: 
Release:

Summary: IO Watchdog for user applications.

License: GPL
Group: System Environment/Base
Source: 
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

##############################################################################

%description
io-watchdog is a facility for monitoring user applications and parallel
jobs for "hangs" which typically have a side effect of ceasing all IO
in a cyclic application (i.e. one that writes something to a log or data
file during each cycle of computation). The io-watchdog attempts to
watch all IO coming from an application and triggers a set of user-defined
actions when IO has stopped for a configurable timeout period.
##############################################################################

%prep
%setup
##############################################################################

%build
%configure --program-prefix=%{?_program_prefix:%{_program_prefix}}
make %{_smp_mflags} CFLAGS="$RPM_OPT_FLAGS"
##############################################################################

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
DESTDIR="$RPM_BUILD_ROOT" make install
# 
# Remove all module .a's as they are not needed on any known RPM platform.
rm -f $RPM_BUILD_ROOT/%{_libdir}/*/*.{a,la}
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.{a,la}

SLURM_LIBDIR=$RPM_BUILD_ROOT/%{_libdir}/slurm
mkdir -p $SLURM_LIBDIR
mv $RPM_BUILD_ROOT/%{_libdir}/io-watchdog.so $SLURM_LIBDIR

##############################################################################

%clean
rm -rf "$RPM_BUILD_ROOT"
##############################################################################

%files
%defattr(-,root,root)
%doc COPYING ChangeLog README doc/example-config
%{_bindir}/*
%{_libdir}/*
%{_libdir}/*/*
#%{_mandir}/*/*

##############################################################################

%changelog
* Mon Jan  8 2007 Mark Grondona <mgrondona@llnl.gov>
-Initial version.
