Name:       mpz
Version:    0.0.1
Release:    1%{?dist}
Summary:    mpz music player
License:    GPLv3+
URL:        https://github.com/olegantonyan/%{name}
Source0:    https://github.com/olegantonyan/%{name}/releases/%{name}-%{version}.tar.gz

BuildRequires:  clang make libqt5-qtbase-devel libqt5-qtmultimedia-devel

Requires: libQt5Core5 libQt5Concurrent5 libQt5Multimedia5 libQt5Gui5 libQt5DBus5 libQt5Network5


%description
Music player for big local collections


%prep
%setup -q


%build
mkdir build-fucking-qmake-ignore-fucking-debug-release
cd build-fucking-qmake-ignore-fucking-debug-release
qmake-qt5 -spec linux-clang CONFIG+=release ..
make %{?_smp_mflags}


%install
cd build-fucking-qmake-ignore-fucking-debug-release
make install INSTALL_ROOT=%{buildroot}


%files
%license license.txt
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/512x512/apps/%{name}.png


%changelog
* Sun Aug 9 2020 Oleg Antonyan <oleg.b.antonyan@gmail.com>
- First public release
