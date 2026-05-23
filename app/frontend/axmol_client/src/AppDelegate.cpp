#include "AppDelegate.h"
#include "ClientCommandPort.h"
#include "GameScene.h"
#include <memory>

namespace pathfinder::axmol_client {

bool AppDelegate::applicationDidFinishLaunching() {
    auto scene = GameScene(std::make_unique<NullClientCommandPort>());
    return scene.start();
}

void AppDelegate::applicationDidEnterBackground() {
}

void AppDelegate::applicationWillEnterForeground() {
}

} // namespace pathfinder::axmol_client
