function Controller() {
    // Headless has no wizard pages, and QtIFW rejects a non-empty target directory
    // right after loading this script, before component scripts exist. The
    // constructor is the only hook that still runs before that check.
    if (installer.isCommandLineInstance())
        removePreviousInstall();
}

// Uninstall a previous version on the first page: purge relaunches the old
// maintenancetool from %TEMP% to delete itself and returns before the folder is
// empty, so starting here lets it finish while the user clicks through the wizard,
// avoiding a busy-wait for QtIFW's "directory not empty" check on the target page.
Controller.prototype.IntroductionPageCallback = function() {
    removePreviousInstall();
};

function removePreviousInstall() {
    // Control scripts also run in the installed maintenancetool; without this the
    // uninstall/update flow would try to purge the very install it is running from.
    if (!installer.isInstaller())
        return;

    var cli = installer.isCommandLineInstance();

    // This early, TargetDir is still the config.xml default: a headless --root
    // override is not visible here, so it purges the default location instead.
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

    // QtIFW only parses options that precede the command; after it they are dropped
    // and the tool prompts instead.
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

// Nothing stands between purge and QtIFW's directory check in headless mode, so wait
// for the detached maintenancetool to actually empty the folder. QtIFW script has no
// sleep; ping is the usual stand-in.
function waitForRemoval(path) {
    for (var i = 0; i < 30 && installer.fileExists(path); ++i)
        installer.execute("cmd", ["/c", "ping", "-n", "2", "127.0.0.1"]);
    return !installer.fileExists(path);
}
