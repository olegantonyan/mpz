function Component() {
}

Component.prototype.createOperations = function() {
    component.createOperations();

    if (installer.value("os") === "win") {
        component.addOperation("CreateShortcut", "@TargetDir@/mpz.exe", "@StartMenuDir@/mpz.lnk");
    }
}
