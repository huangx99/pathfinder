#include "pathfinder/tribe/tribe_conflict.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

namespace pathfinder::tribe {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static constexpr double kEpsilon = 0.000001;

static double clampDelta(double value) {
    return std::max(-1.0, std::min(1.0, value));
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool isValidHostility(HostilityState state) {
    return state != HostilityState::Unknown && state != HostilityState::TestOnly;
}

static bool isValidConflictIntent(ConflictIntentKind kind) {
    return kind != ConflictIntentKind::Unknown && kind != ConflictIntentKind::TestOnly;
}

static bool isValidConflictStance(ConflictStanceKind kind) {
    return kind != ConflictStanceKind::Unknown && kind != ConflictStanceKind::TestOnly;
}

static bool isValidConflictOutcome(ConflictOutcomeKind kind) {
    return kind != ConflictOutcomeKind::Unknown && kind != ConflictOutcomeKind::TestOnly;
}

static bool isValidConflictEvent(ConflictEventKind kind) {
    return kind != ConflictEventKind::Unknown && kind != ConflictEventKind::TestOnly;
}

static Result<void> validateId(const EntityId& id, const std::string& field) {
    if (id.empty())
        return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!pathfinder::foundation::isValidIdString(id.value()))
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateId(const TribeId& id, const std::string& field) {
    if (id.empty())
        return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!pathfinder::foundation::isValidIdString(id.value()))
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateId(const EventId& id, const std::string& field) {
    if (id.empty())
        return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!pathfinder::foundation::isValidIdString(id.value()))
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateRatio(double value, const std::string& field) {
    if (!isValidRatio(value))
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be 0..1"));
    return Result<void>::ok();
}

static Result<void> validateDelta(double value, const std::string& field) {
    if (!std::isfinite(value) || value < -1.0 || value > 1.0)
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be -1..1"));
    return Result<void>::ok();
}

static std::string compactDouble(double value) {
    std::ostringstream stream;
    stream.precision(3);
    stream << std::fixed << value;
    std::string text = stream.str();
    while (text.size() > 1 && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    return text;
}

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

std::string toString(HostilityState state) {
    switch (state) {
        case HostilityState::Unknown:      return "unknown";
        case HostilityState::Neutral:      return "neutral";
        case HostilityState::Wary:         return "wary";
        case HostilityState::Threatened:   return "threatened";
        case HostilityState::Hostile:      return "hostile";
        case HostilityState::OpenConflict: return "open_conflict";
        case HostilityState::Retreating:   return "retreating";
        case HostilityState::Truce:        return "truce";
        case HostilityState::TestOnly:     return "test_only";
    }
    return "unknown";
}

Result<HostilityState> hostilityStateFromString(const std::string& str) {
    if (str == "unknown")       return Result<HostilityState>::ok(HostilityState::Unknown);
    if (str == "neutral")       return Result<HostilityState>::ok(HostilityState::Neutral);
    if (str == "wary")          return Result<HostilityState>::ok(HostilityState::Wary);
    if (str == "threatened")    return Result<HostilityState>::ok(HostilityState::Threatened);
    if (str == "hostile")       return Result<HostilityState>::ok(HostilityState::Hostile);
    if (str == "open_conflict") return Result<HostilityState>::ok(HostilityState::OpenConflict);
    if (str == "retreating")    return Result<HostilityState>::ok(HostilityState::Retreating);
    if (str == "truce")         return Result<HostilityState>::ok(HostilityState::Truce);
    if (str == "test_only")     return Result<HostilityState>::ok(HostilityState::TestOnly);
    return Result<HostilityState>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid HostilityState: " + str));
}

std::string toString(ConflictIntentKind kind) {
    switch (kind) {
        case ConflictIntentKind::Unknown:         return "unknown";
        case ConflictIntentKind::Avoid:           return "avoid";
        case ConflictIntentKind::Intimidate:      return "intimidate";
        case ConflictIntentKind::DefendTerritory: return "defend_territory";
        case ConflictIntentKind::ContestResource: return "contest_resource";
        case ConflictIntentKind::Raid:            return "raid";
        case ConflictIntentKind::Pursue:          return "pursue";
        case ConflictIntentKind::Retreat:         return "retreat";
        case ConflictIntentKind::NegotiateTruce:  return "negotiate_truce";
        case ConflictIntentKind::TestOnly:        return "test_only";
    }
    return "unknown";
}

Result<ConflictIntentKind> conflictIntentKindFromString(const std::string& str) {
    if (str == "unknown")          return Result<ConflictIntentKind>::ok(ConflictIntentKind::Unknown);
    if (str == "avoid")            return Result<ConflictIntentKind>::ok(ConflictIntentKind::Avoid);
    if (str == "intimidate")       return Result<ConflictIntentKind>::ok(ConflictIntentKind::Intimidate);
    if (str == "defend_territory") return Result<ConflictIntentKind>::ok(ConflictIntentKind::DefendTerritory);
    if (str == "contest_resource") return Result<ConflictIntentKind>::ok(ConflictIntentKind::ContestResource);
    if (str == "raid")             return Result<ConflictIntentKind>::ok(ConflictIntentKind::Raid);
    if (str == "pursue")           return Result<ConflictIntentKind>::ok(ConflictIntentKind::Pursue);
    if (str == "retreat")          return Result<ConflictIntentKind>::ok(ConflictIntentKind::Retreat);
    if (str == "negotiate_truce")  return Result<ConflictIntentKind>::ok(ConflictIntentKind::NegotiateTruce);
    if (str == "test_only")        return Result<ConflictIntentKind>::ok(ConflictIntentKind::TestOnly);
    return Result<ConflictIntentKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ConflictIntentKind: " + str));
}

std::string toString(ConflictStanceKind kind) {
    switch (kind) {
        case ConflictStanceKind::Unknown:    return "unknown";
        case ConflictStanceKind::Passive:    return "passive";
        case ConflictStanceKind::Defensive:  return "defensive";
        case ConflictStanceKind::Assertive:  return "assertive";
        case ConflictStanceKind::Aggressive: return "aggressive";
        case ConflictStanceKind::Desperate:  return "desperate";
        case ConflictStanceKind::TestOnly:   return "test_only";
    }
    return "unknown";
}

Result<ConflictStanceKind> conflictStanceKindFromString(const std::string& str) {
    if (str == "unknown")    return Result<ConflictStanceKind>::ok(ConflictStanceKind::Unknown);
    if (str == "passive")    return Result<ConflictStanceKind>::ok(ConflictStanceKind::Passive);
    if (str == "defensive")  return Result<ConflictStanceKind>::ok(ConflictStanceKind::Defensive);
    if (str == "assertive")  return Result<ConflictStanceKind>::ok(ConflictStanceKind::Assertive);
    if (str == "aggressive") return Result<ConflictStanceKind>::ok(ConflictStanceKind::Aggressive);
    if (str == "desperate")  return Result<ConflictStanceKind>::ok(ConflictStanceKind::Desperate);
    if (str == "test_only")  return Result<ConflictStanceKind>::ok(ConflictStanceKind::TestOnly);
    return Result<ConflictStanceKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ConflictStanceKind: " + str));
}

std::string toString(ConflictOutcomeKind kind) {
    switch (kind) {
        case ConflictOutcomeKind::Unknown:           return "unknown";
        case ConflictOutcomeKind::NoContact:         return "no_contact";
        case ConflictOutcomeKind::Avoided:           return "avoided";
        case ConflictOutcomeKind::Intimidated:       return "intimidated";
        case ConflictOutcomeKind::Standoff:          return "standoff";
        case ConflictOutcomeKind::ForcedRetreat:     return "forced_retreat";
        case ConflictOutcomeKind::MinorLossPressure: return "minor_loss_pressure";
        case ConflictOutcomeKind::MajorLossPressure: return "major_loss_pressure";
        case ConflictOutcomeKind::TruceOffered:      return "truce_offered";
        case ConflictOutcomeKind::TestOnly:          return "test_only";
    }
    return "unknown";
}

Result<ConflictOutcomeKind> conflictOutcomeKindFromString(const std::string& str) {
    if (str == "unknown")             return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::Unknown);
    if (str == "no_contact")          return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::NoContact);
    if (str == "avoided")             return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::Avoided);
    if (str == "intimidated")         return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::Intimidated);
    if (str == "standoff")            return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::Standoff);
    if (str == "forced_retreat")      return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::ForcedRetreat);
    if (str == "minor_loss_pressure") return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::MinorLossPressure);
    if (str == "major_loss_pressure") return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::MajorLossPressure);
    if (str == "truce_offered")       return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::TruceOffered);
    if (str == "test_only")           return Result<ConflictOutcomeKind>::ok(ConflictOutcomeKind::TestOnly);
    return Result<ConflictOutcomeKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ConflictOutcomeKind: " + str));
}

std::string toString(ConflictEventKind kind) {
    switch (kind) {
        case ConflictEventKind::Unknown:                return "unknown";
        case ConflictEventKind::RelationChanged:        return "relation_changed";
        case ConflictEventKind::HostilityChanged:       return "hostility_changed";
        case ConflictEventKind::IntimidationApplied:    return "intimidation_applied";
        case ConflictEventKind::RetreatTriggered:       return "retreat_triggered";
        case ConflictEventKind::ResourceContested:      return "resource_contested";
        case ConflictEventKind::ConflictPressureChanged: return "conflict_pressure_changed";
        case ConflictEventKind::TruceProposed:           return "truce_proposed";
        case ConflictEventKind::TestOnly:                return "test_only";
    }
    return "unknown";
}

Result<ConflictEventKind> conflictEventKindFromString(const std::string& str) {
    if (str == "unknown")                    return Result<ConflictEventKind>::ok(ConflictEventKind::Unknown);
    if (str == "relation_changed")           return Result<ConflictEventKind>::ok(ConflictEventKind::RelationChanged);
    if (str == "hostility_changed")          return Result<ConflictEventKind>::ok(ConflictEventKind::HostilityChanged);
    if (str == "intimidation_applied")       return Result<ConflictEventKind>::ok(ConflictEventKind::IntimidationApplied);
    if (str == "retreat_triggered")          return Result<ConflictEventKind>::ok(ConflictEventKind::RetreatTriggered);
    if (str == "resource_contested")         return Result<ConflictEventKind>::ok(ConflictEventKind::ResourceContested);
    if (str == "conflict_pressure_changed")  return Result<ConflictEventKind>::ok(ConflictEventKind::ConflictPressureChanged);
    if (str == "truce_proposed")             return Result<ConflictEventKind>::ok(ConflictEventKind::TruceProposed);
    if (str == "test_only")                  return Result<ConflictEventKind>::ok(ConflictEventKind::TestOnly);
    return Result<ConflictEventKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ConflictEventKind: " + str));
}

// ---------------------------------------------------------------------------
// Forbidden keys
// ---------------------------------------------------------------------------

bool containsConflictForbiddenKey(const std::string& key) {
    const std::string lower = lowerCopy(key);
    static const std::vector<std::string> forbidden = {
        "hidden_truth", "real_effect", "true_trait", "object_truth",
        "raw_state", "gamestate", "game_state", "savegame", "save_game",
        "agentruntime", "agent_runtime", "policy",
        "random_hit", "random_damage", "damage_roll", "critical_hit",
        "true_hp", "actual_hp", "actual_damage", "kill_result", "loot_drop",
        "war_result", "damage", "death", "kill", "loot", "hp",
        "corpse", "hidden_weapon_stat",
        "member_dead", "enemy_dead", "kill_count", "hp_delta",
        "body_count", "loot_count",
        "random_split", "probability_split"
    };
    for (const auto& token : forbidden) {
        if (lower.find(token) != std::string::npos) return true;
    }
    return false;
}

bool containsConflictForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) {
        return containsConflictForbiddenKey(key);
    });
}

static bool isValidStanceForIntent(ConflictIntentKind intent, ConflictStanceKind stance) {
    if (intent == ConflictIntentKind::Avoid) return stance == ConflictStanceKind::Passive;
    if (intent == ConflictIntentKind::Retreat) return stance == ConflictStanceKind::Passive || stance == ConflictStanceKind::Desperate;
    return stance != ConflictStanceKind::Unknown;
}

// ---------------------------------------------------------------------------
// Deterministic formulas
// ---------------------------------------------------------------------------

static double computeConflictStrength(const ConflictParticipantTribe& participant) {
    double coordination = participant.coordination_result.pack_tactic.coordination_score;
    double morale = participant.morale_summary;
    double trust = participant.trust_summary;
    double split_risk = participant.split_risk_summary;
    double safety = participant.safety_pressure_summary;
    double loss = participant.loss_pressure_summary;

    double strength = clampRatio(
        coordination * 0.40 +
        morale * 0.20 +
        trust * 0.15 -
        split_risk * 0.20 -
        safety * 0.15 -
        loss * 0.10
    );
    return strength;
}

static ConflictIntentKind deriveConflictIntent(
    const ConflictParticipantTribe& participant,
    const HostilityState hostility,
    const ConflictPressureSummary& pressure,
    double strength) {

    double total_pressure = clampRatio(
        pressure.resource_pressure * 0.30 +
        pressure.territory_pressure * 0.25 +
        pressure.fear_pressure * 0.20 +
        pressure.misunderstanding_pressure * 0.10 +
        pressure.survival_pressure * 0.10 +
        pressure.visibility_pressure * 0.05
    );

    // Neutral + low pressure -> Avoid
    if (hostility <= HostilityState::Neutral && total_pressure < 0.3) {
        return ConflictIntentKind::Avoid;
    }

    // High loss pressure + low resource -> NegotiateTruce
    if (participant.loss_pressure_summary >= 0.6 &&
        pressure.resource_pressure < 0.4 &&
        hostility <= HostilityState::Hostile) {
        return ConflictIntentKind::NegotiateTruce;
    }

    // High fear + low strength -> Retreat
    if (pressure.fear_pressure >= 0.6 && strength < 0.35) {
        return ConflictIntentKind::Retreat;
    }

    // High strength + opponent weakness -> Intimidate
    if (strength >= 0.65 && hostility >= HostilityState::Threatened) {
        return ConflictIntentKind::Intimidate;
    }

    // High resource pressure + at least Wary -> ContestResource
    if (pressure.resource_pressure >= 0.5 && hostility >= HostilityState::Wary) {
        return ConflictIntentKind::ContestResource;
    }

    // High territory pressure + decent coordination -> DefendTerritory
    if (pressure.territory_pressure >= 0.5 && strength >= 0.4) {
        return ConflictIntentKind::DefendTerritory;
    }

    // OpenConflict + high survival -> Raid (pressure only, no kill/death)
    if (hostility >= HostilityState::OpenConflict && pressure.survival_pressure >= 0.7) {
        return ConflictIntentKind::Raid;
    }

    // Mid strength, mid hostility -> ContestResource
    if (strength >= 0.45 && hostility >= HostilityState::Wary) {
        return ConflictIntentKind::ContestResource;
    }

    // Default: Avoid if low strength, otherwise DefendTerritory
    if (strength < 0.3) return ConflictIntentKind::Avoid;
    return ConflictIntentKind::DefendTerritory;
}

static ConflictStanceKind deriveStance(ConflictIntentKind intent, double strength, double fear_pressure) {
    if (intent == ConflictIntentKind::Avoid) return ConflictStanceKind::Passive;
    if (intent == ConflictIntentKind::Retreat) {
        return fear_pressure >= 0.7 ? ConflictStanceKind::Desperate : ConflictStanceKind::Passive;
    }
    if (intent == ConflictIntentKind::Intimidate) {
        return strength >= 0.7 ? ConflictStanceKind::Aggressive : ConflictStanceKind::Assertive;
    }
    if (intent == ConflictIntentKind::Raid || intent == ConflictIntentKind::Pursue) {
        return strength >= 0.6 ? ConflictStanceKind::Aggressive : ConflictStanceKind::Assertive;
    }
    if (intent == ConflictIntentKind::DefendTerritory) return ConflictStanceKind::Defensive;
    if (intent == ConflictIntentKind::ContestResource) {
        return strength >= 0.5 ? ConflictStanceKind::Assertive : ConflictStanceKind::Defensive;
    }
    if (intent == ConflictIntentKind::NegotiateTruce) return ConflictStanceKind::Passive;
    return ConflictStanceKind::Defensive;
}

static ConflictOutcomeKind resolveOutcome(
    ConflictIntentKind source_intent,
    ConflictIntentKind target_intent,
    double source_strength,
    double target_strength,
    const ConflictResolutionOptions& options,
    HostilityState hostility) {

    double gap = source_strength - target_strength;

    // Both Avoid -> Avoided
    if (source_intent == ConflictIntentKind::Avoid && target_intent == ConflictIntentKind::Avoid) {
        return ConflictOutcomeKind::Avoided;
    }

    // NoContact when one avoids and other is passive
    if ((source_intent == ConflictIntentKind::Avoid && target_intent == ConflictIntentKind::Avoid) ||
        (source_intent == ConflictIntentKind::Avoid && target_intent == ConflictIntentKind::DefendTerritory)) {
        return ConflictOutcomeKind::NoContact;
    }

    // One retreats, other doesn't pursue -> ForcedRetreat
    if (source_intent == ConflictIntentKind::Retreat) {
        return ConflictOutcomeKind::ForcedRetreat;
    }
    if (target_intent == ConflictIntentKind::Retreat) {
        return ConflictOutcomeKind::ForcedRetreat;
    }

    // Intimidate + gap >= threshold -> Intimidated
    if (source_intent == ConflictIntentKind::Intimidate && gap >= options.intimidation_threshold) {
        return ConflictOutcomeKind::Intimidated;
    }
    if (target_intent == ConflictIntentKind::Intimidate && -gap >= options.intimidation_threshold) {
        return ConflictOutcomeKind::Intimidated;
    }

    // NegotiateTruce + other side has low strength -> TruceOffered
    if (source_intent == ConflictIntentKind::NegotiateTruce &&
        target_strength < options.truce_pressure_threshold) {
        return ConflictOutcomeKind::TruceOffered;
    }
    if (target_intent == ConflictIntentKind::NegotiateTruce &&
        source_strength < options.truce_pressure_threshold) {
        return ConflictOutcomeKind::TruceOffered;
    }

    // Close strength + high hostility -> Standoff
    if (std::abs(gap) <= options.standoff_gap_threshold && hostility >= HostilityState::Hostile) {
        return ConflictOutcomeKind::Standoff;
    }

    // High hostility, both assertive/aggressive, high survival -> LossPressure
    if (hostility >= HostilityState::Hostile &&
        std::abs(gap) <= options.standoff_gap_threshold + 0.1) {
        // Determine severity by gap
        if (std::abs(gap) < 0.05) return ConflictOutcomeKind::MajorLossPressure;
        return ConflictOutcomeKind::MinorLossPressure;
    }

    // One side much stronger -> Intimidated/ForcedRetreat
    if (gap >= options.intimidation_threshold) {
        return ConflictOutcomeKind::Intimidated;
    }
    if (-gap >= options.intimidation_threshold) {
        return ConflictOutcomeKind::ForcedRetreat;
    }

    // Default: Standoff for anything unresolved
    return ConflictOutcomeKind::Standoff;
}

// ---------------------------------------------------------------------------
// DTO validation
// ---------------------------------------------------------------------------

Result<void> TribeRelation::validateBasic() const {
    auto src_result = validateId(source_tribe_id, "TribeRelation source_tribe_id");
    if (src_result.is_error()) return src_result;
    auto tgt_result = validateId(target_tribe_id, "TribeRelation target_tribe_id");
    if (tgt_result.is_error()) return tgt_result;
    if (source_tribe_id == target_tribe_id) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeRelation source and target must differ"));
    }
    if (!isValidHostility(hostility_state)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeRelation hostility_state invalid"));
    }
    for (const auto& item : {std::pair<double, const char*>{trust_score, "TribeRelation trust_score"},
                             {fear_pressure, "TribeRelation fear_pressure"},
                             {grievance_pressure, "TribeRelation grievance_pressure"},
                             {resource_tension, "TribeRelation resource_tension"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeRelation reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> TribeRelationDraft::validateBasic() const {
    auto src_result = validateId(source_tribe_id, "TribeRelationDraft source_tribe_id");
    if (src_result.is_error()) return src_result;
    auto tgt_result = validateId(target_tribe_id, "TribeRelationDraft target_tribe_id");
    if (tgt_result.is_error()) return tgt_result;
    if (source_tribe_id == target_tribe_id) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeRelationDraft source and target must differ"));
    }
    if (!isValidHostility(new_hostility_state)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeRelationDraft new_hostility_state invalid"));
    }
    auto td_result = validateDelta(trust_delta, "TribeRelationDraft trust_delta");
    if (td_result.is_error()) return td_result;
    auto fp_result = validateDelta(fear_pressure_delta, "TribeRelationDraft fear_pressure_delta");
    if (fp_result.is_error()) return fp_result;
    auto gp_result = validateDelta(grievance_pressure_delta, "TribeRelationDraft grievance_pressure_delta");
    if (gp_result.is_error()) return gp_result;
    auto rt_result = validateDelta(resource_tension_delta, "TribeRelationDraft resource_tension_delta");
    if (rt_result.is_error()) return rt_result;
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeRelationDraft reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConflictPressureSummary::validateBasic() const {
    if (conflict_key.empty() || containsConflictForbiddenKey(conflict_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictPressureSummary conflict_key invalid"));
    }
    for (const auto& item : {std::pair<double, const char*>{resource_pressure, "ConflictPressureSummary resource_pressure"},
                             {territory_pressure, "ConflictPressureSummary territory_pressure"},
                             {fear_pressure, "ConflictPressureSummary fear_pressure"},
                             {misunderstanding_pressure, "ConflictPressureSummary misunderstanding_pressure"},
                             {survival_pressure, "ConflictPressureSummary survival_pressure"},
                             {visibility_pressure, "ConflictPressureSummary visibility_pressure"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    std::set<std::string> seen;
    for (const auto& id : known_public_knowledge_ids) {
        if (id.empty())
            return Result<void>::fail(makeError(ErrorCode::id_missing, "ConflictPressureSummary knowledge_id missing"));
        if (!pathfinder::foundation::isValidIdString(id.value()))
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ConflictPressureSummary knowledge_id invalid"));
        if (!seen.insert(id.value()).second)
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictPressureSummary duplicate knowledge_id"));
    }
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictPressureSummary reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConflictParticipantTribe::validateBasic() const {
    auto id_result = validateId(tribe_id, "ConflictParticipantTribe tribe_id");
    if (id_result.is_error()) return id_result;
    if (tribe_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "ConflictParticipantTribe tribe_id missing"));
    }
    if (role_in_conflict.empty() || containsConflictForbiddenKey(role_in_conflict)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictParticipantTribe role_in_conflict invalid"));
    }
    if (member_count_summary < 0 || active_population_summary < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ConflictParticipantTribe population summary invalid"));
    }
    for (const auto& item : {std::pair<double, const char*>{morale_summary, "ConflictParticipantTribe morale_summary"},
                             {trust_summary, "ConflictParticipantTribe trust_summary"},
                             {split_risk_summary, "ConflictParticipantTribe split_risk_summary"},
                             {safety_pressure_summary, "ConflictParticipantTribe safety_pressure_summary"},
                             {loss_pressure_summary, "ConflictParticipantTribe loss_pressure_summary"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    auto coord_result = coordination_result.validateBasic();
    if (coord_result.is_error()) return coord_result;
    auto csd_result = coordination_state_draft.validateBasic();
    if (csd_result.is_error()) return csd_result;
    std::set<std::string> seen;
    for (const auto& id : public_knowledge_ids) {
        if (id.empty())
            return Result<void>::fail(makeError(ErrorCode::id_missing, "ConflictParticipantTribe knowledge_id missing"));
        if (!pathfinder::foundation::isValidIdString(id.value()))
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ConflictParticipantTribe knowledge_id invalid"));
        if (!seen.insert(id.value()).second)
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictParticipantTribe duplicate knowledge_id"));
    }
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictParticipantTribe reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConflictIntent::validateBasic() const {
    auto id_result = validateId(tribe_id, "ConflictIntent tribe_id");
    if (id_result.is_error()) return id_result;
    if (!isValidConflictIntent(intent_kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictIntent intent_kind invalid"));
    }
    if (!isValidConflictStance(stance_kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictIntent stance_kind invalid"));
    }
    auto c_result = validateRatio(confidence, "ConflictIntent confidence");
    if (c_result.is_error()) return c_result;
    auto p_result = validateRatio(pressure_score, "ConflictIntent pressure_score");
    if (p_result.is_error()) return p_result;
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictIntent reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConflictStateDraft::validateBasic() const {
    auto id_result = validateId(tribe_id, "ConflictStateDraft tribe_id");
    if (id_result.is_error()) return id_result;
    for (const auto& item : {std::pair<double, const char*>{morale_delta, "ConflictStateDraft morale_delta"},
                             {trust_delta, "ConflictStateDraft trust_delta"},
                             {safety_pressure_delta, "ConflictStateDraft safety_pressure_delta"},
                             {loss_pressure_delta, "ConflictStateDraft loss_pressure_delta"},
                             {split_risk_delta, "ConflictStateDraft split_risk_delta"},
                             {retreat_pressure_delta, "ConflictStateDraft retreat_pressure_delta"}}) {
        auto r = validateDelta(item.first, item.second);
        if (r.is_error()) return r;
    }
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictStateDraft reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConflictEvent::validateBasic() const {
    auto id_result = validateId(event_id, "ConflictEvent event_id");
    if (id_result.is_error()) return id_result;
    if (!isValidConflictEvent(event_kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictEvent event_kind invalid"));
    }
    auto src_result = validateId(source_tribe_id, "ConflictEvent source_tribe_id");
    if (src_result.is_error()) return src_result;
    auto tgt_result = validateId(target_tribe_id, "ConflictEvent target_tribe_id");
    if (tgt_result.is_error()) return tgt_result;
    if (source_tribe_id == target_tribe_id) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictEvent source and target must differ"));
    }
    if (hostility_before.has_value() && !isValidHostility(hostility_before.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictEvent hostility_before invalid"));
    }
    if (hostility_after.has_value() && !isValidHostility(hostility_after.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictEvent hostility_after invalid"));
    }
    if (public_message_key.empty() || containsConflictForbiddenKey(public_message_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictEvent public_message_key invalid"));
    }
    if (containsConflictForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictEvent reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConflictProjection::validateBasic() const {
    auto src_result = validateId(source_tribe_id, "ConflictProjection source_tribe_id");
    if (src_result.is_error()) return src_result;
    auto tgt_result = validateId(target_tribe_id, "ConflictProjection target_tribe_id");
    if (tgt_result.is_error()) return tgt_result;
    if (!isValidHostility(visible_hostility_state)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictProjection visible_hostility_state invalid"));
    }
    if (!isValidConflictOutcome(visible_outcome)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictProjection visible_outcome invalid"));
    }
    if (public_summary_key.empty() || containsConflictForbiddenKey(public_summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictProjection public_summary_key invalid"));
    }
    if (containsConflictForbiddenKey(public_reason_keys) || containsConflictForbiddenKey(visible_pressure_level)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictProjection contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> ConflictTrace::validateBasic() const {
    if (trace_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "ConflictTrace trace_id missing"));
    }
    if (containsConflictForbiddenKey(input_summary_keys) ||
        containsConflictForbiddenKey(score_breakdown) ||
        containsConflictForbiddenKey(outcome_reason_keys) ||
        containsConflictForbiddenKey(relation_transition_reason_keys) ||
        containsConflictForbiddenKey(rejected_unsafe_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictTrace contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> ConflictResolutionOptions::validateBasic() const {
    for (const auto& item : {std::pair<double, const char*>{intimidation_threshold, "ConflictResolutionOptions intimidation_threshold"},
                             {retreat_threshold, "ConflictResolutionOptions retreat_threshold"},
                             {standoff_gap_threshold, "ConflictResolutionOptions standoff_gap_threshold"},
                             {hostility_escalation_threshold, "ConflictResolutionOptions hostility_escalation_threshold"},
                             {truce_pressure_threshold, "ConflictResolutionOptions truce_pressure_threshold"},
                             {max_pressure_delta_per_tick, "ConflictResolutionOptions max_pressure_delta_per_tick"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> ConflictResolutionInput::validateBasic() const {
    if (source.tribe_id == target.tribe_id) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictResolutionInput source and target must differ"));
    }
    auto src_result = source.validateBasic();
    if (src_result.is_error()) return src_result;
    auto tgt_result = target.validateBasic();
    if (tgt_result.is_error()) return tgt_result;
    auto rel_result = source_relation_to_target.validateBasic();
    if (rel_result.is_error()) return rel_result;
    if (source_relation_to_target.source_tribe_id != source.tribe_id ||
        source_relation_to_target.target_tribe_id != target.tribe_id) {
        return Result<void>::fail(makeError(ErrorCode::id_type_mismatch, "ConflictResolutionInput relation mismatch with participants"));
    }
    if (target_relation_to_source.has_value()) {
        auto tr_result = target_relation_to_source.value().validateBasic();
        if (tr_result.is_error()) return tr_result;
    }
    auto ps_result = pressure_summary.validateBasic();
    if (ps_result.is_error()) return ps_result;
    auto opt_result = options.validateBasic();
    if (opt_result.is_error()) return opt_result;
    if (!options.allow_test_only) {
        if (source.coordination_result.intent.kind == GroupIntentKind::Unknown ||
            source.coordination_result.intent.kind == GroupIntentKind::TestOnly ||
            target.coordination_result.intent.kind == GroupIntentKind::Unknown ||
            target.coordination_result.intent.kind == GroupIntentKind::TestOnly) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictResolutionInput coordination_result has invalid intent"));
        }
    }
    return Result<void>::ok();
}

Result<void> ConflictResolutionResult::validateBasic() const {
    if (!ok) {
        if (outcome_kind != ConflictOutcomeKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConflictResolutionResult ok=false must have Unknown outcome"));
        }
        return Result<void>::ok();
    }
    if (!isValidConflictOutcome(outcome_kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConflictResolutionResult outcome_kind invalid"));
    }
    auto ssd_result = source_state_draft.validateBasic();
    if (ssd_result.is_error()) return ssd_result;
    auto tsd_result = target_state_draft.validateBasic();
    if (tsd_result.is_error()) return tsd_result;
    auto srd_result = source_relation_draft.validateBasic();
    if (srd_result.is_error()) return srd_result;
    if (target_relation_draft.has_value()) {
        auto trd_result = target_relation_draft.value().validateBasic();
        if (trd_result.is_error()) return trd_result;
    }
    for (const auto& event : events) {
        auto r = event.validateBasic();
        if (r.is_error()) return r;
    }
    auto pj_result = projection.validateBasic();
    if (pj_result.is_error()) return pj_result;
    auto tr_result = trace.validateBasic();
    if (tr_result.is_error()) return tr_result;
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ConflictProjectionBuilder
// ---------------------------------------------------------------------------

Result<ConflictProjection> ConflictProjectionBuilder::build(
    const ConflictResolutionResult& result) const {
    ConflictProjection proj;
    proj.source_tribe_id = result.source_relation_draft.source_tribe_id;
    proj.target_tribe_id = result.source_relation_draft.target_tribe_id;
    proj.visible_hostility_state = result.source_relation_draft.new_hostility_state;
    proj.visible_outcome = result.outcome_kind;
    proj.public_summary_key = "tribe_conflict_" + toString(result.outcome_kind);
    proj.relation_changed = (result.source_relation_draft.old_hostility_state !=
                              result.source_relation_draft.new_hostility_state);
    proj.state_pressure_changed = (std::abs(result.source_state_draft.loss_pressure_delta) > kEpsilon ||
                                    std::abs(result.source_state_draft.safety_pressure_delta) > kEpsilon);

    // Visible pressure level
    double max_pressure = std::max({result.source_state_draft.loss_pressure_delta,
                                     result.source_state_draft.safety_pressure_delta,
                                     result.source_state_draft.retreat_pressure_delta});
    if (max_pressure >= 0.5) proj.visible_pressure_level = "high";
    else if (max_pressure >= 0.2) proj.visible_pressure_level = "moderate";
    else if (max_pressure > kEpsilon) proj.visible_pressure_level = "low";
    else proj.visible_pressure_level = "none";

    proj.public_reason_keys = {"tribe_conflict_resolved"};

    auto val_result = proj.validateBasic();
    if (val_result.is_error()) return Result<ConflictProjection>::fail(val_result.errors());
    return Result<ConflictProjection>::ok(std::move(proj));
}

// ---------------------------------------------------------------------------
// ConflictStateDraftBuilder
// ---------------------------------------------------------------------------

Result<ConflictStateDraft> ConflictStateDraftBuilder::buildForTribe(
    const ConflictParticipantTribe& participant,
    ConflictOutcomeKind outcome,
    const ConflictIntent& own_intent,
    const ConflictIntent& opponent_intent) const {
    ConflictStateDraft draft;
    draft.tribe_id = participant.tribe_id;

    // Merge P24 coordination state draft effects with P25 conflict outcome
    const auto& csd = participant.coordination_state_draft;

    switch (outcome) {
        case ConflictOutcomeKind::Intimidated:
            if (own_intent.intent_kind == ConflictIntentKind::Intimidate) {
                draft.morale_delta = clampDelta(csd.morale_delta + 0.03);
                draft.safety_pressure_delta = clampDelta(-0.05);
                draft.trust_delta = clampDelta(csd.trust_delta + 0.02);
            }
            break;
        case ConflictOutcomeKind::ForcedRetreat:
            if (own_intent.intent_kind == ConflictIntentKind::Retreat) {
                draft.morale_delta = clampDelta(csd.morale_delta - 0.05);
                draft.retreat_pressure_delta = clampDelta(0.15);
                draft.safety_pressure_delta = clampDelta(0.10);
                draft.trust_delta = clampDelta(csd.trust_delta - 0.03);
            }
            break;
        case ConflictOutcomeKind::Standoff:
            draft.safety_pressure_delta = clampDelta(0.10);
            draft.morale_delta = clampDelta(csd.morale_delta - 0.02);
            draft.split_risk_delta = clampDelta(0.05);
            draft.trust_delta = clampDelta(csd.trust_delta - 0.02);
            break;
        case ConflictOutcomeKind::MinorLossPressure:
            draft.loss_pressure_delta = clampDelta(0.15);
            draft.safety_pressure_delta = clampDelta(0.10);
            draft.morale_delta = clampDelta(csd.morale_delta - 0.05);
            draft.trust_delta = clampDelta(csd.trust_delta - 0.05);
            break;
        case ConflictOutcomeKind::MajorLossPressure:
            draft.loss_pressure_delta = clampDelta(0.30);
            draft.safety_pressure_delta = clampDelta(0.20);
            draft.morale_delta = clampDelta(csd.morale_delta - 0.10);
            draft.split_risk_delta = clampDelta(0.10);
            draft.trust_delta = clampDelta(csd.trust_delta - 0.08);
            break;
        case ConflictOutcomeKind::TruceOffered:
            draft.morale_delta = clampDelta(csd.morale_delta + 0.02);
            draft.safety_pressure_delta = clampDelta(-0.05);
            draft.trust_delta = clampDelta(csd.trust_delta + 0.01);
            break;
        case ConflictOutcomeKind::Avoided:
        case ConflictOutcomeKind::NoContact:
            draft.trust_delta = clampDelta(csd.trust_delta);
            draft.morale_delta = clampDelta(csd.morale_delta);
            break;
        default:
            break;
    }

    // Propagate P24 trust/morale deltas as baseline when not already set by outcome
    if (std::abs(draft.trust_delta) < kEpsilon) draft.trust_delta = clampDelta(csd.trust_delta);
    if (std::abs(draft.morale_delta) < kEpsilon) draft.morale_delta = clampDelta(csd.morale_delta);

    // P24 trust/morale deltas always feed through
    draft.trust_delta = clampDelta(csd.trust_delta);
    draft.reason_keys = {"p25_conflict_state_draft"};

    auto val_result = draft.validateBasic();
    if (val_result.is_error()) return Result<ConflictStateDraft>::fail(val_result.errors());
    return Result<ConflictStateDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// TribeRelationTransitionPolicy
// ---------------------------------------------------------------------------

static HostilityState escalateHostility(HostilityState current, double pressure) {
    if (pressure >= 0.8) {
        switch (current) {
            case HostilityState::Neutral:    return HostilityState::Threatened;
            case HostilityState::Wary:       return HostilityState::Hostile;
            case HostilityState::Threatened: return HostilityState::OpenConflict;
            case HostilityState::Hostile:    return HostilityState::OpenConflict;
            case HostilityState::Truce:      return HostilityState::Wary;
            default:                         return current;
        }
    } else if (pressure >= 0.5) {
        switch (current) {
            case HostilityState::Neutral:    return HostilityState::Wary;
            case HostilityState::Wary:       return HostilityState::Threatened;
            case HostilityState::Threatened: return HostilityState::Hostile;
            case HostilityState::Truce:      return HostilityState::Wary;
            default:                         return current;
        }
    } else {
        switch (current) {
            case HostilityState::Neutral:    return HostilityState::Wary;
            default:                         return current;
        }
    }
}

static HostilityState deescalateHostility(HostilityState current) {
    switch (current) {
        case HostilityState::OpenConflict: return HostilityState::Retreating;
        case HostilityState::Hostile:      return HostilityState::Wary;
        case HostilityState::Threatened:   return HostilityState::Wary;
        case HostilityState::Retreating:   return HostilityState::Wary;
        case HostilityState::Wary:         return HostilityState::Neutral;
        default:                           return current;
    }
}

Result<TribeRelationDraft> TribeRelationTransitionPolicy::transition(
    const TribeRelation& current,
    ConflictOutcomeKind outcome,
    const ConflictPressureSummary& pressure) const {
    TribeRelationDraft draft;
    draft.source_tribe_id = current.source_tribe_id;
    draft.target_tribe_id = current.target_tribe_id;
    draft.old_hostility_state = current.hostility_state;
    draft.new_hostility_state = current.hostility_state;

    double total_pressure = clampRatio(
        pressure.resource_pressure * 0.25 +
        pressure.territory_pressure * 0.20 +
        pressure.fear_pressure * 0.25 +
        pressure.survival_pressure * 0.20 +
        pressure.misunderstanding_pressure * 0.10
    );

    switch (outcome) {
        case ConflictOutcomeKind::Standoff:
            draft.new_hostility_state = escalateHostility(current.hostility_state, total_pressure);
            draft.grievance_pressure_delta = clampDelta(0.10);
            draft.fear_pressure_delta = clampDelta(0.05);
            draft.resource_tension_delta = clampDelta(0.10);
            break;
        case ConflictOutcomeKind::Intimidated:
            draft.new_hostility_state = escalateHostility(current.hostility_state, total_pressure);
            draft.fear_pressure_delta = clampDelta(-0.10);
            draft.trust_delta = -0.05;
            break;
        case ConflictOutcomeKind::ForcedRetreat:
            draft.new_hostility_state = escalateHostility(current.hostility_state, total_pressure * 0.5);
            draft.fear_pressure_delta = clampDelta(0.10);
            draft.trust_delta = -0.05;
            break;
        case ConflictOutcomeKind::MinorLossPressure:
            draft.new_hostility_state = escalateHostility(current.hostility_state, total_pressure + 0.1);
            draft.grievance_pressure_delta = clampDelta(0.15);
            draft.trust_delta = -0.10;
            draft.resource_tension_delta = clampDelta(0.10);
            break;
        case ConflictOutcomeKind::MajorLossPressure:
            draft.new_hostility_state = escalateHostility(current.hostility_state, total_pressure + 0.2);
            draft.grievance_pressure_delta = clampDelta(0.25);
            draft.trust_delta = -0.15;
            draft.resource_tension_delta = clampDelta(0.15);
            break;
        case ConflictOutcomeKind::TruceOffered:
            draft.new_hostility_state = HostilityState::Truce;
            draft.trust_delta = 0.02;
            draft.grievance_pressure_delta = clampDelta(-0.10);
            draft.fear_pressure_delta = clampDelta(-0.05);
            break;
        case ConflictOutcomeKind::Avoided:
        case ConflictOutcomeKind::NoContact:
            // No hostility change for Avoided/NoContact
            break;
        default:
            break;
    }

    draft.reason_keys = {"p25_relation_transition"};

    auto val_result = draft.validateBasic();
    if (val_result.is_error()) return Result<TribeRelationDraft>::fail(val_result.errors());
    return Result<TribeRelationDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// GroupCombatResolver
// ---------------------------------------------------------------------------

Result<ConflictResolutionResult> GroupCombatResolver::resolve(
    const ConflictResolutionInput& input) const {
    auto in_result = input.validateBasic();
    if (in_result.is_error()) {
        ConflictResolutionResult error_result;
        error_result.ok = false;
        error_result.error = in_result.errors().empty()
            ? ErrorCode::validation_failed
            : in_result.errors()[0].code;
        return Result<ConflictResolutionResult>::ok(std::move(error_result));
    }

    ConflictResolutionResult result;
    result.ok = true;
    result.trace.trace_id = EventId("trace_p25_" + input.source.tribe_id.value() +
                                    "_vs_" + input.target.tribe_id.value());

    // Compute deterministic conflict strength
    double source_strength = computeConflictStrength(input.source);
    double target_strength = computeConflictStrength(input.target);

    result.trace.source_coordination_score = input.source.coordination_result.pack_tactic.coordination_score;
    result.trace.target_coordination_score = input.target.coordination_result.pack_tactic.coordination_score;

    // Derive intents
    HostilityState hostility = input.source_relation_to_target.hostility_state;

    ConflictIntentKind source_intent_kind = deriveConflictIntent(
        input.source, hostility, input.pressure_summary, source_strength);
    ConflictIntentKind target_intent_kind = deriveConflictIntent(
        input.target, hostility, input.pressure_summary, target_strength);

    ConflictStanceKind source_stance = deriveStance(
        source_intent_kind, source_strength, input.pressure_summary.fear_pressure);
    ConflictStanceKind target_stance = deriveStance(
        target_intent_kind, target_strength, input.pressure_summary.fear_pressure);

    result.trace.source_intent.tribe_id = input.source.tribe_id;
    result.trace.source_intent.intent_kind = source_intent_kind;
    result.trace.source_intent.stance_kind = source_stance;
    result.trace.source_intent.confidence = source_strength;
    result.trace.source_intent.pressure_score = input.pressure_summary.resource_pressure;

    result.trace.target_intent.tribe_id = input.target.tribe_id;
    result.trace.target_intent.intent_kind = target_intent_kind;
    result.trace.target_intent.stance_kind = target_stance;
    result.trace.target_intent.confidence = target_strength;
    result.trace.target_intent.pressure_score = input.pressure_summary.resource_pressure;

    // Resolve outcome
    result.outcome_kind = resolveOutcome(
        source_intent_kind, target_intent_kind,
        source_strength, target_strength,
        input.options, hostility);

    // Build score breakdown
    result.trace.score_breakdown.push_back(
        "source_strength=" + compactDouble(source_strength));
    result.trace.score_breakdown.push_back(
        "target_strength=" + compactDouble(target_strength));
    result.trace.score_breakdown.push_back(
        "source_coord=" + compactDouble(input.source.coordination_result.pack_tactic.coordination_score));
    result.trace.score_breakdown.push_back(
        "target_coord=" + compactDouble(input.target.coordination_result.pack_tactic.coordination_score));
    result.trace.score_breakdown.push_back(
        "gap=" + compactDouble(source_strength - target_strength));

    // Build relation draft
    TribeRelationTransitionPolicy relation_policy;
    auto srd_result = relation_policy.transition(
        input.source_relation_to_target, result.outcome_kind, input.pressure_summary);
    if (srd_result.is_error()) {
        result.ok = false;
        result.error = srd_result.errors()[0].code;
        return Result<ConflictResolutionResult>::ok(std::move(result));
    }
    result.source_relation_draft = srd_result.value();

    if (input.target_relation_to_source.has_value()) {
        auto trd_result = relation_policy.transition(
            input.target_relation_to_source.value(), result.outcome_kind, input.pressure_summary);
        if (trd_result.is_ok()) {
            result.target_relation_draft = trd_result.value();
        }
    }

    // Build state drafts
    ConflictStateDraftBuilder state_builder;
    ConflictIntent source_intent_full{input.source.tribe_id, source_intent_kind, source_stance, source_strength, input.pressure_summary.resource_pressure, {"p25"}};
    ConflictIntent target_intent_full{input.target.tribe_id, target_intent_kind, target_stance, target_strength, input.pressure_summary.resource_pressure, {"p25"}};

    auto ssd_result = state_builder.buildForTribe(
        input.source, result.outcome_kind, source_intent_full, target_intent_full);
    if (ssd_result.is_ok()) {
        result.source_state_draft = ssd_result.value();
    }

    auto tsd_result = state_builder.buildForTribe(
        input.target, result.outcome_kind, target_intent_full, source_intent_full);
    if (tsd_result.is_ok()) {
        result.target_state_draft = tsd_result.value();
    }

    // Build events
    ConflictEvent ev;
    ev.event_id = EventId("event_p25_" + input.source.tribe_id.value() + "_" +
                          input.target.tribe_id.value());
    ev.source_tribe_id = input.source.tribe_id;
    ev.target_tribe_id = input.target.tribe_id;
    ev.outcome_kind = result.outcome_kind;
    ev.hostility_before = input.source_relation_to_target.hostility_state;
    ev.hostility_after = result.source_relation_draft.new_hostility_state;
    ev.reason_keys = {"p25_conflict_event"};

    if (result.source_relation_draft.old_hostility_state !=
        result.source_relation_draft.new_hostility_state) {
        ev.event_kind = ConflictEventKind::HostilityChanged;
    } else {
        ev.event_kind = ConflictEventKind::ConflictPressureChanged;
    }

    switch (result.outcome_kind) {
        case ConflictOutcomeKind::Intimidated:
            ev.event_kind = ConflictEventKind::IntimidationApplied;
            ev.public_message_key = "tribe_conflict_intimidated";
            break;
        case ConflictOutcomeKind::ForcedRetreat:
            ev.event_kind = ConflictEventKind::RetreatTriggered;
            ev.public_message_key = "tribe_conflict_forced_retreat";
            break;
        case ConflictOutcomeKind::Standoff:
            ev.event_kind = ConflictEventKind::ResourceContested;
            ev.public_message_key = "tribe_conflict_standoff";
            break;
        case ConflictOutcomeKind::MinorLossPressure:
        case ConflictOutcomeKind::MajorLossPressure:
            ev.event_kind = ConflictEventKind::ConflictPressureChanged;
            ev.public_message_key = "tribe_conflict_loss_pressure";
            break;
        case ConflictOutcomeKind::TruceOffered:
            ev.event_kind = ConflictEventKind::TruceProposed;
            ev.public_message_key = "tribe_conflict_truce_offered";
            break;
        case ConflictOutcomeKind::Avoided:
        case ConflictOutcomeKind::NoContact:
            ev.event_kind = ConflictEventKind::HostilityChanged;
            ev.public_message_key = "tribe_conflict_avoided";
            break;
        default:
            break;
    }

    result.events.push_back(std::move(ev));
    result.trace.outcome_reason_keys = {"p25_deterministic_outcome"};

    // Build projection
    ConflictProjectionBuilder proj_builder;
    auto proj_result = proj_builder.build(result);
    if (proj_result.is_ok()) {
        result.projection = proj_result.value();
    }

    auto final_result = result.validateBasic();
    if (final_result.is_error()) {
        result.ok = false;
        result.error = final_result.errors()[0].code;
    }

    return Result<ConflictResolutionResult>::ok(std::move(result));
}

} // namespace pathfinder::tribe
