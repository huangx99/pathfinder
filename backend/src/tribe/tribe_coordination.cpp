#include "pathfinder/tribe/tribe_coordination.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <map>
#include <set>
#include <sstream>

namespace pathfinder::tribe {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static constexpr double kEpsilon = 0.000001;

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool isValidGroupIntent(GroupIntentKind kind) {
    return kind != GroupIntentKind::Unknown && kind != GroupIntentKind::TestOnly;
}

static bool isValidAssistAction(AssistActionKind kind) {
    return kind != AssistActionKind::Unknown && kind != AssistActionKind::TestOnly;
}

static bool isValidDefendAction(DefendActionKind kind) {
    return kind != DefendActionKind::Unknown && kind != DefendActionKind::TestOnly;
}

static bool isValidPackTactic(PackTacticKind kind) {
    return kind != PackTacticKind::Unknown && kind != PackTacticKind::TestOnly;
}

static bool isValidQuality(CoordinationQuality quality) {
    return quality != CoordinationQuality::Unknown && quality != CoordinationQuality::TestOnly;
}

static Result<void> validateId(const EntityId& id, const std::string& field) {
    if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!pathfinder::foundation::isValidIdString(id.value()))
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateId(const TribeId& id, const std::string& field) {
    if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
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

static std::string joinIds(const std::vector<EntityId>& ids) {
    std::ostringstream stream;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) stream << ",";
        stream << ids[i].value();
    }
    return stream.str();
}

static double roleScore(TribeMemberRole role) {
    switch (role) {
        case TribeMemberRole::Pioneer:    return 0.7;
        case TribeMemberRole::Explorer:   return 0.55;
        case TribeMemberRole::Gatherer:   return 0.4;
        case TribeMemberRole::Guardian:   return 0.8;
        case TribeMemberRole::Teacher:    return 0.6;
        case TribeMemberRole::Learner:    return 0.25;
        case TribeMemberRole::Healer:     return 0.5;
        case TribeMemberRole::Crafter:    return 0.45;
        case TribeMemberRole::Elder:      return 0.65;
        case TribeMemberRole::Child:      return 0.1;
        default:                          return 0.0;
    }
}

static double meanThreatPressure(const std::vector<ThreatPressureSummary>& threats) {
    if (threats.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& t : threats) sum += t.threat_pressure;
    return clampRatio(sum / static_cast<double>(threats.size()));
}

// Enums

std::string toString(GroupIntentKind kind) {
    switch (kind) {
        case GroupIntentKind::Unknown:         return "unknown";
        case GroupIntentKind::HoldTogether:     return "hold_together";
        case GroupIntentKind::AssistMember:     return "assist_member";
        case GroupIntentKind::DefendGroup:      return "defend_group";
        case GroupIntentKind::FocusThreat:      return "focus_threat";
        case GroupIntentKind::ScreenRetreat:    return "screen_retreat";
        case GroupIntentKind::EscortVulnerable: return "escort_vulnerable";
        case GroupIntentKind::AvoidEngagement:  return "avoid_engagement";
        case GroupIntentKind::TestOnly:         return "test_only";
    }
    return "unknown";
}

Result<GroupIntentKind> groupIntentKindFromString(const std::string& str) {
    if (str == "unknown") return Result<GroupIntentKind>::ok(GroupIntentKind::Unknown);
    if (str == "hold_together") return Result<GroupIntentKind>::ok(GroupIntentKind::HoldTogether);
    if (str == "assist_member") return Result<GroupIntentKind>::ok(GroupIntentKind::AssistMember);
    if (str == "defend_group") return Result<GroupIntentKind>::ok(GroupIntentKind::DefendGroup);
    if (str == "focus_threat") return Result<GroupIntentKind>::ok(GroupIntentKind::FocusThreat);
    if (str == "screen_retreat") return Result<GroupIntentKind>::ok(GroupIntentKind::ScreenRetreat);
    if (str == "escort_vulnerable") return Result<GroupIntentKind>::ok(GroupIntentKind::EscortVulnerable);
    if (str == "avoid_engagement") return Result<GroupIntentKind>::ok(GroupIntentKind::AvoidEngagement);
    if (str == "test_only") return Result<GroupIntentKind>::ok(GroupIntentKind::TestOnly);
    return Result<GroupIntentKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid GroupIntentKind: " + str));
}

std::string toString(AssistActionKind kind) {
    switch (kind) {
        case AssistActionKind::Unknown:        return "unknown";
        case AssistActionKind::Warn:           return "warn";
        case AssistActionKind::GuidePosition:   return "guide_position";
        case AssistActionKind::ShareTool:       return "share_tool";
        case AssistActionKind::PullBack:        return "pull_back";
        case AssistActionKind::CoverApproach:   return "cover_approach";
        case AssistActionKind::StabilizeMorale: return "stabilize_morale";
        case AssistActionKind::TestOnly:        return "test_only";
    }
    return "unknown";
}

Result<AssistActionKind> assistActionKindFromString(const std::string& str) {
    if (str == "unknown") return Result<AssistActionKind>::ok(AssistActionKind::Unknown);
    if (str == "warn") return Result<AssistActionKind>::ok(AssistActionKind::Warn);
    if (str == "guide_position") return Result<AssistActionKind>::ok(AssistActionKind::GuidePosition);
    if (str == "share_tool") return Result<AssistActionKind>::ok(AssistActionKind::ShareTool);
    if (str == "pull_back") return Result<AssistActionKind>::ok(AssistActionKind::PullBack);
    if (str == "cover_approach") return Result<AssistActionKind>::ok(AssistActionKind::CoverApproach);
    if (str == "stabilize_morale") return Result<AssistActionKind>::ok(AssistActionKind::StabilizeMorale);
    if (str == "test_only") return Result<AssistActionKind>::ok(AssistActionKind::TestOnly);
    return Result<AssistActionKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid AssistActionKind: " + str));
}

std::string toString(DefendActionKind kind) {
    switch (kind) {
        case DefendActionKind::Unknown:       return "unknown";
        case DefendActionKind::GuardMember:   return "guard_member";
        case DefendActionKind::GuardResource: return "guard_resource";
        case DefendActionKind::BlockThreat:   return "block_threat";
        case DefendActionKind::FormPerimeter: return "form_perimeter";
        case DefendActionKind::HoldLine:      return "hold_line";
        case DefendActionKind::ScreenEscape:  return "screen_escape";
        case DefendActionKind::TestOnly:      return "test_only";
    }
    return "unknown";
}

Result<DefendActionKind> defendActionKindFromString(const std::string& str) {
    if (str == "unknown") return Result<DefendActionKind>::ok(DefendActionKind::Unknown);
    if (str == "guard_member") return Result<DefendActionKind>::ok(DefendActionKind::GuardMember);
    if (str == "guard_resource") return Result<DefendActionKind>::ok(DefendActionKind::GuardResource);
    if (str == "block_threat") return Result<DefendActionKind>::ok(DefendActionKind::BlockThreat);
    if (str == "form_perimeter") return Result<DefendActionKind>::ok(DefendActionKind::FormPerimeter);
    if (str == "hold_line") return Result<DefendActionKind>::ok(DefendActionKind::HoldLine);
    if (str == "screen_escape") return Result<DefendActionKind>::ok(DefendActionKind::ScreenEscape);
    if (str == "test_only") return Result<DefendActionKind>::ok(DefendActionKind::TestOnly);
    return Result<DefendActionKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid DefendActionKind: " + str));
}

std::string toString(PackTacticKind kind) {
    switch (kind) {
        case PackTacticKind::Unknown:          return "unknown";
        case PackTacticKind::None:             return "none";
        case PackTacticKind::PairSupport:      return "pair_support";
        case PackTacticKind::GuardedGather:    return "guarded_gather";
        case PackTacticKind::FocusPressure:    return "focus_pressure";
        case PackTacticKind::RotatingDefense:  return "rotating_defense";
        case PackTacticKind::EncircleThreat:   return "encircle_threat";
        case PackTacticKind::RetreatScreen:    return "retreat_screen";
        case PackTacticKind::TestOnly:         return "test_only";
    }
    return "unknown";
}

Result<PackTacticKind> packTacticKindFromString(const std::string& str) {
    if (str == "unknown") return Result<PackTacticKind>::ok(PackTacticKind::Unknown);
    if (str == "none") return Result<PackTacticKind>::ok(PackTacticKind::None);
    if (str == "pair_support") return Result<PackTacticKind>::ok(PackTacticKind::PairSupport);
    if (str == "guarded_gather") return Result<PackTacticKind>::ok(PackTacticKind::GuardedGather);
    if (str == "focus_pressure") return Result<PackTacticKind>::ok(PackTacticKind::FocusPressure);
    if (str == "rotating_defense") return Result<PackTacticKind>::ok(PackTacticKind::RotatingDefense);
    if (str == "encircle_threat") return Result<PackTacticKind>::ok(PackTacticKind::EncircleThreat);
    if (str == "retreat_screen") return Result<PackTacticKind>::ok(PackTacticKind::RetreatScreen);
    if (str == "test_only") return Result<PackTacticKind>::ok(PackTacticKind::TestOnly);
    return Result<PackTacticKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid PackTacticKind: " + str));
}

std::string toString(CoordinationQuality quality) {
    switch (quality) {
        case CoordinationQuality::Unknown: return "unknown";
        case CoordinationQuality::Failed:  return "failed";
        case CoordinationQuality::Weak:    return "weak";
        case CoordinationQuality::Stable:  return "stable";
        case CoordinationQuality::Strong:  return "strong";
        case CoordinationQuality::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CoordinationQuality> coordinationQualityFromString(const std::string& str) {
    if (str == "unknown") return Result<CoordinationQuality>::ok(CoordinationQuality::Unknown);
    if (str == "failed") return Result<CoordinationQuality>::ok(CoordinationQuality::Failed);
    if (str == "weak") return Result<CoordinationQuality>::ok(CoordinationQuality::Weak);
    if (str == "stable") return Result<CoordinationQuality>::ok(CoordinationQuality::Stable);
    if (str == "strong") return Result<CoordinationQuality>::ok(CoordinationQuality::Strong);
    if (str == "test_only") return Result<CoordinationQuality>::ok(CoordinationQuality::TestOnly);
    return Result<CoordinationQuality>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid CoordinationQuality: " + str));
}

CoordinationQuality qualityForScore(double score) {
    const double clamped = clampRatio(score);
    if (clamped < 0.20) return CoordinationQuality::Failed;
    if (clamped < 0.45) return CoordinationQuality::Weak;
    if (clamped < 0.70) return CoordinationQuality::Stable;
    return CoordinationQuality::Strong;
}

// Forbidden keys

bool containsCoordinationForbiddenKey(const std::string& key) {
    const std::string lower = lowerCopy(key);
    static const std::vector<std::string> forbidden = {
        "hidden_truth", "real_effect", "true_trait", "object_truth",
        "raw_state", "gamestate", "game_state", "savegame", "save_game",
        "agentruntime", "agent_runtime", "policy",
        "random_hit", "random_damage", "damage_roll", "critical_hit",
        "true_hp", "actual_damage", "kill_result", "loot_drop", "war_result",
        "damage", "death", "kill", "loot", "hp",
        "random_split", "probability_split"
    };
    for (const auto& token : forbidden) {
        if (lower.find(token) != std::string::npos) return true;
    }
    return false;
}

bool containsCoordinationForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) {
        return containsCoordinationForbiddenKey(key);
    });
}

// Validate structs

Result<void> ThreatPressureSummary::validateBasic() const {
    auto id_result = validateId(threat_id, "ThreatPressureSummary threat_id");
    if (id_result.is_error()) return id_result;
    if (threat_key.empty() || containsCoordinationForbiddenKey(threat_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ThreatPressureSummary threat_key invalid"));
    }
    auto tp_result = validateRatio(threat_pressure, "ThreatPressureSummary threat_pressure");
    if (tp_result.is_error()) return tp_result;
    auto pp_result = validateRatio(proximity_pressure, "ThreatPressureSummary proximity_pressure");
    if (pp_result.is_error()) return pp_result;
    auto target_result = validateRatio(target_pressure, "ThreatPressureSummary target_pressure");
    if (target_result.is_error()) return target_result;
    std::set<std::string> seen;
    for (const auto& id : known_counter_knowledge_ids) {
        if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "ThreatPressureSummary counter knowledge_id missing"));
        if (!pathfinder::foundation::isValidIdString(id.value()))
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ThreatPressureSummary counter knowledge_id invalid"));
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ThreatPressureSummary duplicate counter knowledge_id"));
        }
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ThreatPressureSummary reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> CombatParticipant::validateBasic() const {
    auto id_result = validateId(member_id, "CombatParticipant member_id");
    if (id_result.is_error()) return id_result;
    if (role == TribeMemberRole::Unknown || role == TribeMemberRole::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CombatParticipant role invalid"));
    }
    for (const auto& item : {std::pair<double, const char*>{trust, "CombatParticipant trust"},
                             {morale, "CombatParticipant morale"},
                             {fatigue_pressure, "CombatParticipant fatigue_pressure"},
                             {injury_pressure, "CombatParticipant injury_pressure"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    std::set<std::string> seen;
    for (const auto& id : known_knowledge_ids) {
        if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "CombatParticipant knowledge_id missing"));
        if (!pathfinder::foundation::isValidIdString(id.value()))
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "CombatParticipant knowledge_id invalid"));
        if (!seen.insert(id.value()).second)
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatParticipant duplicate knowledge_id"));
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatParticipant reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> GroupIntent::validateBasic() const {
    if (!isValidGroupIntent(kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "GroupIntent kind invalid"));
    }
    auto p_result = validateRatio(priority, "GroupIntent priority");
    if (p_result.is_error()) return p_result;
    if ((kind == GroupIntentKind::AssistMember || kind == GroupIntentKind::EscortVulnerable) && !target_member_id.has_value()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "GroupIntent requires target_member_id"));
    }
    if ((kind == GroupIntentKind::DefendGroup || kind == GroupIntentKind::FocusThreat || kind == GroupIntentKind::ScreenRetreat) && !target_threat_id.has_value()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "GroupIntent requires target_threat_id"));
    }
    std::set<std::string> seen;
    for (const auto& id : participant_ids) {
        auto r = validateId(id, "GroupIntent participant_id");
        if (r.is_error()) return r;
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "GroupIntent duplicate participant_id"));
        }
    }
    if (summary_key.empty() || containsCoordinationForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "GroupIntent summary_key invalid"));
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "GroupIntent reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> AssistAction::validateBasic() const {
    if (!isValidAssistAction(kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "AssistAction kind invalid"));
    }
    auto actor_result = validateId(actor_member_id, "AssistAction actor_member_id");
    if (actor_result.is_error()) return actor_result;
    auto target_result = validateId(target_member_id, "AssistAction target_member_id");
    if (target_result.is_error()) return target_result;
    for (const auto& item : {std::pair<double, const char*>{support_score, "AssistAction support_score"},
                             {risk_reduction, "AssistAction risk_reduction"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    auto td_result = validateDelta(trust_delta_hint, "AssistAction trust_delta_hint");
    if (td_result.is_error()) return td_result;
    auto md_result = validateDelta(morale_delta_hint, "AssistAction morale_delta_hint");
    if (md_result.is_error()) return md_result;
    if (summary_key.empty() || containsCoordinationForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "AssistAction summary_key invalid"));
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "AssistAction reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> DefendAction::validateBasic() const {
    if (!isValidDefendAction(kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "DefendAction kind invalid"));
    }
    auto actor_result = validateId(actor_member_id, "DefendAction actor_member_id");
    if (actor_result.is_error()) return actor_result;
    if (protected_member_id.has_value()) {
        auto r = validateId(protected_member_id.value(), "DefendAction protected_member_id");
        if (r.is_error()) return r;
    }
    if (protected_key.has_value()) {
        if (protected_key.value().empty() || containsCoordinationForbiddenKey(protected_key.value())) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "DefendAction protected_key invalid"));
        }
    }
    if (threat_id.has_value()) {
        auto r = validateId(threat_id.value(), "DefendAction threat_id");
        if (r.is_error()) return r;
    }
    for (const auto& item : {std::pair<double, const char*>{defense_score, "DefendAction defense_score"},
                             {risk_reduction, "DefendAction risk_reduction"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    if (summary_key.empty() || containsCoordinationForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "DefendAction summary_key invalid"));
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "DefendAction reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> PackTactic::validateBasic() const {
    if (!isValidPackTactic(kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "PackTactic kind invalid"));
    }
    if (!isValidQuality(quality)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "PackTactic quality invalid"));
    }
    std::set<std::string> seen;
    for (const auto& id : participant_ids) {
        auto r = validateId(id, "PackTactic participant_id");
        if (r.is_error()) return r;
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "PackTactic duplicate participant_id"));
        }
    }
    for (const auto& item : {std::pair<double, const char*>{coordination_score, "PackTactic coordination_score"},
                             {risk_reduction, "PackTactic risk_reduction"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    auto md_result = validateDelta(morale_delta_hint, "PackTactic morale_delta_hint");
    if (md_result.is_error()) return md_result;
    auto td_result = validateDelta(trust_delta_hint, "PackTactic trust_delta_hint");
    if (td_result.is_error()) return td_result;
    if (summary_key.empty() || containsCoordinationForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "PackTactic summary_key invalid"));
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "PackTactic reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> MemberCoordinationSummary::validateBasic() const {
    auto id_result = validateId(member_id, "MemberCoordinationSummary member_id");
    if (id_result.is_error()) return id_result;
    if (assist_count < 0 || defend_count < 0 || received_assist_count < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemberCoordinationSummary counts invalid"));
    }
    auto cs_result = validateRatio(coordination_score, "MemberCoordinationSummary coordination_score");
    if (cs_result.is_error()) return cs_result;
    auto md_result = validateDelta(morale_delta_hint, "MemberCoordinationSummary morale_delta_hint");
    if (md_result.is_error()) return md_result;
    auto td_result = validateDelta(trust_delta_hint, "MemberCoordinationSummary trust_delta_hint");
    if (td_result.is_error()) return td_result;
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemberCoordinationSummary reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> CombatCoordinationInput::validateBasic() const {
    auto id_result = validateId(tribe_id, "CombatCoordinationInput tribe_id");
    if (id_result.is_error()) return id_result;
    auto ts_result = tribe_state.validateBasic();
    if (ts_result.is_error()) return ts_result;
    if (tribe_state.tribe_id != tribe_id) {
        return Result<void>::fail(makeError(ErrorCode::id_type_mismatch, "CombatCoordinationInput tribe_id mismatch with tribe_state"));
    }
    std::set<std::string> seen_threat_ids;
    for (const auto& threat : threats) {
        auto r = threat.validateBasic();
        if (r.is_error()) return r;
        if (!seen_threat_ids.insert(threat.threat_id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationInput duplicate threat_id"));
        }
    }
    if (requested_intent.has_value() && !isValidGroupIntent(requested_intent.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CombatCoordinationInput requested_intent invalid"));
    }
    auto rp_result = validateRatio(resource_pressure, "CombatCoordinationInput resource_pressure");
    if (rp_result.is_error()) return rp_result;
    auto sp_result = validateRatio(safety_pressure, "CombatCoordinationInput safety_pressure");
    if (sp_result.is_error()) return sp_result;
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationInput reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> CombatCoordinationOptions::validateBasic() const {
    for (const auto& item : {std::pair<double, const char*>{morale_weight, "CombatCoordinationOptions morale_weight"},
                             {trust_weight, "CombatCoordinationOptions trust_weight"},
                             {role_weight, "CombatCoordinationOptions role_weight"},
                             {threat_weight, "CombatCoordinationOptions threat_weight"},
                             {split_risk_penalty_weight, "CombatCoordinationOptions split_risk_penalty_weight"}}) {
        auto r = validateRatio(item.first, item.second);
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> CombatCoordinationStateDraft::validateBasic() const {
    auto id_result = validateId(tribe_id, "CombatCoordinationStateDraft tribe_id");
    if (id_result.is_error()) return id_result;
    auto md_result = validateDelta(morale_delta, "CombatCoordinationStateDraft morale_delta");
    if (md_result.is_error()) return md_result;
    auto td_result = validateDelta(trust_delta, "CombatCoordinationStateDraft trust_delta");
    if (td_result.is_error()) return td_result;
    auto sp_result = validateRatio(safety_pressure, "CombatCoordinationStateDraft safety_pressure");
    if (sp_result.is_error()) return sp_result;
    auto cp_result = validateRatio(casualty_pressure, "CombatCoordinationStateDraft casualty_pressure");
    if (cp_result.is_error()) return cp_result;
    auto kp_result = validateRatio(knowledge_conflict_pressure, "CombatCoordinationStateDraft knowledge_conflict_pressure");
    if (kp_result.is_error()) return kp_result;
    std::set<std::string> seen;
    for (const auto& summary : member_summaries) {
        auto r = summary.validateBasic();
        if (r.is_error()) return r;
        if (!seen.insert(summary.member_id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationStateDraft duplicate member_id in member_summaries"));
        }
    }
    if (containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationStateDraft reason_keys forbidden"));
    }
    return Result<void>::ok();
}

Result<void> CombatCoordinationProjection::validateBasic() const {
    auto id_result = validateId(tribe_id, "CombatCoordinationProjection tribe_id");
    if (id_result.is_error()) return id_result;
    if (intent_summary.empty() || tactic_summary.empty() || participant_summary.empty() ||
        quality_summary.empty() || risk_summary.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationProjection summary missing"));
    }
    if (containsCoordinationForbiddenKey(intent_summary) || containsCoordinationForbiddenKey(tactic_summary) ||
        containsCoordinationForbiddenKey(participant_summary) || containsCoordinationForbiddenKey(quality_summary) ||
        containsCoordinationForbiddenKey(risk_summary) || containsCoordinationForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationProjection contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> CombatCoordinationTrace::validateBasic() const {
    if (trace_key.empty() || containsCoordinationForbiddenKey(trace_key) ||
        containsCoordinationForbiddenKey(steps) || containsCoordinationForbiddenKey(matched_rule_keys) ||
        containsCoordinationForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationTrace contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> CombatCoordinationResult::validateBasic() const {
    static const std::set<std::string> valid_decisions = {
        "coordinated", "weak_coordination", "avoid_engagement", "no_available_member", "invalid"
    };
    if (valid_decisions.find(decision) == valid_decisions.end()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationResult decision invalid"));
    }
    if (decision != "invalid" && decision != "no_available_member") {
        auto intent_result = intent.validateBasic();
        if (intent_result.is_error()) return intent_result;
    }
    for (const auto& action : assist_actions) {
        auto r = action.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& action : defend_actions) {
        auto r = action.validateBasic();
        if (r.is_error()) return r;
    }
    if (decision != "invalid" && decision != "no_available_member") {
        auto pt_result = pack_tactic.validateBasic();
        if (pt_result.is_error()) return pt_result;
    }
    std::set<std::string> seen;
    for (const auto& summary : member_summaries) {
        auto r = summary.validateBasic();
        if (r.is_error()) return r;
        if (!seen.insert(summary.member_id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationResult duplicate member_id in member_summaries"));
        }
    }
    auto sd_result = state_draft.validateBasic();
    if (sd_result.is_error()) return sd_result;
    auto pj_result = projection.validateBasic();
    if (pj_result.is_error()) return pj_result;
    auto tr_result = trace.validateBasic();
    if (tr_result.is_error()) return tr_result;
    if (containsCoordinationForbiddenKey(reason_keys) || containsCoordinationForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "CombatCoordinationResult contains forbidden key"));
    }
    return Result<void>::ok();
}

// Projection builder

Result<CombatCoordinationProjection> CombatCoordinationProjectionBuilder::build(
    const CombatCoordinationResult& result) const {
    CombatCoordinationProjection proj;
    proj.tribe_id = result.state_draft.tribe_id;
    proj.intent_summary = "intent=" + toString(result.intent.kind) + ";priority=" + compactDouble(result.intent.priority);
    proj.tactic_summary = "tactic=" + toString(result.pack_tactic.kind) +
                          ";quality=" + toString(result.pack_tactic.quality) +
                          ";score=" + compactDouble(result.pack_tactic.coordination_score);
    proj.participant_summary = "participants=" + joinIds(result.intent.participant_ids);
    proj.quality_summary = "quality=" + toString(result.pack_tactic.quality);
    proj.risk_summary = "risk_reduction=" + compactDouble(result.pack_tactic.risk_reduction);
    if (result.pack_tactic.coordination_score < 0.45) {
        proj.warning_keys.push_back("coordination_low");
    }
    if (result.pack_tactic.quality == CoordinationQuality::Failed) {
        proj.warning_keys.push_back("coordination_failed");
    }

    auto val_result = proj.validateBasic();
    if (val_result.is_error()) return Result<CombatCoordinationProjection>::fail(val_result.errors());
    return Result<CombatCoordinationProjection>::ok(std::move(proj));
}

// Resolver

Result<CombatCoordinationResult> CombatCoordinationResolver::resolve(
    const CombatCoordinationInput& input,
    const CombatCoordinationOptions& options) const {
    auto opt_result = options.validateBasic();
    if (opt_result.is_error()) return Result<CombatCoordinationResult>::fail(opt_result.errors());
    auto in_result = input.validateBasic();
    if (in_result.is_error()) return Result<CombatCoordinationResult>::fail(in_result.errors());

    CombatCoordinationResult result;
    result.reason_keys = input.reason_keys;
    result.trace.trace_key = "combat_coordination_resolve";
    result.trace.input_tick = input.input_tick;
    result.trace.reason_keys = input.reason_keys;
    result.trace.matched_rule_keys.push_back("p24_deterministic_coordination");

    // Build participants from TribeState
    std::vector<CombatParticipant> participants;
    for (const auto& member : input.tribe_state.members) {
        if (!member.active) continue;
        CombatParticipant cp;
        cp.member_id = member.member_id;
        cp.role = member.role;
        cp.trust = member.trust;
        cp.morale = member.morale;
        cp.known_knowledge_ids = member.known_knowledge_ids;
        cp.available = (cp.role != TribeMemberRole::Child) && (cp.morale > 0.0) && (cp.trust > 0.0);
        cp.reason_keys = {"extracted_from_tribe_state"};
        participants.push_back(std::move(cp));
    }

    // Determine available participants
    std::vector<CombatParticipant*> available;
    for (auto& p : participants) {
        if (p.available) available.push_back(&p);
    }

    if (available.empty()) {
        result.decision = "no_available_member";
        result.trace.steps.push_back("no_available_member");
        result.intent.kind = GroupIntentKind::HoldTogether;
        result.intent.summary_key = "no_available";
        result.pack_tactic.kind = PackTacticKind::None;
        result.pack_tactic.quality = CoordinationQuality::Failed;
        result.state_draft.tribe_id = input.tribe_id;
        result.state_draft.input_tick = input.input_tick;
        result.state_draft.reason_keys = input.reason_keys;
        CombatCoordinationProjectionBuilder builder;
        auto proj_result = builder.build(result);
        if (proj_result.is_error()) return Result<CombatCoordinationResult>::fail(proj_result.errors());
        result.projection = proj_result.value();
        auto final_result = result.validateBasic();
        if (final_result.is_error()) return Result<CombatCoordinationResult>::fail(final_result.errors());
        return Result<CombatCoordinationResult>::ok(std::move(result));
    }

    // Calculate member scores
    std::map<std::string, double> member_scores;
    double total_score = 0.0;
    for (auto* p : available) {
        double score = clampRatio(
            roleScore(p->role) * options.role_weight +
            p->morale * options.morale_weight +
            p->trust * options.trust_weight +
            (1.0 - p->fatigue_pressure) * 0.10 +
            (1.0 - p->injury_pressure) * 0.10
        );
        member_scores[p->member_id.value()] = score;
        total_score += score;
    }

    double avg_member_score = available.empty() ? 0.0 : total_score / static_cast<double>(available.size());

    // Role coverage bonuses
    double role_bonus = 0.0;
    bool has_pioneer = false, has_elder = false, has_guardian = false, has_explorer = false, has_teacher = false;
    bool has_child = false, has_learner = false;
    for (const auto& p : participants) {
        switch (p.role) {
            case TribeMemberRole::Pioneer: has_pioneer = true; break;
            case TribeMemberRole::Elder: has_elder = true; break;
            case TribeMemberRole::Guardian: has_guardian = true; break;
            case TribeMemberRole::Explorer: has_explorer = true; break;
            case TribeMemberRole::Teacher: has_teacher = true; break;
            case TribeMemberRole::Child: has_child = true; break;
            case TribeMemberRole::Learner: has_learner = true; break;
            default: break;
        }
    }
    if (has_pioneer || has_elder) role_bonus += 0.05;
    if (has_guardian) role_bonus += 0.10;
    if (has_explorer) role_bonus += 0.05;
    if (has_teacher) role_bonus += 0.05;
    if (participants.size() >= 3) role_bonus += 0.05;

    double threat_pressure = meanThreatPressure(input.threats);

    double coordination_score = clampRatio(
        avg_member_score +
        role_bonus -
        input.tribe_state.split_risk.risk * options.split_risk_penalty_weight -
        input.safety_pressure * 0.10 -
        threat_pressure * options.threat_weight
    );
    CoordinationQuality quality = qualityForScore(coordination_score);

    // Intent selection
    GroupIntentKind intent_kind = GroupIntentKind::HoldTogether;
    if (input.requested_intent.has_value()) {
        intent_kind = input.requested_intent.value();
    }

    // Intent downgrade logic
    if (intent_kind == GroupIntentKind::FocusThreat && (!has_guardian || coordination_score < 0.45)) {
        intent_kind = GroupIntentKind::HoldTogether;
        result.trace.steps.push_back("intent_downgraded:FocusThreat->HoldTogether(guardian_or_coordination_insufficient)");
    }
    if (intent_kind == GroupIntentKind::DefendGroup && !has_guardian) {
        intent_kind = GroupIntentKind::HoldTogether;
        result.trace.steps.push_back("intent_downgraded:DefendGroup->HoldTogether(no_guardian)");
    }
    if (intent_kind == GroupIntentKind::EscortVulnerable && !(has_child || has_learner)) {
        intent_kind = GroupIntentKind::HoldTogether;
        result.trace.steps.push_back("intent_downgraded:EscortVulnerable->HoldTogether(no_vulnerable)");
    }

    // Deterministic intent selection when no requested intent
    if (!input.requested_intent.has_value()) {
        if (threat_pressure >= 0.6 && coordination_score < 0.25) {
            intent_kind = GroupIntentKind::AvoidEngagement;
        } else if (threat_pressure >= 0.6 && coordination_score < 0.45) {
            intent_kind = GroupIntentKind::ScreenRetreat;
        } else if ((has_child || has_learner) && threat_pressure >= 0.5) {
            intent_kind = GroupIntentKind::EscortVulnerable;
        } else if (has_guardian && threat_pressure >= 0.4) {
            intent_kind = GroupIntentKind::DefendGroup;
        } else if (available.size() >= 2 && coordination_score >= 0.5) {
            intent_kind = GroupIntentKind::FocusThreat;
        } else {
            intent_kind = GroupIntentKind::HoldTogether;
        }
    }

    EntityId first_threat_id;
    if (!input.threats.empty()) first_threat_id = input.threats[0].threat_id;

    result.intent.kind = intent_kind;
    result.intent.priority = clampRatio(coordination_score);
    result.intent.summary_key = "group_intent_" + toString(intent_kind);
    result.intent.reason_keys = input.reason_keys;
    if (!input.threats.empty() && (intent_kind == GroupIntentKind::DefendGroup ||
                                    intent_kind == GroupIntentKind::FocusThreat ||
                                    intent_kind == GroupIntentKind::ScreenRetreat)) {
        result.intent.target_threat_id = first_threat_id;
    }
    if (intent_kind == GroupIntentKind::AssistMember || intent_kind == GroupIntentKind::EscortVulnerable) {
        // Find a vulnerable member to target
        for (auto& member : input.tribe_state.members) {
            if (!member.active) continue;
            if (member.role == TribeMemberRole::Child || member.role == TribeMemberRole::Learner ||
                (intent_kind == GroupIntentKind::EscortVulnerable && member.role == TribeMemberRole::Gatherer)) {
                result.intent.target_member_id = member.member_id;
                break;
            }
        }
        if (!result.intent.target_member_id.has_value() && !available.empty()) {
            result.intent.target_member_id = available[0]->member_id;
        }
    }

    // Generate assist and defend actions
    if (intent_kind != GroupIntentKind::AvoidEngagement && intent_kind != GroupIntentKind::HoldTogether) {
        for (auto* actor : available) {
            result.intent.participant_ids.push_back(actor->member_id);
        }

        if (intent_kind == GroupIntentKind::DefendGroup || intent_kind == GroupIntentKind::EscortVulnerable) {
            // Find guardians for defense
            for (auto* p : available) {
                if (p->role == TribeMemberRole::Guardian) {
                    DefendAction da;
                    da.kind = DefendActionKind::FormPerimeter;
                    da.actor_member_id = p->member_id;
                    da.defense_score = clampRatio(member_scores[p->member_id.value()]);
                    da.risk_reduction = clampRatio(da.defense_score * 0.3);
                    da.summary_key = "guardian_forms_perimeter";
                    da.reason_keys = input.reason_keys;
                    if (!input.threats.empty()) da.threat_id = first_threat_id;
                    result.defend_actions.push_back(std::move(da));
                    result.trace.steps.push_back("defend:form_perimeter:" + p->member_id.value());

                    // Guardian protects vulnerable
                    for (auto& member : input.tribe_state.members) {
                        if (!member.active) continue;
                        if (member.role == TribeMemberRole::Child || member.role == TribeMemberRole::Learner) {
                            DefendAction gd;
                            gd.kind = DefendActionKind::GuardMember;
                            gd.actor_member_id = p->member_id;
                            gd.protected_member_id = member.member_id;
                            gd.defense_score = clampRatio(member_scores[p->member_id.value()] * 0.8);
                            gd.risk_reduction = clampRatio(gd.defense_score * 0.25);
                            gd.summary_key = "guardian_guards_member";
                            gd.reason_keys = input.reason_keys;
                            result.defend_actions.push_back(std::move(gd));
                            result.trace.steps.push_back("defend:guard_member:" + member.member_id.value());
                        }
                    }
                }
                // Explorers warn
                if (p->role == TribeMemberRole::Explorer) {
                    AssistAction aa;
                    aa.kind = AssistActionKind::Warn;
                    aa.actor_member_id = p->member_id;
                    aa.support_score = clampRatio(member_scores[p->member_id.value()]);
                    aa.risk_reduction = clampRatio(aa.support_score * 0.15);
                    aa.trust_delta_hint = 0.02;
                    aa.morale_delta_hint = 0.01;
                    aa.summary_key = "explorer_warns";
                    aa.reason_keys = input.reason_keys;
                    // Target first vulnerable or the pioneer
                    for (auto& member : input.tribe_state.members) {
                        if (!member.active) continue;
                        if (member.role == TribeMemberRole::Child || member.role == TribeMemberRole::Learner ||
                            member.role == TribeMemberRole::Pioneer) {
                            aa.target_member_id = member.member_id;
                            break;
                        }
                    }
                    if (aa.target_member_id.empty() && !available.empty()) {
                        aa.target_member_id = available[0]->member_id;
                    }
                    result.assist_actions.push_back(std::move(aa));
                    result.trace.steps.push_back("assist:warn:" + p->member_id.value());
                }
            }
        }

        if (intent_kind == GroupIntentKind::FocusThreat) {
            for (auto* p : available) {
                if (p->role == TribeMemberRole::Guardian || p->role == TribeMemberRole::Pioneer) {
                    DefendAction da;
                    da.kind = DefendActionKind::BlockThreat;
                    da.actor_member_id = p->member_id;
                    da.defense_score = clampRatio(member_scores[p->member_id.value()]);
                    da.risk_reduction = clampRatio(da.defense_score * 0.35);
                    da.summary_key = "focus_threat_block";
                    da.reason_keys = input.reason_keys;
                    if (!input.threats.empty()) da.threat_id = first_threat_id;
                    result.defend_actions.push_back(std::move(da));
                    result.trace.steps.push_back("defend:block_threat:" + p->member_id.value());
                }
            }
            // Pair support
            if (available.size() >= 2) {
                for (size_t i = 0; i + 1 < available.size(); i += 2) {
                    AssistAction aa;
                    aa.kind = AssistActionKind::CoverApproach;
                    aa.actor_member_id = available[i]->member_id;
                    aa.target_member_id = available[i+1]->member_id;
                    aa.support_score = clampRatio(member_scores[available[i]->member_id.value()] * 0.5);
                    aa.risk_reduction = clampRatio(aa.support_score * 0.2);
                    aa.trust_delta_hint = 0.03;
                    aa.morale_delta_hint = 0.02;
                    aa.summary_key = "pair_cover_approach";
                    aa.reason_keys = input.reason_keys;
                    result.assist_actions.push_back(std::move(aa));
                    result.trace.steps.push_back("assist:cover_approach:" + available[i]->member_id.value());
                }
            }
        }

        if (intent_kind == GroupIntentKind::ScreenRetreat) {
            for (auto* p : available) {
                if (p->role == TribeMemberRole::Guardian || p->role == TribeMemberRole::Explorer) {
                    DefendAction da;
                    da.kind = DefendActionKind::ScreenEscape;
                    da.actor_member_id = p->member_id;
                    da.defense_score = clampRatio(member_scores[p->member_id.value()]);
                    da.risk_reduction = clampRatio(da.defense_score * 0.4);
                    da.summary_key = "screen_escape";
                    da.reason_keys = input.reason_keys;
                    if (!input.threats.empty()) da.threat_id = first_threat_id;
                    result.defend_actions.push_back(std::move(da));
                    result.trace.steps.push_back("defend:screen_escape:" + p->member_id.value());
                }
            }
        }

        if (intent_kind == GroupIntentKind::AssistMember) {
            for (auto* p : available) {
                AssistAction aa;
                aa.kind = AssistActionKind::GuidePosition;
                aa.actor_member_id = p->member_id;
                aa.support_score = clampRatio(member_scores[p->member_id.value()]);
                aa.risk_reduction = clampRatio(aa.support_score * 0.2);
                aa.trust_delta_hint = 0.03;
                aa.morale_delta_hint = 0.02;
                aa.summary_key = "guide_position";
                aa.reason_keys = input.reason_keys;
                if (available.size() > 1 && available[0]->member_id == p->member_id) {
                    aa.target_member_id = available[1]->member_id;
                } else {
                    aa.target_member_id = available[0]->member_id;
                }
                result.assist_actions.push_back(std::move(aa));
            }
            result.trace.steps.push_back("assist:guide_position");
        }
    }

    // Set participants on intent
    if (result.intent.participant_ids.empty()) {
        for (auto* p : available) {
            result.intent.participant_ids.push_back(p->member_id);
        }
    }

    // Pack tactic
    if (intent_kind == GroupIntentKind::AvoidEngagement) {
        result.pack_tactic.kind = PackTacticKind::None;
    } else if (intent_kind == GroupIntentKind::FocusThreat) {
        result.pack_tactic.kind = PackTacticKind::FocusPressure;
    } else if (intent_kind == GroupIntentKind::DefendGroup) {
        result.pack_tactic.kind = available.size() >= 3 ? PackTacticKind::RotatingDefense : PackTacticKind::PairSupport;
    } else if (intent_kind == GroupIntentKind::ScreenRetreat) {
        result.pack_tactic.kind = PackTacticKind::RetreatScreen;
    } else if (intent_kind == GroupIntentKind::EscortVulnerable) {
        result.pack_tactic.kind = PackTacticKind::GuardedGather;
    } else {
        result.pack_tactic.kind = PackTacticKind::PairSupport;
    }

    result.pack_tactic.quality = quality;
    result.pack_tactic.coordination_score = coordination_score;
    result.pack_tactic.risk_reduction = clampRatio(coordination_score * (1.0 - threat_pressure) * 0.5);
    result.pack_tactic.morale_delta_hint = clampRatio((quality == CoordinationQuality::Strong ? 0.05 : quality == CoordinationQuality::Stable ? 0.02 : quality == CoordinationQuality::Weak ? -0.02 : -0.05));
    if (quality == CoordinationQuality::Strong) result.pack_tactic.morale_delta_hint = 0.05;
    else if (quality == CoordinationQuality::Stable) result.pack_tactic.morale_delta_hint = 0.02;
    else if (quality == CoordinationQuality::Weak) result.pack_tactic.morale_delta_hint = -0.02;
    else result.pack_tactic.morale_delta_hint = -0.05;

    result.pack_tactic.trust_delta_hint = result.pack_tactic.morale_delta_hint;
    if (intent_kind == GroupIntentKind::AvoidEngagement) {
        result.pack_tactic.trust_delta_hint = 0.01;
    }
    result.pack_tactic.summary_key = "pack_tactic_" + toString(result.pack_tactic.kind);
    result.pack_tactic.reason_keys = input.reason_keys;
    if (!input.threats.empty()) result.pack_tactic.focus_threat_id = first_threat_id;
    result.pack_tactic.participant_ids = result.intent.participant_ids;

    // Member summaries
    for (auto* p : available) {
        MemberCoordinationSummary ms;
        ms.member_id = p->member_id;
        ms.participated = true;
        ms.coordination_score = member_scores[p->member_id.value()];
        ms.reason_keys = input.reason_keys;
        for (const auto& aa : result.assist_actions) {
            if (aa.actor_member_id == p->member_id) ms.assist_count++;
            if (aa.target_member_id == p->member_id) ms.received_assist_count++;
        }
        for (const auto& da : result.defend_actions) {
            if (da.actor_member_id == p->member_id) ms.defend_count++;
        }
        ms.morale_delta_hint = result.pack_tactic.morale_delta_hint * 0.5;
        ms.trust_delta_hint = result.pack_tactic.trust_delta_hint * 0.5;
        result.member_summaries.push_back(std::move(ms));
    }

    // State draft
    result.state_draft.tribe_id = input.tribe_id;
    result.state_draft.input_tick = input.input_tick;
    result.state_draft.morale_delta = result.pack_tactic.morale_delta_hint;
    result.state_draft.trust_delta = result.pack_tactic.trust_delta_hint;
    result.state_draft.safety_pressure = clampRatio(input.safety_pressure * (1.0 - result.pack_tactic.risk_reduction));
    result.state_draft.casualty_pressure = clampRatio(input.safety_pressure * 0.1);
    result.state_draft.knowledge_conflict_pressure = 0.0;
    result.state_draft.member_summaries = result.member_summaries;
    result.state_draft.reason_keys = input.reason_keys;

    // Decision
    if (intent_kind == GroupIntentKind::AvoidEngagement) {
        result.decision = "avoid_engagement";
    } else if (quality == CoordinationQuality::Failed || quality == CoordinationQuality::Weak) {
        result.decision = "weak_coordination";
    } else {
        result.decision = "coordinated";
    }

    // Build projection
    CombatCoordinationProjectionBuilder builder;
    auto proj_result = builder.build(result);
    if (proj_result.is_error()) return Result<CombatCoordinationResult>::fail(proj_result.errors());
    result.projection = proj_result.value();
    result.warning_keys = result.projection.warning_keys;

    result.trace.steps.push_back("coordination_score=" + compactDouble(coordination_score));
    result.trace.steps.push_back("quality=" + toString(quality));
    result.trace.steps.push_back("decision=" + result.decision);

    auto final_result = result.validateBasic();
    if (final_result.is_error()) return Result<CombatCoordinationResult>::fail(final_result.errors());
    return Result<CombatCoordinationResult>::ok(std::move(result));
}

} // namespace pathfinder::tribe
