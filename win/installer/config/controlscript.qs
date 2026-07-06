function Controller() {
}

Controller.prototype.TargetDirectoryPageCallback = function() {
    var page = gui.currentPageWidget();
    if (page === null)
        return;

    var targetDir = page.TargetDirectoryLineEdit.text;
    var maintenanceTool = targetDir + "/maintenancetool.exe";
    if (!installer.fileExists(maintenanceTool))
        return;

    var answer = QMessageBox.question("mpz.existing.install", "Existing installation",
        "A previous version of mpz is installed in:\n" + targetDir +
        "\n\nIf mpz is running it will be closed, and the old version removed, " +
        "before installing this version. Continue?",
        QMessageBox.Yes | QMessageBox.No);
    if (answer !== QMessageBox.Yes) {
        gui.clickButton(buttons.CancelButton);
        return;
    }

    installer.execute("taskkill", ["/F", "/IM", "mpz.exe"]);

    installer.gainAdminRights();
    var result = installer.execute(maintenanceTool, ["purge", "--confirm-command"], "y\n");
    if (result[1] != 0) {
        QMessageBox.critical("mpz.purge.failed", "Could not remove previous version",
            "Uninstalling the previous version failed. Please uninstall mpz manually " +
            "(Settings > Apps, or run maintenancetool in " + targetDir +
            "), then run this installer again.", QMessageBox.Ok);
        gui.clickButton(buttons.CancelButton);
        return;
    }

    // purge relaunches maintenancetool from %TEMP% to delete itself, so it returns
    // before the folder is gone; wait for the old install to actually disappear or
    // the target-directory page validation will still block Next.
    for (var i = 0; i < 20 && installer.fileExists(maintenanceTool); i++)
        installer.execute("cmd", ["/c", "ping -n 2 127.0.0.1 >nul"]);
};
