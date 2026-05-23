#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/config/validation.h"
#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <map>
#include <memory>

namespace pathfinder::content {

// ============================================================
// Enums
// ============================================================

enum class ContentFileCategory {
    Objects,
    Actions,
    Effects,
    Feedbacks,
    Reactions,
    Agents,
    Threats,
    Knowledge,
    Scenarios,
    Locales,
    Worldgen,
    Tests
};

std::string toString(ContentFileCategory category);
pathfinder::foundation::Result<ContentFileCategory> contentFileCategoryFromString(const std::string& value);

enum class ContentDefinitionKind {
    Object,
    Action,
    Effect,
    Feedback,
    Reaction,
    Agent,
    Threat,
    KnowledgeTemplate,
    Scenario,
    Locale,
    WorldgenProfile
};

std::string toString(ContentDefinitionKind kind);
pathfinder::foundation::Result<ContentDefinitionKind> contentDefinitionKindFromString(const std::string& value);

enum class ContentLoadStage {
    ReadManifest,
    ReadFiles,
    ParseJson,
    ValidateStructure,
    ValidateReferences,
    BuildRegistry,
    BindRuntimeAdapters
};

std::string toString(ContentLoadStage stage);

enum class ContentOverridePolicy {
    Forbidden,
    AllowSamePackagePatch,
    AllowHigherPriorityPackage
};

std::string toString(ContentOverridePolicy policy);
pathfinder::foundation::Result<ContentOverridePolicy> contentOverridePolicyFromString(const std::string& value);

enum class ContentVisibility {
    RuntimeVisible,
    HiddenUntilDiscovered,
    SystemOnly,
    TestOnly
};

std::string toString(ContentVisibility visibility);
pathfinder::foundation::Result<ContentVisibility> contentVisibilityFromString(const std::string& value);

// ============================================================
// StrongId type aliases
// ============================================================

// Tag types for content IDs
struct ContentPackageKeyTag {};
struct ContentDefinitionKeyTag {};
struct ObjectDefinitionKeyTag {};
struct ActionDefinitionKeyTag {};
struct EffectDefinitionKeyTag {};
struct FeedbackDefinitionKeyTag {};
struct ReactionDefinitionKeyTag {};
struct AgentTemplateKeyTag {};
struct ThreatDefinitionKeyTag {};
struct KnowledgeTemplateKeyTag {};
struct ScenarioDefinitionKeyTag {};

using ContentPackageKey = pathfinder::foundation::StrongId<ContentPackageKeyTag>;
using ContentDefinitionKey = pathfinder::foundation::StrongId<ContentDefinitionKeyTag>;
using ObjectDefinitionKey = pathfinder::foundation::StrongId<ObjectDefinitionKeyTag>;
using ActionDefinitionKey = pathfinder::foundation::StrongId<ActionDefinitionKeyTag>;
using EffectDefinitionKey = pathfinder::foundation::StrongId<EffectDefinitionKeyTag>;
using FeedbackDefinitionKey = pathfinder::foundation::StrongId<FeedbackDefinitionKeyTag>;
using ReactionDefinitionKey = pathfinder::foundation::StrongId<ReactionDefinitionKeyTag>;
using AgentTemplateKey = pathfinder::foundation::StrongId<AgentTemplateKeyTag>;
using ThreatDefinitionKey = pathfinder::foundation::StrongId<ThreatDefinitionKeyTag>;
using KnowledgeTemplateKey = pathfinder::foundation::StrongId<KnowledgeTemplateKeyTag>;
using ScenarioDefinitionKey = pathfinder::foundation::StrongId<ScenarioDefinitionKeyTag>;

// ============================================================
// Manifest DTOs
// ============================================================

struct ContentFileRefDto {
    std::string path;
    ContentFileCategory category{ContentFileCategory::Objects};
    bool required = true;
};

struct ContentManifestDto {
    std::string package_key;
    std::string display_name;
    std::string content_version;
    std::string schema_version;
    std::string locale_default;
    int load_order = 0;
    std::vector<ContentFileRefDto> files;
};

// ============================================================
// Object DTOs
// ============================================================

struct ObjectInitialStateDto {
    int quantity = 0;
    std::map<std::string, double> numeric;
    std::vector<std::string> tags;
};

struct ObjectProjectionDto {
    std::vector<std::string> safe_trait_keys;
    std::string unknown_use_text_key;
};

struct ObjectDefinitionDto {
    std::string object_key;
    std::string display_key;
    std::string description_key;
    std::string kind;
    ContentVisibility visibility{ContentVisibility::RuntimeVisible};
    std::vector<std::string> safe_tags;
    std::vector<std::string> allowed_actions;
    ObjectInitialStateDto initial_state;
    std::vector<std::string> knowledge_hints;
    ObjectProjectionDto projection;
};

struct ObjectDefinitionFileDto {
    std::vector<ObjectDefinitionDto> objects;
};

// ============================================================
// Action DTOs
// ============================================================

struct ActionSafetyDto {
    bool requires_player_confirmation = false;
    bool can_target_other_actor = false;
};

struct ActionDefinitionDto {
    std::string action_key;
    std::string display_key;
    std::string intent_kind;
    std::string targeting;
    std::vector<std::string> allowed_actor_scales;
    int default_time_cost = 1;
    ActionSafetyDto safety;
};

struct ActionDefinitionFileDto {
    std::vector<ActionDefinitionDto> actions;
};

// ============================================================
// Effect DTOs
// ============================================================

struct EffectOperationDto {
    std::string op;
    std::string target;
    int quantity = 0;
    std::string state_key;
    std::string need_key;
    double delta = 0.0;
    double value = 0.0;
    std::string source;
    std::string summary_key;
};

struct EffectLearningDto {
    std::string knowledge_effect_key;
    double confidence_delta = 0.0;
    bool teachable = false;
};

struct EffectAgentDto {
    bool usable_by_ai = false;
    int risk_score = 0;
    int time_cost = 1;
};

struct EffectDefinitionDto {
    std::string effect_key;
    std::string display_key;
    std::string semantic_kind;
    std::vector<std::string> goal_kinds;
    std::string target_kind;
    std::vector<std::string> preconditions;
    std::vector<EffectOperationDto> operations;
    EffectLearningDto learning;
    EffectAgentDto agent;
};

struct EffectDefinitionFileDto {
    std::vector<EffectDefinitionDto> effects;
};

// ============================================================
// Feedback DTOs
// ============================================================

struct FeedbackKnowledgeRefDto {
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
};

struct FeedbackDefinitionDto {
    std::string feedback_key;
    std::string object_key;
    std::string action_key;
    std::string target_object_key;
    std::string effect_key;
    int priority = 100;
    std::vector<std::string> conditions;
    std::string reply_key;
    double utility_delta = 0.0;
    double risk_delta = 0.0;
    std::vector<std::string> causal_tags;
    std::vector<std::string> outcome_signal_keys;
    FeedbackKnowledgeRefDto knowledge;
};

struct FeedbackDefinitionFileDto {
    std::vector<FeedbackDefinitionDto> feedbacks;
};

// ============================================================
// Reaction DTOs
// ============================================================

struct ReactionInputDto {
    std::string object_key;
    std::string state;
    int min = 1;
    int quantity = 0;
};

struct ReactionOutputDto {
    std::string object_key;
    int quantity_delta = 0;
};

struct ReactionConsumeDto {
    std::string object_key;
    std::string state;
    int delta = 0;
};

struct ReactionDefinitionDto {
    std::string reaction_key;
    std::vector<ReactionInputDto> inputs;
    std::string action_key;
    std::string effect_key;
    std::vector<ReactionOutputDto> outputs;
    std::vector<ReactionConsumeDto> consume;
    std::string summary_key;
    std::vector<std::string> knowledge_templates;
};

struct ReactionDefinitionFileDto {
    std::vector<ReactionDefinitionDto> reactions;
};

// ============================================================
// Agent DTOs
// ============================================================

struct AgentDefaultNeedsDto {
    double fear = 0.0;
    double hunger = 0.0;
    double trust = 0.0;
};

struct AgentTemplateDto {
    std::string agent_key;
    std::string display_key;
    std::string scale;
    std::string cognition_band;
    std::string embodiment;
    AgentDefaultNeedsDto default_needs;
    std::vector<std::string> instinct_effect_keys;
    std::string default_policy_key;
    bool can_learn = true;
    bool can_teach = false;
};

struct AgentTemplateFileDto {
    std::vector<AgentTemplateDto> agents;
};

// ============================================================
// Threat DTOs
// ============================================================

struct ThreatPhaseDto {
    int min = 0;
    std::string phase;
    std::string summary_key;
};

struct ThreatProgressionDto {
    int wait_delta = 25;
    std::vector<ThreatPhaseDto> phases;
};

struct ThreatCountermeasureDto {
    std::string effect_key;
    int level_delta = 0;
};

struct ThreatReentryDto {
    bool enabled = false;
    int waits = 3;
    int level = 50;
    std::string tag;
};

struct ThreatDefinitionDto {
    std::string threat_key;
    std::string display_key;
    std::string agent_key;
    int initial_level = 0;
    ThreatProgressionDto progression;
    std::vector<ThreatCountermeasureDto> countermeasures;
    ThreatReentryDto reentry;
};

struct ThreatDefinitionFileDto {
    std::vector<ThreatDefinitionDto> threats;
};

// ============================================================
// Knowledge DTOs
// ============================================================

struct KnowledgeTemplateDto {
    std::string knowledge_key;
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
    std::string target_object_key;
    std::string default_status;
    bool teachable = true;
    bool usable_by_ai = true;
    std::string summary_key;
};

struct KnowledgeTemplateFileDto {
    std::vector<KnowledgeTemplateDto> knowledge_templates;
};

// ============================================================
// Scenario DTOs
// ============================================================

struct ScenarioInitialObjectDto {
    std::string object_key;
    int quantity = 1;
    bool visible = true;
    std::map<std::string, double> numeric;
};

struct ScenarioInitialAgentDto {
    std::string agent_key;
    bool visible = true;
    double trust = 0.5;
};

struct ScenarioInitialThreatDto {
    std::string threat_key;
    int level = 0;
};

struct ScenarioActionTemplateDto {
    std::string action_key;
    std::string label_text;
    std::string input_text;
    std::string object_key;
    std::string required_effect_key;
    std::string target_object_key;
    std::vector<std::string> reason_keys;
};

struct ScenarioThreatTemplateDto {
    std::string template_key;
    std::string threat_object_key;
    std::string required_effect_key;
    std::vector<std::string> fallback_effect_keys;
};

struct ScenarioDefinitionDto {
    std::string scenario_key;
    std::string display_key;
    std::string welcome_text_key;
    std::vector<ScenarioInitialObjectDto> initial_objects;
    std::vector<ScenarioInitialAgentDto> initial_agents;
    std::vector<ScenarioInitialThreatDto> initial_threats;
    std::vector<std::string> quick_action_input_texts;
    std::vector<ScenarioActionTemplateDto> suggested_action_templates;
    std::vector<ScenarioThreatTemplateDto> threat_knowledge_templates;
    std::vector<std::string> default_player_knowledge;
    std::vector<std::string> default_agent_knowledge;
};

// ============================================================
// Locale DTOs
// ============================================================

using LocaleMap = std::map<std::string, std::string>;

// ============================================================
// World generation DTOs
// ============================================================

struct WorldgenTerrainWeightDto {
    std::string terrain_key;
    int weight = 0;
};

struct WorldgenNoiseChannelDto {
    std::string channel;
    std::string algorithm;
    double scale = 24.0;
    int octaves = 3;
    double persistence = 0.5;
    double lacunarity = 2.0;
    double bias = 0.0;
    double weight = 1.0;
    uint64_t salt = 0;
};

struct WorldgenTerrainThresholdRuleDto {
    std::string terrain_key;
    double min_elevation = -1.0;
    double max_elevation = 1.0;
    double min_moisture = -1.0;
    double max_moisture = 1.0;
    double min_roughness = -1.0;
    double max_roughness = 1.0;
    int priority = 0;
    std::vector<std::string> tag_keys;
};

struct WorldgenConnectivityPolicyDto {
    bool enabled = true;
    int spawn_safe_radius = 3;
    double min_walkable_ratio = 0.72;
    double max_blocked_ratio_in_spawn_radius = 0.10;
    int min_reachable_cells_from_spawn = 80;
    bool carve_cardinal_corridors = true;
    int corridor_half_width = 1;
    std::vector<std::string> repair_preferred_terrain_keys;
};

struct WorldgenResourceRuleDto {
    std::string resource_key;
    std::vector<std::string> allowed_terrain_tags;
    double density = 0.0;
    int min_distance_from_spawn = 0;
    int max_distance_from_spawn = -1;
    std::vector<std::string> tag_keys;
    std::string node_kind;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> output_object_keys;
    int charges = 1;
};

struct WorldgenGroundItemRuleDto {
    std::string object_key;
    std::vector<std::string> allowed_terrain_tags;
    double density = 0.0;
    int min_distance_from_spawn = 0;
    int max_distance_from_spawn = -1;
    int quantity = 0;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> tag_keys;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
};

struct WorldgenSpawnSafetyDto {
    int safe_radius = 2;
    int basic_food_min_count = 1;
    int basic_material_min_count = 2;
    int tool_hint_min_count = 1;
    int immediate_threat_max_count = 0;
    std::vector<std::string> guaranteed_resource_keys;
    std::vector<std::string> forbidden_danger_keys;
};

struct WorldgenDropRuleDto {
    std::string drop_rule_key;
    std::string source_object_key;
    std::vector<std::string> output_object_keys;
};

struct WorldgenProfileDefinitionDto {
    std::string profile_key;
    int region_size = 16;
    std::string default_layer = "surface";
    std::string terrain_generation_mode = "NoiseField";
    std::vector<WorldgenTerrainWeightDto> terrain_weights;
    std::vector<WorldgenNoiseChannelDto> noise_channels;
    std::vector<WorldgenTerrainThresholdRuleDto> terrain_threshold_rules;
    WorldgenConnectivityPolicyDto connectivity_policy;
    std::vector<WorldgenResourceRuleDto> resource_rules;
    std::vector<WorldgenGroundItemRuleDto> ground_item_rules;
    std::vector<WorldgenDropRuleDto> drop_rules;
    WorldgenSpawnSafetyDto spawn_safety;
};

struct WorldgenProfileDefinitionFileDto {
    std::vector<WorldgenProfileDefinitionDto> worldgen_profiles;
};

// ============================================================
// Content file entry (runtime representation)
// ============================================================

struct ContentFileEntry {
    std::string path;
    ContentFileCategory category;
    bool required = true;
};

// ============================================================
// Content package manifest (runtime representation)
// ============================================================

struct ContentPackageManifest {
    ContentPackageKey package_key;
    std::string display_name;
    std::string content_version;
    std::string schema_version;
    std::string locale_default;
    int load_order = 0;
    std::vector<ContentFileEntry> files;
};

// ============================================================
// Load options and result
// ============================================================

enum class ContentLoadMode {
    StrictContentRequired,   // Fatal if content fails validation — reject startup
    AllowBuiltinFallback     // Fall back to built-in scenario if content cannot load
};

std::string toString(ContentLoadMode mode);
pathfinder::foundation::Result<ContentLoadMode> contentLoadModeFromString(const std::string& value);

struct ContentLoadOptions {
    std::string root_path;
    std::vector<std::string> enabled_package_keys;
    bool allow_test_content = false;
    bool allow_deprecated_fields = true;
    ContentOverridePolicy override_policy{ContentOverridePolicy::Forbidden};
    ContentLoadMode load_mode{ContentLoadMode::StrictContentRequired};
};

// Forward declaration
class ContentRegistry;

struct ContentLoadResult {
    std::shared_ptr<const ContentRegistry> registry;
    pathfinder::config::ConfigValidationReport validation_report;
    std::vector<std::string> trace_keys;
};

// ============================================================
// Content definition (runtime representation after registration)
// ============================================================

struct ObjectDefinitionContent {
    ObjectDefinitionKey key;
    std::string display_key;
    std::string description_key;
    std::string kind;
    ContentVisibility visibility{ContentVisibility::RuntimeVisible};
    std::vector<std::string> safe_tags;
    std::vector<std::string> allowed_actions;
    int default_quantity = 0;
    std::map<std::string, double> default_numeric;
    std::vector<std::string> knowledge_hints;
    std::vector<std::string> safe_trait_keys;
    std::string unknown_use_text_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ActionDefinitionContent {
    ActionDefinitionKey key;
    std::string display_key;
    std::string intent_kind;
    std::string targeting;
    std::vector<std::string> allowed_actor_scales;
    int default_time_cost = 1;
    bool requires_player_confirmation = false;
    bool can_target_other_actor = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct EffectDefinitionContent {
    EffectDefinitionKey key;
    std::string display_key;
    std::string semantic_kind;
    std::vector<std::string> goal_kinds;
    std::string target_kind;
    std::vector<std::string> preconditions;
    std::vector<EffectOperationDto> operations;
    std::string knowledge_effect_key;
    double confidence_delta = 0.0;
    bool teachable = false;
    bool usable_by_ai = false;
    int risk_score = 0;
    int time_cost = 1;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct FeedbackDefinitionContent {
    FeedbackDefinitionKey key;
    std::string object_key;
    std::string action_key;
    std::string target_object_key;
    std::string effect_key;
    int priority = 100;
    std::vector<std::string> conditions;
    std::string reply_key;
    double utility_delta = 0.0;
    double risk_delta = 0.0;
    std::vector<std::string> causal_tags;
    std::vector<std::string> outcome_signal_keys;
    std::string subject_object_key;
    std::string knowledge_action_key;
    std::string knowledge_effect_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionDefinitionContent {
    ReactionDefinitionKey key;
    std::vector<ReactionInputDto> inputs;
    std::string action_key;
    std::string effect_key;
    std::vector<ReactionOutputDto> outputs;
    std::vector<ReactionConsumeDto> consume;
    std::string summary_key;
    std::vector<std::string> knowledge_templates;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentTemplateContent {
    AgentTemplateKey key;
    std::string display_key;
    std::string scale;
    std::string cognition_band;
    std::string embodiment;
    double default_fear = 0.0;
    double default_hunger = 0.0;
    double default_trust = 0.0;
    std::vector<std::string> instinct_effect_keys;
    std::string default_policy_key;
    bool can_learn = true;
    bool can_teach = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ThreatDefinitionContent {
    ThreatDefinitionKey key;
    std::string display_key;
    std::string agent_key;
    int initial_level = 0;
    int wait_delta = 25;
    std::vector<ThreatPhaseDto> phases;
    std::vector<ThreatCountermeasureDto> countermeasures;
    bool reentry_enabled = false;
    int reentry_waits = 3;
    int reentry_level = 50;
    std::string reentry_tag;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeTemplateContent {
    KnowledgeTemplateKey key;
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
    std::string target_object_key;
    std::string default_status;
    bool teachable = true;
    bool usable_by_ai = true;
    std::string summary_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ScenarioDefinitionContent {
    ScenarioDefinitionKey key;
    std::string display_key;
    std::string welcome_text_key;
    std::vector<ScenarioInitialObjectDto> initial_objects;
    std::vector<ScenarioInitialAgentDto> initial_agents;
    std::vector<ScenarioInitialThreatDto> initial_threats;
    std::vector<std::string> quick_action_input_texts;
    std::vector<ScenarioActionTemplateDto> suggested_action_templates;
    std::vector<ScenarioThreatTemplateDto> threat_knowledge_templates;
    std::vector<std::string> default_player_knowledge;
    std::vector<std::string> default_agent_knowledge;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldgenProfileDefinitionContent {
    std::string profile_key;
    int region_size = 16;
    std::string default_layer = "surface";
    std::string terrain_generation_mode = "NoiseField";
    std::vector<WorldgenTerrainWeightDto> terrain_weights;
    std::vector<WorldgenNoiseChannelDto> noise_channels;
    std::vector<WorldgenTerrainThresholdRuleDto> terrain_threshold_rules;
    WorldgenConnectivityPolicyDto connectivity_policy;
    std::vector<WorldgenResourceRuleDto> resource_rules;
    std::vector<WorldgenGroundItemRuleDto> ground_item_rules;
    std::vector<WorldgenDropRuleDto> drop_rules;
    WorldgenSpawnSafetyDto spawn_safety;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// ContentDraftRegistry - temporary collection during loading
// ============================================================

class ContentDraftRegistry {
public:
    void addObject(ObjectDefinitionContent obj);
    void addAction(ActionDefinitionContent action);
    void addEffect(EffectDefinitionContent effect);
    void addFeedback(FeedbackDefinitionContent feedback);
    void addReaction(ReactionDefinitionContent reaction);
    void addAgent(AgentTemplateContent agent);
    void addThreat(ThreatDefinitionContent threat);
    void addKnowledgeTemplate(KnowledgeTemplateContent knowledge);
    void addScenario(ScenarioDefinitionContent scenario);
    void addWorldgenProfile(WorldgenProfileDefinitionContent profile);
    void addLocale(const std::string& locale_key, LocaleMap locales);

    const std::vector<ObjectDefinitionContent>& objects() const { return objects_; }
    const std::vector<ActionDefinitionContent>& actions() const { return actions_; }
    const std::vector<EffectDefinitionContent>& effects() const { return effects_; }
    const std::vector<FeedbackDefinitionContent>& feedbacks() const { return feedbacks_; }
    const std::vector<ReactionDefinitionContent>& reactions() const { return reactions_; }
    const std::vector<AgentTemplateContent>& agents() const { return agents_; }
    const std::vector<ThreatDefinitionContent>& threats() const { return threats_; }
    const std::vector<KnowledgeTemplateContent>& knowledge_templates() const { return knowledge_templates_; }
    const std::vector<ScenarioDefinitionContent>& scenarios() const { return scenarios_; }
    const std::vector<WorldgenProfileDefinitionContent>& worldgen_profiles() const { return worldgen_profiles_; }
    const std::map<std::string, LocaleMap>& locales() const { return locales_; }

    void clear();

private:
    std::vector<ObjectDefinitionContent> objects_;
    std::vector<ActionDefinitionContent> actions_;
    std::vector<EffectDefinitionContent> effects_;
    std::vector<FeedbackDefinitionContent> feedbacks_;
    std::vector<ReactionDefinitionContent> reactions_;
    std::vector<AgentTemplateContent> agents_;
    std::vector<ThreatDefinitionContent> threats_;
    std::vector<KnowledgeTemplateContent> knowledge_templates_;
    std::vector<ScenarioDefinitionContent> scenarios_;
    std::vector<WorldgenProfileDefinitionContent> worldgen_profiles_;
    std::map<std::string, LocaleMap> locales_;
};

} // namespace pathfinder::content
