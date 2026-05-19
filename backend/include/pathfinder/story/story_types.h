#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/h5_projection/projection.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::story {

enum class StoryScenarioKind {
    Unknown,
    FirstDaySurvival,
    TestOnly
};

enum class StoryTimePhase {
    Unknown,
    Morning,
    Afternoon,
    Dusk,
    Night,
    Survived,
    Failed,
    TestOnly
};

enum class StoryObjectiveStatus {
    Unknown,
    Hidden,
    Active,
    Blocked,
    Completed,
    Failed,
    TestOnly
};

enum class StoryThreatKind {
    Unknown,
    ColdNight,
    ApproachingBeast,
    HungerPressure,
    TestOnly
};

enum class StoryMaterialRole {
    Unknown,
    FoodCandidate,
    Tool,
    Fuel,
    IgnitionSource,
    LightSource,
    ThreatCounter,
    CraftResult,
    TestOnly
};

enum class StoryActionRole {
    Unknown,
    Explore,
    Experiment,
    TargetUse,
    Combine,
    Teach,
    Wait,
    Inspect,
    TestOnly
};

std::string toString(StoryScenarioKind value);
std::string toString(StoryTimePhase value);
std::string toString(StoryObjectiveStatus value);
std::string toString(StoryThreatKind value);
std::string toString(StoryMaterialRole value);
std::string toString(StoryActionRole value);

pathfinder::foundation::Result<StoryScenarioKind> storyScenarioKindFromString(const std::string& value);
pathfinder::foundation::Result<StoryTimePhase> storyTimePhaseFromString(const std::string& value);
pathfinder::foundation::Result<StoryObjectiveStatus> storyObjectiveStatusFromString(const std::string& value);
pathfinder::foundation::Result<StoryThreatKind> storyThreatKindFromString(const std::string& value);
pathfinder::foundation::Result<StoryMaterialRole> storyMaterialRoleFromString(const std::string& value);
pathfinder::foundation::Result<StoryActionRole> storyActionRoleFromString(const std::string& value);

bool containsStoryForbiddenKey(const std::string& value);
bool containsStoryForbiddenKey(const std::vector<std::string>& values);

struct StoryMaterialEntry {
    std::string object_key;
    std::string display_name_zh_cn;
    std::vector<StoryMaterialRole> roles;
    std::vector<std::string> required_for_objectives;
    std::vector<StoryActionRole> expected_actions;
    std::vector<std::string> knowledge_paths;
    std::vector<std::string> reaction_rule_keys;
    std::vector<std::string> danger_counter_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryObjectiveDefinition {
    std::string objective_key;
    std::string title_zh_cn;
    std::string hint_zh_cn;
    StoryObjectiveStatus initial_status{StoryObjectiveStatus::Unknown};
    std::vector<std::string> required_knowledge_keys;
    std::vector<std::string> required_object_keys;
    std::vector<std::string> required_threat_states;
    std::vector<std::string> unlocks_objective_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryThreatDefinition {
    std::string threat_key;
    StoryThreatKind kind{StoryThreatKind::Unknown};
    StoryTimePhase starts_at_phase{StoryTimePhase::Unknown};
    StoryTimePhase resolves_at_phase{StoryTimePhase::Unknown};
    std::string warning_zh_cn;
    std::vector<std::string> counter_knowledge_keys;
    std::vector<std::string> counter_object_keys;
    std::string success_feedback_key;
    std::string failure_feedback_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StorySuggestedActionDefinition {
    std::string action_key;
    std::string label_zh_cn;
    std::string input_text;
    std::string required_effect_key;
    std::string target_object_key;
    std::vector<std::string> generated_object_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryTimeBudget {
    int morning_to_afternoon{2};
    int afternoon_to_dusk{5};
    int dusk_to_night{8};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryScenarioDefinition {
    std::string scenario_key;
    StoryScenarioKind kind{StoryScenarioKind::Unknown};
    std::string title_zh_cn;
    StoryTimePhase initial_time_phase{StoryTimePhase::Morning};
    std::string initial_location_key;
    std::vector<std::string> visible_object_keys;
    std::vector<StoryMaterialEntry> material_entries;
    std::vector<StoryObjectiveDefinition> objectives;
    std::vector<StoryThreatDefinition> threats;
    std::vector<StorySuggestedActionDefinition> suggested_actions;
    StoryTimeBudget time_budget;
    std::string success_objective_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryObjectiveState {
    std::string objective_key;
    StoryObjectiveStatus status{StoryObjectiveStatus::Unknown};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryThreatRuntimeState {
    std::string threat_key;
    bool active{false};
    bool resolved{false};
    bool mitigated{false};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryRuntimeState {
    std::string scenario_key;
    StoryTimePhase current_phase{StoryTimePhase::Unknown};
    uint64_t turn_index{0};
    int time_points_used{0};
    std::vector<StoryObjectiveState> objective_states;
    std::vector<StoryThreatRuntimeState> threat_states;
    std::vector<std::string> discovered_object_keys;
    std::vector<std::string> generated_object_keys;
    std::vector<std::string> consumed_object_keys;
    std::vector<std::string> last_story_event_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryEvaluationContext {
    std::vector<std::string> actor_knowledge_effect_keys;
    std::vector<std::string> recipient_knowledge_effect_keys;
    std::vector<std::string> available_object_keys;
    std::vector<std::string> completed_action_keys;
    bool teach_action_happened{false};
    std::string last_input_text;
    std::string last_intent_kind;
    std::string last_action_key;
    std::string last_object_key;
    std::string last_target_object_key;
    std::string last_effect_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryTurnResult {
    bool accepted{true};
    StoryRuntimeState new_state;
    std::vector<std::string> story_event_keys;
    std::vector<std::string> feedback_zh_cn;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryObjectiveCardProjection {
    std::string objective_key;
    StoryObjectiveStatus status{StoryObjectiveStatus::Unknown};
    pathfinder::h5_projection::SafeTextProjection title;
    pathfinder::h5_projection::SafeTextProjection hint;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct StoryProjection {
    std::string scenario_key;
    pathfinder::h5_projection::SafeTextProjection scenario_title;
    pathfinder::h5_projection::SafeTextProjection time_phase_label;
    pathfinder::h5_projection::SafeTextProjection main_objective;
    std::vector<StoryObjectiveCardProjection> objective_cards;
    std::vector<pathfinder::h5_projection::SafeTextProjection> threat_warnings;
    std::vector<pathfinder::h5_projection::SafeTextProjection> material_hints;
    std::vector<pathfinder::h5_projection::ActionProjection> suggested_actions;
    std::vector<std::string> debug_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::story
