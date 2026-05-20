#include <cassert>
#include <iostream>
#include <string>
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/content/json_content_loader.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/content/content_runtime_adapter.h"

using namespace pathfinder::content;
using namespace pathfinder::h5_dialog;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: flow" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "flow") {
        // Test: End-to-end flow from JSON content to DialogScenario
        ContentLoadOptions options;
#ifdef PATHFINDER_CONTENT_ROOT
        options.root_path = PATHFINDER_CONTENT_ROOT;
#else
        options.root_path = "../../../content";
#endif
        options.enabled_package_keys = {"core"};

        JsonContentLoader loader;
        auto load_result = loader.load(options);

        // Should load successfully
        assert(load_result.is_ok());
        const auto& result = load_result.value();

        // Should have a registry
        assert(result.registry != nullptr);

        // Should not have fatal errors
        assert(!result.validation_report.hasFatal());

        // Should not have errors
        assert(!result.validation_report.hasErrors());

        const auto& registry = *result.registry;

        // Verify objects loaded
        const auto* red_berry = registry.findObject("red_berry");
        assert(red_berry != nullptr);
        assert(red_berry->kind == "food");
        assert(red_berry->allowed_actions.size() >= 1);

        const auto* axe = registry.findObject("axe");
        assert(axe != nullptr);
        assert(axe->kind == "tool");

        const auto* torch = registry.findObject("torch");
        assert(torch != nullptr);

        const auto* beast = registry.findObject("beast_shadow");
        assert(beast != nullptr);

        // Verify effects loaded
        const auto* restore_hunger = registry.findEffect("restore_hunger");
        assert(restore_hunger != nullptr);
        assert(restore_hunger->operations.size() >= 1);
        assert(restore_hunger->teachable == true);

        const auto* repel_beast = registry.findEffect("repel_beast");
        assert(repel_beast != nullptr);

        const auto* make_torch = registry.findEffect("make_torch");
        assert(make_torch != nullptr);

        // Verify feedbacks loaded with correct references
        auto fb_berry = registry.feedbacksFor("red_berry", "eat");
        assert(!fb_berry.empty());
        assert(fb_berry[0]->effect_key == "restore_hunger");

        auto fb_torch = registry.feedbacksFor("torch", "use", "beast_shadow");
        assert(!fb_torch.empty());

        // Verify reactions loaded
        auto torch_reactions = registry.reactionsProducing("torch");
        assert(!torch_reactions.empty());

        // Verify agents loaded
        const auto* beast_agent = registry.findAgent("beast_shadow");
        assert(beast_agent != nullptr);
        assert(beast_agent->can_learn == true);

        const auto* companion = registry.findAgent("companion");
        assert(companion != nullptr);
        assert(companion->can_teach == true);

        // Verify threats loaded
        const auto* beast_threat = registry.findThreat("beast_shadow");
        assert(beast_threat != nullptr);
        assert(!beast_threat->countermeasures.empty());
        assert(beast_threat->countermeasures[0].effect_key == "repel_beast");
        assert(beast_threat->reentry_enabled == true);

        // Verify scenario loaded with all new P42 fields
        const auto* first_night = registry.findScenario("first_night");
        assert(first_night != nullptr);
        assert(first_night->initial_objects.size() >= 12);
        assert(!first_night->welcome_text_key.empty());
        assert(!first_night->quick_action_input_texts.empty());
        assert(!first_night->suggested_action_templates.empty());
        assert(!first_night->threat_knowledge_templates.empty());

        // Verify knowledge templates loaded (was missing from manifest)
        const auto* kt = registry.findKnowledgeTemplate("knowledge_make_torch");
        assert(kt != nullptr);

        // Verify locale loaded
        assert(registry.translate("zh_cn", "object.red_berry.name") == "红果");
        assert(registry.translate("zh_cn", "object.torch.name") == "火把");

        // Verify content adapter can build a scenario
        ContentRuntimeAdapter adapter(result.registry);
        assert(adapter.hasScenario("first_night"));

        auto effect_semantics = adapter.buildEffectSemantics();
        assert(effect_semantics.is_ok());
        bool has_restore_hunger_semantics = false;
        bool has_repel_beast_semantics = false;
        for (const auto& semantics : effect_semantics.value()) {
            if (semantics.effect_key == "restore_hunger") has_restore_hunger_semantics = true;
            if (semantics.effect_key == "repel_beast") has_repel_beast_semantics = true;
        }
        assert(has_restore_hunger_semantics);
        assert(has_repel_beast_semantics);

        auto effect_specs = adapter.buildEffectSpecs();
        assert(effect_specs.is_ok());
        pathfinder::agent_reasoning::EffectExecutionSpecRegistry spec_registry(false);
        bool has_make_torch_spec = false;
        for (const auto& spec : effect_specs.value()) {
            assert(spec_registry.registerSpec(spec).is_ok());
            if (spec.effect_key == "make_torch") has_make_torch_spec = true;
        }
        assert(has_make_torch_spec);
        assert(spec_registry.findByEffectKey("restore_hunger").is_ok());

        auto scenario_result = adapter.buildDialogScenario("first_night");
        assert(scenario_result.is_ok());

        const auto& scenario = scenario_result.value();
        assert(scenario.scenario_key == "first_night");

        // Should have welcome_text from locale (not hardcoded fallback)
        assert(!scenario.welcome_text.empty());

        // Should have objects from the scenario (all 12)
        bool has_red_berry = false, has_axe = false, has_torch = false, has_beast = false;
        bool has_decayed = false, has_bitter = false, has_stone = false;
        bool has_wood = false, has_whetstone = false, has_dry_grass = false;
        bool has_fire_seed = false, has_camp_fire = false;
        for (const auto& obj : scenario.objects) {
            if (obj.object_key == "red_berry") {
                has_red_berry = true;
                assert(obj.initial_quantity == 3.0);
            }
            if (obj.object_key == "axe") {
                has_axe = true;
                assert(obj.initial_numeric_states.at("sharpness") == 3.0);
            }
            if (obj.object_key == "torch") has_torch = true;
            if (obj.object_key == "beast_shadow") has_beast = true;
            if (obj.object_key == "decayed_red_berry") has_decayed = true;
            if (obj.object_key == "bitter_leaf") has_bitter = true;
            if (obj.object_key == "stone_flake") has_stone = true;
            if (obj.object_key == "wood") {
                has_wood = true;
                assert(obj.initial_quantity == 2.0);
                assert(obj.initial_numeric_states.at("processed") == 0.0);
            }
            if (obj.object_key == "whetstone") has_whetstone = true;
            if (obj.object_key == "dry_grass") has_dry_grass = true;
            if (obj.object_key == "fire_seed") has_fire_seed = true;
            if (obj.object_key == "camp_fire") {
                has_camp_fire = true;
                assert(obj.initial_quantity == 0.0);
            }
        }
        assert(has_red_berry);
        assert(has_axe);
        assert(has_torch);
        assert(has_beast);
        assert(has_decayed);
        assert(has_bitter);
        assert(has_stone);
        assert(has_wood);
        assert(has_whetstone);
        assert(has_dry_grass);
        assert(has_fire_seed);
        assert(has_camp_fire);

        // Should have feedbacks
        assert(!scenario.feedbacks.empty());

        // Should have quick_action_input_texts from JSON
        assert(!scenario.quick_action_input_texts.empty());

        // Should have suggested_action_templates from JSON
        assert(!scenario.suggested_action_templates.empty());
        bool has_make_torch_action = false;
        for (const auto& act : scenario.suggested_action_templates) {
            if (act.action_key == "make_torch") has_make_torch_action = true;
        }
        assert(has_make_torch_action);

        // Should have threat_knowledge_templates from JSON
        assert(!scenario.threat_knowledge_templates.empty());
        assert(scenario.threat_knowledge_templates[0].threat_object_key == "beast_shadow");

        std::cout << "integration_flow_test: all passed" << std::endl;
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
