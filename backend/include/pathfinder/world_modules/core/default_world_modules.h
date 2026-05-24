#pragma once

#include "pathfinder/world_modules/core/world_module_context.h"

namespace pathfinder::world_modules {

void registerDefaultWorldModules(WorldModuleContext& context);
void clearDefaultWorldModuleRuntimeState();

} // namespace pathfinder::world_modules
