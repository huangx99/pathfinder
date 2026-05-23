#include "GameScene.h"

namespace pathfinder::axmol_client {

GameScene::GameScene(std::unique_ptr<ClientCommandPort> command_port)
    : command_port_(std::move(command_port)) {
}

bool GameScene::start() {
    if (!command_port_) return false;
    return command_port_->bootstrap();
}

void GameScene::update(float) {
}

void GameScene::render() {
    // Rendering is intentionally empty in the first skeleton.
    // Future Axmol scene code should draw ProjectionModel only; rules stay in backend.
}

void GameScene::onTileTapped(int, int) {
    if (!command_port_) return;
    // Future implementation must pick from availableCommands(), not invent commands.
}

} // namespace pathfinder::axmol_client
