#pragma once

#include "ClientCommandPort.h"
#include "ProjectionModel.h"
#include <memory>

namespace pathfinder::axmol_client {

class GameScene final {
public:
    explicit GameScene(std::unique_ptr<ClientCommandPort> command_port);

    bool start();
    void update(float delta_seconds);
    void render();

    void onTileTapped(int x, int y);

private:
    std::unique_ptr<ClientCommandPort> command_port_;
    ProjectionModel projection_;
};

} // namespace pathfinder::axmol_client
