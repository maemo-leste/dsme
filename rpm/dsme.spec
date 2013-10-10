# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.27
# 

Name:       dsme

# >> macros
%{!?qtc_qmake5:%define qtc_qmake5 qmake}
%{!?qtc_qmake:%define qtc_qmake qmake}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}
# << macros

Summary:    Device State Management Entity
Version:    0.62.23
Release:    0
Group:      System/System Control
License:    LGPLv2+
URL:        https://github.com/nemomobile/dsme
Source0:    %{name}-%{version}.tar.gz
Source1:    dsme.service
Source2:    dsme-rpmlintrc
Source100:  dsme.yaml
Requires:   systemd
Requires:   statefs
Requires(preun): systemd
Requires(post): systemd
Requires(postun): systemd
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires:  pkgconfig(libiphb)
BuildRequires:  pkgconfig(dsme)
BuildRequires:  pkgconfig(systemd)
BuildRequires:  pkgconfig(mce) >= 1.12.3
BuildRequires:  python
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  automake

%description
Device State Management Entity (with debug symbols). This package contains the Device State Management Entity which provides state management features such as service monitoring, process watchdog and inactivity tracking.


%package tests
Summary:    DSME test cases
Group:      Development/System
BuildArch:  noarch
Requires:   %{name} = %{version}-%{release}
Requires:   dbus

%description tests
Test cases and xml test description for DSME

%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
unset LD_AS_NEEDED
# >> build pre
./verify_version.sh
chmod a+x autogen.sh
./autogen.sh
chmod a+x configure
# << build pre

%configure --disable-static \
    --without-bmeipc \
    --disable-poweron-timer \
    --disable-upstart \
    --enable-runlevel \
    --enable-systemd \
    --enable-pwrkeymonitor \
    --disable-validatorlistener

make %{?_smp_mflags}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%make_install

# >> install post
install -D -m 644 %{SOURCE1} %{buildroot}/lib/systemd/system/%{name}.service
install -d %{buildroot}/lib/systemd/system/multi-user.target.wants/
ln -s ../%{name}.service %{buildroot}/lib/systemd/system/multi-user.target.wants/%{name}.service
install -d %{buildroot}/var/lib/dsme
[ ! -f %{buildroot}/var/lib/dsme/alarm_queue_status ] && echo 0 > %{buildroot}/var/lib/dsme/alarm_queue_status
# << install post

%preun
if [ "$1" -eq 0 ]; then
systemctl stop %{name}.service
fi

%post
systemctl daemon-reload
systemctl reload-or-try-restart %{name}.service

%postun
systemctl daemon-reload

%files
%defattr(-,root,root,-)
%{_libdir}/dsme/*
%attr(755,root,root)%{_sbindir}/*
%config %{_sysconfdir}/dsme/lifeguard.uids
%config %{_sysconfdir}/dbus-1/system.d/dsme.conf
%doc debian/copyright COPYING
/lib/systemd/system/%{name}.service
/lib/systemd/system/multi-user.target.wants/%{name}.service
/var/lib/dsme
%config(noreplace) /var/lib/dsme/alarm_queue_status
# >> files
# << files

%files tests
%defattr(-,root,root,-)
/opt/tests/dsme-tests/*
# >> files tests
# << files tests
