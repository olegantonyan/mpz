function Controller() {
}

// Uninstall a previous version on the first page: purge relaunches the old
// maintenancetool from %TEMP% to delete itself and returns before the folder is
// empty, so starting here lets it finish while the user clicks through the wizard,
// avoiding a busy-wait for QtIFW's "directory not empty" check on the target page.
Controller.prototype.IntroductionPageCallback = function() {
    // Control scripts also run in the installed maintenancetool; without this the
    // uninstall/update flow would try to purge the very install it is running from.
    if (!installer.isInstaller())
        return;

    var targetDir = installer.value("TargetDir");
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
        QMessageBox.critical("mpz.remove.failed", "Could not remove previous version",
            "Removing the previous version in " + targetDir + " failed. Please uninstall " +
            "mpz manually, then run this installer again.", QMessageBox.Ok);
        gui.clickButton(buttons.CancelButton);
    }
};
