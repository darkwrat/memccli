%define debug_package %{nil}

Name:       memccli
Version:    1.0
Release:    1%{?dist}
Summary:    memccli

License:    Proprietary
URL:        https://e.mail.ru
Source0:    memccli.tar.bz2

BuildRequires: git
BuildRequires: libmemcached-devel

%description
%{lua:
if rpm.expand("%{__autobuild__}") == '1' 
then
print("From tag: GIT_TAG\n")
print("Git hash: GITHASH\n")
print("Build by: BUILD_USER\n")
end}

%prep
rm -rf %{name}*
%setup -q -n %{name}

%build
make %{?_smp_mflags}

%install
mkdir -p %{buildroot}/usr/local/bin/
install -m 755 memccli %{buildroot}/usr/local/bin/memccli

%files
/usr/local/bin/memccli