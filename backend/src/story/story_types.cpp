#include "pathfinder/story/story_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <unordered_map>

namespace pathfinder::story {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

namespace {

template <typename E>
std::string enumToString(E value, const std::vector<std::pair<E, const char*>>& pairs) {
    for (const auto& pair : pairs) {
        if (pair.first == value) return pair.second;
    }
    return "unknown";
}

template <typename E>
Result<E> enumFromString(const std::string& value, const std::vector<std::pair<E, const char*>>& pairs, const std::string& name) {
    for (const auto& pair : pairs) {
        if (value == pair.second) return Result<E>::ok(pair.first);
    }
    return Result<E>::fail(makeError(ErrorCode::validation_enum_unknown, name + ": " + value));
}

Result<void> ok() { return Result<void>::ok(); }
Result<void> fail(const std::string& message) { return Result<void>::fail(makeError(ErrorCode::validation_failed, message)); }

bool emptyOrForbidden(const std::string& value) {
    return value.empty() || containsStoryForbiddenKey(value);
}

bool hasDuplicate(const std::vector<std::string>& values) {
    std::vector<std::string> seen;
    for (const auto& value : values) {
        if (std::find(seen.begin(), seen.end(), value) != seen.end()) return true;
        seen.push_back(value);
    }
    return false;
}

template <typename T>
bool hasUnknown(const std::vector<T>& values, T unknown, T test_only) {
    for (auto value : values) {
        if (value == unknown || value == test_only) return true;
    }
    return false;
}

} // namespace

std::string toString(StoryScenarioKind value) {
    static const std::vector<std::pair<StoryScenarioKind, const char*>> pairs{{StoryScenarioKind::Unknown, "unknown"}, {StoryScenarioKind::FirstDaySurvival, "first_day_survival"}, {StoryScenarioKind::TestOnly, "test_only"}};
    return enumToString(value, pairs);
}

std::string toString(StoryTimePhase value) {
    static const std::vector<std::pair<StoryTimePhase, const char*>> pairs{{StoryTimePhase::Unknown, "unknown"}, {StoryTimePhase::Morning, "morning"}, {StoryTimePhase::Afternoon, "afternoon"}, {StoryTimePhase::Dusk, "dusk"}, {StoryTimePhase::Night, "night"}, {StoryTimePhase::Survived, "survived"}, {StoryTimePhase::Failed, "failed"}, {StoryTimePhase::TestOnly, "test_only"}};
    return enumToString(value, pairs);
}

std::string toString(StoryObjectiveStatus value) {
    static const std::vector<std::pair<StoryObjectiveStatus, const char*>> pairs{{StoryObjectiveStatus::Unknown, "unknown"}, {StoryObjectiveStatus::Hidden, "hidden"}, {StoryObjectiveStatus::Active, "active"}, {StoryObjectiveStatus::Blocked, "blocked"}, {StoryObjectiveStatus::Completed, "completed"}, {StoryObjectiveStatus::Failed, "failed"}, {StoryObjectiveStatus::TestOnly, "test_only"}};
    return enumToString(value, pairs);
}

std::string toString(StoryThreatKind value) {
    static const std::vector<std::pair<StoryThreatKind, const char*>> pairs{{StoryThreatKind::Unknown, "unknown"}, {StoryThreatKind::ColdNight, "cold_night"}, {StoryThreatKind::ApproachingBeast, "approaching_beast"}, {StoryThreatKind::HungerPressure, "hunger_pressure"}, {StoryThreatKind::TestOnly, "test_only"}};
    return enumToString(value, pairs);
}

std::string toString(StoryMaterialRole value) {
    static const std::vector<std::pair<StoryMaterialRole, const char*>> pairs{{StoryMaterialRole::Unknown, "unknown"}, {StoryMaterialRole::FoodCandidate, "food_candidate"}, {StoryMaterialRole::Tool, "tool"}, {StoryMaterialRole::Fuel, "fuel"}, {StoryMaterialRole::IgnitionSource, "ignition_source"}, {StoryMaterialRole::LightSource, "light_source"}, {StoryMaterialRole::ThreatCounter, "threat_counter"}, {StoryMaterialRole::CraftResult, "craft_result"}, {StoryMaterialRole::TestOnly, "test_only"}};
    return enumToString(value, pairs);
}

std::string toString(StoryActionRole value) {
    static const std::vector<std::pair<StoryActionRole, const char*>> pairs{{StoryActionRole::Unknown, "unknown"}, {StoryActionRole::Explore, "explore"}, {StoryActionRole::Experiment, "experiment"}, {StoryActionRole::TargetUse, "target_use"}, {StoryActionRole::Combine, "combine"}, {StoryActionRole::Teach, "teach"}, {StoryActionRole::Wait, "wait"}, {StoryActionRole::Inspect, "inspect"}, {StoryActionRole::TestOnly, "test_only"}};
    return enumToString(value, pairs);
}

Result<StoryScenarioKind> storyScenarioKindFromString(const std::string& value) {
    static const std::vector<std::pair<StoryScenarioKind, const char*>> pairs{{StoryScenarioKind::Unknown, "unknown"}, {StoryScenarioKind::FirstDaySurvival, "first_day_survival"}, {StoryScenarioKind::TestOnly, "test_only"}};
    return enumFromString(value, pairs, "StoryScenarioKind");
}

Result<StoryTimePhase> storyTimePhaseFromString(const std::string& value) {
    static const std::vector<std::pair<StoryTimePhase, const char*>> pairs{{StoryTimePhase::Unknown, "unknown"}, {StoryTimePhase::Morning, "morning"}, {StoryTimePhase::Afternoon, "afternoon"}, {StoryTimePhase::Dusk, "dusk"}, {StoryTimePhase::Night, "night"}, {StoryTimePhase::Survived, "survived"}, {StoryTimePhase::Failed, "failed"}, {StoryTimePhase::TestOnly, "test_only"}};
    return enumFromString(value, pairs, "StoryTimePhase");
}

Result<StoryObjectiveStatus> storyObjectiveStatusFromString(const std::string& value) {
    static const std::vector<std::pair<StoryObjectiveStatus, const char*>> pairs{{StoryObjectiveStatus::Unknown, "unknown"}, {StoryObjectiveStatus::Hidden, "hidden"}, {StoryObjectiveStatus::Active, "active"}, {StoryObjectiveStatus::Blocked, "blocked"}, {StoryObjectiveStatus::Completed, "completed"}, {StoryObjectiveStatus::Failed, "failed"}, {StoryObjectiveStatus::TestOnly, "test_only"}};
    return enumFromString(value, pairs, "StoryObjectiveStatus");
}

Result<StoryThreatKind> storyThreatKindFromString(const std::string& value) {
    static const std::vector<std::pair<StoryThreatKind, const char*>> pairs{{StoryThreatKind::Unknown, "unknown"}, {StoryThreatKind::ColdNight, "cold_night"}, {StoryThreatKind::ApproachingBeast, "approaching_beast"}, {StoryThreatKind::HungerPressure, "hunger_pressure"}, {StoryThreatKind::TestOnly, "test_only"}};
    return enumFromString(value, pairs, "StoryThreatKind");
}

Result<StoryMaterialRole> storyMaterialRoleFromString(const std::string& value) {
    static const std::vector<std::pair<StoryMaterialRole, const char*>> pairs{{StoryMaterialRole::Unknown, "unknown"}, {StoryMaterialRole::FoodCandidate, "food_candidate"}, {StoryMaterialRole::Tool, "tool"}, {StoryMaterialRole::Fuel, "fuel"}, {StoryMaterialRole::IgnitionSource, "ignition_source"}, {StoryMaterialRole::LightSource, "light_source"}, {StoryMaterialRole::ThreatCounter, "threat_counter"}, {StoryMaterialRole::CraftResult, "craft_result"}, {StoryMaterialRole::TestOnly, "test_only"}};
    return enumFromString(value, pairs, "StoryMaterialRole");
}

Result<StoryActionRole> storyActionRoleFromString(const std::string& value) {
    static const std::vector<std::pair<StoryActionRole, const char*>> pairs{{StoryActionRole::Unknown, "unknown"}, {StoryActionRole::Explore, "explore"}, {StoryActionRole::Experiment, "experiment"}, {StoryActionRole::TargetUse, "target_use"}, {StoryActionRole::Combine, "combine"}, {StoryActionRole::Teach, "teach"}, {StoryActionRole::Wait, "wait"}, {StoryActionRole::Inspect, "inspect"}, {StoryActionRole::TestOnly, "test_only"}};
    return enumFromString(value, pairs, "StoryActionRole");
}

bool containsStoryForbiddenKey(const std::string& value) {
    static const std::vector<std::string> forbidden{"hidden_truth", "true_trait", "real_effect", "raw_state", "actual_hp", "true_hp", "hp_delta", "death", "kill", "corpse", "loot", "drop", "random_damage", "frontend_unlock", "direct_state", "true_beast_strength", "beast_hp", "exact_damage_roll", "condition_debug_expression"};
    for (const auto& key : forbidden) {
        if (value.find(key) != std::string::npos) return true;
    }
    return false;
}

bool containsStoryForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& value : values) {
        if (containsStoryForbiddenKey(value)) return true;
    }
    return false;
}

Result<void> StoryMaterialEntry::validateBasic() const {
    if (emptyOrForbidden(object_key) || emptyOrForbidden(display_name_zh_cn)) return fail("story material invalid key or name");
    if (roles.empty() || hasUnknown(roles, StoryMaterialRole::Unknown, StoryMaterialRole::TestOnly)) return fail("story material requires roles");
    if (expected_actions.empty() || hasUnknown(expected_actions, StoryActionRole::Unknown, StoryActionRole::TestOnly)) return fail("story material requires expected actions");
    if (containsStoryForbiddenKey(required_for_objectives) || containsStoryForbiddenKey(knowledge_paths) || containsStoryForbiddenKey(reaction_rule_keys) || containsStoryForbiddenKey(danger_counter_keys)) return fail("story material contains forbidden key");
    return ok();
}

Result<void> StoryObjectiveDefinition::validateBasic() const {
    if (emptyOrForbidden(objective_key) || emptyOrForbidden(title_zh_cn) || emptyOrForbidden(hint_zh_cn)) return fail("story objective invalid text");
    if (initial_status == StoryObjectiveStatus::Unknown || initial_status == StoryObjectiveStatus::TestOnly) return fail("story objective invalid status");
    if (containsStoryForbiddenKey(required_knowledge_keys) || containsStoryForbiddenKey(required_object_keys) || containsStoryForbiddenKey(required_threat_states) || containsStoryForbiddenKey(unlocks_objective_keys)) return fail("story objective contains forbidden key");
    return ok();
}

Result<void> StoryThreatDefinition::validateBasic() const {
    if (emptyOrForbidden(threat_key) || emptyOrForbidden(warning_zh_cn) || emptyOrForbidden(success_feedback_key) || emptyOrForbidden(failure_feedback_key)) return fail("story threat invalid text");
    if (kind == StoryThreatKind::Unknown || kind == StoryThreatKind::TestOnly) return fail("story threat invalid kind");
    if (starts_at_phase == StoryTimePhase::Unknown || resolves_at_phase == StoryTimePhase::Unknown) return fail("story threat invalid phase");
    if (counter_knowledge_keys.empty() || counter_object_keys.empty()) return fail("story threat requires countermeasure");
    if (containsStoryForbiddenKey(counter_knowledge_keys) || containsStoryForbiddenKey(counter_object_keys)) return fail("story threat contains forbidden key");
    return ok();
}

Result<void> StorySuggestedActionDefinition::validateBasic() const {
    if (emptyOrForbidden(action_key) || emptyOrForbidden(label_zh_cn) || emptyOrForbidden(input_text)) return fail("story suggested action invalid text");
    if (containsStoryForbiddenKey(required_effect_key) || containsStoryForbiddenKey(target_object_key) || containsStoryForbiddenKey(generated_object_keys)) return fail("story suggested action contains forbidden key");
    return ok();
}

Result<void> StoryTimeBudget::validateBasic() const {
    if (morning_to_afternoon <= 0 || afternoon_to_dusk <= morning_to_afternoon || dusk_to_night <= afternoon_to_dusk) return fail("story time budget invalid order");
    return ok();
}

Result<void> StoryScenarioDefinition::validateBasic() const {
    if (emptyOrForbidden(scenario_key) || emptyOrForbidden(title_zh_cn) || emptyOrForbidden(initial_location_key) || emptyOrForbidden(success_objective_key)) return fail("story scenario invalid text");
    if (kind == StoryScenarioKind::Unknown || kind == StoryScenarioKind::TestOnly) return fail("story scenario invalid kind");
    if (initial_time_phase != StoryTimePhase::Morning) return fail("story scenario must start at morning");
    if (visible_object_keys.size() < 6 || hasDuplicate(visible_object_keys) || containsStoryForbiddenKey(visible_object_keys)) return fail("story scenario invalid visible objects");
    std::vector<std::string> material_keys;
    for (const auto& material : material_entries) {
        auto valid = material.validateBasic();
        if (valid.is_error()) return valid;
        material_keys.push_back(material.object_key);
    }
    if (material_entries.size() < 8 || hasDuplicate(material_keys)) return fail("story scenario invalid materials");
    std::vector<std::string> objective_keys;
    for (const auto& objective : objectives) {
        auto valid = objective.validateBasic();
        if (valid.is_error()) return valid;
        objective_keys.push_back(objective.objective_key);
    }
    if (objectives.size() < 3 || hasDuplicate(objective_keys)) return fail("story scenario invalid objectives");
    if (std::find(objective_keys.begin(), objective_keys.end(), success_objective_key) == objective_keys.end()) return fail("story scenario missing success objective");
    if (threats.empty()) return fail("story scenario requires threat");
    std::vector<std::string> threat_keys;
    for (const auto& threat : threats) {
        auto valid = threat.validateBasic();
        if (valid.is_error()) return valid;
        threat_keys.push_back(threat.threat_key);
    }
    if (hasDuplicate(threat_keys)) return fail("story scenario duplicate threat");
    if (suggested_actions.empty()) return fail("story scenario requires suggested actions");
    std::vector<std::string> suggested_action_keys;
    for (const auto& action : suggested_actions) {
        auto valid = action.validateBasic();
        if (valid.is_error()) return valid;
        suggested_action_keys.push_back(action.action_key);
    }
    if (hasDuplicate(suggested_action_keys)) return fail("story scenario duplicate suggested action");
    return time_budget.validateBasic();
}

Result<void> StoryObjectiveState::validateBasic() const {
    if (emptyOrForbidden(objective_key)) return fail("story objective state invalid key");
    if (status == StoryObjectiveStatus::Unknown || status == StoryObjectiveStatus::TestOnly) return fail("story objective state invalid status");
    if (containsStoryForbiddenKey(reason_keys)) return fail("story objective state forbidden key");
    return ok();
}

Result<void> StoryThreatRuntimeState::validateBasic() const {
    if (emptyOrForbidden(threat_key)) return fail("story threat state invalid key");
    if (containsStoryForbiddenKey(reason_keys)) return fail("story threat state forbidden key");
    return ok();
}

Result<void> StoryRuntimeState::validateBasic() const {
    if (emptyOrForbidden(scenario_key)) return fail("story state invalid scenario");
    if (current_phase == StoryTimePhase::Unknown || current_phase == StoryTimePhase::TestOnly) return fail("story state invalid phase");
    if (time_points_used < 0) return fail("story state invalid time");
    std::vector<std::string> objective_keys;
    for (const auto& objective : objective_states) {
        auto valid = objective.validateBasic();
        if (valid.is_error()) return valid;
        objective_keys.push_back(objective.objective_key);
    }
    if (hasDuplicate(objective_keys)) return fail("story state duplicate objective");
    std::vector<std::string> threat_keys;
    for (const auto& threat : threat_states) {
        auto valid = threat.validateBasic();
        if (valid.is_error()) return valid;
        threat_keys.push_back(threat.threat_key);
    }
    if (hasDuplicate(threat_keys)) return fail("story state duplicate threat");
    if (containsStoryForbiddenKey(discovered_object_keys) || containsStoryForbiddenKey(generated_object_keys) || containsStoryForbiddenKey(consumed_object_keys) || containsStoryForbiddenKey(last_story_event_keys)) return fail("story state forbidden key");
    return ok();
}

Result<void> StoryEvaluationContext::validateBasic() const {
    if (containsStoryForbiddenKey(actor_knowledge_effect_keys) || containsStoryForbiddenKey(recipient_knowledge_effect_keys) || containsStoryForbiddenKey(available_object_keys) || containsStoryForbiddenKey(completed_action_keys) || containsStoryForbiddenKey(last_input_text) || containsStoryForbiddenKey(last_intent_kind) || containsStoryForbiddenKey(last_action_key) || containsStoryForbiddenKey(last_object_key) || containsStoryForbiddenKey(last_target_object_key) || containsStoryForbiddenKey(last_effect_key)) return fail("story context forbidden key");
    return ok();
}

Result<void> StoryTurnResult::validateBasic() const {
    auto valid = new_state.validateBasic();
    if (valid.is_error()) return valid;
    if (containsStoryForbiddenKey(story_event_keys) || containsStoryForbiddenKey(feedback_zh_cn)) return fail("story turn result forbidden key");
    return ok();
}

Result<void> StoryObjectiveCardProjection::validateBasic() const {
    if (emptyOrForbidden(objective_key)) return fail("story objective projection invalid key");
    if (status == StoryObjectiveStatus::Unknown || status == StoryObjectiveStatus::TestOnly) return fail("story objective projection invalid status");
    auto title_valid = title.validateBasic();
    if (title_valid.is_error()) return title_valid;
    return hint.validateBasic();
}

Result<void> StoryProjection::validateBasic() const {
    if (emptyOrForbidden(scenario_key)) return fail("story projection invalid key");
    auto title_valid = scenario_title.validateBasic();
    if (title_valid.is_error()) return title_valid;
    auto phase_valid = time_phase_label.validateBasic();
    if (phase_valid.is_error()) return phase_valid;
    auto objective_valid = main_objective.validateBasic();
    if (objective_valid.is_error()) return objective_valid;
    for (const auto& card : objective_cards) {
        auto valid = card.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& warning : threat_warnings) {
        auto valid = warning.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& hint : material_hints) {
        auto valid = hint.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& action : suggested_actions) {
        auto valid = action.validateBasic();
        if (valid.is_error()) return valid;
    }
    if (containsStoryForbiddenKey(debug_keys)) return fail("story projection forbidden debug key");
    return ok();
}

} // namespace pathfinder::story
