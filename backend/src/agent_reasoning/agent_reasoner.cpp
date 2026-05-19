#include "pathfinder/agent_reasoning/agent_reasoner.h"
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/reaction/reaction_fixtures.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <unordered_map>

namespace pathfinder::agent_reasoning {

using pathfinder::condition::ConditionEvaluationContext;
using pathfinder::condition::ConditionExpressionEvaluator;
using pathfinder::condition::ConditionExpressionRef;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::h5_dialog::DialogActionKind;
using pathfinder::knowledge::KnowledgeClaim;
using pathfinder::knowledge::KnowledgeOwnerKind;
using pathfinder::knowledge::KnowledgeRelationType;
using pathfinder::knowledge::KnowledgeStatus;
using pathfinder::world_interaction::AgentAutonomyActionKind;
using pathfinder::world_interaction::AgentAutonomyResult;
using pathfinder::world_interaction::InteractionFailureKind;
using pathfinder::world_interaction::WorldChange;
using pathfinder::world_interaction::WorldChangeKind;
using pathfinder::world_interaction::WorldInteractionInput;
using pathfinder::world_interaction::WorldSnapshot;

namespace {

struct PlannerState {
    uint32_t expanded_count{0};
    bool cycle_detected{false};
    bool limit_reached{false};
};

Result<void> failVoid(const std::string& message) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, message));
}

std::string idValue(const KnowledgeId& id) {
    return id.empty() ? "knowledge.synthetic" : id.value();
}

std::string displayForObject(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.objects_by_key.find(key);
    if (it != snapshot.objects_by_key.end() && !it->second.display_name_zh_cn.empty()) return it->second.display_name_zh_cn;
    if (key == "red_berry") return "红果";
    if (key == "rotten_berry") return "腐烂红果";
    if (key == "dry_grass") return "干草";
    if (key == "camp_fire") return "火堆";
    if (key == "torch") return "火把";
    if (key == "wood") return "木头";
    if (key == "wood_processed") return "处理好的木头";
    if (key == "axe") return "斧头";
    if (key == "whetstone") return "磨石";
    if (key == "house") return "房子";
    return key;
}

int quantityOf(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.objects_by_key.find(key);
    int quantity = it == snapshot.objects_by_key.end() ? 0 : it->second.quantity;
    for (const auto& [_, actor] : snapshot.actors_by_key) {
        quantity += static_cast<int>(std::count(actor.held_object_keys.begin(), actor.held_object_keys.end(), key));
    }
    return quantity;
}

double numericStateOf(const WorldSnapshot& snapshot, const std::string& object_key, const std::string& state_key) {
    auto it = snapshot.objects_by_key.find(object_key);
    if (it == snapshot.objects_by_key.end()) return 0.0;
    auto state_it = it->second.numeric_states.find(state_key);
    if (state_it == it->second.numeric_states.end()) return 0.0;
    return state_it->second;
}

std::string safeFactForQuantity(const std::string& key, int required) {
    return "object.quantity." + key + ".gte." + std::to_string(required);
}

std::string safeFactForState(const std::string& object_key, const std::string& state_key, int required) {
    return "object.state." + object_key + "." + state_key + ".gte." + std::to_string(required);
}

bool evaluateFact(const std::vector<std::string>& facts, const std::string& fact) {
    ConditionEvaluationContext context;
    context.context_type = "agent_reasoning";
    context.safe_context_keys = facts;
    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:test:eq:" + fact;
    ConditionExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(ref, context);
    return result.is_ok() && result.value().matched;
}

std::vector<std::string> safeFactsFromSnapshot(const WorldSnapshot& snapshot) {
    std::vector<std::string> facts;
    for (const auto& [key, object] : snapshot.objects_by_key) {
        for (int threshold = 1; threshold <= 6; ++threshold) {
            if (object.quantity >= threshold) facts.push_back(safeFactForQuantity(key, threshold));
        }
        for (const auto& [state_key, value] : object.numeric_states) {
            for (int threshold = 1; threshold <= 6; ++threshold) {
                if (value >= threshold) facts.push_back(safeFactForState(key, state_key, threshold));
                if (key == "wood" && state_key == "processed" && value >= threshold) facts.push_back(safeFactForQuantity("wood_processed", threshold));
            }
        }
        for (const auto& tag : object.state_tags) {
            facts.push_back("object.tag." + key + "." + tag);
        }
    }
    std::unordered_map<std::string, int> held_quantities;
    for (const auto& [_, actor] : snapshot.actors_by_key) {
        for (const auto& held_object_key : actor.held_object_keys) {
            ++held_quantities[held_object_key];
        }
    }
    for (const auto& [held_object_key, held_quantity] : held_quantities) {
        for (int threshold = 1; threshold <= 6; ++threshold) {
            if (held_quantity >= threshold) {
                const auto fact = safeFactForQuantity(held_object_key, threshold);
                if (std::find(facts.begin(), facts.end(), fact) == facts.end()) facts.push_back(fact);
            }
        }
    }
    for (const auto& [key, threat] : snapshot.threats_by_key) {
        if (threat.active && !threat.resolved) facts.push_back("threat.active." + key);
        if (threat.level >= 50.0) facts.push_back("threat.level." + key + ".gte.50");
    }
    return facts;
}

void addUniqueFact(std::vector<std::string>& facts, const std::string& fact) {
    if (std::find(facts.begin(), facts.end(), fact) == facts.end()) facts.push_back(fact);
}

int highestSatisfiedQuantity(const std::vector<std::string>& facts, const std::string& key) {
    int highest = 0;
    for (int threshold = 1; threshold <= 6; ++threshold) {
        if (evaluateFact(facts, safeFactForQuantity(key, threshold))) highest = threshold;
    }
    return highest;
}

void addQuantityProgress(std::vector<std::string>& facts, const std::string& key, int delta) {
    const int next = std::min(6, highestSatisfiedQuantity(facts, key) + delta);
    for (int threshold = 1; threshold <= next; ++threshold) addUniqueFact(facts, safeFactForQuantity(key, threshold));
}

void applyExpectedFacts(const ActionCandidate& candidate, std::vector<std::string>& facts) {
    for (const auto& delta : candidate.semantics.state_deltas) {
        if (delta.domain == "object" && delta.value > 0.0) {
            addQuantityProgress(facts, delta.key, static_cast<int>(std::max(1.0, delta.value)));
        } else if (delta.domain == "object_state" && delta.value > 0.0) {
            const auto pos = delta.key.rfind('.');
            if (pos != std::string::npos) {
                const auto object_key = delta.key.substr(0, pos);
                const auto state_key = delta.key.substr(pos + 1);
                const int required = static_cast<int>(std::max(1.0, delta.value));
                for (int threshold = 1; threshold <= std::min(6, required); ++threshold) {
                    addUniqueFact(facts, safeFactForState(object_key, state_key, threshold));
                }
            }
        }
    }
}

PlanPrecondition precondition(std::string domain, std::string key, std::string required, bool can_plan) {
    PlanPrecondition item;
    item.condition_id = "condition." + domain + "." + key + "." + required;
    item.condition_expression = "condition:test:eq:" + domain + "." + key + "." + required;
    item.missing_domain = std::move(domain);
    item.missing_key = std::move(key);
    item.required_value = std::move(required);
    item.can_be_planned = can_plan;
    return item;
}

PlanPrecondition quantityPrecondition(const std::string& key, int required, bool can_plan) {
    return precondition("object.quantity", key, "gte." + std::to_string(required), can_plan);
}

PlanPrecondition statePrecondition(const std::string& object_key, const std::string& state_key, int required, bool can_plan) {
    return precondition("object.state", object_key + "." + state_key, "gte." + std::to_string(required), can_plan);
}

std::vector<PlanPrecondition> requiredPreconditionsFor(const ActionCandidate& candidate) {
    EffectExecutionSpecRegistry specs;
    ExecutionConditionResolver resolver;
    auto resolved = resolver.preconditionsForEffect(candidate.effect_key, specs, pathfinder::reaction::fixtures::coreP28Rules());
    if (resolved.is_ok() && !resolved.value().empty()) return resolved.value();
    if (!candidate.object_key.empty()) return {quantityPrecondition(candidate.object_key, 1, false)};
    return {};
}

std::string producedMissingKey(const EffectSemantics& semantics) {
    for (const auto& delta : semantics.state_deltas) {
        if (delta.domain == "object") return delta.key;
        if (delta.domain == "object_state") return delta.key;
        if (delta.domain == "capability") return delta.key;
    }
    return {};
}

bool producesMissing(const ActionCandidate& candidate, const PlanPrecondition& precondition) {
    for (const auto& delta : candidate.semantics.state_deltas) {
        if (precondition.missing_domain == "object.quantity" && delta.domain == "object" && delta.key == precondition.missing_key) return true;
        if (precondition.missing_domain == "object.state" && delta.domain == "object_state" && delta.key == precondition.missing_key) return true;
        if (precondition.missing_domain == "capability" && delta.domain == "capability" && delta.key == precondition.missing_key) return true;
    }
    return false;
}

PlanStepKind stepKindForCandidate(const ActionCandidate& candidate) {
    if (candidate.semantics.semantic_kind == EffectSemanticKind::ObjectStateDelta) return PlanStepKind::RestoreTool;
    if (candidate.semantics.semantic_kind == EffectSemanticKind::CapabilityDelta) return PlanStepKind::BuildStructure;
    if (candidate.semantics.semantic_kind == EffectSemanticKind::ObjectQuantityDelta) return PlanStepKind::PrepareObject;
    return PlanStepKind::DirectAction;
}

std::string executionObjectForCandidate(const ActionCandidate& candidate) {
    if (!candidate.object_key.empty()) return candidate.object_key;
    for (const auto& precondition : candidate.blocking_conditions) {
        if (precondition.missing_domain == "object.quantity") return precondition.missing_key;
    }
    return candidate.object_key;
}

std::string explanationForStep(const ActionCandidate& candidate, const WorldSnapshot& snapshot) {
    const auto object = candidate.object_key.empty() ? std::string("当前对象") : displayForObject(snapshot, candidate.object_key);
    const auto target = candidate.target_key.has_value() ? "，目标是" + displayForObject(snapshot, *candidate.target_key) : std::string{};
    return "执行已知行动：" + object + " -> " + candidate.semantics.display_zh_cn + target + "。";
}

AgentPlanStep stepFromCandidate(const ActionCandidate& candidate, const WorldSnapshot& snapshot, size_t index) {
    AgentPlanStep step;
    step.step_id = "step." + std::to_string(index) + "." + candidate.effect_key;
    step.kind = stepKindForCandidate(candidate);
    step.actor_key = candidate.actor_key;
    step.object_key = executionObjectForCandidate(candidate);
    step.target_key = candidate.target_key;
    step.action_key = candidate.action_key;
    step.effect_key = candidate.effect_key;
    step.expected_semantics = {candidate.semantics};
    step.estimated_ticks = candidate.semantics.time_cost;
    step.risk_score = candidate.semantics.risk_score;
    step.explanation_zh_cn = explanationForStep(candidate, snapshot);
    return step;
}

bool statusAtLeast(KnowledgeStatus status, KnowledgeStatus floor) {
    auto rank = [](KnowledgeStatus value) {
        switch (value) {
            case KnowledgeStatus::Candidate: return 1;
            case KnowledgeStatus::Hypothesis: return 2;
            case KnowledgeStatus::Active: return 3;
            case KnowledgeStatus::Teachable: return 4;
            case KnowledgeStatus::Shared: return 5;
            case KnowledgeStatus::Operational: return 6;
            case KnowledgeStatus::TestOnly: return 7;
            default: return 0;
        }
    };
    return rank(status) >= rank(floor);
}

bool ownerMatchesActor(const KnowledgeClaim& claim, const std::string& actor_key) {
    if (claim.owner.kind == KnowledgeOwnerKind::Tribe) return true;
    if (claim.owner.kind == KnowledgeOwnerKind::Agent || claim.owner.kind == KnowledgeOwnerKind::Actor) {
        return claim.owner.entity_id.value() == actor_key || claim.owner.external_key == actor_key;
    }
    if (claim.owner.kind == KnowledgeOwnerKind::TestOnly) return true;
    return false;
}

KnowledgeUsability usabilityFor(const KnowledgeClaim& claim, const EffectSemantics& semantics, const ReasoningOptions& options, double urgency) {
    if (semantics.risk_score >= 70.0 || claim.predicate.effect_key == "poison") return KnowledgeUsability::Dangerous;
    if (claim.status == KnowledgeStatus::Deprecated || claim.status == KnowledgeStatus::Disproven) return KnowledgeUsability::Conflicted;
    if (claim.confidence.conflict_count > 0) return KnowledgeUsability::Conflicted;
    if (statusAtLeast(claim.status, semantics.confidence_floor) && claim.confidence.confidence >= options.min_confidence_score) return KnowledgeUsability::Usable;
    if ((claim.status == KnowledgeStatus::Candidate || claim.status == KnowledgeStatus::Hypothesis) &&
        (options.allow_tentative_knowledge || (options.emergency_allows_tentative && urgency >= 80.0)) &&
        claim.confidence.confidence >= std::min(0.25, options.min_confidence_score)) {
        return KnowledgeUsability::Tentative;
    }
    return KnowledgeUsability::Conflicted;
}

bool terminalEffectSolvesGoal(const AgentGoal& goal, const EffectSemantics& semantics) {
    const bool has_affinity = std::find(semantics.goal_affinities.begin(), semantics.goal_affinities.end(), goal.kind) != semantics.goal_affinities.end();
    if (!has_affinity) return false;
    for (const auto& delta : semantics.state_deltas) {
        if (goal.kind == AgentGoalKind::ReduceThreat && delta.domain == "threat" && delta.value < 0.0) return true;
        if (goal.kind == AgentGoalKind::ReduceHunger && delta.domain == "need" && delta.key == "hunger" && delta.value < 0.0) return true;
        if (goal.kind == AgentGoalKind::ReduceCold && delta.domain == "need" && delta.key == "cold" && delta.value < 0.0) return true;
        if (goal.kind == AgentGoalKind::IncreaseWarmth && delta.key == "warmth" && delta.value > 0.0) return true;
        if (goal.kind == AgentGoalKind::IncreaseShelterCapacity && delta.domain == "capability" && delta.key == "shelter_capacity" && delta.value > 0.0) return true;
        if (goal.kind == AgentGoalKind::AcquireObject && delta.domain == "object" && delta.value > 0.0) return true;
        if (goal.kind == AgentGoalKind::RestoreToolState && delta.domain == "object_state" && delta.value > 0.0) return true;
        if (goal.kind == AgentGoalKind::MaintainFire && delta.domain == "object" && delta.key == "camp_fire" && delta.value > 0.0) return true;
    }
    return false;
}

bool goalMatches(const AgentGoal& goal, const EffectSemantics& semantics) {
    return terminalEffectSolvesGoal(goal, semantics);
}

std::optional<std::string> targetFromClaim(const KnowledgeClaim& claim, const AgentGoal& goal) {
    if (goal.target_key) return goal.target_key;
    if (!claim.subject.related_subject_ids.empty()) return claim.subject.related_subject_ids.front();
    return std::nullopt;
}

std::vector<ActionCandidate> buildCandidatesCore(
    const std::string& actor_key,
    const std::optional<AgentGoal>& goal,
    const WorldSnapshot& snapshot,
    const std::vector<KnowledgeClaim>& claims,
    const ReasoningOptions& options) {
    EffectSemanticsRegistry registry;
    std::vector<ActionCandidate> candidates;
    for (const auto& claim : claims) {
        if (!ownerMatchesActor(claim, actor_key)) continue;
        if (!claim.projection_flags.usable_by_ai) continue;
        if (claim.predicate.relation_type != KnowledgeRelationType::HasEffect && claim.predicate.relation_type != KnowledgeRelationType::Produces && claim.predicate.relation_type != KnowledgeRelationType::IsUsableFor) continue;
        if (claim.predicate.effect_key.empty() || claim.predicate.action_key.empty() || claim.subject.subject_id.empty()) continue;
        auto semantics = registry.findByEffectKey(claim.predicate.effect_key);
        if (semantics.is_error()) continue;
        if (goal && !goalMatches(*goal, semantics.value())) continue;
        const auto usability = usabilityFor(claim, semantics.value(), options, goal ? goal->urgency : 0.0);
        if (usability != KnowledgeUsability::Usable && usability != KnowledgeUsability::Tentative) continue;
        ActionCandidate candidate;
        candidate.candidate_id = "candidate." + actor_key + "." + claim.predicate.effect_key + "." + std::to_string(candidates.size());
        candidate.actor_key = actor_key;
        candidate.source_knowledge_id = idValue(claim.knowledge_id);
        candidate.object_key = claim.subject.subject_id;
        candidate.target_key = goal ? targetFromClaim(claim, *goal) : (!claim.subject.related_subject_ids.empty() ? std::optional<std::string>(claim.subject.related_subject_ids.front()) : std::nullopt);
        candidate.action_key = claim.predicate.action_key;
        candidate.effect_key = claim.predicate.effect_key;
        candidate.semantics = semantics.value();
        candidate.knowledge_usability = usability;
        candidate.knowledge_confidence = std::clamp(claim.confidence.confidence, 0.0, 1.0);
        const auto facts = safeFactsFromSnapshot(snapshot);
        bool direct = true;
        for (const auto& required : requiredPreconditionsFor(candidate)) {
            std::string fact;
            if (required.missing_domain == "object.quantity") {
                fact = safeFactForQuantity(required.missing_key, std::stoi(required.required_value.substr(4)));
            } else if (required.missing_domain == "object.state") {
                const auto pos = required.missing_key.rfind('.');
                fact = safeFactForState(required.missing_key.substr(0, pos), required.missing_key.substr(pos + 1), std::stoi(required.required_value.substr(4)));
            } else {
                fact = required.missing_domain + "." + required.missing_key + "." + required.required_value;
            }
            if (!evaluateFact(facts, fact)) {
                direct = false;
                candidate.blocking_conditions.push_back(required);
            }
        }
        candidate.is_directly_executable = direct;
        candidates.push_back(std::move(candidate));
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.semantics.risk_score != rhs.semantics.risk_score) return lhs.semantics.risk_score < rhs.semantics.risk_score;
        if (lhs.semantics.time_cost != rhs.semantics.time_cost) return lhs.semantics.time_cost < rhs.semantics.time_cost;
        return lhs.candidate_id < rhs.candidate_id;
    });
    return candidates;
}

bool preconditionSatisfied(const std::vector<std::string>& facts, const PlanPrecondition& required) {
    if (required.missing_domain == "object.quantity") {
        return evaluateFact(facts, safeFactForQuantity(required.missing_key, std::stoi(required.required_value.substr(4))));
    }
    if (required.missing_domain == "object.state") {
        const auto pos = required.missing_key.rfind('.');
        return evaluateFact(facts, safeFactForState(required.missing_key.substr(0, pos), required.missing_key.substr(pos + 1), std::stoi(required.required_value.substr(4))));
    }
    return evaluateFact(facts, required.missing_domain + "." + required.missing_key + "." + required.required_value);
}

const ActionCandidate* findProducer(const std::vector<ActionCandidate>& candidates, const PlanPrecondition& required) {
    const ActionCandidate* best = nullptr;
    for (const auto& candidate : candidates) {
        if (!producesMissing(candidate, required)) continue;
        if (!best || candidate.semantics.time_cost < best->semantics.time_cost) best = &candidate;
    }
    return best;
}

Result<void> appendResolvedSteps(
    const ActionCandidate& candidate,
    const std::vector<ActionCandidate>& all_candidates,
    const WorldSnapshot& snapshot,
    const ReasoningOptions& options,
    uint32_t depth,
    std::set<std::string>& visited_effects,
    std::set<std::string>& visited_missing,
    PlannerState& state,
    std::vector<std::string>& facts,
    std::vector<AgentPlanStep>& steps,
    std::optional<std::string>& blocking_reason) {
    if (state.expanded_count >= options.max_total_expansions) {
        state.limit_reached = true;
        blocking_reason = "搜索达到上限，计划被安全停止。";
        return failVoid("search limit reached");
    }
    if (depth > options.max_depth) {
        state.limit_reached = true;
        blocking_reason = "前置链超过允许深度，计划被安全停止。";
        return failVoid("depth limit reached");
    }
    if (!visited_effects.insert(candidate.effect_key).second) {
        state.cycle_detected = true;
        blocking_reason = "检测到循环依赖，计划被阻断。";
        return failVoid("cycle detected");
    }
    ++state.expanded_count;

    for (const auto& required : requiredPreconditionsFor(candidate)) {
        if (preconditionSatisfied(facts, required)) continue;
        const std::string missing_key = required.missing_domain + ":" + required.missing_key;
        if (!visited_missing.insert(missing_key).second) {
            state.cycle_detected = true;
            blocking_reason = "检测到重复前置条件，计划被阻断。";
            visited_effects.erase(candidate.effect_key);
            return failVoid("missing key cycle detected");
        }
        if (!required.can_be_planned) {
            blocking_reason = "缺少" + required.missing_key + "，当前无法继续准备。";
            visited_effects.erase(candidate.effect_key);
            return failVoid("precondition cannot be planned");
        }
        const auto* producer = findProducer(all_candidates, required);
        if (producer && visited_effects.find(producer->effect_key) != visited_effects.end()) {
            state.cycle_detected = true;
            blocking_reason = "检测到循环依赖，计划被阻断。";
            visited_effects.erase(candidate.effect_key);
            return failVoid("producer cycle detected");
        }
        if (!producer) {
            blocking_reason = "缺少" + required.missing_key + "，也没有掌握可准备它的知识。";
            visited_effects.erase(candidate.effect_key);
            return failVoid("producer not found");
        }
        if (producer->effect_key == candidate.effect_key) {
            state.cycle_detected = true;
            blocking_reason = "检测到循环依赖，计划被阻断。";
            visited_effects.erase(candidate.effect_key);
            return failVoid("self producer cycle detected");
        }
        const int required_count = required.missing_domain == "object.quantity" && required.required_value.rfind("gte.", 0) == 0 ? std::stoi(required.required_value.substr(4)) : 1;
        const int current_count = required.missing_domain == "object.quantity" ? quantityOf(snapshot, required.missing_key) : 0;
        const int repeat_count = std::max(1, required_count - current_count);
        for (int index = 0; index < repeat_count; ++index) {
            auto before_size = steps.size();
            std::set<std::string> branch_visited_effects = visited_effects;
            std::set<std::string> branch_visited_missing = visited_missing;
            auto resolved = appendResolvedSteps(*producer, all_candidates, snapshot, options, depth + 1, branch_visited_effects, branch_visited_missing, state, facts, steps, blocking_reason);
            if (resolved.is_error()) {
                visited_effects.erase(candidate.effect_key);
                return resolved;
            }
            if (index > 0 && steps.size() > before_size) {
                for (size_t step_index = before_size; step_index < steps.size(); ++step_index) {
                    steps[step_index].step_id += ".repeat" + std::to_string(index);
                }
            }
        }
        visited_missing.erase(missing_key);
    }

    if (steps.size() >= options.max_plan_steps) {
        state.limit_reached = true;
        blocking_reason = "计划步骤超过上限，计划被安全停止。";
        visited_effects.erase(candidate.effect_key);
        return failVoid("plan step limit reached");
    }
    steps.push_back(stepFromCandidate(candidate, snapshot, steps.size() + 1));
    applyExpectedFacts(candidate, facts);
    visited_effects.erase(candidate.effect_key);
    return Result<void>::ok();
}

std::string goalExplanation(AgentGoalKind kind) {
    switch (kind) {
        case AgentGoalKind::ReduceHunger: return "降低饥饿";
        case AgentGoalKind::ReduceCold: return "缓解寒冷";
        case AgentGoalKind::IncreaseWarmth: return "提升保暖";
        case AgentGoalKind::IncreaseShelterCapacity: return "提升庇护容量";
        case AgentGoalKind::ReduceThreat: return "降低威胁";
        case AgentGoalKind::RestoreToolState: return "恢复工具状态";
        case AgentGoalKind::AcquireObject: return "获得所需物品";
        default: return "改善当前目标";
    }
}

WorldChange makeChange(std::string id, WorldChangeKind kind, std::string summary) {
    WorldChange change;
    change.change_id = std::move(id);
    change.kind = kind;
    change.player_summary_zh_cn = std::move(summary);
    return change;
}

} // namespace

Result<std::vector<AgentGoal>> AgentGoalGenerator::generateGoals(const GoalGenerationInput& input) const {
    auto need_valid = input.need_state.validateBasic();
    if (need_valid.is_error()) return Result<std::vector<AgentGoal>>::fail(need_valid.errors());
    std::vector<AgentGoal> goals;
    const auto tick = input.world_snapshot.turn_index;
    auto add_goal = [&](AgentGoalKind kind, double urgency, double desired_delta, std::vector<std::string> sources, std::optional<std::string> target = std::nullopt) {
        AgentGoal goal;
        goal.goal_id = "goal." + input.need_state.actor_key + "." + toString(kind) + "." + std::to_string(tick);
        goal.actor_key = input.need_state.actor_key;
        goal.kind = kind;
        goal.target_key = std::move(target);
        goal.urgency = std::clamp(urgency, 0.0, 100.0);
        goal.desired_delta = desired_delta;
        goal.source_keys = std::move(sources);
        if (goal.urgency >= 80.0) goal.expires_after_ticks = 1;
        goals.push_back(std::move(goal));
    };
    if (input.need_state.hunger >= 60.0) add_goal(AgentGoalKind::ReduceHunger, input.need_state.hunger, -30.0, {"need:hunger"});
    if (input.need_state.cold >= 50.0) {
        add_goal(AgentGoalKind::ReduceCold, input.need_state.cold, -40.0, {"need:cold"});
        add_goal(AgentGoalKind::IncreaseWarmth, input.need_state.cold, 40.0, {"need:cold", "weather:cold"});
    }
    if (input.need_state.fear >= 25.0) add_goal(AgentGoalKind::ReduceThreat, input.need_state.fear, -50.0, {"need:fear"});
    for (const auto& [key, threat] : input.world_snapshot.threats_by_key) {
        if (threat.active && !threat.resolved && threat.level >= 25.0) add_goal(AgentGoalKind::ReduceThreat, threat.level, -100.0, {"threat:" + key}, key);
    }
    auto tribe = input.world_snapshot.objects_by_key.find("tribe_status");
    if (tribe != input.world_snapshot.objects_by_key.end()) {
        const double population = numericStateOf(input.world_snapshot, "tribe_status", "population");
        const double shelter = numericStateOf(input.world_snapshot, "tribe_status", "shelter_capacity");
        if (population > shelter && input.need_state.cold >= 30.0) {
            add_goal(AgentGoalKind::IncreaseShelterCapacity, std::clamp(50.0 + (population - shelter) * 10.0, 0.0, 100.0), population - shelter, {"tribe:shelter_capacity", "weather:cold"}, "house");
        }
    }
    std::sort(goals.begin(), goals.end(), [](const auto& lhs, const auto& rhs) { return lhs.urgency > rhs.urgency; });
    return Result<std::vector<AgentGoal>>::ok(std::move(goals));
}

Result<std::vector<ActionCandidate>> KnowledgeActionCandidateBuilder::buildCandidates(const CandidateBuildInput& input) const {
    auto options_valid = input.options.validateBasic();
    if (options_valid.is_error()) return Result<std::vector<ActionCandidate>>::fail(options_valid.errors());
    auto candidates = buildCandidatesCore(input.actor_key, input.goal, input.world_snapshot, input.agent_knowledge, input.options);
    if (candidates.size() > input.options.max_candidates_per_goal) candidates.resize(input.options.max_candidates_per_goal);
    for (const auto& candidate : candidates) {
        auto valid = candidate.validateBasic();
        if (valid.is_error()) return Result<std::vector<ActionCandidate>>::fail(valid.errors());
    }
    return Result<std::vector<ActionCandidate>>::ok(std::move(candidates));
}

Result<AgentPlan> PlanPreconditionResolver::resolvePreconditions(const PreconditionResolveInput& input) const {
    AgentPlan plan;
    plan.plan_id = "plan." + input.candidate.actor_key + "." + input.candidate.effect_key;
    plan.actor_key = input.candidate.actor_key;
    plan.goal = input.goal;
    plan.selection_reason = PlanSelectionReason::HighestUtility;

    PlannerState state;
    std::set<std::string> visited_effects;
    std::set<std::string> visited_missing;
    std::optional<std::string> blocking_reason;
    auto facts = safeFactsFromSnapshot(input.world_snapshot);
    auto resolved = appendResolvedSteps(input.candidate, input.all_candidates, input.world_snapshot, input.options, 1, visited_effects, visited_missing, state, facts, plan.steps, blocking_reason);
    if (resolved.is_error()) {
        plan.blocked = true;
        plan.blocking_reason_zh_cn = blocking_reason.value_or("计划前置条件不足。");
        AgentPlanStep abort;
        abort.step_id = "step.abort." + input.candidate.effect_key;
        abort.kind = PlanStepKind::Abort;
        abort.actor_key = input.candidate.actor_key;
        abort.object_key = input.candidate.object_key.empty() ? "unknown" : input.candidate.object_key;
        abort.action_key = input.candidate.action_key.empty() ? "wait" : input.candidate.action_key;
        abort.effect_key = input.candidate.effect_key;
        abort.expected_semantics = {input.candidate.semantics};
        abort.estimated_ticks = 0;
        abort.risk_score = 0.0;
        abort.explanation_zh_cn = *plan.blocking_reason_zh_cn;
        plan.steps.push_back(std::move(abort));
        plan.selection_reason = state.cycle_detected ? PlanSelectionReason::CycleDetected : (state.limit_reached ? PlanSelectionReason::SearchLimitReached : PlanSelectionReason::BlockedByCondition);
    }
    for (auto& step : plan.steps) {
        if (step.step_id.rfind("step.", 0) != 0) step.step_id = "step." + step.step_id;
    }
    for (const auto& step : plan.steps) {
        plan.total_estimated_ticks += step.estimated_ticks;
        plan.total_risk_score += step.risk_score;
    }
    plan.trace_keys.push_back("reasoning.expanded:" + std::to_string(state.expanded_count));
    if (state.cycle_detected) plan.trace_keys.push_back("reasoning.cycle_detected");
    if (state.limit_reached) plan.trace_keys.push_back("reasoning.limit_reached");
    auto valid = plan.validateBasic();
    if (valid.is_error()) return Result<AgentPlan>::fail(valid.errors());
    return Result<AgentPlan>::ok(std::move(plan));
}

Result<PlanScore> PlanUtilityScorer::score(const PlanScoreInput& input) const {
    PlanScore score;
    if (input.plan.blocked) {
        score.utility_score = -100000.0;
        score.reason = input.plan.selection_reason;
        score.explanation_zh_cn = input.plan.blocking_reason_zh_cn.value_or("计划被阻塞。");
        return Result<PlanScore>::ok(score);
    }
    double expected_benefit = std::abs(input.goal.desired_delta);
    if (!input.plan.steps.empty()) {
        for (const auto& delta : input.plan.steps.back().expected_semantics.front().state_deltas) {
            if ((input.goal.kind == AgentGoalKind::ReduceHunger && delta.key == "hunger") ||
                (input.goal.kind == AgentGoalKind::ReduceCold && delta.key == "cold") ||
                (input.goal.kind == AgentGoalKind::ReduceThreat && delta.domain == "threat") ||
                (input.goal.kind == AgentGoalKind::IncreaseShelterCapacity && delta.key == "shelter_capacity") ||
                (input.goal.kind == AgentGoalKind::IncreaseWarmth && (delta.key == "warmth" || delta.key == "cold"))) {
                expected_benefit = std::max(expected_benefit, std::abs(delta.value));
            }
        }
    }
    const double time_weight = input.goal.urgency >= 80.0 ? 8.0 : 3.0;
    const double risk_weight = input.need_state.health <= 30.0 ? 3.0 : 1.5;
    const double durability_bonus = input.goal.urgency >= 80.0 ? 0.0 : (input.plan.steps.back().effect_key == "build_house" ? 90.0 : 0.0);
    double emergency_goal_bonus = input.goal.urgency >= 80.0 && input.goal.kind == AgentGoalKind::ReduceCold ? 1000.0 : 0.0;
    if (input.goal.urgency >= 80.0 && (input.goal.kind == AgentGoalKind::ReduceCold || input.goal.kind == AgentGoalKind::IncreaseWarmth)) {
        if (input.plan.steps.front().effect_key == "ignite_fire" || input.plan.steps.back().effect_key == "provide_warmth") emergency_goal_bonus += 2000.0;
        if (input.plan.steps.back().effect_key == "build_house") emergency_goal_bonus -= 1200.0;
    }
    const double confidence_bonus = std::max(0.0, 20.0 - static_cast<double>(input.plan.steps.size()) * 2.0);
    score.utility_score = input.goal.urgency * expected_benefit
        - static_cast<double>(input.plan.total_estimated_ticks) * time_weight
        - input.plan.total_risk_score * risk_weight
        - static_cast<double>(input.plan.steps.size()) * 6.0
        + confidence_bonus
        + durability_bonus
        + emergency_goal_bonus;
    score.reason = input.plan.steps.size() == 1 ? PlanSelectionReason::ShortestChain : PlanSelectionReason::HighestUtility;
    if (input.goal.urgency >= 80.0 && input.plan.total_estimated_ticks <= 2) score.reason = PlanSelectionReason::EmergencyOverride;
    score.explanation_zh_cn = "选择" + input.plan.steps.back().explanation_zh_cn;
    return Result<PlanScore>::ok(score);
}

Result<ReasoningResult> AgentReasoner::reason(const ReasoningRequest& request) const {
    auto request_valid = request.validateBasic();
    if (request_valid.is_error()) return Result<ReasoningResult>::fail(request_valid.errors());

    ReasoningResult result;
    result.trace.trace_id = "trace." + request.request_id;
    EffectSemanticsRegistry registry;
    auto registry_valid = registry.validateAll();
    if (registry_valid.is_error()) return Result<ReasoningResult>::fail(registry_valid.errors());

    AgentGoalGenerator goal_generator;
    auto goals = goal_generator.generateGoals({request.need_state, request.world_snapshot});
    if (goals.is_error()) return Result<ReasoningResult>::fail(goals.errors());
    result.goals = goals.value();
    result.trace.goal_count = static_cast<uint32_t>(result.goals.size());
    if (result.goals.empty()) {
        result.safe_explanation_zh_cn = "当前没有需要立刻处理的目标。";
        result.trace.public_reason_keys.push_back("reasoning.no_goal");
        return Result<ReasoningResult>::ok(std::move(result));
    }

    const auto all_candidates = buildCandidatesCore(request.actor_key, std::nullopt, request.world_snapshot, request.agent_knowledge, request.options);
    result.trace.candidate_count = static_cast<uint32_t>(all_candidates.size());

    KnowledgeActionCandidateBuilder builder;
    PlanPreconditionResolver resolver;
    PlanUtilityScorer scorer;
    std::vector<AgentPlan> all_plans;
    for (const auto& goal : result.goals) {
        auto candidates = builder.buildCandidates({request.actor_key, goal, request.world_snapshot, request.agent_knowledge, request.options});
        if (candidates.is_error()) return Result<ReasoningResult>::fail(candidates.errors());
        for (const auto& candidate : candidates.value()) {
            auto plan = resolver.resolvePreconditions({goal, candidate, all_candidates, request.world_snapshot, request.options});
            if (plan.is_error()) return Result<ReasoningResult>::fail(plan.errors());
            auto scored = scorer.score({goal, plan.value(), request.need_state});
            if (scored.is_error()) return Result<ReasoningResult>::fail(scored.errors());
            auto plan_value = plan.value();
            plan_value.utility_score = scored.value().utility_score;
            if (!plan_value.blocked) plan_value.selection_reason = scored.value().reason;
            plan_value.plan_id = "plan." + request.actor_key + "." + toString(goal.kind) + "." + std::to_string(all_plans.size());
            all_plans.push_back(std::move(plan_value));
        }
    }

    for (const auto& plan : all_plans) {
        for (const auto& key : plan.trace_keys) {
            if (key == "reasoning.cycle_detected") result.trace.cycle_detected = true;
            if (key == "reasoning.limit_reached") result.trace.limit_reached = true;
            if (key.rfind("reasoning.expanded:", 0) == 0) result.trace.expanded_count += static_cast<uint32_t>(std::stoul(key.substr(19)));
        }
    }
    std::sort(all_plans.begin(), all_plans.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.blocked != rhs.blocked) return !lhs.blocked;
        if (lhs.utility_score != rhs.utility_score) return lhs.utility_score > rhs.utility_score;
        if (lhs.total_estimated_ticks != rhs.total_estimated_ticks) return lhs.total_estimated_ticks < rhs.total_estimated_ticks;
        return lhs.plan_id < rhs.plan_id;
    });

    if (all_plans.empty() || all_plans.front().blocked) {
        result.ok = false;
        result.rejected_plans = std::move(all_plans);
        result.safe_explanation_zh_cn = all_plans.empty() ? "知识不足，暂时不能形成计划。" : all_plans.front().blocking_reason_zh_cn.value_or("计划被阻塞。");
        result.trace.public_reason_keys.push_back("reasoning.blocked");
        auto valid = result.validateBasic();
        if (valid.is_error()) return Result<ReasoningResult>::fail(valid.errors());
        return Result<ReasoningResult>::ok(std::move(result));
    }

    result.ok = true;
    result.selected_plan = all_plans.front();
    result.trace.selected_plan_id = result.selected_plan->plan_id;
    result.safe_explanation_zh_cn = result.selected_plan->steps.front().explanation_zh_cn;
    if ((result.selected_plan->goal.kind == AgentGoalKind::ReduceCold || result.selected_plan->goal.kind == AgentGoalKind::IncreaseWarmth) && result.selected_plan->steps.front().effect_key == "ignite_fire") {
        result.safe_explanation_zh_cn = "同伴选择生火，因为它能最快缓解寒冷。";
    } else if (result.selected_plan->goal.kind == AgentGoalKind::ReduceHunger) {
        result.safe_explanation_zh_cn = "同伴选择" + result.selected_plan->steps.front().explanation_zh_cn;
    } else if (result.selected_plan->goal.kind == AgentGoalKind::ReduceThreat) {
        result.safe_explanation_zh_cn = "同伴选择火把，因为它能直接降低野兽威胁。";
    }
    result.trace.public_reason_keys.push_back("goal:" + toString(result.selected_plan->goal.kind));
    result.trace.public_reason_keys.push_back("step:" + result.selected_plan->steps.front().effect_key);
    for (size_t index = 1; index < all_plans.size(); ++index) result.rejected_plans.push_back(all_plans[index]);
    auto valid = result.validateBasic();
    if (valid.is_error()) return Result<ReasoningResult>::fail(valid.errors());
    return Result<ReasoningResult>::ok(std::move(result));
}

Result<AgentAutonomyResult> AgentPlanExecutorAdapter::toAutonomyResult(const AgentPlan& plan, const WorldSnapshot& snapshot) const {
    EffectExecutionSpecRegistry specs;
    AgentExecutionCoordinator coordinator;
    auto result = coordinator.executePlan(snapshot, plan, AgentStepExecutionMode::ExecuteOneStep, specs);
    if (result.is_error()) return result;
    result.value().trace_keys.push_back("reasoning.plan:" + plan.plan_id);
    return result;
}

Result<WorldInteractionInput> AgentPlanExecutorAdapter::toWorldInteractionInput(const AgentPlanStep& step, const WorldSnapshot& snapshot) const {
    (void)snapshot;
    auto valid = step.validateBasic();
    if (valid.is_error()) return Result<WorldInteractionInput>::fail(valid.errors());
    WorldInteractionInput input;
    input.interaction_id = "reasoning." + step.step_id;
    input.actor_key = step.actor_key;
    input.object_key = step.object_key;
    input.target_object_key = step.target_key.value_or("");
    input.effect_key = step.effect_key;
    input.feedback_key = "reasoning.plan_step";
    input.reason_keys = {"reasoning.step:" + step.effect_key};
    input.action = step.action_key == "eat" ? DialogActionKind::Eat : DialogActionKind::Use;
    return Result<WorldInteractionInput>::ok(std::move(input));
}

Result<ReasoningProjection> ReasoningProjectionMapper::buildProjection(const ReasoningResult& result) const {
    auto valid = result.validateBasic();
    if (valid.is_error()) return Result<ReasoningProjection>::fail(valid.errors());
    ReasoningProjection projection;
    if (result.selected_plan) {
        projection.current_goal_zh_cn = goalExplanation(result.selected_plan->goal.kind);
        projection.next_step_zh_cn = result.selected_plan->steps.front().explanation_zh_cn;
        projection.public_reason_lines_zh_cn.push_back(result.safe_explanation_zh_cn);
        if (result.selected_plan->blocked && result.selected_plan->blocking_reason_zh_cn) projection.blocked_reason_zh_cn = *result.selected_plan->blocking_reason_zh_cn;
    } else {
        projection.current_goal_zh_cn = "暂无可执行目标";
        projection.blocked_reason_zh_cn = result.safe_explanation_zh_cn;
        projection.public_reason_lines_zh_cn.push_back(result.safe_explanation_zh_cn);
    }
    auto projection_valid = projection.validateBasic();
    if (projection_valid.is_error()) return Result<ReasoningProjection>::fail(projection_valid.errors());
    return Result<ReasoningProjection>::ok(std::move(projection));
}

} // namespace pathfinder::agent_reasoning
