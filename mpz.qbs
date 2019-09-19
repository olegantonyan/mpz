import qbs.FileInfo

QtApplication {
    Depends { name: "Qt.widgets" }

    // The following define makes your compiler emit warnings if you use
    // any Qt feature that has been marked deprecated (the exact warnings
    // depend on your compiler). Please consult the documentation of the
    // deprecated API in order to know how to port your code away from it.
    // You can also make your code fail to compile if it uses deprecated APIs.
    // In order to do so, uncomment the second entry in the list.
    // You can also select to disable deprecated APIs only up to a certain version of Qt.
    cpp.defines: [
        "QT_DEPRECATED_WARNINGS",
        /* "QT_DISABLE_DEPRECATED_BEFORE=0x060000" */ // disables all the APIs deprecated before Qt 6.0.0
    ]

    files: [
        "src/directory/directorymodel.cpp",
        "src/directory/directorymodel.h",
        "src/directory/directoryview.cpp",
        "src/directory/directoryview.h",
        "src/main.cpp",
        "src/mainwindow.cpp",
        "src/mainwindow.h",
        "src/mainwindow.ui",
        "src/playlist/playlistmodel.cpp",
        "src/playlist/playlistmodel.h",
        "src/playlist/playlistview.cpp",
        "src/playlist/playlistview.h",
        "src/playlists/playlistitem.cpp",
        "src/playlists/playlistitem.h",
        "src/playlists/playlistsmodel.cpp",
        "src/playlists/playlistsmodel.h",
        "src/playlists/playlistsview.cpp",
        "src/playlists/playlistsview.h",
    ]

    install: true
    installDir: qbs.targetOS.contains("qnx") ? FileInfo.joinPaths("/tmp", name, "bin") : base
}
