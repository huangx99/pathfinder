#include <cassert>
#include <iostream>
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/content/content_runtime_adapter.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/content/content_types.h"

using namespace pathfinder::content;
using namespace pathfinder::h5_dialog;

void run_content_runtime_adapter_tests() {
    // Test 1: Adapter can check for scenario existence
    {
        ContentDraftRegistry draft;
        ScenarioDefinitionContent scenario;
        scenario.key = ScenarioDefinitionKey("first_night");
        scenario.display_key = "scenario.first_night.name";
        ScenarioInitialObjectDto init_obj;
        init_obj.object_key = "red_berry";
        init_obj.quantity = 3;
        scenario.initial_objects.push_back(init_obj);
        draft.addScenario(std::move(scenario));

        auto registry = ContentRegistry::build(draft);
        ContentRuntimeAdapter adapter(registry);

        assert(adapter.hasScenario("first_night"));
        assert(!adapter.hasScenario("nonexistent"));

        std::cout << "adapter_test_1_has_scenario: passed" << std::endl;
    }

    // Test 2: Build DialogScenarioObject from object definition
    {
        ContentDraftRegistry draft;
        ObjectDefinitionContent obj;
        obj.key = ObjectDefinitionKey("red_berry");
        obj.display_key = "object.red_berry.name";
        obj.description_key = "object.red_berry.desc";
        obj.kind = "food";
        obj.allowed_actions = {"eat", "inspect"};
        obj.safe_trait_keys = {"可食用", "植物果实"};
        draft.addObject(std::move(obj));

        // Add locale
        LocaleMap locale;
        locale["object.red_berry.name"] = "红果";
        locale["object.red_berry.desc"] = "一种鲜红的果实";
        draft.addLocale("zh_cn", std::move(locale));

        auto registry = ContentRegistry::build(draft);
        ContentRuntimeAdapter adapter(registry);

        const auto* obj_def = registry->findObject("red_berry");
        assert(obj_def != nullptr);

        auto result = adapter.buildDialogObject(*obj_def);
        assert(result.is_ok());

        const auto& dialog_obj = result.value();
        assert(dialog_obj.object_key == "red_berry");
        assert(dialog_obj.display_name == "红果");
        assert(dialog_obj.player_description == "一种鲜红的果实");
        assert(!dialog_obj.allowed_actions.empty());

        std::cout << "adapter_test_2_build_object: passed" << std::endl;
    }

    // Test 3: Build DialogFeedbackTemplate from feedback definition
    {
        ContentDraftRegistry draft;
        FeedbackDefinitionContent fb;
        fb.key = FeedbackDefinitionKey("test_fb");
        fb.object_key = "red_berry";
        fb.action_key = "eat";
        fb.effect_key = "restore_hunger";
        fb.priority = 100;
        fb.utility_delta = 0.6;
        fb.risk_delta = 0.0;
        fb.causal_tags = {"food", "hunger"};
        draft.addFeedback(std::move(fb));

        auto registry = ContentRegistry::build(draft);
        ContentRuntimeAdapter adapter(registry);

        const auto* fb_def = registry->findFeedback("test_fb");
        assert(fb_def != nullptr);

        auto result = adapter.buildDialogFeedback(*fb_def);
        assert(result.is_ok());

        const auto& dialog_fb = result.value();
        assert(dialog_fb.feedback_key == "test_fb");
        assert(dialog_fb.object_key == "red_berry");
        assert(dialog_fb.effect_key == "restore_hunger");
        assert(dialog_fb.utility_delta == 0.6);

        std::cout << "adapter_test_3_build_feedback: passed" << std::endl;
    }

    // Test 4: Build complete DialogScenario
    {
        ContentDraftRegistry draft;

        // Objects
        ObjectDefinitionContent berry;
        berry.key = ObjectDefinitionKey("red_berry");
        berry.display_key = "object.red_berry.name";
        berry.description_key = "object.red_berry.desc";
        berry.kind = "food";
        berry.allowed_actions = {"eat"};
        berry.safe_trait_keys = {"可食用"};
        draft.addObject(std::move(berry));

        ObjectDefinitionContent axe;
        axe.key = ObjectDefinitionKey("axe");
        axe.display_key = "object.axe.name";
        axe.description_key = "object.axe.desc";
        axe.kind = "tool";
        axe.allowed_actions = {"use"};
        axe.safe_trait_keys = {"伐木工具"};
        draft.addObject(std::move(axe));

        // Feedback
        FeedbackDefinitionContent fb;
        fb.key = FeedbackDefinitionKey("red_berry_eat");
        fb.object_key = "red_berry";
        fb.action_key = "eat";
        fb.effect_key = "restore_hunger";
        fb.priority = 100;
        fb.utility_delta = 0.6;
        fb.causal_tags = {"food"};
        draft.addFeedback(std::move(fb));

        // Scenario
        ScenarioDefinitionContent scenario;
        scenario.key = ScenarioDefinitionKey("first_night");
        scenario.display_key = "scenario.first_night.name";
        ScenarioInitialObjectDto init_berry;
        init_berry.object_key = "red_berry";
        init_berry.quantity = 3;
        scenario.initial_objects.push_back(init_berry);
        ScenarioInitialObjectDto init_axe;
        init_axe.object_key = "axe";
        init_axe.quantity = 1;
        scenario.initial_objects.push_back(init_axe);
        draft.addScenario(std::move(scenario));

        // Locale
        LocaleMap locale;
        locale["object.red_berry.name"] = "红果";
        locale["object.red_berry.desc"] = "一种鲜红的果实";
        locale["object.axe.name"] = "斧头";
        locale["object.axe.desc"] = "一把石斧";
        locale["scenario.first_night.name"] = "第一个夜晚";
        draft.addLocale("zh_cn", std::move(locale));

        auto registry = ContentRegistry::build(draft);
        ContentRuntimeAdapter adapter(registry);

        auto result = adapter.buildDialogScenario("first_night");
        assert(result.is_ok());

        const auto& dialog_scenario = result.value();
        assert(dialog_scenario.scenario_key == "first_night");
        assert(dialog_scenario.objects.size() == 2);
        assert(dialog_scenario.feedbacks.size() >= 1);

        // Verify first object is red_berry
        bool found_berry = false;
        bool found_axe = false;
        for (const auto& obj : dialog_scenario.objects) {
            if (obj.object_key == "red_berry") {
                found_berry = true;
                assert(obj.initial_quantity == 3.0);
            }
            if (obj.object_key == "axe") {
                found_axe = true;
                assert(obj.initial_quantity == 1.0);
            }
        }
        assert(found_berry);
        assert(found_axe);

        std::cout << "adapter_test_4_build_scenario: passed" << std::endl;
    }

    // Test 5: Non-existent scenario returns error
    {
        ContentDraftRegistry draft;
        auto registry = ContentRegistry::build(draft);
        ContentRuntimeAdapter adapter(registry);

        auto result = adapter.buildDialogScenario("nonexistent");
        assert(result.is_error());

        std::cout << "adapter_test_5_missing_scenario: passed" << std::endl;
    }

    // Test 6: Build effect semantics and execution specs from JSON content definitions
    {
        ContentDraftRegistry draft;

        EffectDefinitionContent effect;
        effect.key = EffectDefinitionKey("test_restore_hunger");
        effect.display_key = "effect.test_restore_hunger.name";
        effect.semantic_kind = "actor_need_delta";
        effect.goal_kinds = {"satisfy_need"};
        effect.target_kind = "actor_self";
        effect.preconditions = {"condition:test:eq:object.quantity.test_food.gte.1"};
        effect.usable_by_ai = true;
        effect.risk_score = 1;
        effect.time_cost = 1;
        EffectOperationDto consume;
        consume.op = "consume_object_quantity";
        consume.target = "$object";
        consume.quantity = 1;
        consume.summary_key = "effect.test_restore_hunger.consume";
        effect.operations.push_back(consume);
        EffectOperationDto need;
        need.op = "change_actor_need";
        need.target = "$actor";
        need.need_key = "hunger";
        need.delta = -30.0;
        need.summary_key = "effect.test_restore_hunger.need";
        effect.operations.push_back(need);
        draft.addEffect(std::move(effect));

        LocaleMap locale;
        locale["effect.test_restore_hunger.name"] = "测试缓解饥饿";
        locale["effect.test_restore_hunger.consume"] = "消耗测试食物。";
        locale["effect.test_restore_hunger.need"] = "饥饿下降。";
        draft.addLocale("zh_cn", std::move(locale));

        auto registry = ContentRegistry::build(draft);
        ContentRuntimeAdapter adapter(registry);

        auto semantics = adapter.buildEffectSemantics();
        assert(semantics.is_ok());
        assert(semantics.value().size() == 1);
        assert(semantics.value()[0].effect_key == "test_restore_hunger");
        assert(!semantics.value()[0].goal_affinities.empty());
        assert(!semantics.value()[0].state_deltas.empty());

        auto specs = adapter.buildEffectSpecs();
        assert(specs.is_ok());
        assert(specs.value().size() == 1);
        assert(specs.value()[0].effect_key == "test_restore_hunger");
        assert(specs.value()[0].operations.size() == 2);

        pathfinder::agent_reasoning::EffectExecutionSpecRegistry spec_registry(false);
        assert(spec_registry.registerSpec(specs.value()[0]).is_ok());
        assert(spec_registry.findByEffectKey("test_restore_hunger").is_ok());

        std::cout << "adapter_test_6_build_effect_runtime_specs: passed" << std::endl;
    }

    std::cout << "content_runtime_adapter_tests: all passed" << std::endl;
}
