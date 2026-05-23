#pragma once

#include "axmol/axmol.h"

namespace pf::art {

ax::Node* createGrassTile(float size);
ax::Node* createForestTile(float size);
ax::Node* createWaterTile(float size);
ax::Node* createRockyTile(float size);

ax::Node* createPlayer(float size);
ax::Node* createCompanion(float size);
ax::Node* createWolf(float size);

ax::Node* createTree(float size);
ax::Node* createBerryBush(float size);
ax::Node* createRedBerry(float size);
ax::Node* createDecayedRedBerry(float size);
ax::Node* createAxe(float size);
ax::Node* createLooseStone(float size);
ax::Node* createStonePile(float size);
ax::Node* createStoneFlake(float size);
ax::Node* createWood(float size);
ax::Node* createTorch(float size);
ax::Node* createCampFire(float size);

ax::Node* createPathDot(float size);
ax::Node* createSelectionTile(float size);

} // namespace pf::art
