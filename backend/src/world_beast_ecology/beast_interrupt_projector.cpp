#include "pathfinder/world_beast_ecology/beast_interrupt_projector.h"

namespace pathfinder::world_beast_ecology {

using pathfinder::goal_execution::ExternalInterruptSignal;
using pathfinder::goal_execution::ExternalInterruptKind;

std::vector<ExternalInterruptSignal> BeastInterruptProjector::project(
    const BeastActionIntent& intent,
    const std::vector<BeastPerceptionItem>& perceptions,
    uint64_t tick) {

    std::vector<ExternalInterruptSignal> signals;

    if (intent.kind != BeastActionIntentKind::Approach && intent.kind != BeastActionIntentKind::Attack) {
        return signals;
    }

    for (const auto& item : perceptions) {
        if (item.kind != BeastPerceptionKind::Actor) continue;
        if (item.target_ref.empty()) continue;
        if (item.distance > 2) continue;

        ExternalInterruptSignal sig;
        sig.interrupt_id = "beast_interrupt_" + intent.actor_key + "_" + item.target_ref + "_" + std::to_string(tick);
        sig.kind = item.distance <= 1 ? ExternalInterruptKind::ThreatEscalated : ExternalInterruptKind::ThreatAppeared;
        sig.source_event_id = intent.intent_id;
        sig.target_actor_keys.push_back(item.target_ref);
        sig.threat_key = intent.actor_key;
        sig.severity = intent.risk_score;
        sig.urgency = static_cast<double>(3 - item.distance) * 25.0;
        sig.can_be_ignored = false;
        sig.public_summary_zh_cn = "beast_approaching";
        sig.reason_keys.push_back("beast_nearby");
        signals.push_back(sig);
    }

    return signals;
}

} // namespace pathfinder::world_beast_ecology
