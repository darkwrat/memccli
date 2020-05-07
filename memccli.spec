%define debug_package %{nil}
%define __strip /bin/true
%define __spec_install_post /usr/lib/rpm/brp-compress

Name:      memccli
Version:   1.1.2
Release:   1%{?dist}
Summary:   Memcached CLI Tool
Group:     Development/Tools
License:   BSD
URL:       https://github.com/darkwrat/memccli
Source0:   https://github.com/darkwrat/memccli/archive/%{version}.tar.gz
BuildRequires: libmemcached-devel
Requires: libmemcached

%description
Simple CLI tool for binary interaction with memcached

%prep
%setup -q -n %{name}-%{version}

%build
make %{?_smp_mflags}

%install
mkdir -p ${RPM_BUILD_ROOT}/usr/local/bin
make %{?_smp_mflags} install PREFIX=/usr/local DESTDIR=${RPM_BUILD_ROOT}

%files
%defattr(-,root,root,-)
/usr/local/bin/%{name}

%changelog
* Thu May 7 2020 Maxim Galaganov <m.galaganov@corp.mail.ru> - 1.1.2-1
- Initial spec
