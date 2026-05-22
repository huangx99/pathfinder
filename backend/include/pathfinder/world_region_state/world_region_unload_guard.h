#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/foundation/result.h"
#include <set>

namespace pathfinder::world_region_state {

// P59: 判定某 region 当前能否安全 detach
// 第一版要检查：
// - 玩家所在 region 不卸载
// - 正在执行命令触达的 region 不卸载
// - 存在未支持归属/容器/跨 region 关系时不卸载
// - snapshot build/validation 未成功不卸载
class WorldRegionUnloadGuard {
public:
    explicit WorldRegionUnloadGuard(world_runtime::IWorldRuntime& world_runtime);

    struct GuardCheckResult {
        bool can_unload = false;
        std::vector<std::string> blocking_reason_keys;
    };

    GuardCheckResult checkCanUnload(
        const world_generation::WorldRegionKey& region_key,
        const std::set<std::string>& active_command_region_ids = {});

    // For testing: explicit check that region contains no InInventory/InContainer/Equipped entities
    GuardCheckResult checkNoComplexOwnership(
        const world_generation::WorldRegionKey& region_key);

private:
    world_runtime::IWorldRuntime& world_runtime_;

    bool isActorInRegion(const std::string& actor_key, const world_generation::WorldRegionKey& region_key);
};

} // namespace pathfinder::world_region_state
