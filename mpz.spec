Name:       mpz
Version:    0.0.14
Release:    1%{?dist}
Summary:    Music player for the large local collections
License:    GPL-3.0-or-later
URL:        https://github.com/olegantonyan/%{name}
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  gcc make libqt5-qtbase-devel libqt5-qtmultimedia-devel libqt5-qtx11extras-devel


%description
Music player for big local collections. Treats your folders with music as a library.
Features 3-column UI: directory tree viewer, playlists list and tracks from current playlist.
Similar to "album list" in Foobar2000.


%prep
%setup -q


%build
mkdir build
cd build
qmake-qt5 CONFIG+=release CONFIG+=force_debug_info ..
make %{?_smp_mflags}


%install
cd build
make install INSTALL_ROOT=%{buildroot}


%files
%license license.txt
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/512x512/apps/%{name}.png

%dir %{_datadir}/applications/
%dir %{_datadir}/icons/hicolor/
%dir %{_datadir}/icons/hicolor/512x512
%dir %{_datadir}/icons/hicolor/512x512/apps/


%changelog
* Sun Aug 9 2020 Oleg Antonyan <oleg.b.antonyan@gmail.com>
- First public release
