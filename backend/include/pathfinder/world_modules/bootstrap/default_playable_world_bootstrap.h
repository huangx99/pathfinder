#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_runtime/world_runtime_types.h"

namespace pathfinder::world_modules {

// Seeds the default playable scenario after the authoritative world runtime is initialized.
// This is intentionally outside client_http so default content can evolve or be replaced
// without turning the HTTP gateway factory into a gameplay rule container.
pathfinder::foundation::Result<void> spawnDefaultPlayableWorldEntities(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    const pathfinder::world_runtime::WorldRuntimeConfig& config);

} // namespace pathfinder::world_modules
