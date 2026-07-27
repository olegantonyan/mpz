Name:       mpz
Version:    2.1.1
Release:    1%{?dist}
Summary:    Music player for the large local collections
License:    GPL-3.0-or-later
URL:        https://github.com/olegantonyan/mpz
Source0:    %{name}-%{version}.tar.gz

%bcond_with qt6
%if %{with qt6}
BuildRequires: gcc make cmake qt6-base-common-devel qt6-multimedia-devel qt6-widgets-devel qt6-concurrent-devel qt6-svg-devel
%else
BuildRequires: gcc make cmake libqt5-qtbase-devel libqt5-qtmultimedia-devel libqt5-qtx11extras-devel libqt5-qtsvg-devel
%endif


%description
Music player for big local collections. Treats your folders with music as a library.
Features 3-column UI: directory tree viewer, playlists list and tracks from current playlist.
Similar to "album list" in Foobar2000.


%prep
%setup -q


%build
%if %{with qt6}
cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_prefix}
%else
cmake -S . -B _build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_prefix} -DUSE_QT5=ON
%endif
cmake --build _build %{?_smp_mflags}


%install
DESTDIR=%{buildroot} cmake --install _build


%files
%license license.txt
%{_bindir}/%{name}
%{_datadir}/applications/org.mpz_player.mpz.desktop
%{_datadir}/metainfo/org.mpz_player.mpz.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/org.mpz_player.mpz.svg
%{_datadir}/icons/hicolor/*/apps/org.mpz_player.mpz.png


%changelog
* Sun Aug 9 2020 Oleg Antonyan <oleg.b.antonyan@gmail.com>
- First public release
