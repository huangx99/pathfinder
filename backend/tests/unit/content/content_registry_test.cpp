#include <cassert>
#include <iostream>
#include "pathfinder/content/content_registry.h"
#include "pathfinder/content/content_types.h"

using namespace pathfinder::content;

void run_content_registry_tests() {
    // Test 1: Empty registry returns nullptr for all queries
    {
        ContentDraftRegistry draft;
        auto registry = ContentRegistry::build(draft);

        assert(registry->findObject("nonexistent") == nullptr);
        assert(registry->findEffect("nonexistent") == nullptr);
        assert(registry->findFeedback("nonexistent") == nullptr);
        assert(registry->findReaction("nonexistent") == nullptr);
        assert(registry->findAgent("nonexistent") == nullptr);
        assert(registry->findThreat("nonexistent") == nullptr);
        assert(registry->findKnowledgeTemplate("nonexistent") == nullptr);
        assert(registry->findScenario("nonexistent") == nullptr);

        std::cout << "registry_test_1_empty: passed" << std::endl;
    }

    // Test 2: Register and find object
    {
        ContentDraftRegistry draft;
        ObjectDefinitionContent obj;
        obj.key = ObjectDefinitionKey("red_berry");
        obj.display_key = "object.red_berry.name";
        obj.kind = "food";
        obj.safe_tags = {"fruit", "red"};
        obj.allowed_actions = {"eat"};
        obj.default_quantity = 3;
        draft.addObject(std::move(obj));

        auto registry = ContentRegistry::build(draft);
        const auto* found = registry->findObject("red_berry");
        assert(found != nullptr);
        assert(found->key.value() == "red_berry");
        assert(found->kind == "food");
        assert(found->default_quantity == 3);
        assert(found->safe_tags.size() == 2);

        std::cout << "registry_test_2_find_object: passed" << std::endl;
    }

    // Test 3: Register and find effect
    {
        ContentDraftRegistry draft;
        EffectDefinitionContent eff;
        eff.key = EffectDefinitionKey("restore_hunger");
        eff.display_key = "effect.restore_hunger.name";
        eff.semantic_kind = "actor_need_delta";
        eff.usable_by_ai = true;
        eff.risk_score = 1;
        draft.addEffect(std::move(eff));

        auto registry = ContentRegistry::build(draft);
        const auto* found = registry->findEffect("restore_hunger");
        assert(found != nullptr);
        assert(found->key.value() == "restore_hunger");
        assert(found->usable_by_ai == true);

        std::cout << "registry_test_3_find_effect: passed" << std::endl;
    }

    // Test 4: feedbacksFor query
    {
        ContentDraftRegistry draft;
        FeedbackDefinitionContent fb;
        fb.key = FeedbackDefinitionKey("fb_1");
        fb.object_key = "red_berry";
        fb.action_key = "eat";
        fb.effect_key = "restore_hunger";
        fb.priority = 100;
        draft.addFeedback(std::move(fb));

        auto registry = ContentRegistry::build(draft);
        auto results = registry->feedbacksFor("red_berry", "eat");
        assert(results.size() == 1);
        assert(results[0]->effect_key == "restore_hunger");

        // Wrong object should return empty
        auto empty = registry->feedbacksFor("nonexistent", "eat");
        assert(empty.empty());

        std::cout << "registry_test_4_feedbacks_for: passed" << std::endl;
    }

    // Test 5: reactionsProducing query
    {
        ContentDraftRegistry draft;
        ReactionDefinitionContent reaction;
        reaction.key = ReactionDefinitionKey("make_torch");
        reaction.action_key = "use";
        reaction.effect_key = "make_torch";
        ReactionOutputDto output;
        output.object_key = "torch";
        output.quantity_delta = 1;
        reaction.outputs.push_back(output);
        draft.addReaction(std::move(reaction));

        auto registry = ContentRegistry::build(draft);
        auto producing = registry->reactionsProducing("torch");
        assert(producing.size() == 1);
        assert(producing[0]->key.value() == "make_torch");

        auto empty = registry->reactionsProducing("nonexistent");
        assert(empty.empty());

        std::cout << "registry_test_5_reactions_producing: passed" << std::endl;
    }

    // Test 6: allObjects and allEffects
    {
        ContentDraftRegistry draft;
        ObjectDefinitionContent obj1;
        obj1.key = ObjectDefinitionKey("red_berry");
        obj1.kind = "food";
        draft.addObject(std::move(obj1));

        ObjectDefinitionContent obj2;
        obj2.key = ObjectDefinitionKey("axe");
        obj2.kind = "tool";
        draft.addObject(std::move(obj2));

        auto registry = ContentRegistry::build(draft);
        auto all = registry->allObjects();
        assert(all.size() == 2);

        std::cout << "registry_test_6_all_objects: passed" << std::endl;
    }

    // Test 7: Scenario and agent queries
    {
        ContentDraftRegistry draft;
        AgentTemplateContent agent;
        agent.key = AgentTemplateKey("beast_shadow");
        agent.scale = "small_agent";
        agent.can_learn = true;
        draft.addAgent(std::move(agent));

        ScenarioDefinitionContent scenario;
        scenario.key = ScenarioDefinitionKey("first_night");
        scenario.display_key = "scenario.first_night.name";
        ScenarioInitialObjectDto init_obj;
        init_obj.object_key = "red_berry";
        init_obj.quantity = 3;
        scenario.initial_objects.push_back(init_obj);
        draft.addScenario(std::move(scenario));

        auto registry = ContentRegistry::build(draft);
        assert(registry->findAgent("beast_shadow") != nullptr);
        assert(registry->findScenario("first_night") != nullptr);
        assert(registry->findScenario("first_night")->initial_objects.size() == 1);

        std::cout << "registry_test_7_scenario_agent: passed" << std::endl;
    }

    // Test 8: Locale translate
    {
        ContentDraftRegistry draft;
        LocaleMap locale;
        locale["object.red_berry.name"] = "红果";
        locale["object.red_berry.desc"] = "一种鲜红的果实";
        draft.addLocale("zh_cn", std::move(locale));

        auto registry = ContentRegistry::build(draft);
        assert(registry->translate("zh_cn", "object.red_berry.name") == "红果");
        assert(registry->translate("zh_cn", "nonexistent") == "nonexistent");
        assert(registry->translate("en_us", "object.red_berry.name") == "object.red_berry.name");

        std::cout << "registry_test_8_locale: passed" << std::endl;
    }

    std::cout << "content_registry_tests: all passed" << std::endl;
}
