#include "pathfinder/story/story_system.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::story {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::h5_projection::ActionAffordanceKind;
using pathfinder::h5_projection::ProjectionAudience;
using pathfinder::h5_projection::SafeTextKind;
using pathfinder::h5_projection::makeSafeText;

namespace {

Result<void> failVoid(const std::string& message) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, message));
}

template <typename T>
bool containsValue(const std::vector<T>& values, const T& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool containsAny(const std::vector<std::string>& values, const std::vector<std::string>& candidates) {
    for (const auto& candidate : candidates) {
        if (containsValue(values, candidate)) return true;
    }
    return false;
}

StoryMaterialEntry material(std::string key,
                            std::string name,
                            std::vector<StoryMaterialRole> roles,
                            std::vector<StoryActionRole> actions,
                            std::vector<std::string> knowledge_paths = {},
                            std::vector<std::string> reaction_rules = {},
                            std::vector<std::string> danger_counters = {}) {
    StoryMaterialEntry entry;
    entry.object_key = std::move(key);
    entry.display_name_zh_cn = std::move(name);
    entry.roles = std::move(roles);
    entry.expected_actions = std::move(actions);
    entry.knowledge_paths = std::move(knowledge_paths);
    entry.reaction_rule_keys = std::move(reaction_rules);
    entry.danger_counter_keys = std::move(danger_counters);
    return entry;
}

StoryObjectiveDefinition objective(std::string key, std::string title, std::string hint, StoryObjectiveStatus status) {
    StoryObjectiveDefinition definition;
    definition.objective_key = std::move(key);
    definition.title_zh_cn = std::move(title);
    definition.hint_zh_cn = std::move(hint);
    definition.initial_status = status;
    return definition;
}

std::string phaseText(StoryTimePhase phase) {
    switch (phase) {
        case StoryTimePhase::Morning: return "早晨";
        case StoryTimePhase::Afternoon: return "午后";
        case StoryTimePhase::Dusk: return "黄昏";
        case StoryTimePhase::Night: return "夜晚";
        case StoryTimePhase::Survived: return "已撑过第一夜";
        case StoryTimePhase::Failed: return "第一夜失败";
        case StoryTimePhase::Unknown: return "未知";
        case StoryTimePhase::TestOnly: return "测试";
    }
    return "未知";
}

bool isProgressingInput(const std::string& input) {
    if (input.empty()) return false;
    if (input.find("查看") != std::string::npos) return false;
    if (input.find("帮助") != std::string::npos) return false;
    return true;
}

StoryTimePhase phaseForBudget(const StoryTimeBudget& budget, int used) {
    if (used >= budget.dusk_to_night + 1) return StoryTimePhase::Night;
    if (used >= budget.dusk_to_night) return StoryTimePhase::Night;
    if (used >= budget.afternoon_to_dusk) return StoryTimePhase::Dusk;
    if (used >= budget.morning_to_afternoon) return StoryTimePhase::Afternoon;
    return StoryTimePhase::Morning;
}

bool hasCounterObject(const StoryEvaluationContext& context, const StoryScenarioDefinition& scenario) {
    for (const auto& threat : scenario.threats) {
        if (containsAny(context.available_object_keys, threat.counter_object_keys)) return true;
    }
    return containsAny(context.completed_action_keys, {"made_torch", "lit_fire", "repelled_beast"});
}

bool hasCounterKnowledge(const StoryEvaluationContext& context, const StoryScenarioDefinition& scenario) {
    for (const auto& threat : scenario.threats) {
        if (containsAny(context.actor_knowledge_effect_keys, threat.counter_knowledge_keys)) return true;
        if (containsAny(context.recipient_knowledge_effect_keys, threat.counter_knowledge_keys)) return true;
        if (containsValue(threat.counter_knowledge_keys, context.last_effect_key)) return true;
    }
    return false;
}

bool lastEffectMatches(const StoryEvaluationContext& context, const std::string& effect_key) {
    return !effect_key.empty() && context.last_effect_key == effect_key;
}

const StorySuggestedActionDefinition* findSuggestedActionByEffect(const StoryScenarioDefinition& scenario, const std::string& effect_key) {
    if (effect_key.empty()) return nullptr;
    for (const auto& action : scenario.suggested_actions) {
        if (action.required_effect_key == effect_key) return &action;
    }
    return nullptr;
}

bool taughtCompanion(const StoryEvaluationContext& context) {
    return context.teach_action_happened || !context.recipient_knowledge_effect_keys.empty();
}

pathfinder::h5_projection::ActionProjection storyAction(const std::string& key, const std::string& label, const std::string& input) {
    pathfinder::h5_projection::ActionProjection action;
    action.action_key = "story.action." + key;
    action.affordance = ActionAffordanceKind::Inspect;
    action.label = makeSafeText(action.action_key + ".label", SafeTextKind::Feedback, label);
    action.input_text = input;
    action.enabled = true;
    action.command_preview_key = "story.command." + key;
    return action;
}

std::vector<pathfinder::h5_projection::ActionProjection> suggestedActionsForScenario(const StoryScenarioDefinition& scenario) {
    std::vector<pathfinder::h5_projection::ActionProjection> actions;
    for (const auto& config : scenario.suggested_actions) {
        actions.push_back(storyAction(config.action_key, config.label_zh_cn, config.input_text));
    }
    return actions;
}

} // namespace

// Story is a temporary H5 prototype projection layer: it may summarize objectives,
// hints, and chapter pacing, but must not become the long-term authority for world
// objects, map state, NPC autonomy, resources, or real interaction rules. When the
// open-world map/content package lands, move those responsibilities to world/content
// systems and keep Story as a presentation/planning projection only.
Result<StoryScenarioDefinition> StoryScenarioRegistry::firstDaySurvival() const {
    StoryScenarioDefinition scenario;
    scenario.scenario_key = "first_day_survival";
    scenario.kind = StoryScenarioKind::FirstDaySurvival;
    scenario.title_zh_cn = "第一天生存";
    scenario.initial_time_phase = StoryTimePhase::Morning;
    scenario.initial_location_key = "forest_edge";
    scenario.visible_object_keys = {"red_berry", "decayed_red_berry", "bitter_leaf", "stone_flake", "axe", "wood", "whetstone", "dry_grass", "fire_seed"};
    scenario.material_entries = {
        material("red_berry", "红果", {StoryMaterialRole::FoodCandidate}, {StoryActionRole::Experiment}, {"restore_hunger"}),
        material("decayed_red_berry", "腐烂红果", {StoryMaterialRole::FoodCandidate}, {StoryActionRole::Experiment}, {"poison"}),
        material("bitter_leaf", "苦叶", {StoryMaterialRole::FoodCandidate}, {StoryActionRole::Experiment}, {"no_visible_effect"}),
        material("stone_flake", "石片", {StoryMaterialRole::Tool}, {StoryActionRole::Experiment}, {"use_hint"}),
        material("axe", "斧头", {StoryMaterialRole::Tool}, {StoryActionRole::TargetUse}, {"cut_wood", "tool_dull"}),
        material("wood", "木头", {StoryMaterialRole::Fuel}, {StoryActionRole::TargetUse, StoryActionRole::Combine}, {"cut_wood"}, {"wood_fire_seed_to_camp_fire"}),
        material("whetstone", "磨石", {StoryMaterialRole::Tool}, {StoryActionRole::TargetUse}, {"restore_sharpness"}),
        material("dry_grass", "干草", {StoryMaterialRole::Fuel}, {StoryActionRole::Combine}, {"ignite_fire"}, {"dry_grass_fire_seed_to_camp_fire"}),
        material("fire_seed", "火种", {StoryMaterialRole::IgnitionSource}, {StoryActionRole::Combine}, {"ignite_fire"}, {"dry_grass_fire_seed_to_camp_fire"}),
        material("camp_fire", "火堆", {StoryMaterialRole::LightSource, StoryMaterialRole::ThreatCounter, StoryMaterialRole::CraftResult}, {StoryActionRole::Combine, StoryActionRole::TargetUse}, {"ignite_fire", "repel_beast"}, {}, {"approaching_beast"}),
        material("torch", "火把", {StoryMaterialRole::LightSource, StoryMaterialRole::ThreatCounter, StoryMaterialRole::CraftResult}, {StoryActionRole::Combine, StoryActionRole::TargetUse}, {"make_torch", "repel_beast"}, {"wood_fire_to_torch"}, {"approaching_beast"})
    };
    auto explore = objective("explore_food_and_tools", "探索周围材料", "先尝试食物和工具，形成最早的知识。", StoryObjectiveStatus::Active);
    auto fire = objective("prepare_fire_source", "准备火源或火把", "黄昏前准备能照明或驱赶野兽的东西。", StoryObjectiveStatus::Active);
    fire.required_object_keys = {"torch", "camp_fire"};
    fire.required_knowledge_keys = {"ignite_fire", "make_torch", "repel_beast"};
    auto teach = objective("teach_companion_key_knowledge", "教同伴关键知识", "把火、火把或工具的关键用途告诉同伴。", StoryObjectiveStatus::Active);
    auto survive = objective("survive_first_night", "撑过第一个夜晚", "夜晚来临时，用火光或火把解决靠近的危险。", StoryObjectiveStatus::Blocked);
    survive.required_threat_states = {"approaching_beast:mitigated"};
    scenario.objectives = {explore, fire, teach, survive};
    StoryThreatDefinition beast;
    beast.threat_key = "approaching_beast";
    beast.kind = StoryThreatKind::ApproachingBeast;
    beast.starts_at_phase = StoryTimePhase::Dusk;
    beast.resolves_at_phase = StoryTimePhase::Night;
    beast.warning_zh_cn = "树林里传来低吼声，有什么东西正在靠近。";
    beast.counter_knowledge_keys = {"repel_beast", "ignite_fire", "make_torch"};
    beast.counter_object_keys = {"torch", "camp_fire", "fire_seed"};
    beast.success_feedback_key = "beast_repelled_by_fire";
    beast.failure_feedback_key = "beast_pressure_unanswered";
    scenario.threats = {beast};
    scenario.suggested_actions = {
        StorySuggestedActionDefinition{"make_torch", "制作火把", "制作火把", "make_torch", "", {"torch"}},
        StorySuggestedActionDefinition{"light_fire", "点燃火源", "点燃火源", "ignite_fire", "", {"camp_fire"}},
        StorySuggestedActionDefinition{"repel_beast", "用火把驱赶野兽", "用火把驱赶野兽", "repel_beast", "beast_shadow", {}},
        StorySuggestedActionDefinition{"inspect_objective", "查看目标", "查看目标", "", "", {}}
    };
    scenario.time_budget = StoryTimeBudget{2, 5, 8};
    scenario.success_objective_key = "survive_first_night";
    auto valid = scenario.validateBasic();
    if (valid.is_error()) return Result<StoryScenarioDefinition>::fail(valid.errors());
    return Result<StoryScenarioDefinition>::ok(std::move(scenario));
}

Result<StoryScenarioDefinition> StoryScenarioRegistry::getScenarioByKey(const std::string& key) const {
    if (key == "first_day_survival") return firstDaySurvival();
    return Result<StoryScenarioDefinition>::fail(makeError(ErrorCode::id_not_found, "story scenario not found: " + key));
}

Result<StoryRuntimeState> StoryRuntimeFactory::createInitialState(const StoryScenarioDefinition& scenario) const {
    auto valid = scenario.validateBasic();
    if (valid.is_error()) return Result<StoryRuntimeState>::fail(valid.errors());
    StoryRuntimeState state;
    state.scenario_key = scenario.scenario_key;
    state.current_phase = scenario.initial_time_phase;
    state.turn_index = 0;
    state.time_points_used = 0;
    state.discovered_object_keys = scenario.visible_object_keys;
    for (const auto& objective_definition : scenario.objectives) {
        StoryObjectiveState objective_state;
        objective_state.objective_key = objective_definition.objective_key;
        objective_state.status = objective_definition.initial_status;
        state.objective_states.push_back(objective_state);
    }
    for (const auto& threat_definition : scenario.threats) {
        StoryThreatRuntimeState threat_state;
        threat_state.threat_key = threat_definition.threat_key;
        state.threat_states.push_back(threat_state);
    }
    auto state_valid = state.validateBasic();
    if (state_valid.is_error()) return Result<StoryRuntimeState>::fail(state_valid.errors());
    return Result<StoryRuntimeState>::ok(std::move(state));
}

Result<std::vector<StoryObjectiveState>> StoryObjectiveEvaluator::evaluate(
    const StoryScenarioDefinition& scenario,
    const StoryRuntimeState& state,
    const StoryEvaluationContext& context) const {
    auto context_valid = context.validateBasic();
    if (context_valid.is_error()) return Result<std::vector<StoryObjectiveState>>::fail(context_valid.errors());
    std::vector<StoryObjectiveState> objectives = state.objective_states;
    auto setStatus = [&](const std::string& key, StoryObjectiveStatus status, std::string reason) {
        for (auto& objective : objectives) {
            if (objective.objective_key == key) {
                objective.status = status;
                if (!containsValue(objective.reason_keys, reason)) objective.reason_keys.push_back(std::move(reason));
            }
        }
    };
    if (context.actor_knowledge_effect_keys.size() >= 2 || state.time_points_used >= 2) {
        setStatus("explore_food_and_tools", StoryObjectiveStatus::Completed, "story.explored_materials");
    }
    if (hasCounterObject(context, scenario) || hasCounterKnowledge(context, scenario) || containsAny(state.generated_object_keys, {"torch", "camp_fire"})) {
        setStatus("prepare_fire_source", StoryObjectiveStatus::Completed, "story.prepared_fire_source");
    } else if (state.current_phase == StoryTimePhase::Dusk || state.current_phase == StoryTimePhase::Night) {
        setStatus("prepare_fire_source", StoryObjectiveStatus::Blocked, "story.need_fire_source");
    }
    if (taughtCompanion(context)) {
        setStatus("teach_companion_key_knowledge", StoryObjectiveStatus::Completed, "story.taught_companion");
    }
    bool beast_mitigated = false;
    for (const auto& threat : state.threat_states) {
        if (threat.threat_key == "approaching_beast" && threat.mitigated) beast_mitigated = true;
    }
    if (state.current_phase == StoryTimePhase::Survived || beast_mitigated) {
        setStatus("survive_first_night", StoryObjectiveStatus::Completed, "story.survived_first_night");
    } else if (state.current_phase == StoryTimePhase::Night || state.current_phase == StoryTimePhase::Dusk) {
        setStatus("survive_first_night", StoryObjectiveStatus::Active, "story.night_threat_active");
    }
    for (const auto& objective : objectives) {
        auto valid = objective.validateBasic();
        if (valid.is_error()) return Result<std::vector<StoryObjectiveState>>::fail(valid.errors());
    }
    return Result<std::vector<StoryObjectiveState>>::ok(std::move(objectives));
}

Result<StoryTurnResult> StoryProgressionService::applyTurn(
    const StoryScenarioDefinition& scenario,
    const StoryRuntimeState& current_state,
    const StoryEvaluationContext& context) const {
    auto scenario_valid = scenario.validateBasic();
    if (scenario_valid.is_error()) return Result<StoryTurnResult>::fail(scenario_valid.errors());
    auto state_valid = current_state.validateBasic();
    if (state_valid.is_error()) return Result<StoryTurnResult>::fail(state_valid.errors());
    auto context_valid = context.validateBasic();
    if (context_valid.is_error()) return Result<StoryTurnResult>::fail(context_valid.errors());

    StoryTurnResult result;
    result.new_state = current_state;
    result.new_state.turn_index += 1;
    result.new_state.last_story_event_keys.clear();
    if (isProgressingInput(context.last_input_text)) {
        result.new_state.time_points_used += 1;
    }
    result.new_state.current_phase = phaseForBudget(scenario.time_budget, result.new_state.time_points_used);

    if (const auto* action = findSuggestedActionByEffect(scenario, context.last_effect_key)) {
        for (const auto& object_key : action->generated_object_keys) {
            if (!containsValue(result.new_state.generated_object_keys, object_key)) result.new_state.generated_object_keys.push_back(object_key);
        }
        if (lastEffectMatches(context, "make_torch")) {
            result.story_event_keys.push_back("story.torch_prepared");
            result.feedback_zh_cn.push_back("你把材料整理成可以举起的火把。火光也许能让夜里的东西退开。");
        } else if (lastEffectMatches(context, "ignite_fire")) {
            result.story_event_keys.push_back("story.fire_prepared");
        }
    }

    for (auto& threat : result.new_state.threat_states) {
        if (threat.threat_key != "approaching_beast") continue;
        if (result.new_state.current_phase == StoryTimePhase::Dusk || result.new_state.current_phase == StoryTimePhase::Night) {
            threat.active = true;
            if (!containsValue(threat.reason_keys, std::string("story.beast_warning"))) threat.reason_keys.push_back("story.beast_warning");
        }
        if (result.new_state.current_phase == StoryTimePhase::Night) {
            threat.resolved = true;
            threat.mitigated = hasCounterObject(context, scenario) || hasCounterKnowledge(context, scenario) || containsAny(result.new_state.generated_object_keys, {"torch", "camp_fire"});
            result.new_state.current_phase = threat.mitigated ? StoryTimePhase::Survived : StoryTimePhase::Failed;
            result.story_event_keys.push_back(threat.mitigated ? "story.beast_repelled" : "story.beast_unanswered");
            result.feedback_zh_cn.push_back(threat.mitigated ? "火光让靠近的影子退开了，你和同伴撑过了第一个夜晚。" : "夜里的低吼越来越近。你还没有准备足够的火源，只能慌乱撤离。第一夜没有成功撑过去。");
        }
    }

    auto objectives = evaluator_.evaluate(scenario, result.new_state, context);
    if (objectives.is_error()) return Result<StoryTurnResult>::fail(objectives.errors());
    result.new_state.objective_states = std::move(objectives.value());
    for (const auto& event_key : result.story_event_keys) result.new_state.last_story_event_keys.push_back(event_key);
    auto valid = result.validateBasic();
    if (valid.is_error()) return Result<StoryTurnResult>::fail(valid.errors());
    return Result<StoryTurnResult>::ok(std::move(result));
}

Result<StoryProjection> StoryProjectionAdapter::composeProjection(
    const StoryScenarioDefinition& scenario,
    const StoryRuntimeState& state,
    const StoryEvaluationContext& context,
    ProjectionAudience audience) const {
    auto scenario_valid = scenario.validateBasic();
    if (scenario_valid.is_error()) return Result<StoryProjection>::fail(scenario_valid.errors());
    auto state_valid = state.validateBasic();
    if (state_valid.is_error()) return Result<StoryProjection>::fail(state_valid.errors());

    StoryProjection projection;
    projection.scenario_key = scenario.scenario_key;
    projection.scenario_title = makeSafeText("story.scenario." + scenario.scenario_key, SafeTextKind::DisplayName, scenario.title_zh_cn);
    projection.time_phase_label = makeSafeText("story.phase." + toString(state.current_phase), SafeTextKind::Hint, "时间：" + phaseText(state.current_phase));
    projection.main_objective = makeSafeText("story.objective.main", SafeTextKind::Hint, "目标：在入夜前准备火源或火把，并把关键知识告诉同伴。随时间推进，树林里的危险会靠近。");

    for (const auto& objective_state : state.objective_states) {
        const auto* definition = static_cast<const StoryObjectiveDefinition*>(nullptr);
        for (const auto& candidate : scenario.objectives) {
            if (candidate.objective_key == objective_state.objective_key) definition = &candidate;
        }
        if (!definition) continue;
        StoryObjectiveCardProjection card;
        card.objective_key = objective_state.objective_key;
        card.status = objective_state.status;
        card.title = makeSafeText("story.objective." + objective_state.objective_key + ".title", SafeTextKind::DisplayName, definition->title_zh_cn + "（" + toString(objective_state.status) + "）");
        card.hint = makeSafeText("story.objective." + objective_state.objective_key + ".hint", SafeTextKind::Hint, definition->hint_zh_cn);
        projection.objective_cards.push_back(std::move(card));
    }

    for (const auto& threat_state : state.threat_states) {
        if (!threat_state.active) continue;
        for (const auto& threat : scenario.threats) {
            if (threat.threat_key == threat_state.threat_key) {
                projection.threat_warnings.push_back(makeSafeText("story.threat." + threat.threat_key, SafeTextKind::Warning, threat.warning_zh_cn));
            }
        }
    }
    if (state.current_phase == StoryTimePhase::Morning) {
        projection.material_hints.push_back(makeSafeText("story.hint.morning", SafeTextKind::Hint, "先尝试食物和工具，形成可靠知识。"));
    } else if (state.current_phase == StoryTimePhase::Afternoon) {
        projection.material_hints.push_back(makeSafeText("story.hint.afternoon", SafeTextKind::Hint, "午后适合准备木头、干草和火源。"));
    } else if (state.current_phase == StoryTimePhase::Dusk) {
        projection.material_hints.push_back(makeSafeText("story.hint.dusk", SafeTextKind::Warning, "黄昏到了，最好准备火把或火堆。"));
    }
    if (!taughtCompanion(context)) {
        projection.material_hints.push_back(makeSafeText("story.hint.teach", SafeTextKind::Hint, "同伴不会自动知道你的经验，记得教同伴。"));
    }

    auto configured_actions = suggestedActionsForScenario(scenario);
    projection.suggested_actions.insert(projection.suggested_actions.end(), configured_actions.begin(), configured_actions.end());
    if (audience != ProjectionAudience::Player) projection.debug_keys.push_back("story.projection.first_day_survival");

    auto valid = projection.validateBasic();
    if (valid.is_error()) return Result<StoryProjection>::fail(valid.errors());
    return Result<StoryProjection>::ok(std::move(projection));
}

} // namespace pathfinder::story
