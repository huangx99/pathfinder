#include "pathfinder/reaction/reaction_resolver.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <utility>

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ObjectId;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static const ReactionObjectRef* findObject(const ReactionInputSet& input, ReactionObjectRole role) {
    for (const auto& object : input.objects) {
        if (object.role == role) return &object;
    }
    return nullptr;
}

static bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

static bool matchesPattern(const ReactionInputSet& input, const ReactionObjectPattern& pattern) {
    const auto* object = findObject(input, pattern.role);
    if (!object) return false;
    if (pattern.definition_id && object->definition_id != *pattern.definition_id) return false;
    if (pattern.category && object->category != *pattern.category) return false;
    if (pattern.quantity_required && object->quantity <= 0) return false;
    for (const auto& tag : pattern.required_tag_keys) {
        if (!contains(object->tag_keys, tag)) return false;
    }
    for (const auto& tag : pattern.forbidden_tag_keys) {
        if (contains(object->tag_keys, tag)) return false;
    }
    return true;
}

static bool matchesPatterns(const ReactionInputSet& input, const ObjectReactionRule& rule) {
    if (input.action_kind != rule.action_kind) return false;
    for (const auto& pattern : rule.object_patterns) {
        if (!matchesPattern(input, pattern)) return false;
    }
    return true;
}

static Result<pathfinder::knowledge::KnowledgeCondition> makeKnowledgeCondition(
    const pathfinder::condition::ConditionEvaluationResult& evaluation) {
    pathfinder::knowledge::KnowledgeCondition condition;
    condition.condition_ref.inline_canonical_key = evaluation.canonical_condition_key;
    condition.canonical_condition_key = evaluation.canonical_condition_key;
    condition.condition_key = evaluation.canonical_condition_key;
    condition.condition_summary_key = evaluation.summary_key.empty() ? "condition.summary.generic" : evaluation.summary_key;
    auto valid = condition.validateBasic();
    if (valid.is_error()) return Result<pathfinder::knowledge::KnowledgeCondition>::fail(valid.errors());
    return Result<pathfinder::knowledge::KnowledgeCondition>::ok(std::move(condition));
}

static std::string feedbackText(const std::string& feedback_key) {
    if (feedback_key == "reaction.fire_branch_torch") return "木棍被点燃，变成了可以携带的火把。";
    if (feedback_key == "reaction.fire_wet_branch_blocked") return "潮湿的木棍没有被点燃，只冒出一点烟。";
    if (feedback_key == "reaction.water_extinguish_fire") return "水浇灭了火源。";
    if (feedback_key == "reaction.no_matching_rule") return "这两个东西暂时没有发生明显反应。";
    if (feedback_key == "reaction.condition_blocked") return "条件不满足，反应没有成功。";
    return "对象之间发生了可观察的反应。";
}

static ReactionObjectChangeDraft makeChangeDraft(
    const ReactionInputSet& input,
    const ObjectReactionRule& rule,
    const ReactionOutputTemplate& output) {
    ReactionObjectChangeDraft draft;
    draft.change_kind = output.output_kind;
    draft.role = output.target_role;
    draft.to_definition_id = output.product_definition_id;
    draft.state_key = output.state_key;
    draft.resource_key = output.resource_key;
    draft.resource_delta = output.resource_delta;
    draft.quantity_delta = output.quantity_delta;
    draft.reason_keys = {rule.rule_key};
    if (const auto* object = findObject(input, output.target_role)) {
        draft.object_id = object->object_id;
        draft.from_definition_id = object->definition_id;
    }
    return draft;
}

static Result<ReactionResult> buildReactedResult(
    const ReactionInputSet& input,
    const ObjectReactionRule& rule,
    const std::vector<pathfinder::condition::ConditionEvaluationResult>& evaluations,
    ReactionTrace trace) {

    ReactionResult result;
    result.decision = ReactionDecision::Reacted;
    result.selected_rule_key = rule.rule_key;
    result.trace = std::move(trace);
    result.trace.decision = result.decision;
    result.trace.selected_rule_key = rule.rule_key;

    for (const auto& output : rule.output_templates) {
        if (output.output_kind == ReactionOutputKind::FeedbackOnly || output.output_kind == ReactionOutputKind::KnowledgeOnly) {
            continue;
        }
        auto draft = makeChangeDraft(input, rule, output);
        auto valid = draft.validateBasic();
        if (valid.is_error()) return Result<ReactionResult>::fail(valid.errors());
        result.trace.output_summary_keys.push_back(toString(output.output_kind));
        result.object_changes.push_back(std::move(draft));
    }

    ReactionFeedbackDraft feedback;
    feedback.feedback_key = rule.feedback_key.empty() ? "reaction.reacted" : rule.feedback_key;
    feedback.safe_text = rule.feedback_text.empty() ? feedbackText(feedback.feedback_key) : rule.feedback_text;
    feedback.reason_keys = {rule.rule_key};
    auto feedback_valid = feedback.validateBasic();
    if (feedback_valid.is_error()) return Result<ReactionResult>::fail(feedback_valid.errors());
    result.feedbacks.push_back(std::move(feedback));

    if (!rule.knowledge_effect_key.empty()) {
        ReactionKnowledgeDraft knowledge;
        knowledge.knowledge_key = "reaction_knowledge_" + rule.rule_key;
        const auto* target = findObject(input, ReactionObjectRole::Material);
        if (!target) target = findObject(input, ReactionObjectRole::Target);
        knowledge.subject_id = target ? target->object_key : rule.rule_key;
        knowledge.action_key = toString(input.action_kind);
        knowledge.effect_key = rule.knowledge_effect_key;
        knowledge.reason_keys = {rule.rule_key};
        for (const auto& evaluation : evaluations) {
            if (!evaluation.matched) continue;
            auto condition = makeKnowledgeCondition(evaluation);
            if (condition.is_error()) return Result<ReactionResult>::fail(condition.errors());
            knowledge.conditions.push_back(std::move(condition.value()));
        }
        auto knowledge_valid = knowledge.validateBasic();
        if (knowledge_valid.is_error()) return Result<ReactionResult>::fail(knowledge_valid.errors());
        result.knowledge_drafts.push_back(std::move(knowledge));
    }

    ReactionEventDraft event;
    event.event_key = "reaction." + rule.rule_key;
    event.reason_keys = {rule.rule_key};
    auto event_valid = event.validateBasic();
    if (event_valid.is_error()) return Result<ReactionResult>::fail(event_valid.errors());
    result.events.push_back(std::move(event));

    auto valid = result.validateBasic();
    if (valid.is_error()) return Result<ReactionResult>::fail(valid.errors());
    return Result<ReactionResult>::ok(std::move(result));
}

Result<void> ReactionResult::validateBasic() const {
    if (decision == ReactionDecision::Unknown || decision == ReactionDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionResult decision invalid"));
    }
    if (containsReactionForbiddenKey(selected_rule_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionResult selected_rule_key forbidden"));
    }
    for (const auto& change : object_changes) {
        auto valid = change.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& feedback : feedbacks) {
        auto valid = feedback.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& knowledge : knowledge_drafts) {
        auto valid = knowledge.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& event : events) {
        auto valid = event.validateBasic();
        if (valid.is_error()) return valid;
    }
    return trace.validateBasic();
}

Result<ReactionResult> ObjectReactionResolver::resolve(
    const ReactionInputSet& input,
    const std::vector<ObjectReactionRule>& rules) const {

    auto input_valid = input.validateBasic();
    if (input_valid.is_error()) return Result<ReactionResult>::fail(input_valid.errors());

    ReactionTrace base_trace;
    base_trace.input_key = input.input_key;

    ReactionConditionContextBuilder builder;
    auto context_result = builder.build(input);
    if (context_result.is_error()) return Result<ReactionResult>::fail(context_result.errors());
    const auto& context = context_result.value();

    struct Candidate {
        const ObjectReactionRule* rule{nullptr};
        size_t index{0};
        std::vector<pathfinder::condition::ConditionEvaluationResult> evaluations;
        bool all_conditions_met{false};
    };

    std::vector<Candidate> candidates;
    pathfinder::condition::ConditionExpressionEvaluator evaluator;

    for (size_t index = 0; index < rules.size(); ++index) {
        const auto& rule = rules[index];
        auto rule_valid = rule.validateBasic();
        if (rule_valid.is_error()) return Result<ReactionResult>::fail(rule_valid.errors());
        if (!matchesPatterns(input, rule)) continue;

        Candidate candidate;
        candidate.rule = &rule;
        candidate.index = index;
        candidate.all_conditions_met = true;
        base_trace.matched_rule_keys.push_back(rule.rule_key);
        for (const auto& condition_ref : rule.condition_refs) {
            auto evaluation = evaluator.evaluate(condition_ref, context);
            if (evaluation.is_error()) return Result<ReactionResult>::fail(evaluation.errors());
            base_trace.condition_traces.push_back(evaluation.value().trace);
            if (!evaluation.value().matched) candidate.all_conditions_met = false;
            candidate.evaluations.push_back(std::move(evaluation.value()));
        }
        if (!candidate.all_conditions_met) base_trace.blocked_rule_keys.push_back(rule.rule_key);
        candidates.push_back(std::move(candidate));
    }

    if (candidates.empty()) {
        ReactionResult result;
        result.decision = ReactionDecision::NoMatchingRule;
        result.trace = std::move(base_trace);
        result.trace.decision = result.decision;
        result.trace.no_match_reason_keys = {"no_matching_rule"};
        ReactionFeedbackDraft feedback;
        feedback.feedback_key = "reaction.no_matching_rule";
        feedback.safe_text = feedbackText(feedback.feedback_key);
        feedback.reason_keys = {"no_matching_rule"};
        result.feedbacks.push_back(std::move(feedback));
        auto valid = result.validateBasic();
        if (valid.is_error()) return Result<ReactionResult>::fail(valid.errors());
        return Result<ReactionResult>::ok(std::move(result));
    }

    std::vector<Candidate> passed;
    for (auto& candidate : candidates) {
        if (candidate.all_conditions_met) passed.push_back(std::move(candidate));
    }
    if (passed.empty()) {
        ReactionResult result;
        result.decision = ReactionDecision::ConditionBlocked;
        result.trace = std::move(base_trace);
        result.trace.decision = result.decision;
        ReactionFeedbackDraft feedback;
        feedback.feedback_key = "reaction.condition_blocked";
        feedback.safe_text = feedbackText(feedback.feedback_key);
        feedback.reason_keys = result.trace.blocked_rule_keys;
        result.feedbacks.push_back(std::move(feedback));
        auto valid = result.validateBasic();
        if (valid.is_error()) return Result<ReactionResult>::fail(valid.errors());
        return Result<ReactionResult>::ok(std::move(result));
    }

    std::sort(passed.begin(), passed.end(), [](const Candidate& left, const Candidate& right) {
        if (left.rule->priority != right.rule->priority) return left.rule->priority > right.rule->priority;
        return left.index < right.index;
    });
    auto selected = std::move(passed.front());
    return buildReactedResult(input, *selected.rule, selected.evaluations, std::move(base_trace));
}

} // namespace pathfinder::reaction
