#include "macupdater.h"

#import <Sparkle/Sparkle.h>

MacUpdater::MacUpdater(QObject *parent) : QObject(parent) {
  SPUStandardUpdaterController *controller =
    [[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES updaterDelegate:nil userDriverDelegate:nil];
  updater_controller = (__bridge_retained void *)controller;
}

MacUpdater::~MacUpdater() {
  if (updater_controller != nullptr) {
    (void)(__bridge_transfer SPUStandardUpdaterController *)updater_controller;
    updater_controller = nullptr;
  }
}

void MacUpdater::checkForUpdates() {
  SPUStandardUpdaterController *controller = (__bridge SPUStandardUpdaterController *)updater_controller;
  [controller.updater checkForUpdates];
}
