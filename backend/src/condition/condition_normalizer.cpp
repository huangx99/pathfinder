#include "pathfinder/condition/condition_normalizer.h"
#include "pathfinder/condition/condition_summary.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <map>
#include <sstream>

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static std::string numericString(double value) {
    std::ostringstream out;
    out << value;
    return out.str();
}

Result<void> NormalizedCondition::validateBasic() const {
    auto ref_result = expression_ref.validateBasic();
    if (ref_result.is_error()) return ref_result;
    if (canonical_condition_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "NormalizedCondition canonical_condition_key empty"));
    if (summary_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "NormalizedCondition summary_key empty"));
    if (containsConditionForbiddenKey(canonical_condition_key) || containsConditionForbiddenKey(summary_key) || containsConditionForbiddenKey(safe_summary_text) || containsConditionForbiddenKey(source_legacy_keys) || containsConditionForbiddenKey(normalization_trace)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "NormalizedCondition contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<NormalizedCondition> ConditionNormalizer::normalizeKey(std::string_view condition_key) const {
    LegacyConditionAdapter adapter;
    auto canonical = adapter.canonicalizeKey(std::string(condition_key));
    if (canonical.is_error()) return Result<NormalizedCondition>::fail(canonical.errors());

    ConditionSummaryBuilder summary_builder;
    auto summary = summary_builder.summarizeCanonicalKey(canonical.value());
    if (summary.is_error()) return Result<NormalizedCondition>::fail(summary.errors());

    NormalizedCondition normalized;
    normalized.expression_ref.inline_canonical_key = canonical.value();
    normalized.expression_ref.inline_legacy_condition = true;
    normalized.canonical_condition_key = canonical.value();
    normalized.summary_key = summary.value().summary_key;
    normalized.safe_summary_text = summary.value().safe_summary_text;
    normalized.source_legacy_keys = {std::string(condition_key)};
    normalized.normalization_trace = {"normalize_key", canonical.value()};
    normalized.from_legacy = canonical.value() != condition_key;
    auto valid = normalized.validateBasic();
    if (valid.is_error()) return Result<NormalizedCondition>::fail(valid.errors());
    return Result<NormalizedCondition>::ok(std::move(normalized));
}

Result<NormalizedCondition> ConditionNormalizer::normalizeKeys(const std::vector<std::string>& condition_keys) const {
    LegacyConditionInput input;
    input.condition_keys = condition_keys;
    return normalizeLegacyInput(input);
}

Result<NormalizedCondition> ConditionNormalizer::normalizeLegacyInput(const LegacyConditionInput& input) const {
    LegacyConditionAdapter adapter;
    auto canonical_keys = adapter.collectCanonicalKeys(input);
    if (canonical_keys.is_error()) return Result<NormalizedCondition>::fail(canonical_keys.errors());
    if (canonical_keys.value().empty()) {
        return Result<NormalizedCondition>::fail(makeError(ErrorCode::validation_failed, "no condition keys to normalize"));
    }
    std::string combined;
    for (size_t i = 0; i < canonical_keys.value().size(); ++i) {
        if (i > 0) combined += "+";
        combined += canonical_keys.value()[i];
    }
    ConditionSummaryBuilder summary_builder;
    auto summary = summary_builder.summarizeCanonicalKey(canonical_keys.value().front());
    if (summary.is_error()) return Result<NormalizedCondition>::fail(summary.errors());

    NormalizedCondition normalized;
    normalized.expression_ref.inline_canonical_key = canonical_keys.value().size() == 1 ? canonical_keys.value().front() : "condition:and:all:eq:" + combined;
    normalized.expression_ref.inline_legacy_condition = true;
    normalized.canonical_condition_key = canonical_keys.value().size() == 1 ? canonical_keys.value().front() : normalized.expression_ref.inline_canonical_key;
    normalized.summary_key = canonical_keys.value().size() == 1 ? summary.value().summary_key : "condition.summary.combined";
    normalized.safe_summary_text = canonical_keys.value().size() == 1 ? summary.value().safe_summary_text : "组合条件";
    normalized.source_legacy_keys = input.condition_keys;
    if (!input.condition_key.empty()) normalized.source_legacy_keys.push_back(input.condition_key);
    normalized.source_legacy_keys.insert(normalized.source_legacy_keys.end(), input.object_state_keys.begin(), input.object_state_keys.end());
    normalized.source_legacy_keys.insert(normalized.source_legacy_keys.end(), input.actor_requirement_keys.begin(), input.actor_requirement_keys.end());
    normalized.normalization_trace = {"normalize_legacy_input", normalized.canonical_condition_key};
    normalized.from_legacy = true;
    auto valid = normalized.validateBasic();
    if (valid.is_error()) return Result<NormalizedCondition>::fail(valid.errors());
    return Result<NormalizedCondition>::ok(std::move(normalized));
}

Result<NormalizedCondition> ConditionNormalizer::normalizeCapabilityCondition(const std::string& condition_type, double required_score) const {
    if (condition_type.empty()) return Result<NormalizedCondition>::fail(makeError(ErrorCode::validation_failed, "capability condition_type empty"));
    if (containsConditionForbiddenKey(condition_type)) return Result<NormalizedCondition>::fail(makeError(ErrorCode::validation_failed, "capability condition_type forbidden"));
    static const std::map<std::string, std::string> op_by_type = {
        {"known_edible_count", "gte"}, {"known_danger_count", "gte"}, {"stable_tribe_knowledge_count", "gte"},
        {"teachable_knowledge_count", "gte"}, {"coverage", "gte"}, {"success_rate", "gte"},
        {"truce_success_count", "gte"}, {"forced_retreat_low_loss_count", "gte"}, {"standoff_controlled_count", "gte"},
        {"coordination_score", "gte"}, {"intimidation_success_count", "gte"}, {"retreat_success_count", "gte"},
        {"resource_defense_success_count", "gte"}, {"stable_coordination_count", "gte"},
        {"misunderstanding_rate", "lte"}, {"conflicted_knowledge_count", "lte"},
        {"open_conflict_pressure", "lte"}, {"loss_pressure", "lte"}
    };
    auto op_it = op_by_type.find(condition_type);
    if (op_it == op_by_type.end()) return Result<NormalizedCondition>::fail(makeError(ErrorCode::validation_failed, "unsupported capability condition_type: " + condition_type));
    const auto canonical = "condition:civilization:" + condition_type + ":" + op_it->second + ":" + numericString(required_score);
    ConditionSummaryBuilder summary_builder;
    auto summary = summary_builder.summarizeCanonicalKey(canonical);
    if (summary.is_error()) return Result<NormalizedCondition>::fail(summary.errors());

    NormalizedCondition normalized;
    normalized.expression_ref.inline_canonical_key = canonical;
    normalized.canonical_condition_key = canonical;
    normalized.summary_key = summary.value().summary_key;
    normalized.safe_summary_text = summary.value().safe_summary_text;
    normalized.source_legacy_keys = {condition_type};
    normalized.normalization_trace = {"normalize_capability_condition", canonical};
    auto valid = normalized.validateBasic();
    if (valid.is_error()) return Result<NormalizedCondition>::fail(valid.errors());
    return Result<NormalizedCondition>::ok(std::move(normalized));
}

} // namespace pathfinder::condition
