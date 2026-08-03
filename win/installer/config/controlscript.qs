function Controller() {
    // Qt Installer Framework rejects a non-empty target directory before component
    // scripts load, so this is the only hook early enough in headless mode.
    if (installer.isCommandLineInstance())
        removePreviousInstall();
}

// Uninstall a previous version on the first page: purge relaunches the old
// maintenancetool from %TEMP% to delete itself and returns before the folder is
// empty, so starting here lets it finish while the user clicks through the wizard,
// avoiding a busy-wait for Qt Installer Framework's "directory not empty" check.
Controller.prototype.IntroductionPageCallback = function() {
    removePreviousInstall();
};

function removePreviousInstall() {
    // Control scripts also run in the installed maintenancetool; without this the
    // uninstall/update flow would try to purge the very install it is running from.
    if (!installer.isInstaller())
        return;

    var cli = installer.isCommandLineInstance();

    // This early TargetDir is still the config.xml default, so a headless --root
    // override purges the default location instead.
    var targetDir = installer.value("TargetDir");
    if (targetDir === "")
        targetDir = installer.value("ApplicationsDir") + "/mpz-player";

    var maintenanceTool = targetDir + "/maintenancetool.exe";
    if (!installer.fileExists(maintenanceTool))
        return;

    if (!cli) {
        var answer = QMessageBox.question("mpz.existing.install", "Existing installation",
            "A previous version of mpz is installed in:\n" + targetDir +
            "\n\nIf mpz is running it will be closed, and the old version removed, " +
            "before installing this version. Continue?",
            QMessageBox.Yes | QMessageBox.No);
        if (answer !== QMessageBox.Yes) {
            gui.clickButton(buttons.CancelButton);
            return;
        }
    }

    installer.execute("taskkill", ["/F", "/IM", "mpz.exe"]);
    if (!cli)
        installer.gainAdminRights();

    // Qt Installer Framework drops options placed after the command.
    var result = installer.execute(maintenanceTool,
        ["--confirm-command", "--accept-messages", "purge"], "y\n");
    if (result[1] == 0 && cli && !waitForRemoval(maintenanceTool))
        result[1] = 1;

    if (result[1] != 0) {
        QMessageBox.critical("mpz.remove.failed", "Could not remove previous version",
            "Removing the previous version in " + targetDir + " failed. Please uninstall " +
            "mpz manually, then run this installer again.", QMessageBox.Ok);
        if (cli)
            installer.setCanceled();
        else
            gui.clickButton(buttons.CancelButton);
    }
}

// purge returns before the detached maintenancetool has emptied the folder, and
// headless has no wizard to absorb the delay. Script has no sleep; ping stands in.
function waitForRemoval(path) {
    for (var i = 0; i < 30 && installer.fileExists(path); ++i)
        installer.execute("cmd", ["/c", "ping", "-n", "2", "127.0.0.1"]);
    return !installer.fileExists(path);
}
