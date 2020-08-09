Name:       mpz
Version:    0.0.1
Release:    1%{?dist}
Summary:    mpz music player
License:    GPLv3+
URL:        https://github.com/olegantonyan/%{name}
Source0:    https://github.com/olegantonyan/%{name}/releases/%{name}-%{version}.tar.gz

BuildRequires:  clang
BuildRequires:  make

%description
Music player for big local collections

%prep
echo %{buildroot}
echo %{_bindir}
%setup -q

%build
mkdir build-fucking-qmake-ignore-fucking-debug-release
cd build-fucking-qmake-ignore-fucking-debug-release
qmake-qt5 -spec linux-clang CONFIG+=release ..
make %{?_smp_mflags}

%install
cd build-fucking-qmake-ignore-fucking-debug-release
# %make_install
mkdir -p %{buildroot}/usr/bin/
install -m 0755 app/mpz %{buildroot}/usr/bin/%{name}

%files
%license license.txt
%{_bindir}/%{name}

%changelog
* Sun Aug 9 2020 Oleg Antonyan <oleg.b.antonyan@gmail.com>
- First public release
