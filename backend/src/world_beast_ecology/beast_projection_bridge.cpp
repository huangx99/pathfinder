#include "pathfinder/world_beast_ecology/beast_projection_bridge.h"

namespace pathfinder::world_beast_ecology {

BeastProjectionBridge::SafeBeastProjection BeastProjectionBridge::project(const BeastTickResult& result) {
    SafeBeastProjection projection;
    projection.actor_key = result.actor_key;
    if (result.selected_intent.has_value()) {
        projection.intent_kind_str = toString(result.selected_intent.value().kind);
        projection.reason_kind_str = toString(result.selected_intent.value().reason_kind);
        projection.safe_summary_zh_cn = result.selected_intent.value().safe_summary_zh_cn;
    } else {
        projection.intent_kind_str = "none";
        projection.reason_kind_str = "none";
    }
    projection.reason_keys = result.reason_keys;
    return projection;
}

} // namespace pathfinder::world_beast_ecology
