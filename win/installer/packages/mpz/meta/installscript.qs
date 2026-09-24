function Component() {
    // Without this Qt Installer Framework generates a random GUID per run and registers
    // a new Apps & Features entry each install. config.xml can't set it (QTIFW-1321).
    if (installer.value("os") === "win") {
        installer.setValue("ProductUUID", "{CB3FB4C2-AC12-4EAB-A0D3-CB6C62752B47}");
    }
}

Component.prototype.createOperations = function() {
    component.createOperations();

    if (installer.value("os") === "win") {
        component.addOperation("CreateShortcut", "@TargetDir@/mpz.exe", "@StartMenuDir@/mpz.lnk");
        component.addOperation("CreateShortcut", "@TargetDir@/maintenancetool.exe", "@StartMenuDir@/Uninstall mpz.lnk");

        // Qt Installer Framework writes no QuietUninstallString, so winget and other
        // package managers fall back to the GUI. It has no registry operation either.
        var tool = installer.value("TargetDir").replace(/\//g, "\\") + "\\maintenancetool.exe";
        component.addOperation("Execute", "reg", "add",
            "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + installer.value("ProductUUID"),
            "/v", "QuietUninstallString", "/t", "REG_SZ",
            "/d", '"' + tool + '" --confirm-command --accept-messages purge', "/f");
    }
}
