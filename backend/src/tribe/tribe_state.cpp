#include "pathfinder/tribe/tribe_state.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

namespace pathfinder::tribe {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static constexpr double kEpsilon = 0.000001;

static bool isValidChangeKind(TribeStateChangeKind kind) {
    return kind != TribeStateChangeKind::Unknown && kind != TribeStateChangeKind::TestOnly;
}

static bool isMemberEventKind(TribeStateChangeKind kind) {
    return kind == TribeStateChangeKind::MemberAdded ||
           kind == TribeStateChangeKind::MemberRemoved ||
           kind == TribeStateChangeKind::MemberRoleChanged ||
           kind == TribeStateChangeKind::TrustChanged ||
           kind == TribeStateChangeKind::MoraleChanged;
}

static bool isValidRoleForInput(TribeMemberRole role) {
    return role != TribeMemberRole::Unknown && role != TribeMemberRole::TestOnly;
}

static Result<void> validateRatioField(double value, const std::string& field) {
    if (!isValidRatio(value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be 0..1"));
    }
    return Result<void>::ok();
}

static Result<void> validateDeltaField(double value, const std::string& field) {
    if (!std::isfinite(value) || value < -1.0 || value > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be -1..1"));
    }
    return Result<void>::ok();
}

static Result<void> validateId(const TribeId& id, const std::string& field) {
    if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!isValidIdString(id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateId(const EntityId& id, const std::string& field) {
    if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!isValidIdString(id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateId(const KnowledgeId& id, const std::string& field) {
    if (id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, field + " missing"));
    if (!isValidIdString(id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, field + " invalid"));
    return Result<void>::ok();
}

static Result<void> validateReasonKeys(const std::vector<std::string>& keys, const std::string& owner) {
    if (containsTribeForbiddenKey(keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " contains forbidden key"));
    }
    return Result<void>::ok();
}

static Result<void> validateKnowledgeIds(const std::vector<KnowledgeId>& ids, const std::string& owner) {
    std::set<std::string> seen;
    for (const auto& id : ids) {
        auto result = validateId(id, owner + " knowledge_id");
        if (result.is_error()) return result;
        if (!seen.insert(id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, owner + " duplicate knowledge_id"));
        }
    }
    return Result<void>::ok();
}

static void appendUniqueReasons(std::vector<std::string>& target, const std::vector<std::string>& source) {
    for (const auto& key : source) {
        if (std::find(target.begin(), target.end(), key) == target.end()) {
            target.push_back(key);
        }
    }
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

static TribeStateChangeDraft makeValueChange(TribeStateChangeKind kind,
                                             double old_value,
                                             double new_value,
                                             std::string summary_key,
                                             std::vector<std::string> reason_keys) {
    TribeStateChangeDraft draft;
    draft.kind = kind;
    draft.old_value = old_value;
    draft.new_value = new_value;
    draft.summary_key = std::move(summary_key);
    draft.reason_keys = std::move(reason_keys);
    return draft;
}

static void recalculatePopulation(TribeState& state) {
    state.population_total = static_cast<int>(state.members.size());
    state.active_population = static_cast<int>(std::count_if(state.members.begin(), state.members.end(), [](const TribeMember& member) {
        return member.active;
    }));
}

static TribeSplitRisk calculateSplitRisk(const TribeState& state, const TribeStateInput& input) {
    TribeSplitRisk risk;
    risk.resource_pressure = clampRatio(input.resource_pressure);
    risk.trust_pressure = clampRatio(1.0 - ((state.trust.leader_trust + state.trust.member_trust + state.trust.teaching_fairness) / 3.0));
    risk.knowledge_conflict_pressure = clampRatio(input.knowledge_conflict_pressure);
    risk.casualty_pressure = clampRatio(input.casualty_pressure);
    risk.risk = clampRatio(risk.resource_pressure * 0.30 +
                           risk.trust_pressure * 0.30 +
                           risk.knowledge_conflict_pressure * 0.25 +
                           risk.casualty_pressure * 0.15);
    risk.cohesion_state = cohesionStateForRisk(risk.risk);
    risk.reason_keys = input.reason_keys;
    risk.reason_keys.push_back("deterministic_split_risk");
    return risk;
}

Result<void> TribeState::validateBasic() const {
    auto id_result = validateId(tribe_id, "TribeState tribe_id");
    if (id_result.is_error()) return id_result;
    if (display_key.empty() || containsTribeForbiddenKey(display_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeState display_key invalid"));
    }
    if (population_total != static_cast<int>(members.size())) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeState population_total mismatch"));
    }
    int active_count = 0;
    std::set<std::string> member_ids;
    for (const auto& member : members) {
        auto result = member.validateBasic();
        if (result.is_error()) return result;
        if (!member_ids.insert(member.member_id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeState duplicate member_id"));
        }
        if (member.active) ++active_count;
    }
    if (active_population != active_count) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeState active_population mismatch"));
    }
    if (active_population > population_total) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeState active_population exceeds population_total"));
    }
    auto morale_result = morale.validateBasic();
    if (morale_result.is_error()) return morale_result;
    auto trust_result = trust.validateBasic();
    if (trust_result.is_error()) return trust_result;
    auto risk_result = split_risk.validateBasic();
    if (risk_result.is_error()) return risk_result;
    auto knowledge_result = validateKnowledgeIds(shared_knowledge_ids, "TribeState shared_knowledge_ids");
    if (knowledge_result.is_error()) return knowledge_result;
    return validateReasonKeys(reason_keys, "TribeState reason_keys");
}

Result<void> TribeMemberEvent::validateBasic() const {
    if (!isMemberEventKind(kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeMemberEvent kind invalid"));
    }
    auto id_result = validateId(member_id, "TribeMemberEvent member_id");
    if (id_result.is_error()) return id_result;
    if ((kind == TribeStateChangeKind::MemberAdded || kind == TribeStateChangeKind::MemberRoleChanged) && !role.has_value()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeMemberEvent role missing"));
    }
    if (kind == TribeStateChangeKind::TrustChanged && !trust.has_value()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeMemberEvent trust missing for TrustChanged"));
    }
    if (kind == TribeStateChangeKind::MoraleChanged && !morale.has_value()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeMemberEvent morale missing for MoraleChanged"));
    }
    if (role.has_value() && !isValidRoleForInput(role.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeMemberEvent role invalid"));
    }
    if (trust.has_value()) {
        auto result = validateRatioField(trust.value(), "TribeMemberEvent trust");
        if (result.is_error()) return result;
    }
    if (morale.has_value()) {
        auto result = validateRatioField(morale.value(), "TribeMemberEvent morale");
        if (result.is_error()) return result;
    }
    auto knowledge_result = validateKnowledgeIds(known_knowledge_ids, "TribeMemberEvent known_knowledge_ids");
    if (knowledge_result.is_error()) return knowledge_result;
    return validateReasonKeys(reason_keys, "TribeMemberEvent reason_keys");
}

Result<void> TribeKnowledgeEvent::validateBasic() const {
    auto id_result = validateId(knowledge_id, "TribeKnowledgeEvent knowledge_id");
    if (id_result.is_error()) return id_result;
    return validateReasonKeys(reason_keys, "TribeKnowledgeEvent reason_keys");
}

Result<void> TribeStateInput::validateBasic() const {
    auto id_result = validateId(tribe_id, "TribeStateInput tribe_id");
    if (id_result.is_error()) return id_result;
    std::set<std::string> seen_member_ids;
    for (const auto& event : member_events) {
        auto result = event.validateBasic();
        if (result.is_error()) return result;
        if (!seen_member_ids.insert(event.member_id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeStateInput duplicate member_id in member_events"));
        }
    }
    std::set<std::string> seen_knowledge_ids;
    for (const auto& event : knowledge_events) {
        auto result = event.validateBasic();
        if (result.is_error()) return result;
        if (!seen_knowledge_ids.insert(event.knowledge_id.value()).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeStateInput duplicate knowledge_id in knowledge_events"));
        }
    }
    for (const auto& item : {std::pair<double, const char*>{resource_pressure, "TribeStateInput resource_pressure"},
                            {safety_pressure, "TribeStateInput safety_pressure"},
                            {knowledge_conflict_pressure, "TribeStateInput knowledge_conflict_pressure"},
                            {casualty_pressure, "TribeStateInput casualty_pressure"}}) {
        auto result = validateRatioField(item.first, item.second);
        if (result.is_error()) return result;
    }
    auto morale_result = validateDeltaField(morale_delta, "TribeStateInput morale_delta");
    if (morale_result.is_error()) return morale_result;
    auto trust_result = validateDeltaField(trust_delta, "TribeStateInput trust_delta");
    if (trust_result.is_error()) return trust_result;
    return validateReasonKeys(reason_keys, "TribeStateInput reason_keys");
}

Result<void> TribeStateOptions::validateBasic() const {
    if (!std::isfinite(morale_delta_weight) || morale_delta_weight < 0.0 || morale_delta_weight > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "TribeStateOptions morale_delta_weight must be 0..1"));
    }
    if (!std::isfinite(trust_delta_weight) || trust_delta_weight < 0.0 || trust_delta_weight > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "TribeStateOptions trust_delta_weight must be 0..1"));
    }
    return Result<void>::ok();
}

Result<void> TribeStateChangeDraft::validateBasic() const {
    if (!isValidChangeKind(kind)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeStateChangeDraft kind invalid"));
    }
    if (kind == TribeStateChangeKind::MemberAdded) {
        if (!member_id.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberAdded requires member_id"));
        }
        if (!new_role.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberAdded requires new_role"));
        }
    } else if (kind == TribeStateChangeKind::MemberRemoved) {
        if (!member_id.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberRemoved requires member_id"));
        }
        if (!old_role.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberRemoved requires old_role"));
        }
    } else if (kind == TribeStateChangeKind::MemberRoleChanged) {
        if (!member_id.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberRoleChanged requires member_id"));
        }
        if (!old_role.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberRoleChanged requires old_role"));
        }
        if (!new_role.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "TribeStateChangeDraft MemberRoleChanged requires new_role"));
        }
    } else if (kind == TribeStateChangeKind::TrustChanged ||
               kind == TribeStateChangeKind::MoraleChanged ||
               kind == TribeStateChangeKind::SplitRiskChanged ||
               kind == TribeStateChangeKind::CohesionChanged) {
        if (!isValidRatio(old_value) || !isValidRatio(new_value)) {
            return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "TribeStateChangeDraft value out of 0..1 range"));
        }
    } else if (kind == TribeStateChangeKind::KnowledgeLinked) {
        if (new_value < 0.0 || !std::isfinite(new_value)) {
            return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "TribeStateChangeDraft KnowledgeLinked new_value invalid"));
        }
    }
    if (member_id.has_value()) {
        auto id_result = validateId(member_id.value(), "TribeStateChangeDraft member_id");
        if (id_result.is_error()) return id_result;
    }
    if (old_role.has_value() && !isValidRoleForInput(old_role.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeStateChangeDraft old_role invalid"));
    }
    if (new_role.has_value() && !isValidRoleForInput(new_role.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "TribeStateChangeDraft new_role invalid"));
    }
    if (summary_key.empty() || containsTribeForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeStateChangeDraft summary_key invalid"));
    }
    return validateReasonKeys(reason_keys, "TribeStateChangeDraft reason_keys");
}

Result<void> TribeProjection::validateBasic() const {
    auto id_result = validateId(tribe_id, "TribeProjection tribe_id");
    if (id_result.is_error()) return id_result;
    if (display_key.empty() || population_summary.empty() || morale_summary.empty() || trust_summary.empty() || split_risk_summary.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeProjection summary missing"));
    }
    if (containsTribeForbiddenKey(display_key) || containsTribeForbiddenKey(population_summary) ||
        containsTribeForbiddenKey(morale_summary) || containsTribeForbiddenKey(trust_summary) ||
        containsTribeForbiddenKey(split_risk_summary) || containsTribeForbiddenKey(role_summary_lines) ||
        containsTribeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeProjection contains forbidden key"));
    }
    if (shared_knowledge_count < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "TribeProjection shared_knowledge_count invalid"));
    }
    return Result<void>::ok();
}

Result<void> TribeTrace::validateBasic() const {
    if (trace_key.empty() || containsTribeForbiddenKey(trace_key) || containsTribeForbiddenKey(steps) ||
        containsTribeForbiddenKey(matched_policy_keys) || containsTribeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeTrace contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> TribeStateResolveResult::validateBasic() const {
    if (decision != "updated" && decision != "no_change") {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeStateResolveResult decision invalid"));
    }
    auto state_result = updated_state.validateBasic();
    if (state_result.is_error()) return state_result;
    for (const auto& draft : state_changes) {
        auto result = draft.validateBasic();
        if (result.is_error()) return result;
    }
    auto projection_result = projection.validateBasic();
    if (projection_result.is_error()) return projection_result;
    auto trace_result = trace.validateBasic();
    if (trace_result.is_error()) return trace_result;
    if (containsTribeForbiddenKey(reason_keys) || containsTribeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "TribeStateResolveResult contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<TribeProjection> TribeProjectionBuilder::build(const TribeState& state) const {
    auto state_result = state.validateBasic();
    if (state_result.is_error()) return Result<TribeProjection>::fail(state_result.errors());

    TribeProjection projection;
    projection.tribe_id = state.tribe_id;
    projection.display_key = state.display_key;
    projection.population_summary = "population_total=" + std::to_string(state.population_total) +
                                    ";active_population=" + std::to_string(state.active_population);
    projection.morale_summary = "overall=" + compactDouble(state.morale.overall) +
                                ";food_pressure=" + compactDouble(state.morale.food_pressure) +
                                ";safety_pressure=" + compactDouble(state.morale.safety_pressure);
    projection.trust_summary = "leader=" + compactDouble(state.trust.leader_trust) +
                               ";member=" + compactDouble(state.trust.member_trust) +
                               ";fairness=" + compactDouble(state.trust.teaching_fairness);
    projection.split_risk_summary = "risk=" + compactDouble(state.split_risk.risk) +
                                    ";cohesion=" + toString(state.split_risk.cohesion_state);
    projection.shared_knowledge_count = static_cast<int>(state.shared_knowledge_ids.size());

    std::map<std::string, int> role_counts;
    for (const auto& member : state.members) {
        role_counts[toString(member.role)] += 1;
    }
    for (const auto& [role, count] : role_counts) {
        projection.role_summary_lines.push_back(role + "=" + std::to_string(count));
    }
    if (projection.role_summary_lines.empty()) {
        projection.role_summary_lines.push_back("none=0");
    }
    if (state.split_risk.risk >= 0.65) {
        projection.warning_keys.push_back("tribe_split_risk_high");
    }

    auto projection_result = projection.validateBasic();
    if (projection_result.is_error()) return Result<TribeProjection>::fail(projection_result.errors());
    return Result<TribeProjection>::ok(std::move(projection));
}

Result<TribeStateResolveResult> TribeStateResolver::resolve(const TribeState& current,
                                                            const TribeStateInput& input,
                                                            const TribeStateOptions& options) const {
    auto options_result = options.validateBasic();
    if (options_result.is_error()) return Result<TribeStateResolveResult>::fail(options_result.errors());
    auto input_result = input.validateBasic();
    if (input_result.is_error()) return Result<TribeStateResolveResult>::fail(input_result.errors());
    if (!current.tribe_id.empty()) {
        auto current_result = current.validateBasic();
        if (current_result.is_error()) return Result<TribeStateResolveResult>::fail(current_result.errors());
        if (current.tribe_id != input.tribe_id) {
            return Result<TribeStateResolveResult>::fail(makeError(ErrorCode::id_type_mismatch, "TribeStateInput tribe_id differs from current"));
        }
    }

    TribeState state = current;
    state.tribe_id = input.tribe_id;
    if (state.display_key.empty()) {
        state.display_key = input.tribe_id.value();
    }
    state.updated_tick = input.input_tick;
    state.version = state.version.next();
    appendUniqueReasons(state.reason_keys, input.reason_keys);

    TribeStateResolveResult result;
    result.decision = "no_change";
    result.reason_keys = input.reason_keys;
    result.trace.trace_key = "tribe_state_resolve";
    result.trace.input_tick = input.input_tick;
    result.trace.reason_keys = input.reason_keys;
    result.trace.matched_policy_keys.push_back("p23_deterministic_tribe_state");

    for (const auto& event : input.member_events) {
        appendUniqueReasons(state.reason_keys, event.reason_keys);
        auto found = std::find_if(state.members.begin(), state.members.end(), [&](const TribeMember& member) {
            return member.member_id == event.member_id;
        });

        if (event.kind == TribeStateChangeKind::MemberAdded) {
            if (found == state.members.end()) {
                TribeMember member;
                member.member_id = event.member_id;
                member.role = event.role.value();
                member.trust = event.trust.value_or(state.trust.member_trust);
                member.morale = event.morale.value_or(state.morale.overall);
                member.known_knowledge_ids = event.known_knowledge_ids;
                member.joined_tick = input.input_tick;
                member.updated_tick = input.input_tick;
                member.reason_keys = event.reason_keys;
                state.members.push_back(std::move(member));
                TribeStateChangeDraft draft;
                draft.kind = TribeStateChangeKind::MemberAdded;
                draft.member_id = event.member_id;
                draft.new_role = event.role.value();
                draft.new_value = 1.0;
                draft.summary_key = "member_added";
                draft.reason_keys = event.reason_keys;
                result.state_changes.push_back(std::move(draft));
                result.trace.steps.push_back("member_added:" + event.member_id.value());
            }
        } else if (event.kind == TribeStateChangeKind::MemberRemoved) {
            if (found != state.members.end()) {
                TribeStateChangeDraft draft;
                draft.kind = TribeStateChangeKind::MemberRemoved;
                draft.member_id = event.member_id;
                draft.old_role = found->role;
                draft.old_value = 1.0;
                draft.summary_key = "member_removed";
                draft.reason_keys = event.reason_keys;
                result.state_changes.push_back(std::move(draft));
                result.trace.steps.push_back("member_removed:" + event.member_id.value());
                state.members.erase(found);
            }
        } else if (event.kind == TribeStateChangeKind::MemberRoleChanged && found != state.members.end()) {
            if (found->role != event.role.value()) {
                TribeStateChangeDraft draft;
                draft.kind = TribeStateChangeKind::MemberRoleChanged;
                draft.member_id = event.member_id;
                draft.old_role = found->role;
                draft.new_role = event.role.value();
                draft.summary_key = "member_role_changed";
                draft.reason_keys = event.reason_keys;
                result.state_changes.push_back(std::move(draft));
                found->role = event.role.value();
                found->updated_tick = input.input_tick;
                appendUniqueReasons(found->reason_keys, event.reason_keys);
                result.trace.steps.push_back("member_role_changed:" + event.member_id.value());
            }
        } else if (event.kind == TribeStateChangeKind::TrustChanged && found != state.members.end() && event.trust.has_value()) {
            const double old_value = found->trust;
            found->trust = event.trust.value();
            found->updated_tick = input.input_tick;
            appendUniqueReasons(found->reason_keys, event.reason_keys);
            result.state_changes.push_back(makeValueChange(TribeStateChangeKind::TrustChanged, old_value, found->trust, "member_trust_changed", event.reason_keys));
            result.state_changes.back().member_id = event.member_id;
            result.trace.steps.push_back("member_trust_changed:" + event.member_id.value());
        } else if (event.kind == TribeStateChangeKind::MoraleChanged && found != state.members.end() && event.morale.has_value()) {
            const double old_value = found->morale;
            found->morale = event.morale.value();
            found->updated_tick = input.input_tick;
            appendUniqueReasons(found->reason_keys, event.reason_keys);
            result.state_changes.push_back(makeValueChange(TribeStateChangeKind::MoraleChanged, old_value, found->morale, "member_morale_changed", event.reason_keys));
            result.state_changes.back().member_id = event.member_id;
            result.trace.steps.push_back("member_morale_changed:" + event.member_id.value());
        }
    }

    for (const auto& event : input.knowledge_events) {
        if (std::none_of(state.shared_knowledge_ids.begin(), state.shared_knowledge_ids.end(), [&](const KnowledgeId& id) {
                return id == event.knowledge_id;
            })) {
            state.shared_knowledge_ids.push_back(event.knowledge_id);
            TribeStateChangeDraft draft;
            draft.kind = TribeStateChangeKind::KnowledgeLinked;
            draft.new_value = static_cast<double>(state.shared_knowledge_ids.size());
            draft.summary_key = "knowledge_linked";
            draft.reason_keys = event.reason_keys;
            result.state_changes.push_back(std::move(draft));
            result.trace.steps.push_back("knowledge_linked:" + event.knowledge_id.value());
        }
    }

    recalculatePopulation(state);

    const double old_morale = state.morale.overall;
    state.morale.food_pressure = input.resource_pressure;
    state.morale.safety_pressure = input.safety_pressure;
    state.morale.recent_success = input.morale_delta > 0.0 ? clampRatio(input.morale_delta) : 0.0;
    state.morale.recent_loss = input.morale_delta < 0.0 ? clampRatio(-input.morale_delta) : 0.0;
    state.morale.overall = clampRatio(state.morale.overall + input.morale_delta * options.morale_delta_weight - input.resource_pressure * 0.10 - input.safety_pressure * 0.05);
    state.morale.reason_keys = input.reason_keys;
    if (std::fabs(old_morale - state.morale.overall) > kEpsilon) {
        result.state_changes.push_back(makeValueChange(TribeStateChangeKind::MoraleChanged, old_morale, state.morale.overall, "tribe_morale_changed", input.reason_keys));
        result.trace.steps.push_back("morale_changed:" + compactDouble(old_morale) + "->" + compactDouble(state.morale.overall));
    }

    const double old_leader_trust = state.trust.leader_trust;
    const double old_member_trust = state.trust.member_trust;
    state.trust.leader_trust = clampRatio(state.trust.leader_trust + input.trust_delta * options.trust_delta_weight);
    state.trust.member_trust = clampRatio(state.trust.member_trust + input.trust_delta * options.trust_delta_weight * 0.5);
    state.trust.teaching_fairness = clampRatio(state.trust.teaching_fairness - input.knowledge_conflict_pressure * 0.10);
    state.trust.knowledge_conflict_pressure = input.knowledge_conflict_pressure;
    state.trust.reason_keys = input.reason_keys;
    if (std::fabs(old_leader_trust - state.trust.leader_trust) > kEpsilon || std::fabs(old_member_trust - state.trust.member_trust) > kEpsilon) {
        result.state_changes.push_back(makeValueChange(TribeStateChangeKind::TrustChanged, old_leader_trust, state.trust.leader_trust, "tribe_trust_changed", input.reason_keys));
        result.trace.steps.push_back("trust_changed:" + compactDouble(old_leader_trust) + "->" + compactDouble(state.trust.leader_trust));
    }

    const double old_risk = state.split_risk.risk;
    const TribeCohesionState old_cohesion = state.split_risk.cohesion_state;
    state.split_risk = calculateSplitRisk(state, input);
    if (std::fabs(old_risk - state.split_risk.risk) > kEpsilon) {
        result.state_changes.push_back(makeValueChange(TribeStateChangeKind::SplitRiskChanged, old_risk, state.split_risk.risk, "split_risk_changed", state.split_risk.reason_keys));
        result.trace.steps.push_back("split_risk_changed:" + compactDouble(old_risk) + "->" + compactDouble(state.split_risk.risk));
    }
    if (old_cohesion != state.split_risk.cohesion_state) {
        TribeStateChangeDraft draft;
        draft.kind = TribeStateChangeKind::CohesionChanged;
        draft.old_value = old_risk;
        draft.new_value = state.split_risk.risk;
        draft.summary_key = "cohesion_changed_" + toString(state.split_risk.cohesion_state);
        draft.reason_keys = state.split_risk.reason_keys;
        result.state_changes.push_back(std::move(draft));
        result.trace.steps.push_back("cohesion_changed:" + toString(old_cohesion) + "->" + toString(state.split_risk.cohesion_state));
    }

    TribeProjectionBuilder builder;
    auto projection_result = builder.build(state);
    if (projection_result.is_error()) return Result<TribeStateResolveResult>::fail(projection_result.errors());

    result.updated_state = std::move(state);
    result.projection = projection_result.value();
    result.warning_keys = result.projection.warning_keys;
    result.decision = result.state_changes.empty() ? "no_change" : "updated";
    if (result.trace.steps.empty()) {
        result.trace.steps.push_back("no_state_change");
    }

    auto final_result = result.validateBasic();
    if (final_result.is_error()) return Result<TribeStateResolveResult>::fail(final_result.errors());
    return Result<TribeStateResolveResult>::ok(std::move(result));
}

} // namespace pathfinder::tribe
