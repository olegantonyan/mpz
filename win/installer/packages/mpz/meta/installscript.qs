function Component() {
    // Without this QtIFW generates a random GUID per run and registers a new
    // Apps & Features entry each install. config.xml can't set it (QTIFW-1321).
    if (installer.value("os") === "win") {
        installer.setValue("ProductUUID", "{CB3FB4C2-AC12-4EAB-A0D3-CB6C62752B47}");
    }
}

Component.prototype.createOperations = function() {
    component.createOperations();

    if (installer.value("os") === "win") {
        component.addOperation("CreateShortcut", "@TargetDir@/mpz.exe", "@StartMenuDir@/mpz.lnk");
        component.addOperation("CreateShortcut", "@TargetDir@/maintenancetool.exe", "@StartMenuDir@/Uninstall mpz.lnk");
    }
}
