%define name uTCPIPPublisher
%define version 1.1.0
%define build_timestamp %{lua: print(os.date("%Y%m%d"))}

Name:           %{name}
Version:        %{version}
Release:        %{build_timestamp}.git%{?dist}
Summary:        Unmanarc TCP Port Publisher
License:        LGPLv3
URL:            https://github.com/unmanarc/uTCPIPPublisher
Source0:        https://github.com/unmanarc/uTCPIPPublisher/archive/main.tar.gz#/%{name}-%{version}-%{build_timestamp}.tar.gz
Group:          Applications/Internet

%define cmake cmake

%if 0%{?rhel} == 6
%define cmake cmake3
%endif

%if 0%{?rhel} == 7
%define cmake cmake3
%endif

%if 0%{?rhel} == 8
%define debug_package %{nil}
%endif

%if 0%{?rhel} == 9
%define debug_package %{nil}
%endif

%if 0%{?fedora} >= 33
%define debug_package %{nil}
%endif

BuildRequires: libMantids-devel >= 2.7.0
BuildRequires:  %{cmake} systemd openssl-devel zlib-devel boost-devel gcc-c++
Requires: libMantids >= 2.7.0
Requires: zlib openssl

%description
This package contains a simple TCP port publisher for scenarios under NAT/IPv4

%prep
%autosetup -n %{name}-main

%build
%{cmake} -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_SSL_SUPPORT=ON
%{cmake} -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_SSL_SUPPORT=ON
make %{?_smp_mflags}

%clean
rm -rf $RPM_BUILD_ROOT

%install
rm -rf $RPM_BUILD_ROOT

%if 0%{?fedora} >= 33
ln -s . %{_host}
%endif

%if 0%{?rhel} >= 9
ln -s . %{_host}
ln -s . redhat-linux-build
%endif

%if 0%{?fedora} == 35
ln -s . redhat-linux-build
%endif

%if "%{_host}" == "powerpc64le-redhat-linux-gnu"
ln -s . ppc64le-redhat-linux-gnu
%endif

%if "%{_host}" == "s390x-ibm-linux-gnu"
ln -s . s390x-redhat-linux-gnu
%endif

%if "%{cmake}" == "cmake3"
%cmake3_install
%else
%cmake_install
%endif

mkdir -vp $RPM_BUILD_ROOT/etc
cp -a etc/uTCPIPPublisher $RPM_BUILD_ROOT/etc/

mkdir -vp $RPM_BUILD_ROOT/usr/lib/systemd/system
cp usr/lib/systemd/system/utcpippubserver.service $RPM_BUILD_ROOT/usr/lib/systemd/system/utcpippubserver.service
cp usr/lib/systemd/system/utcpippubclient.service $RPM_BUILD_ROOT/usr/lib/systemd/system/utcpippubclient.service
chmod 644 $RPM_BUILD_ROOT/usr/lib/systemd/system/utcpippubclient.service
chmod 644 $RPM_BUILD_ROOT/usr/lib/systemd/system/utcpippubserver.service
chmod 600 $RPM_BUILD_ROOT/etc/uTCPIPPublisher/client_config.ini
chmod 600 $RPM_BUILD_ROOT/etc/uTCPIPPublisher/server_config.ini
chmod 600 $RPM_BUILD_ROOT/etc/uTCPIPPublisher/services.json

%files
%doc
%{_bindir}/uTCPIPPublisher
%config(noreplace) /etc/uTCPIPPublisher/client_config.ini
%config(noreplace) /etc/uTCPIPPublisher/server_config.ini
%config(noreplace) /etc/uTCPIPPublisher/services.json

/usr/lib/systemd/system/utcpippubclient.service
/usr/lib/systemd/system/utcpippubserver.service

%post
%systemd_post utcpippubclient.service
%systemd_post utcpippubserver.service

%preun
%systemd_preun utcpippubclient.service
%systemd_preun utcpippubserver.service

%postun
%systemd_postun_with_restart utcpippubclient.service
%systemd_postun_with_restart utcpippubserver.service

%changelog
