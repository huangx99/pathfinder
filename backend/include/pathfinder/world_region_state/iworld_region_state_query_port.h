#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_region_state {

// P59: 从 runtime 读取指定 region 的 cell/entity/node 状态
// 硬约束：不能用客户端 projection 读取；不能把 snapshotForDebug() 作为长期 production port
class IWorldRegionStateQueryPort {
public:
    virtual ~IWorldRegionStateQueryPort() = default;

    virtual foundation::Result<WorldRegionRuntimeSlice> readRegionSlice(
        const world_generation::WorldRegionKey& key) const = 0;
};

} // namespace pathfinder::world_region_state
