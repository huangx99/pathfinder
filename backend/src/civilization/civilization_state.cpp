#include "pathfinder/civilization/civilization_state.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace pathfinder::civilization {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;
using pathfinder::foundation::EventId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::StateVersion;
using pathfinder::foundation::Tick;

static double clampRatio(double value) {
    if (!std::isfinite(value)) return 0.0;
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

static bool isValidRatio(double value) {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

// Deterministic ID generation helpers
static EventId makeCandidateId(const TribeId& tribe, CapabilityType type) {
    return EventId(tribe.value() + "_cand_" + toString(type));
}

static EventId makeChangeId(const TribeId& tribe, const std::string& suffix) {
    return EventId(tribe.value() + "_chg_" + suffix);
}

static EventId makeDraftId(const TribeId& tribe, CapabilityType type, const std::string& effect_key) {
    return EventId(tribe.value() + "_eff_" + toString(type) + "_" + effect_key);
}

static EventId makeEventId(const TribeId& tribe, const std::string& type_key) {
    return EventId(tribe.value() + "_evt_" + type_key);
}

static EventId makeTraceId(const TribeId& tribe, Tick tick) {
    return EventId(tribe.value() + "_trace_" + std::to_string(tick.value()));
}

// Safeguard: forbid all unsafe keys in any string
static bool containsUnsafe(const std::string& s) {
    return containsCivilizationForbiddenKey(s);
}

static std::string sanitize(const std::string& s) {
    if (containsUnsafe(s)) return "sanitized";
    return s;
}

// ---------------------------------------------------------------------------
// Helpers: find capability in state
// ---------------------------------------------------------------------------

static const CivilizationCapabilityState* findCapability(
    const CivilizationState& state, CapabilityType type) {
    for (const auto& cap : state.capabilities) {
        if (cap.capability_type == type) return &cap;
    }
    return nullptr;
}

static const CivilizationCapability* findDefinition(
    const std::vector<CivilizationCapability>& defs, CapabilityType type) {
    for (const auto& def : defs) {
        if (def.capability_type == type) return &def;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Evaluation helpers
// ---------------------------------------------------------------------------

static double evaluateConditionScore(const std::string& condition_type, double required,
                                      const CivilizationResolveInput& input) {
    const auto& ks = input.knowledge_summary;
    const auto& ps = input.propagation_summary;
    const auto& cs = input.coordination_summary;
    const auto& cfs = input.conflict_summary;

    // Higher-is-better conditions
    if (condition_type == "known_edible_count") {
        return clampRatio(static_cast<double>(ks.known_edible_count) / std::max(1.0, required));
    }
    if (condition_type == "known_danger_count") {
        return clampRatio(static_cast<double>(ks.known_danger_count) / std::max(1.0, required));
    }
    if (condition_type == "stable_tribe_knowledge_count") {
        return clampRatio(static_cast<double>(ks.stable_tribe_knowledge_count) / std::max(1.0, required));
    }
    if (condition_type == "teachable_knowledge_count") {
        return clampRatio(static_cast<double>(ks.teachable_knowledge_count) / std::max(1.0, required));
    }
    if (condition_type == "coverage") {
        return clampRatio(ps.coverage / std::max(0.01, required));
    }
    if (condition_type == "success_rate") {
        return clampRatio(ps.success_rate / std::max(0.01, required));
    }
    if (condition_type == "truce_success_count") {
        return clampRatio(static_cast<double>(cfs.truce_success_count) / std::max(1.0, required));
    }
    if (condition_type == "forced_retreat_low_loss_count") {
        return clampRatio(static_cast<double>(cfs.forced_retreat_low_loss_count) / std::max(1.0, required));
    }
    if (condition_type == "standoff_controlled_count") {
        return clampRatio(static_cast<double>(cfs.standoff_controlled_count) / std::max(1.0, required));
    }
    if (condition_type == "coordination_score") {
        return clampRatio(cs.coordination_score / std::max(0.01, required));
    }
    if (condition_type == "intimidation_success_count") {
        return clampRatio(static_cast<double>(cfs.intimidation_success_count) / std::max(1.0, required));
    }
    if (condition_type == "retreat_success_count") {
        return clampRatio(static_cast<double>(cs.retreat_success_count) / std::max(1.0, required));
    }
    if (condition_type == "resource_defense_success_count") {
        return clampRatio(static_cast<double>(cs.resource_defense_success_count) / std::max(1.0, required));
    }
    if (condition_type == "stable_coordination_count") {
        return clampRatio(static_cast<double>(cs.stable_coordination_count) / std::max(1.0, required));
    }

    // Lower-is-better conditions (invert)
    if (condition_type == "misunderstanding_rate") {
        double inv = 1.0 - ps.misunderstanding_rate;
        double inv_required = 1.0 - std::min(required, 0.99);
        return clampRatio(inv / std::max(0.01, inv_required));
    }
    if (condition_type == "conflicted_knowledge_count") {
        // 0 conflicted is ideal, lower is better
        if (required <= 0.0) return ks.conflicted_knowledge_count == 0 ? 1.0 : 0.0;
        return clampRatio(1.0 - (static_cast<double>(ks.conflicted_knowledge_count) / std::max(1.0, required * 10.0)));
    }
    if (condition_type == "open_conflict_pressure") {
        double inv = 1.0 - cfs.open_conflict_pressure;
        double inv_required = 1.0 - std::min(required, 0.99);
        return clampRatio(inv / std::max(0.01, inv_required));
    }
    if (condition_type == "loss_pressure") {
        double inv = 1.0 - cfs.loss_pressure;
        double inv_required = 1.0 - std::min(required, 0.99);
        return clampRatio(inv / std::max(0.01, inv_required));
    }

    // Unknown condition type
    return 0.0;
}

static bool isCountCondition(const std::string& condition_type) {
    return condition_type == "known_edible_count" ||
           condition_type == "known_danger_count" ||
           condition_type == "stable_tribe_knowledge_count" ||
           condition_type == "teachable_knowledge_count" ||
           condition_type == "truce_success_count" ||
           condition_type == "forced_retreat_low_loss_count" ||
           condition_type == "standoff_controlled_count" ||
           condition_type == "intimidation_success_count" ||
           condition_type == "retreat_success_count" ||
           condition_type == "resource_defense_success_count" ||
           condition_type == "stable_coordination_count" ||
           condition_type == "conflicted_knowledge_count";
}

static double getCurrentCountValue(const std::string& condition_type, const CivilizationResolveInput& input) {
    const auto& ks = input.knowledge_summary;
    const auto& ps = input.propagation_summary;
    const auto& cs = input.coordination_summary;
    const auto& cfs = input.conflict_summary;

    if (condition_type == "known_edible_count") return static_cast<double>(ks.known_edible_count);
    if (condition_type == "known_danger_count") return static_cast<double>(ks.known_danger_count);
    if (condition_type == "stable_tribe_knowledge_count") return static_cast<double>(ks.stable_tribe_knowledge_count);
    if (condition_type == "teachable_knowledge_count") return static_cast<double>(ks.teachable_knowledge_count);
    if (condition_type == "truce_success_count") return static_cast<double>(cfs.truce_success_count);
    if (condition_type == "forced_retreat_low_loss_count") return static_cast<double>(cfs.forced_retreat_low_loss_count);
    if (condition_type == "standoff_controlled_count") return static_cast<double>(cfs.standoff_controlled_count);
    if (condition_type == "intimidation_success_count") return static_cast<double>(cfs.intimidation_success_count);
    if (condition_type == "retreat_success_count") return static_cast<double>(cs.retreat_success_count);
    if (condition_type == "resource_defense_success_count") return static_cast<double>(cs.resource_defense_success_count);
    if (condition_type == "stable_coordination_count") return static_cast<double>(cs.stable_coordination_count);
    if (condition_type == "conflicted_knowledge_count") return static_cast<double>(ks.conflicted_knowledge_count);
    if (condition_type == "coverage") return ps.coverage;
    if (condition_type == "success_rate") return ps.success_rate;
    if (condition_type == "misunderstanding_rate") return ps.misunderstanding_rate;
    if (condition_type == "coordination_score") return cs.coordination_score;
    if (condition_type == "open_conflict_pressure") return cfs.open_conflict_pressure;
    if (condition_type == "loss_pressure") return cfs.loss_pressure;
    return 0.0;
}

// ---------------------------------------------------------------------------
// CivilizationCandidateBuilder
// ---------------------------------------------------------------------------

Result<std::vector<CivilizationCapabilityCandidate>>
CivilizationCandidateBuilder::build(const CivilizationResolveInput& input) const {
    std::vector<CivilizationCapabilityCandidate> candidates;
    const auto& tribe = input.current_state.tribe_id;
    Tick tick = input.input_tick;

    // --- IdentifyEdible ---
    {
        const auto& ks = input.knowledge_summary;
        const auto& ps = input.propagation_summary;
        bool has_candidate = ks.known_edible_count >= 1 && ks.teachable_knowledge_count >= 1;

        if (has_candidate) {
            CivilizationCapabilityCandidate c;
            c.candidate_id = makeCandidateId(tribe, CapabilityType::IdentifyEdible);
            c.tribe_id = tribe;
            c.capability_type = CapabilityType::IdentifyEdible;

            double knowledge_score = clampRatio(static_cast<double>(ks.known_edible_count) / 3.0);
            double teach_score = clampRatio(static_cast<double>(ks.teachable_knowledge_count) / 2.0);
            double cov_score = ps.coverage;
            double mis_score = 1.0 - ps.misunderstanding_rate;
            c.readiness = clampRatio((knowledge_score * 0.3 + teach_score * 0.2 + cov_score * 0.3 + mis_score * 0.2));

            c.evidence.knowledge_ids = ks.knowledge_ids;
            c.evidence.confidence = clampRatio((knowledge_score + cov_score) / 2.0);
            c.evidence.reason_keys = {"identify_edible_candidate"};
            c.reason_keys = {"identify_edible_candidate"};

            candidates.push_back(std::move(c));
        }
    }

    // --- IdentifyDanger ---
    {
        const auto& ks = input.knowledge_summary;
        const auto& ps = input.propagation_summary;
        bool has_candidate = ks.known_danger_count >= 1;

        if (has_candidate) {
            CivilizationCapabilityCandidate c;
            c.candidate_id = makeCandidateId(tribe, CapabilityType::IdentifyDanger);
            c.tribe_id = tribe;
            c.capability_type = CapabilityType::IdentifyDanger;

            double danger_score = clampRatio(static_cast<double>(ks.known_danger_count) / 3.0);
            double cov_score = ps.coverage;
            double mis_score = 1.0 - ps.misunderstanding_rate;
            double conflict_free = ks.conflicted_knowledge_count == 0 ? 1.0 : 0.5;
            c.readiness = clampRatio((danger_score * 0.35 + cov_score * 0.25 + mis_score * 0.25 + conflict_free * 0.15));

            c.evidence.knowledge_ids = ks.knowledge_ids;
            c.evidence.confidence = clampRatio((danger_score + cov_score) / 2.0);
            c.evidence.reason_keys = {"identify_danger_candidate"};
            c.reason_keys = {"identify_danger_candidate"};

            candidates.push_back(std::move(c));
        }
    }

    // --- SafeForaging ---
    {
        const auto* id_edible = findCapability(input.current_state, CapabilityType::IdentifyEdible);
        const auto* id_danger = findCapability(input.current_state, CapabilityType::IdentifyDanger);

        bool prereq_met = id_edible && id_danger &&
            (id_edible->maturity == CapabilityMaturityState::Emerging ||
             id_edible->maturity == CapabilityMaturityState::Stable ||
             id_edible->maturity == CapabilityMaturityState::Institutionalized) &&
            (id_danger->maturity == CapabilityMaturityState::Emerging ||
             id_danger->maturity == CapabilityMaturityState::Stable ||
             id_danger->maturity == CapabilityMaturityState::Institutionalized);

        if (prereq_met) {
            const auto& ks = input.knowledge_summary;
            const auto& ps = input.propagation_summary;

            CivilizationCapabilityCandidate c;
            c.candidate_id = makeCandidateId(tribe, CapabilityType::SafeForaging);
            c.tribe_id = tribe;
            c.capability_type = CapabilityType::SafeForaging;

            double food_score = clampRatio(static_cast<double>(ks.known_edible_count) / 3.0);
            double danger_score = clampRatio(static_cast<double>(ks.known_danger_count) / 2.0);
            double cov_score = ps.coverage;
            double succ_score = ps.success_rate;
            double mis_score = 1.0 - ps.misunderstanding_rate;
            c.readiness = clampRatio((food_score * 0.25 + danger_score * 0.15 + cov_score * 0.25 + succ_score * 0.2 + mis_score * 0.15));

            std::set<std::string> seen_ids;
            for (const auto& kid : ks.knowledge_ids) {
                if (seen_ids.insert(kid.value()).second) {
                    c.evidence.knowledge_ids.push_back(kid);
                }
            }
            if (id_edible) {
                for (const auto& kid : id_edible->evidence.knowledge_ids) {
                    if (seen_ids.insert(kid.value()).second) {
                        c.evidence.knowledge_ids.push_back(kid);
                    }
                }
            }
            c.evidence.propagation_event_ids = ps.teaching_event_ids;
            c.evidence.confidence = clampRatio((cov_score + succ_score) / 2.0);
            c.evidence.reason_keys = {"safe_foraging_candidate"};
            c.reason_keys = {"safe_foraging_candidate", "requires_prerequisites"};

            candidates.push_back(std::move(c));
        }
    }

    // --- ConflictDeescalation ---
    {
        const auto& cfs = input.conflict_summary;
        const auto& cs = input.coordination_summary;

        bool has_candidate = cfs.truce_success_count > 0 ||
                             cfs.forced_retreat_low_loss_count > 0 ||
                             cfs.standoff_controlled_count > 0;

        if (has_candidate) {
            CivilizationCapabilityCandidate c;
            c.candidate_id = makeCandidateId(tribe, CapabilityType::ConflictDeescalation);
            c.tribe_id = tribe;
            c.capability_type = CapabilityType::ConflictDeescalation;

            double truce_score = clampRatio(static_cast<double>(cfs.truce_success_count) / 3.0);
            double retreat_score = clampRatio(static_cast<double>(cfs.forced_retreat_low_loss_count) / 3.0);
            double standoff_score = clampRatio(static_cast<double>(cfs.standoff_controlled_count) / 3.0);
            double peace_pressure = 1.0 - clampRatio(cfs.open_conflict_pressure);
            double low_loss = 1.0 - clampRatio(cfs.loss_pressure);
            double coord = cs.coordination_score;
            c.readiness = clampRatio((std::max({truce_score, retreat_score, standoff_score}) * 0.35 +
                                      peace_pressure * 0.2 + low_loss * 0.25 + coord * 0.2));

            c.evidence.conflict_event_ids = cfs.source_event_ids;
            c.evidence.confidence = clampRatio((peace_pressure + low_loss) / 2.0);
            c.evidence.reason_keys = {"conflict_deescalation_candidate"};
            c.reason_keys = {"conflict_deescalation_candidate", "from_p25_conflict_summary"};

            candidates.push_back(std::move(c));
        }
    }

    return Result<std::vector<CivilizationCapabilityCandidate>>::ok(std::move(candidates));
}

// ---------------------------------------------------------------------------
// CapabilityRequirementEvaluator
// ---------------------------------------------------------------------------

Result<CapabilityRequirementSnapshot>
CapabilityRequirementEvaluator::evaluate(
    const CivilizationCapability& capability,
    const CivilizationResolveInput& input) const {

    CapabilityRequirementSnapshot snapshot;
    snapshot.evaluated_tick = input.input_tick;

    double total = 0.0;
    int condition_count = 0;

    for (const auto& cond : capability.required_conditions) {
        condition_count++;
        double score = evaluateConditionScore(cond.condition_type, cond.required_score, input);
        if (score >= 0.8) {
            snapshot.met_condition_keys.push_back(cond.condition_key);
        } else if (score >= 0.4) {
            snapshot.warning_requirements.push_back(cond.condition_key);
        } else {
            snapshot.missing_condition_keys.push_back(cond.condition_key);
            snapshot.failed_requirements.push_back(cond.condition_key);
        }
        total += score;
    }

    if (condition_count > 0) {
        snapshot.total_score = clampRatio(total / static_cast<double>(condition_count));
    } else {
        snapshot.total_score = 1.0;
    }

    return Result<CapabilityRequirementSnapshot>::ok(snapshot);
}

// ---------------------------------------------------------------------------
// CapabilityStateResolver
// ---------------------------------------------------------------------------

Result<CivilizationCapabilityState>
CapabilityStateResolver::resolveState(
    const CivilizationCapabilityState& current,
    const CivilizationCapabilityCandidate& candidate,
    const CivilizationCapability& definition) const {

    CivilizationCapabilityState result = current;
    result.tribe_id = candidate.tribe_id;
    result.capability_type = candidate.capability_type;
    result.evidence = candidate.evidence;
    result.coverage = result.evidence.confidence;
    result.last_changed_tick = candidate.requirement_snapshot.evaluated_tick;

    // Update progress from candidate
    result.progress.candidate_score = candidate.readiness;
    result.progress.emerging_score = candidate.requirement_snapshot.total_score;
    const double evidence_score = candidate.evidence.confidence;
    const double derived_stability = clampRatio(
        candidate.readiness * 0.35 +
        candidate.requirement_snapshot.total_score * 0.40 +
        evidence_score * 0.25);

    CapabilityMaturityState prev_maturity = current.maturity;

    // Determine maturity transition
    if (current.maturity == CapabilityMaturityState::Unknown) {
        // First time: if candidate exists, become Candidate
        if (candidate.readiness >= 0.1) {
            result.maturity = CapabilityMaturityState::Candidate;
        } else {
            result.maturity = CapabilityMaturityState::Unknown;
        }
    } else if (current.maturity == CapabilityMaturityState::Candidate) {
        // Candidate -> Emerging: enough conditions met, decent readiness
        if (candidate.requirement_snapshot.total_score >= 0.6 && candidate.readiness >= 0.3) {
            result.maturity = CapabilityMaturityState::Emerging;
        } else {
            result.maturity = CapabilityMaturityState::Candidate;
        }
    } else if (current.maturity == CapabilityMaturityState::Emerging) {
        // Emerging -> Stable: repeated success, strong conditions
        if (candidate.requirement_snapshot.total_score >= 0.8 &&
            candidate.readiness >= 0.6 &&
            current.progress.success_count > current.progress.failure_count) {
            result.maturity = CapabilityMaturityState::Stable;
        } else if (current.progress.failure_count > current.progress.success_count &&
                   candidate.evidence.confidence < 0.3) {
            result.maturity = CapabilityMaturityState::Degraded;
        } else {
            result.maturity = CapabilityMaturityState::Emerging;
        }
    } else if (current.maturity == CapabilityMaturityState::Stable) {
        // Stable -> Degraded: repeated failure or low confidence
        if (current.progress.failure_count > current.progress.success_count &&
            candidate.evidence.confidence < 0.3) {
            result.maturity = CapabilityMaturityState::Degraded;
        } else {
            result.maturity = CapabilityMaturityState::Stable;
        }
    } else if (current.maturity == CapabilityMaturityState::Degraded) {
        // Degraded -> Lost: no evidence
        if (candidate.evidence.knowledge_ids.empty() &&
            candidate.evidence.propagation_event_ids.empty() &&
            candidate.evidence.practice_event_ids.empty() &&
            candidate.evidence.conflict_event_ids.empty()) {
            result.maturity = CapabilityMaturityState::Lost;
        } else if (candidate.requirement_snapshot.total_score >= 0.7) {
            result.maturity = CapabilityMaturityState::Emerging;
        } else {
            result.maturity = CapabilityMaturityState::Degraded;
        }
    } else if (current.maturity == CapabilityMaturityState::Lost) {
        // Lost: can recover to Candidate if evidence returns
        if (!candidate.evidence.knowledge_ids.empty() ||
            !candidate.evidence.propagation_event_ids.empty() ||
            !candidate.evidence.practice_event_ids.empty() ||
            !candidate.evidence.conflict_event_ids.empty()) {
            result.maturity = CapabilityMaturityState::Candidate;
        } else {
            result.maturity = CapabilityMaturityState::Lost;
        }
    } else if (current.maturity == CapabilityMaturityState::Institutionalized) {
        result.maturity = CapabilityMaturityState::Institutionalized;
    }

    // Update progress scores based on maturity
    if (result.maturity == CapabilityMaturityState::Stable ||
        result.maturity == CapabilityMaturityState::Institutionalized) {
        result.progress.stable_score = std::max(result.progress.stable_score, candidate.requirement_snapshot.total_score);
    }

    if (result.maturity == CapabilityMaturityState::Candidate) {
        result.stability = std::max(current.stability, clampRatio(derived_stability * 0.35));
    } else if (result.maturity == CapabilityMaturityState::Emerging) {
        result.stability = std::max(current.stability, clampRatio(derived_stability * 0.70));
    } else if (result.maturity == CapabilityMaturityState::Stable ||
               result.maturity == CapabilityMaturityState::Institutionalized) {
        result.stability = std::max(current.stability, derived_stability);
    } else if (result.maturity == CapabilityMaturityState::Degraded) {
        result.stability = clampRatio(std::min(current.stability, derived_stability) * 0.75);
    } else if (result.maturity == CapabilityMaturityState::Lost) {
        result.stability = 0.0;
    }

    // Track success/failure
    if (result.maturity == CapabilityMaturityState::Stable ||
        result.maturity == CapabilityMaturityState::Emerging) {
        result.progress.success_count = current.progress.success_count + 1;
    }
    if (result.maturity == CapabilityMaturityState::Degraded) {
        result.progress.failure_count = current.progress.failure_count + 1;
    }

    if (result.maturity != prev_maturity) {
        if (result.maturity == CapabilityMaturityState::Stable ||
            result.maturity == CapabilityMaturityState::Emerging) {
            result.progress.last_success_tick = candidate.requirement_snapshot.evaluated_tick;
        }
        if (result.maturity == CapabilityMaturityState::Degraded ||
            result.maturity == CapabilityMaturityState::Lost) {
            result.progress.last_failure_tick = candidate.requirement_snapshot.evaluated_tick;
        }
    }

    return Result<CivilizationCapabilityState>::ok(result);
}

// ---------------------------------------------------------------------------
// CapabilityUsabilityResolver
// ---------------------------------------------------------------------------

Result<CivilizationCapabilityState>
CapabilityUsabilityResolver::resolveUsability(
    const CivilizationCapabilityState& state,
    const CivilizationResolveInput& input) const {

    CivilizationCapabilityState result = state;
    result.blocked_reasons.clear();

    // Lost capabilities are never usable
    if (result.maturity == CapabilityMaturityState::Lost) {
        result.usability = CapabilityUsabilityState::Suspended;
        result.blocked_reasons.push_back(CapabilityChangeReason::RepeatedFailure);
        return Result<CivilizationCapabilityState>::ok(result);
    }

    // Check BlockedByConflict: high open conflict or loss pressure
    const auto& cfs = input.conflict_summary;
    if (cfs.open_conflict_pressure > 0.65 || cfs.loss_pressure > 0.65) {
        result.usability = CapabilityUsabilityState::BlockedByConflict;
        result.blocked_reasons.push_back(CapabilityChangeReason::ConflictChanged);
        return Result<CivilizationCapabilityState>::ok(result);
    }

    // Check BlockedByCohesion: low coordination score
    const auto& cs = input.coordination_summary;
    if (cs.coordination_score < 0.25) {
        result.usability = CapabilityUsabilityState::BlockedByCohesion;
        result.blocked_reasons.push_back(CapabilityChangeReason::PropagationFailed);
        return Result<CivilizationCapabilityState>::ok(result);
    }

    // Check Degraded
    if (result.maturity == CapabilityMaturityState::Degraded) {
        result.usability = CapabilityUsabilityState::Suspended;
        result.blocked_reasons.push_back(CapabilityChangeReason::RepeatedFailure);
        return Result<CivilizationCapabilityState>::ok(result);
    }

    // Candidate/Emerging/Stable/Institutionalized without blockage -> Usable
    if (result.maturity == CapabilityMaturityState::Candidate ||
        result.maturity == CapabilityMaturityState::Emerging ||
        result.maturity == CapabilityMaturityState::Stable ||
        result.maturity == CapabilityMaturityState::Institutionalized) {
        result.usability = CapabilityUsabilityState::Usable;
    }

    return Result<CivilizationCapabilityState>::ok(result);
}

// ---------------------------------------------------------------------------
// CapabilityEffectResolver
// ---------------------------------------------------------------------------

Result<std::vector<CapabilityEffectDraft>>
CapabilityEffectResolver::resolveEffects(
    const CivilizationState& state,
    const std::vector<CivilizationCapability>& definitions) const {

    std::vector<CapabilityEffectDraft> drafts;

    for (const auto& cap_state : state.capabilities) {
        // Only Stable/Institutionalized + Usable produce effects
        if ((cap_state.maturity != CapabilityMaturityState::Stable &&
             cap_state.maturity != CapabilityMaturityState::Institutionalized) ||
            cap_state.usability != CapabilityUsabilityState::Usable) {
            continue;
        }

        // Find matching definition
        const auto* def = findDefinition(definitions, cap_state.capability_type);
        if (!def) continue;

        for (const auto& eff_def : def->effect_definitions) {
            CapabilityEffectDraft draft;
            draft.draft_id = makeDraftId(state.tribe_id, cap_state.capability_type, eff_def.effect_key);
            draft.tribe_id = state.tribe_id;
            draft.capability_type = cap_state.capability_type;
            draft.effect_key = eff_def.effect_key;
            draft.target = eff_def.target;
            draft.operation = eff_def.operation;
            draft.value_key = eff_def.value_key;
            draft.value_number = eff_def.value_number;
            draft.strength = cap_state.stability;
            draft.reason_keys = {"effect_from_definition", eff_def.effect_key};

            // Add evidence IDs from the capability state
            draft.source_evidence_ids = cap_state.evidence.practice_event_ids;
            for (const auto& id : cap_state.evidence.propagation_event_ids) {
                draft.source_evidence_ids.push_back(id);
            }

            drafts.push_back(std::move(draft));
        }
    }

    return Result<std::vector<CapabilityEffectDraft>>::ok(std::move(drafts));
}

// ---------------------------------------------------------------------------
// CivilizationStageResolver
// ---------------------------------------------------------------------------

Result<CivilizationStageResolveResult>
CivilizationStageResolver::resolveStage(const CivilizationState& state) const {
    CivilizationStageResolveResult result;

    // Collect capability maturities
    const auto* id_edible = findCapability(state, CapabilityType::IdentifyEdible);
    const auto* id_danger = findCapability(state, CapabilityType::IdentifyDanger);
    const auto* safe_foraging = findCapability(state, CapabilityType::SafeForaging);
    const auto* conflict_deesc = findCapability(state, CapabilityType::ConflictDeescalation);

    bool has_any_candidate = false;
    for (const auto& cap : state.capabilities) {
        if (cap.maturity != CapabilityMaturityState::Unknown &&
            cap.maturity != CapabilityMaturityState::Lost) {
            has_any_candidate = true;
            break;
        }
    }

    // Determine stage from capability set
    // Foraging requires: IdentifyEdible(Stable) + IdentifyDanger(>=Emerging) + SafeForaging(Stable)
    bool has_foraging =
        id_edible && id_edible->maturity == CapabilityMaturityState::Stable &&
        id_danger && (id_danger->maturity == CapabilityMaturityState::Emerging ||
                      id_danger->maturity == CapabilityMaturityState::Stable ||
                      id_danger->maturity == CapabilityMaturityState::Institutionalized) &&
        safe_foraging && safe_foraging->maturity == CapabilityMaturityState::Stable;

    if (has_foraging) {
        result.stage = CivilizationStage::Foraging;
        result.confidence = 0.85;
        result.stage_label_key = "foraging_stage";
        result.stage_steps.push_back("IdentifyEdible stable");
        result.stage_steps.push_back("IdentifyDanger emerging+");
        result.stage_steps.push_back("SafeForaging stable");
        result.stage_steps.push_back("derived Foraging stage");
    } else if (has_any_candidate) {
        result.stage = CivilizationStage::Awakening;
        result.confidence = 0.6;
        result.stage_label_key = "awakening_stage";
        result.stage_steps.push_back("at least one capability candidate+");
        result.stage_steps.push_back("derived Awakening stage");
    } else {
        result.stage = CivilizationStage::Awakening;
        result.confidence = 0.3;
        result.stage_label_key = "awakening_stage";
        result.stage_steps.push_back("no capabilities yet, default Awakening");
    }

    return Result<CivilizationStageResolveResult>::ok(result);
}

// ---------------------------------------------------------------------------
// CivilizationResolver (12-step pipeline)
// ---------------------------------------------------------------------------

Result<CivilizationResolveResult>
CivilizationResolver::resolve(const CivilizationResolveInput& input) const {
    CivilizationResolveResult result;
    result.ok = false;

    // Step 1: Validate input
    auto validate_input = input.validateBasic();
    if (validate_input.is_error()) {
        result.error = ErrorCode::validation_failed;
        return Result<CivilizationResolveResult>::ok(result);
    }

    // Step 2: Sanitize summaries
    CivilizationResolveInput sanitized = input;
    for (auto& key : sanitized.reason_keys) {
        key = sanitize(key);
    }
    CivilizationKnowledgeSummary& ks = sanitized.knowledge_summary;
    for (auto& key : ks.reason_keys) key = sanitize(key);
    CivilizationPropagationSummary& ps = sanitized.propagation_summary;
    for (auto& key : ps.reason_keys) key = sanitize(key);
    CivilizationCoordinationSummary& cs = sanitized.coordination_summary;
    for (auto& key : cs.reason_keys) key = sanitize(key);
    CivilizationConflictSummary& cfs = sanitized.conflict_summary;
    for (auto& key : cfs.reason_keys) key = sanitize(key);

    CivilizationTrace trace;
    trace.trace_id = makeTraceId(input.current_state.tribe_id, input.input_tick);
    trace.input_summary_keys = {
        "tribe:" + input.current_state.tribe_id.value(),
        "tick:" + std::to_string(input.input_tick.value()),
        "known_edible:" + std::to_string(ks.known_edible_count),
        "known_danger:" + std::to_string(ks.known_danger_count),
        "coverage:" + std::to_string(static_cast<int>(ps.coverage * 100)) + "%"
    };

    // Step 3: Build candidates
    auto build_result = candidate_builder_.build(sanitized);
    if (build_result.is_error()) {
        result.error = ErrorCode::validation_failed;
        return Result<CivilizationResolveResult>::ok(result);
    }
    auto candidates = std::move(build_result).value();
    for (const auto& c : candidates) {
        trace.candidate_steps.push_back("candidate:" + toString(c.capability_type) +
                                        " readiness:" + std::to_string(static_cast<int>(c.readiness * 100)) + "%");
    }
    result.candidates = candidates;

    // Step 4-5: Evaluate requirements and resolve states
    std::vector<CivilizationCapabilityState> new_capabilities;
    std::vector<CivilizationStateChangeDraft> state_changes;

    // Start with existing capabilities
    for (auto& cap_state : sanitized.current_state.capabilities) {
        // Find matching candidate
        const CivilizationCapabilityCandidate* matching_cand = nullptr;
        for (const auto& cand : candidates) {
            if (cand.capability_type == cap_state.capability_type) {
                matching_cand = &cand;
                break;
            }
        }

        // Find matching definition
        const auto* def = findDefinition(sanitized.capability_definitions, cap_state.capability_type);

        if (matching_cand && def) {
            // Step 4: Evaluate requirements
            auto req_result = requirement_evaluator_.evaluate(*def, sanitized);
            if (req_result.is_error()) continue;
            auto snapshot = std::move(req_result).value();

            // Update candidate with requirement snapshot
            CivilizationCapabilityCandidate updated_cand = *matching_cand;
            updated_cand.requirement_snapshot = snapshot;
            updated_cand.can_promote = snapshot.missing_condition_keys.empty();

            if (!snapshot.missing_condition_keys.empty()) {
                for (const auto& mk : snapshot.missing_condition_keys) {
                    updated_cand.recommended_next_steps.push_back("meet_condition:" + mk);
                    trace.requirement_steps.push_back("missing:" + mk + " for " + toString(cap_state.capability_type));
                }
            }
            for (const auto& met : snapshot.met_condition_keys) {
                trace.requirement_steps.push_back("met:" + met + " for " + toString(cap_state.capability_type));
            }

            // Step 5: Resolve state
            auto state_result = state_resolver_.resolveState(cap_state, updated_cand, *def);
            if (state_result.is_error()) continue;
            auto new_state = std::move(state_result).value();

            // Step 6: Resolve usability
            auto usability_result = usability_resolver_.resolveUsability(new_state, sanitized);
            if (usability_result.is_error()) continue;
            new_state = std::move(usability_result).value();

            // Step 9: Build state change draft if changed
            if (new_state.maturity != cap_state.maturity ||
                new_state.usability != cap_state.usability) {
                CivilizationStateChangeDraft change;
                change.change_id = makeChangeId(sanitized.current_state.tribe_id,
                    toString(cap_state.capability_type) + "_" +
                    std::to_string(sanitized.input_tick.value()));
                change.tribe_id = sanitized.current_state.tribe_id;
                change.capability_type = cap_state.capability_type;
                change.maturity_before = cap_state.maturity;
                change.maturity_after = new_state.maturity;
                change.usability_before = cap_state.usability;
                change.usability_after = new_state.usability;
                change.deterministic_key = toString(cap_state.capability_type) + "_transition";

                if (new_state.maturity == CapabilityMaturityState::Stable &&
                    cap_state.maturity != CapabilityMaturityState::Stable) {
                    change.reasons.push_back(CapabilityChangeReason::RepeatedSuccess);
                    change.reasons.push_back(CapabilityChangeReason::RequirementMet);
                }
                if (new_state.maturity == CapabilityMaturityState::Degraded) {
                    change.reasons.push_back(CapabilityChangeReason::RepeatedFailure);
                }
                if (new_state.maturity == CapabilityMaturityState::Emerging &&
                    cap_state.maturity == CapabilityMaturityState::Candidate) {
                    change.reasons.push_back(CapabilityChangeReason::NewEvidence);
                    change.reasons.push_back(CapabilityChangeReason::PropagationImproved);
                }
                if (new_state.usability != CapabilityUsabilityState::Usable &&
                    cap_state.usability == CapabilityUsabilityState::Usable) {
                    change.reasons.push_back(CapabilityChangeReason::ConflictChanged);
                }
                if (!change.reasons.empty()) {
                    change.reason_keys.push_back("capability_state_changed");
                    state_changes.push_back(std::move(change));
                    trace.state_transition_steps.push_back(
                        toString(cap_state.capability_type) + ":" +
                        toString(cap_state.maturity) + "->" + toString(new_state.maturity));
                }
            }

            new_capabilities.push_back(std::move(new_state));
        } else if (def) {
            // No candidate — evaluate for degradation
            auto req_result = requirement_evaluator_.evaluate(*def, sanitized);
            if (req_result.is_ok()) {
                auto snapshot = std::move(req_result).value();
                CivilizationCapabilityCandidate empty_cand;
                empty_cand.candidate_id = makeCandidateId(sanitized.current_state.tribe_id, cap_state.capability_type);
                empty_cand.tribe_id = sanitized.current_state.tribe_id;
                empty_cand.capability_type = cap_state.capability_type;
                empty_cand.readiness = 0.0;
                empty_cand.requirement_snapshot = snapshot;
                empty_cand.can_promote = false;

                auto state_result = state_resolver_.resolveState(cap_state, empty_cand, *def);
                if (state_result.is_ok()) {
                    auto degraded_state = std::move(state_result).value();
                    auto usability_result = usability_resolver_.resolveUsability(degraded_state, sanitized);
                    if (usability_result.is_ok()) {
                        new_capabilities.push_back(std::move(usability_result).value());
                    } else {
                        new_capabilities.push_back(cap_state);
                    }
                } else {
                    new_capabilities.push_back(cap_state);
                }
            } else {
                new_capabilities.push_back(cap_state);
            }
        } else {
            new_capabilities.push_back(cap_state);
        }
    }

    // Add new capabilities from candidates that don't exist yet
    for (const auto& cand : candidates) {
        bool exists = false;
        for (const auto& cap : new_capabilities) {
            if (cap.capability_type == cand.capability_type) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            const auto* def = findDefinition(sanitized.capability_definitions, cand.capability_type);
            if (def) {
                auto req_result = requirement_evaluator_.evaluate(*def, sanitized);
                if (req_result.is_ok()) {
                    auto snapshot = std::move(req_result).value();
                    CivilizationCapabilityCandidate updated_cand = cand;
                    updated_cand.requirement_snapshot = snapshot;
                    updated_cand.can_promote = snapshot.missing_condition_keys.empty();

                    CivilizationCapabilityState fresh_state;
                    fresh_state.tribe_id = sanitized.current_state.tribe_id;
                    fresh_state.capability_type = cand.capability_type;
                    fresh_state.maturity = CapabilityMaturityState::Unknown;
                    fresh_state.usability = CapabilityUsabilityState::Unknown;

                    auto state_result = state_resolver_.resolveState(fresh_state, updated_cand, *def);
                    if (state_result.is_ok()) {
                        auto new_state = std::move(state_result).value();
                        auto usability_result = usability_resolver_.resolveUsability(new_state, sanitized);
                        if (usability_result.is_ok()) {
                            new_state = std::move(usability_result).value();

                            CivilizationStateChangeDraft change;
                            change.change_id = makeChangeId(sanitized.current_state.tribe_id,
                                toString(cand.capability_type) + "_new_" +
                                std::to_string(sanitized.input_tick.value()));
                            change.tribe_id = sanitized.current_state.tribe_id;
                            change.capability_type = cand.capability_type;
                            change.maturity_before = CapabilityMaturityState::Unknown;
                            change.maturity_after = new_state.maturity;
                            change.usability_before = CapabilityUsabilityState::Unknown;
                            change.usability_after = new_state.usability;
                            change.deterministic_key = toString(cand.capability_type) + "_new";
                            if (new_state.maturity == CapabilityMaturityState::Candidate) {
                                change.reasons.push_back(CapabilityChangeReason::NewEvidence);
                                change.reason_keys.push_back("new_candidate_created");
                                state_changes.push_back(std::move(change));
                                trace.state_transition_steps.push_back(
                                    "new:" + toString(cand.capability_type) + "->Candidate");
                            }

                            new_capabilities.push_back(std::move(new_state));
                        }
                    }
                }
            }
        }
    }

    // Step 7: Resolve effects
    CivilizationState working_state;
    working_state.tribe_id = sanitized.current_state.tribe_id;
    working_state.stage = sanitized.current_state.stage;
    working_state.stage_confidence = sanitized.current_state.stage_confidence;
    working_state.capabilities = new_capabilities;
    working_state.version = sanitized.current_state.version;

    auto effects_result = effect_resolver_.resolveEffects(working_state, sanitized.capability_definitions);
    if (effects_result.is_ok()) {
        result.effect_drafts = std::move(effects_result).value();
        for (const auto& eff : result.effect_drafts) {
            trace.effect_steps.push_back("effect:" + eff.effect_key + " for " + toString(eff.capability_type));
            working_state.active_effects.push_back(eff);
        }
    }

    // Track blocked capabilities
    for (const auto& cap : working_state.capabilities) {
        if (cap.usability != CapabilityUsabilityState::Usable &&
            cap.maturity != CapabilityMaturityState::Unknown) {
            working_state.blocked_capabilities.push_back(cap.capability_type);
        }
    }

    // Step 8: Resolve stage
    auto stage_result = stage_resolver_.resolveStage(working_state);
    if (stage_result.is_ok()) {
        auto stage_res = std::move(stage_result).value();
        CivilizationStage prev_stage = working_state.stage;

        working_state.stage = stage_res.stage;
        working_state.stage_confidence = stage_res.confidence;
        for (const auto& step : stage_res.stage_steps) {
            trace.stage_steps.push_back(step);
        }

        // Stage change event
        if (stage_res.stage != prev_stage && prev_stage != CivilizationStage::Unknown) {
            CivilizationStateChangeDraft stage_change;
            stage_change.change_id = makeChangeId(sanitized.current_state.tribe_id, "stage_" +
                std::to_string(sanitized.input_tick.value()));
            stage_change.tribe_id = sanitized.current_state.tribe_id;
            stage_change.stage_before = prev_stage;
            stage_change.stage_after = stage_res.stage;
            stage_change.deterministic_key = "stage_transition";
            stage_change.reasons.push_back(CapabilityChangeReason::RequirementMet);
            stage_change.reason_keys.push_back("stage_changed");
            state_changes.push_back(std::move(stage_change));
        }
    }

    working_state.last_resolved_tick = sanitized.input_tick;

    result.state_changes = std::move(state_changes);

    // Step 10: Build event drafts
    for (const auto& cap : working_state.capabilities) {
        const auto* prev = findCapability(sanitized.current_state, cap.capability_type);

        // capability_unlocked: first time reaching Stable
        if (cap.maturity == CapabilityMaturityState::Stable &&
            (!prev || prev->maturity != CapabilityMaturityState::Stable)) {
            CivilizationEventDraft ev;
            ev.event_id = makeEventId(sanitized.current_state.tribe_id, "capability_unlocked_" + toString(cap.capability_type));
            ev.event_type_key = "capability_unlocked";
            ev.tribe_id = sanitized.current_state.tribe_id;
            ev.capability_type = cap.capability_type;
            ev.stage_before = sanitized.current_state.stage;
            ev.stage_after = working_state.stage;
            ev.message_key = "capability_unlocked:" + toString(cap.capability_type);
            ev.reason_keys = {"capability_stable"};
            result.event_drafts.push_back(std::move(ev));
        }

        // capability_blocked
        if (cap.usability != CapabilityUsabilityState::Usable &&
            cap.maturity != CapabilityMaturityState::Unknown &&
            cap.maturity != CapabilityMaturityState::Lost &&
            (!prev || prev->usability == CapabilityUsabilityState::Usable)) {
            CivilizationEventDraft ev;
            ev.event_id = makeEventId(sanitized.current_state.tribe_id, "capability_blocked_" + toString(cap.capability_type));
            ev.event_type_key = "capability_blocked";
            ev.tribe_id = sanitized.current_state.tribe_id;
            ev.capability_type = cap.capability_type;
            ev.message_key = "capability_blocked:" + toString(cap.capability_type);
            ev.reason_keys = {"capability_blocked"};
            result.event_drafts.push_back(std::move(ev));
        }

        // capability_degraded
        if (cap.maturity == CapabilityMaturityState::Degraded &&
            prev && prev->maturity != CapabilityMaturityState::Degraded) {
            CivilizationEventDraft ev;
            ev.event_id = makeEventId(sanitized.current_state.tribe_id, "capability_degraded_" + toString(cap.capability_type));
            ev.event_type_key = "capability_degraded";
            ev.tribe_id = sanitized.current_state.tribe_id;
            ev.capability_type = cap.capability_type;
            ev.message_key = "capability_degraded:" + toString(cap.capability_type);
            ev.reason_keys = {"capability_degraded"};
            result.event_drafts.push_back(std::move(ev));
        }
    }

    // Stage change event
    if (working_state.stage != sanitized.current_state.stage &&
        sanitized.current_state.stage != CivilizationStage::Unknown) {
        CivilizationEventDraft ev;
        ev.event_id = makeEventId(sanitized.current_state.tribe_id, "civilization_stage_changed");
        ev.event_type_key = "civilization_stage_changed";
        ev.tribe_id = sanitized.current_state.tribe_id;
        ev.stage_before = sanitized.current_state.stage;
        ev.stage_after = working_state.stage;
        ev.message_key = "stage:" + toString(sanitized.current_state.stage) + "->" + toString(working_state.stage);
        ev.reason_keys = {"stage_changed"};
        result.event_drafts.push_back(std::move(ev));
    }

    // Step 11: Build projection
    CivilizationProjection projection;
    projection.tribe_id = sanitized.current_state.tribe_id;
    projection.stage = working_state.stage;
    projection.stage_label_key = "stage_" + toString(working_state.stage);

    for (const auto& cap : working_state.capabilities) {
        switch (cap.maturity) {
            case CapabilityMaturityState::Stable:
            case CapabilityMaturityState::Institutionalized:
                projection.stable_capabilities.push_back(cap.capability_type);
                break;
            case CapabilityMaturityState::Emerging:
                projection.emerging_capabilities.push_back(cap.capability_type);
                break;
            case CapabilityMaturityState::Candidate:
                projection.next_candidates.push_back(cap.capability_type);
                break;
            default:
                break;
        }
        if (cap.usability != CapabilityUsabilityState::Usable &&
            cap.maturity != CapabilityMaturityState::Unknown) {
            projection.blocked_capabilities.push_back(cap.capability_type);
            projection.risk_warning_keys.push_back("blocked:" + toString(cap.capability_type));
        }
    }

    for (const auto& eff : working_state.active_effects) {
        projection.active_effect_summary_keys.push_back(eff.effect_key);
    }

    projection.explanation_key = "civilization_projection_v1";
    result.projection = std::move(projection);

    // Step 12: Build trace (finalize)
    trace.rejected_unsafe_keys = {};
    result.trace = std::move(trace);

    // Step 13: Validate result
    result.ok = true;
    result.updated_state = std::move(working_state);

    auto validate_result = result.validateBasic();
    if (validate_result.is_error()) {
        result.ok = false;
        result.error = ErrorCode::validation_failed;
    }

    return Result<CivilizationResolveResult>::ok(std::move(result));
}

} // namespace pathfinder::civilization
