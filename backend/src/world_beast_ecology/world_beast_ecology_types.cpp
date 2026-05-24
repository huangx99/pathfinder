#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cctype>

namespace pathfinder::world_beast_ecology {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// BeastTemperamentKind
// ---------------------------------------------------------------------------

std::string toString(BeastTemperamentKind kind) {
    switch (kind) {
        case BeastTemperamentKind::Unknown: return "unknown";
        case BeastTemperamentKind::Passive: return "passive";
        case BeastTemperamentKind::Skittish: return "skittish";
        case BeastTemperamentKind::Curious: return "curious";
        case BeastTemperamentKind::Territorial: return "territorial";
        case BeastTemperamentKind::Predatory: return "predatory";
        case BeastTemperamentKind::Pack: return "pack";
        case BeastTemperamentKind::Constructed: return "constructed";
        case BeastTemperamentKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<BeastTemperamentKind> beastTemperamentKindFromString(const std::string& str) {
    std::string lower;
    lower.reserve(str.size());
    for (char c : str) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "unknown") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Unknown);
    if (lower == "passive") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Passive);
    if (lower == "skittish") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Skittish);
    if (lower == "curious") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Curious);
    if (lower == "territorial") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Territorial);
    if (lower == "predatory") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Predatory);
    if (lower == "pack") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Pack);
    if (lower == "constructed") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::Constructed);
    if (lower == "test_only") return Result<BeastTemperamentKind>::ok(BeastTemperamentKind::TestOnly);
    return Result<BeastTemperamentKind>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_beast_temperament_kind"));
}

// ---------------------------------------------------------------------------
// BeastNeedKind
// ---------------------------------------------------------------------------

std::string toString(BeastNeedKind kind) {
    switch (kind) {
        case BeastNeedKind::Unknown: return "unknown";
        case BeastNeedKind::Hunger: return "hunger";
        case BeastNeedKind::Fear: return "fear";
        case BeastNeedKind::Territory: return "territory";
        case BeastNeedKind::Curiosity: return "curiosity";
        case BeastNeedKind::Pain: return "pain";
        case BeastNeedKind::Rest: return "rest";
        case BeastNeedKind::ProtectYoung: return "protect_young";
        case BeastNeedKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<BeastNeedKind> beastNeedKindFromString(const std::string& str) {
    std::string lower;
    lower.reserve(str.size());
    for (char c : str) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "unknown") return Result<BeastNeedKind>::ok(BeastNeedKind::Unknown);
    if (lower == "hunger") return Result<BeastNeedKind>::ok(BeastNeedKind::Hunger);
    if (lower == "fear") return Result<BeastNeedKind>::ok(BeastNeedKind::Fear);
    if (lower == "territory") return Result<BeastNeedKind>::ok(BeastNeedKind::Territory);
    if (lower == "curiosity") return Result<BeastNeedKind>::ok(BeastNeedKind::Curiosity);
    if (lower == "pain") return Result<BeastNeedKind>::ok(BeastNeedKind::Pain);
    if (lower == "rest") return Result<BeastNeedKind>::ok(BeastNeedKind::Rest);
    if (lower == "protect_young") return Result<BeastNeedKind>::ok(BeastNeedKind::ProtectYoung);
    if (lower == "test_only") return Result<BeastNeedKind>::ok(BeastNeedKind::TestOnly);
    return Result<BeastNeedKind>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_beast_need_kind"));
}

// ---------------------------------------------------------------------------
// BeastPerceptionKind
// ---------------------------------------------------------------------------

std::string toString(BeastPerceptionKind kind) {
    switch (kind) {
        case BeastPerceptionKind::Unknown: return "unknown";
        case BeastPerceptionKind::Actor: return "actor";
        case BeastPerceptionKind::Object: return "object";
        case BeastPerceptionKind::Resource: return "resource";
        case BeastPerceptionKind::Effect: return "effect";
        case BeastPerceptionKind::Sound: return "sound";
        case BeastPerceptionKind::Area: return "area";
        case BeastPerceptionKind::MemoryCue: return "memory_cue";
        case BeastPerceptionKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<BeastPerceptionKind> beastPerceptionKindFromString(const std::string& str) {
    std::string lower;
    lower.reserve(str.size());
    for (char c : str) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "unknown") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Unknown);
    if (lower == "actor") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Actor);
    if (lower == "object") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Object);
    if (lower == "resource") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Resource);
    if (lower == "effect") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Effect);
    if (lower == "sound") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Sound);
    if (lower == "area") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::Area);
    if (lower == "memory_cue") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::MemoryCue);
    if (lower == "test_only") return Result<BeastPerceptionKind>::ok(BeastPerceptionKind::TestOnly);
    return Result<BeastPerceptionKind>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_beast_perception_kind"));
}

// ---------------------------------------------------------------------------
// BeastActionIntentKind
// ---------------------------------------------------------------------------

std::string toString(BeastActionIntentKind kind) {
    switch (kind) {
        case BeastActionIntentKind::Unknown: return "unknown";
        case BeastActionIntentKind::Wait: return "wait";
        case BeastActionIntentKind::Wander: return "wander";
        case BeastActionIntentKind::Approach: return "approach";
        case BeastActionIntentKind::Flee: return "flee";
        case BeastActionIntentKind::Attack: return "attack";
        case BeastActionIntentKind::Threaten: return "threaten";
        case BeastActionIntentKind::Observe: return "observe";
        case BeastActionIntentKind::AvoidArea: return "avoid_area";
        case BeastActionIntentKind::CallPack: return "call_pack";
        case BeastActionIntentKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<BeastActionIntentKind> beastActionIntentKindFromString(const std::string& str) {
    std::string lower;
    lower.reserve(str.size());
    for (char c : str) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "unknown") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Unknown);
    if (lower == "wait") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Wait);
    if (lower == "wander") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Wander);
    if (lower == "approach") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Approach);
    if (lower == "flee") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Flee);
    if (lower == "attack") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Attack);
    if (lower == "threaten") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Threaten);
    if (lower == "observe") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::Observe);
    if (lower == "avoid_area") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::AvoidArea);
    if (lower == "call_pack") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::CallPack);
    if (lower == "test_only") return Result<BeastActionIntentKind>::ok(BeastActionIntentKind::TestOnly);
    return Result<BeastActionIntentKind>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_beast_action_intent_kind"));
}

// ---------------------------------------------------------------------------
// BeastDecisionReasonKind
// ---------------------------------------------------------------------------

std::string toString(BeastDecisionReasonKind kind) {
    switch (kind) {
        case BeastDecisionReasonKind::Unknown: return "unknown";
        case BeastDecisionReasonKind::InstinctNeed: return "instinct_need";
        case BeastDecisionReasonKind::PerceivedPrey: return "perceived_prey";
        case BeastDecisionReasonKind::PerceivedDanger: return "perceived_danger";
        case BeastDecisionReasonKind::LearnedRisk: return "learned_risk";
        case BeastDecisionReasonKind::TerritoryIntrusion: return "territory_intrusion";
        case BeastDecisionReasonKind::CommandBlocked: return "command_blocked";
        case BeastDecisionReasonKind::NoValidAction: return "no_valid_action";
        case BeastDecisionReasonKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<BeastDecisionReasonKind> beastDecisionReasonKindFromString(const std::string& str) {
    std::string lower;
    lower.reserve(str.size());
    for (char c : str) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "unknown") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::Unknown);
    if (lower == "instinct_need") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::InstinctNeed);
    if (lower == "perceived_prey") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::PerceivedPrey);
    if (lower == "perceived_danger") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::PerceivedDanger);
    if (lower == "learned_risk") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::LearnedRisk);
    if (lower == "territory_intrusion") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::TerritoryIntrusion);
    if (lower == "command_blocked") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::CommandBlocked);
    if (lower == "no_valid_action") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::NoValidAction);
    if (lower == "test_only") return Result<BeastDecisionReasonKind>::ok(BeastDecisionReasonKind::TestOnly);
    return Result<BeastDecisionReasonKind>::fail(makeError(ErrorCode::command_invalid_argument, "unknown_beast_decision_reason_kind"));
}

// ---------------------------------------------------------------------------
// validateBasic
// ---------------------------------------------------------------------------

Result<void> BeastInstinctCapability::validateBasic() const {
    if (capability_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "capability_key_empty"));
    if (action_kind == BeastActionIntentKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "action_kind_unknown"));
    if (command_kind == pathfinder::world_command::WorldCommandKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "command_kind_unknown"));
    if (risk_limit < 0.0 || risk_limit > 100.0) return Result<void>::fail(makeError(ErrorCode::validation_failed, "risk_limit_out_of_range"));
    return Result<void>::ok();
}

Result<void> BeastAgentProfile::validateBasic() const {
    if (profile_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "profile_key_empty"));
    if (temperament == BeastTemperamentKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "temperament_unknown"));
    if (vision_radius < 0 || vision_radius > 64) return Result<void>::fail(makeError(ErrorCode::validation_failed, "vision_radius_out_of_range"));
    if (base_aggression < 0.0 || base_aggression > 100.0) return Result<void>::fail(makeError(ErrorCode::validation_failed, "base_aggression_out_of_range"));
    if (base_fear < 0.0 || base_fear > 100.0) return Result<void>::fail(makeError(ErrorCode::validation_failed, "base_fear_out_of_range"));
    if (attack_damage < 1 || attack_damage > 1000) return Result<void>::fail(makeError(ErrorCode::validation_failed, "attack_damage_out_of_range"));
    bool can_wait_or_move = false;
    for (const auto& cap : instinct_capabilities) {
        auto v = cap.validateBasic();
        if (v.is_error()) return v;
        if (cap.action_kind == BeastActionIntentKind::Wait || cap.action_kind == BeastActionIntentKind::Wander) {
            can_wait_or_move = true;
        }
    }
    if (!can_wait_or_move) return Result<void>::fail(makeError(ErrorCode::validation_failed, "instinct_must_allow_wait_or_move"));
    return Result<void>::ok();
}

Result<void> BeastPerceptionItem::validateBasic() const {
    if (perception_id.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "perception_id_empty"));
    if (kind == BeastPerceptionKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "perception_kind_unknown"));
    if (target_ref.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "target_ref_empty"));
    if (target_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "target_key_empty"));
    if (distance < 0) return Result<void>::fail(makeError(ErrorCode::validation_failed, "distance_negative"));
    if (intensity < 0.0 || intensity > 100.0) return Result<void>::fail(makeError(ErrorCode::validation_failed, "intensity_out_of_range"));
    return Result<void>::ok();
}

Result<void> BeastTickRequest::validateBasic() const {
    if (request_id.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "request_id_empty"));
    if (actor_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_key_empty"));
    return Result<void>::ok();
}

Result<void> BeastActionIntent::validateBasic() const {
    if (intent_id.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "intent_id_empty"));
    if (actor_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_key_empty"));
    if (kind == BeastActionIntentKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "intent_kind_unknown"));
    if (command_kind == pathfinder::world_command::WorldCommandKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "command_kind_unknown"));
    if (reason_kind == BeastDecisionReasonKind::Unknown) return Result<void>::fail(makeError(ErrorCode::validation_failed, "reason_kind_unknown"));
    if (risk_score < 0.0 || risk_score > 100.0) return Result<void>::fail(makeError(ErrorCode::validation_failed, "risk_score_out_of_range"));
    if (attack_damage < 1 || attack_damage > 1000) return Result<void>::fail(makeError(ErrorCode::validation_failed, "intent_attack_damage_out_of_range"));
    if ((kind == BeastActionIntentKind::Approach || kind == BeastActionIntentKind::Attack || kind == BeastActionIntentKind::Flee) && target_ref.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "target_ref_required_for_intent"));
    }
    return Result<void>::ok();
}

Result<void> BeastTickResult::validateBasic() const {
    if (actor_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_key_empty"));
    if (selected_intent.has_value()) {
        auto v = selected_intent.value().validateBasic();
        if (v.is_error()) return v;
    }
    if (issued_command.has_value()) {
        auto v = issued_command.value().validateBasic();
        if (v.is_error()) return v;
        if (issued_command.value().source != pathfinder::world_command::WorldCommandSource::BeastDecision) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "command_source_must_be_beast_decision"));
        }
    }
    if (command_result.has_value()) {
        auto v = command_result.value().validateBasic();
        if (v.is_error()) return v;
    }
    if (projection_patch.has_value()) {
        auto v = projection_patch.value().validateBasic();
        if (v.is_error()) return v;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_beast_ecology
