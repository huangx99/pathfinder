#include "pathfinder/tribe/tribe_types.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <set>

namespace pathfinder::tribe {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool isValidRole(TribeMemberRole role) {
    return role != TribeMemberRole::Unknown && role != TribeMemberRole::TestOnly;
}

static bool isValidCohesion(TribeCohesionState state) {
    return state != TribeCohesionState::Unknown && state != TribeCohesionState::TestOnly;
}

static Result<void> validateKnowledgeIdsUnique(const std::vector<KnowledgeId>& ids, const std::string& owner) {
    std::set<std::string> seen;
    for (const auto& id : ids) {
        if (id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::id_missing, owner + " knowledge_id missing"));
        }
        if (!isValidIdString(id.value())) {
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, owner + " knowledge_id invalid"));
        }
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " knowledge_id duplicated"));
        }
    }
    return Result<void>::ok();
}

static Result<void> validateReasonKeys(const std::vector<std::string>& keys, const std::string& owner) {
    if (containsTribeForbiddenKey(keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

static Result<void> validateRatio(double value, const std::string& field) {
    if (!isValidRatio(value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be 0..1"));
    }
    return Result<void>::ok();
}

std::string toString(TribeMemberRole role) {
    switch (role) {
        case TribeMemberRole::Unknown: return "unknown";
        case TribeMemberRole::Pioneer: return "pioneer";
        case TribeMemberRole::Explorer: return "explorer";
        case TribeMemberRole::Gatherer: return "gatherer";
        case TribeMemberRole::Guardian: return "guardian";
        case TribeMemberRole::Teacher: return "teacher";
        case TribeMemberRole::Learner: return "learner";
        case TribeMemberRole::Healer: return "healer";
        case TribeMemberRole::Crafter: return "crafter";
        case TribeMemberRole::Elder: return "elder";
        case TribeMemberRole::Child: return "child";
        case TribeMemberRole::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<TribeMemberRole> tribeMemberRoleFromString(const std::string& str) {
    if (str == "unknown") return Result<TribeMemberRole>::ok(TribeMemberRole::Unknown);
    if (str == "pioneer") return Result<TribeMemberRole>::ok(TribeMemberRole::Pioneer);
    if (str == "explorer") return Result<TribeMemberRole>::ok(TribeMemberRole::Explorer);
    if (str == "gatherer") return Result<TribeMemberRole>::ok(TribeMemberRole::Gatherer);
    if (str == "guardian") return Result<TribeMemberRole>::ok(TribeMemberRole::Guardian);
    if (str == "teacher") return Result<TribeMemberRole>::ok(TribeMemberRole::Teacher);
    if (str == "learner") return Result<TribeMemberRole>::ok(TribeMemberRole::Learner);
    if (str == "healer") return Result<TribeMemberRole>::ok(TribeMemberRole::Healer);
    if (str == "crafter") return Result<TribeMemberRole>::ok(TribeMemberRole::Crafter);
    if (str == "elder") return Result<TribeMemberRole>::ok(TribeMemberRole::Elder);
    if (str == "child") return Result<TribeMemberRole>::ok(TribeMemberRole::Child);
    if (str == "test_only") return Result<TribeMemberRole>::ok(TribeMemberRole::TestOnly);
    return Result<TribeMemberRole>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid TribeMemberRole: " + str));
}

std::string toString(TribeCohesionState state) {
    switch (state) {
        case TribeCohesionState::Unknown: return "unknown";
        case TribeCohesionState::Stable: return "stable";
        case TribeCohesionState::Watchful: return "watchful";
        case TribeCohesionState::Tense: return "tense";
        case TribeCohesionState::Fracturing: return "fracturing";
        case TribeCohesionState::Splintered: return "splintered";
        case TribeCohesionState::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<TribeCohesionState> tribeCohesionStateFromString(const std::string& str) {
    if (str == "unknown") return Result<TribeCohesionState>::ok(TribeCohesionState::Unknown);
    if (str == "stable") return Result<TribeCohesionState>::ok(TribeCohesionState::Stable);
    if (str == "watchful") return Result<TribeCohesionState>::ok(TribeCohesionState::Watchful);
    if (str == "tense") return Result<TribeCohesionState>::ok(TribeCohesionState::Tense);
    if (str == "fracturing") return Result<TribeCohesionState>::ok(TribeCohesionState::Fracturing);
    if (str == "splintered") return Result<TribeCohesionState>::ok(TribeCohesionState::Splintered);
    if (str == "test_only") return Result<TribeCohesionState>::ok(TribeCohesionState::TestOnly);
    return Result<TribeCohesionState>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid TribeCohesionState: " + str));
}

std::string toString(TribeStateChangeKind kind) {
    switch (kind) {
        case TribeStateChangeKind::Unknown: return "unknown";
        case TribeStateChangeKind::MemberAdded: return "member_added";
        case TribeStateChangeKind::MemberRemoved: return "member_removed";
        case TribeStateChangeKind::MemberRoleChanged: return "member_role_changed";
        case TribeStateChangeKind::MoraleChanged: return "morale_changed";
        case TribeStateChangeKind::TrustChanged: return "trust_changed";
        case TribeStateChangeKind::SplitRiskChanged: return "split_risk_changed";
        case TribeStateChangeKind::CohesionChanged: return "cohesion_changed";
        case TribeStateChangeKind::KnowledgeLinked: return "knowledge_linked";
        case TribeStateChangeKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<TribeStateChangeKind> tribeStateChangeKindFromString(const std::string& str) {
    if (str == "unknown") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::Unknown);
    if (str == "member_added") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::MemberAdded);
    if (str == "member_removed") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::MemberRemoved);
    if (str == "member_role_changed") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::MemberRoleChanged);
    if (str == "morale_changed") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::MoraleChanged);
    if (str == "trust_changed") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::TrustChanged);
    if (str == "split_risk_changed") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::SplitRiskChanged);
    if (str == "cohesion_changed") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::CohesionChanged);
    if (str == "knowledge_linked") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::KnowledgeLinked);
    if (str == "test_only") return Result<TribeStateChangeKind>::ok(TribeStateChangeKind::TestOnly);
    return Result<TribeStateChangeKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid TribeStateChangeKind: " + str));
}

bool containsTribeForbiddenKey(const std::string& key) {
    const std::string lower = lowerCopy(key);
    static const std::vector<std::string> forbidden = {
        "hidden_truth",
        "real_effect",
        "true_trait",
        "object_truth",
        "raw_state",
        "gamestate",
        "game_state",
        "savegame",
        "save_game",
        "agentruntime",
        "agent_runtime",
        "policy",
        "random_split",
        "probability_split"
    };
    for (const auto& token : forbidden) {
        if (lower.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsTribeForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) {
        return containsTribeForbiddenKey(key);
    });
}

bool isValidRatio(double value) {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

double clampRatio(double value) {
    if (!std::isfinite(value)) return 0.0;
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

TribeCohesionState cohesionStateForRisk(double risk) {
    const double clamped = clampRatio(risk);
    if (clamped < 0.25) return TribeCohesionState::Stable;
    if (clamped < 0.45) return TribeCohesionState::Watchful;
    if (clamped < 0.65) return TribeCohesionState::Tense;
    if (clamped < 0.85) return TribeCohesionState::Fracturing;
    return TribeCohesionState::Splintered;
}

Result<void> TribeMember::validateBasic() const {
    if (member_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "TribeMember member_id missing"));
    if (!isValidIdString(member_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "TribeMember member_id invalid"));
    if (!isValidRole(role)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeMember role invalid"));
    auto trust_result = validateRatio(trust, "TribeMember trust");
    if (trust_result.is_error()) return trust_result;
    auto morale_result = validateRatio(morale, "TribeMember morale");
    if (morale_result.is_error()) return morale_result;
    auto knowledge_result = validateKnowledgeIdsUnique(known_knowledge_ids, "TribeMember");
    if (knowledge_result.is_error()) return knowledge_result;
    return validateReasonKeys(reason_keys, "TribeMember");
}

Result<void> TribeMorale::validateBasic() const {
    for (const auto& item : {std::pair<double, const char*>{overall, "TribeMorale overall"},
                            {food_pressure, "TribeMorale food_pressure"},
                            {safety_pressure, "TribeMorale safety_pressure"},
                            {recent_success, "TribeMorale recent_success"},
                            {recent_loss, "TribeMorale recent_loss"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    return validateReasonKeys(reason_keys, "TribeMorale");
}

Result<void> TribeTrust::validateBasic() const {
    for (const auto& item : {std::pair<double, const char*>{leader_trust, "TribeTrust leader_trust"},
                            {member_trust, "TribeTrust member_trust"},
                            {teaching_fairness, "TribeTrust teaching_fairness"},
                            {knowledge_conflict_pressure, "TribeTrust knowledge_conflict_pressure"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    return validateReasonKeys(reason_keys, "TribeTrust");
}

Result<void> TribeSplitRisk::validateBasic() const {
    for (const auto& item : {std::pair<double, const char*>{risk, "TribeSplitRisk risk"},
                            {resource_pressure, "TribeSplitRisk resource_pressure"},
                            {trust_pressure, "TribeSplitRisk trust_pressure"},
                            {knowledge_conflict_pressure, "TribeSplitRisk knowledge_conflict_pressure"},
                            {casualty_pressure, "TribeSplitRisk casualty_pressure"}}) {
        auto result = validateRatio(item.first, item.second);
        if (result.is_error()) return result;
    }
    if (!isValidCohesion(cohesion_state)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeSplitRisk cohesion_state invalid"));
    }
    if (cohesion_state != cohesionStateForRisk(risk)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeSplitRisk cohesion_state does not match deterministic risk"));
    }
    return validateReasonKeys(reason_keys, "TribeSplitRisk");
}

} // namespace pathfinder::tribe
