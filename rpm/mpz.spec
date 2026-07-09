%global __brp_check_rpaths %{nil}
Name:       mpz
Version:    2.0.15
Release:    1%{?dist}
Summary:    Music player for the large local collections
License:    GPL-3.0-or-later
URL:        https://github.com/olegantonyan/%{name}
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
mkdir build
cd build
%if %{with qt6}
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{buildroot}/usr -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
%else
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{buildroot}/usr -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DUSE_QT5=ON ..
%endif
make %{?_smp_mflags}


%install
cd build
make install


%files
%license license.txt
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg
%{_datadir}/icons/hicolor/48x48/apps/%{name}.png
%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%{_datadir}/icons/hicolor/24x24/apps/%{name}.png
%{_datadir}/icons/hicolor/22x22/apps/%{name}.png
%{_datadir}/icons/hicolor/16x16/apps/%{name}.png

%dir %{_datadir}/applications/
%dir %{_datadir}/icons/hicolor/
%dir %{_datadir}/icons/hicolor/scalable
%dir %{_datadir}/icons/hicolor/scalable/apps/
%dir %{_datadir}/icons/hicolor/48x48
%dir %{_datadir}/icons/hicolor/48x48/apps/
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps/
%dir %{_datadir}/icons/hicolor/24x24
%dir %{_datadir}/icons/hicolor/24x24/apps/
%dir %{_datadir}/icons/hicolor/22x22
%dir %{_datadir}/icons/hicolor/22x22/apps/
%dir %{_datadir}/icons/hicolor/16x16
%dir %{_datadir}/icons/hicolor/16x16/apps/


%changelog
* Sun Aug 9 2020 Oleg Antonyan <oleg.b.antonyan@gmail.com>
- First public release
