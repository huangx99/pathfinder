#include "pathfinder/world_beast_ecology/beast_instinct_gate.h"

namespace pathfinder::world_beast_ecology {

BeastInstinctGate::GateResult BeastInstinctGate::check(const BeastAgentProfile& profile, const BeastActionIntent& intent) {
    GateResult result;
    for (const auto& cap : profile.instinct_capabilities) {
        if (cap.action_kind == intent.kind) {
            if (intent.risk_score > cap.risk_limit) {
                result.blocker_key = "instinct_risk_limit_exceeded";
                result.reason_keys.push_back("risk_exceeds_instinct_limit");
                return result;
            }
            result.allowed = true;
            result.reason_keys.push_back("instinct_capability_match");
            return result;
        }
    }
    result.blocker_key = "missing_instinct_capability";
    result.reason_keys.push_back("no_matching_instinct_capability");
    return result;
}

} // namespace pathfinder::world_beast_ecology
