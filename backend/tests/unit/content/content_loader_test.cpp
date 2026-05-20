#include <cassert>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include "pathfinder/content/json_content_loader.h"
#include "pathfinder/content/content_types.h"
#include "pathfinder/content/content_registry.h"

using namespace pathfinder::content;

// Forward declarations from other test files (linked into same executable)
void run_content_registry_tests();
void run_content_validation_tests();
void run_content_runtime_adapter_tests();

void run_content_loader_tests() {
    // Test 1: Path safety checks
    {
        assert(!JsonContentLoader::isPathSafe(""));
        assert(!JsonContentLoader::isPathSafe("../escape.json"));
        assert(!JsonContentLoader::isPathSafe("/absolute/path.json"));
        assert(JsonContentLoader::isPathSafe("objects/test.json"));
        assert(JsonContentLoader::isPathSafe("subdir/file.json"));

        std::cout << "loader_test_1_path_safety: passed" << std::endl;
    }

    // Test 2: Manifest parsing
    {
        std::string manifest_json = R"({
            "package_key": "core",
            "display_name": "核心内容包",
            "content_version": "0.1.0",
            "schema_version": "content_schema_1",
            "locale_default": "zh_cn",
            "load_order": 0,
            "files": [
                {"path": "objects/test.json", "category": "objects", "required": true},
                {"path": "effects/test.json", "category": "effects", "required": true}
            ]
        })";

        auto result = JsonContentParser::parseManifest(manifest_json);
        assert(result.is_ok());

        const auto& manifest = result.value();
        assert(manifest.package_key == "core");
        assert(manifest.content_version == "0.1.0");
        assert(manifest.files.size() == 2);
        assert(manifest.files[0].path == "objects/test.json");
        assert(manifest.files[0].category == ContentFileCategory::Objects);

        std::cout << "loader_test_2_manifest_parse: passed" << std::endl;
    }

    // Test 3: Objects JSON parsing
    {
        std::string objects_json = R"({
            "objects": [
                {
                    "object_key": "red_berry",
                    "display_key": "object.red_berry.name",
                    "description_key": "object.red_berry.desc",
                    "kind": "food",
                    "visibility": "runtime_visible",
                    "safe_tags": ["fruit", "red"],
                    "allowed_actions": ["eat", "inspect"],
                    "initial_state": {
                        "quantity": 3,
                        "numeric": {},
                        "tags": []
                    },
                    "projection": {
                        "safe_trait_keys": ["可食用"],
                        "unknown_use_text_key": "object.common.unknown_use"
                    }
                }
            ]
        })";

        auto result = JsonContentParser::parseObjects(objects_json);
        assert(result.is_ok());

        const auto& file_dto = result.value();
        assert(file_dto.objects.size() == 1);
        assert(file_dto.objects[0].object_key == "red_berry");
        assert(file_dto.objects[0].kind == "food");
        assert(file_dto.objects[0].allowed_actions.size() == 2);

        std::cout << "loader_test_3_objects_parse: passed" << std::endl;
    }

    // Test 4: Effects JSON parsing with operations
    {
        std::string effects_json = R"({
            "effects": [
                {
                    "effect_key": "restore_hunger",
                    "display_key": "effect.restore_hunger.name",
                    "semantic_kind": "actor_need_delta",
                    "goal_kinds": ["satisfy_need"],
                    "target_kind": "actor_self",
                    "preconditions": ["condition:test:eq:object.quantity.$object.gte.1"],
                    "operations": [
                        {
                            "op": "consume_object_quantity",
                            "target": "$object",
                            "quantity": 1,
                            "summary_key": "effect.restore_hunger.consume"
                        },
                        {
                            "op": "change_actor_need",
                            "target": "$actor",
                            "need_key": "hunger",
                            "delta": -30,
                            "summary_key": "effect.restore_hunger.need"
                        }
                    ],
                    "learning": {
                        "knowledge_effect_key": "restore_hunger",
                        "confidence_delta": 0.35,
                        "teachable": true
                    },
                    "agent": {
                        "usable_by_ai": true,
                        "risk_score": 1,
                        "time_cost": 1
                    }
                }
            ]
        })";

        auto result = JsonContentParser::parseEffects(effects_json);
        assert(result.is_ok());

        const auto& file_dto = result.value();
        assert(file_dto.effects.size() == 1);
        assert(file_dto.effects[0].effect_key == "restore_hunger");
        assert(file_dto.effects[0].operations.size() == 2);
        assert(file_dto.effects[0].learning.teachable == true);

        std::cout << "loader_test_4_effects_parse: passed" << std::endl;
    }

    // Test 5: Feedbacks JSON parsing
    {
        std::string feedbacks_json = R"({
            "feedbacks": [
                {
                    "feedback_key": "red_berry_eat",
                    "object_key": "red_berry",
                    "action_key": "eat",
                    "target_object_key": "",
                    "effect_key": "restore_hunger",
                    "priority": 100,
                    "conditions": ["condition:test:eq:object.quantity.red_berry.gte.1"],
                    "reply_key": "feedback.red_berry.eat.restore",
                    "utility_delta": 0.6,
                    "risk_delta": 0,
                    "causal_tags": ["food", "hunger"],
                    "knowledge": {
                        "subject_object_key": "red_berry",
                        "action_key": "eat",
                        "effect_key": "restore_hunger"
                    }
                }
            ]
        })";

        auto result = JsonContentParser::parseFeedbacks(feedbacks_json);
        assert(result.is_ok());

        const auto& file_dto = result.value();
        assert(file_dto.feedbacks.size() == 1);
        assert(file_dto.feedbacks[0].object_key == "red_berry");
        assert(file_dto.feedbacks[0].knowledge.subject_object_key == "red_berry");

        std::cout << "loader_test_5_feedbacks_parse: passed" << std::endl;
    }

    // Test 6: Reactions JSON parsing
    {
        std::string reactions_json = R"({
            "reactions": [
                {
                    "reaction_key": "make_torch",
                    "inputs": [
                        {"object_key": "wood", "state": "processed", "min": 1},
                        {"object_key": "camp_fire", "quantity": 1}
                    ],
                    "action_key": "use",
                    "effect_key": "make_torch",
                    "outputs": [
                        {"object_key": "torch", "quantity_delta": 1}
                    ],
                    "consume": [
                        {"object_key": "wood", "state": "processed", "delta": -1}
                    ],
                    "summary_key": "reaction.make_torch.summary",
                    "knowledge_templates": ["knowledge.make_torch"]
                }
            ]
        })";

        auto result = JsonContentParser::parseReactions(reactions_json);
        assert(result.is_ok());

        const auto& file_dto = result.value();
        assert(file_dto.reactions.size() == 1);
        assert(file_dto.reactions[0].inputs.size() == 2);
        assert(file_dto.reactions[0].outputs[0].object_key == "torch");

        std::cout << "loader_test_6_reactions_parse: passed" << std::endl;
    }

    // Test 7: Invalid JSON returns error
    {
        std::string invalid_json = "{ this is not valid json }";
        auto result = JsonContentParser::parseManifest(invalid_json);
        assert(result.is_error());

        std::cout << "loader_test_7_invalid_json: passed" << std::endl;
    }

    // Test 8: Scenario parsing
    {
        std::string scenario_json = R"({
            "scenario_key": "first_night",
            "display_key": "scenario.first_night.name",
            "initial_objects": [
                {"object_key": "red_berry", "quantity": 3, "visible": true}
            ],
            "initial_agents": [
                {"agent_key": "companion", "visible": true, "trust": 0.8}
            ],
            "initial_threats": [
                {"threat_key": "beast_shadow", "level": 0}
            ],
            "default_player_knowledge": [],
            "default_agent_knowledge": []
        })";

        auto result = JsonContentParser::parseScenario(scenario_json);
        assert(result.is_ok());

        const auto& scenario = result.value();
        assert(scenario.scenario_key == "first_night");
        assert(scenario.initial_objects.size() == 1);
        assert(scenario.initial_agents[0].agent_key == "companion");

        std::cout << "loader_test_8_scenario_parse: passed" << std::endl;
    }

    // Test 9: Threats JSON parsing
    {
        std::string threats_json = R"({
            "threats": [
                {
                    "threat_key": "beast_shadow",
                    "display_key": "threat.beast_shadow.name",
                    "agent_key": "beast_shadow",
                    "initial_level": 0,
                    "progression": {
                        "wait_delta": 25,
                        "phases": [
                            {"min": 25, "phase": "foreshadowing", "summary_key": "threat.beast.foreshadow"},
                            {"min": 50, "phase": "approaching", "summary_key": "threat.beast.approach"}
                        ]
                    },
                    "countermeasures": [
                        {"effect_key": "repel_beast", "level_delta": -100}
                    ],
                    "reentry": {
                        "enabled": true,
                        "waits": 3,
                        "level": 50,
                        "tag": "flanking_probe"
                    }
                }
            ]
        })";

        auto result = JsonContentParser::parseThreats(threats_json);
        assert(result.is_ok());

        const auto& file_dto = result.value();
        assert(file_dto.threats.size() == 1);
        assert(file_dto.threats[0].countermeasures[0].effect_key == "repel_beast");

        std::cout << "loader_test_9_threats_parse: passed" << std::endl;
    }

    // Test 10: Locale JSON parsing
    {
        std::string locale_json = R"({
            "object.red_berry.name": "红果",
            "object.red_berry.desc": "一种鲜红的果实。"
        })";

        auto result = JsonContentParser::parseLocale(locale_json);
        assert(result.is_ok());

        const auto& locale = result.value();
        assert(locale.at("object.red_berry.name") == "红果");

        std::cout << "loader_test_10_locale_parse: passed" << std::endl;
    }

    // Test 11: Agents JSON parsing
    {
        std::string agents_json = R"({
            "agents": [
                {
                    "agent_key": "beast_shadow",
                    "display_key": "agent.beast_shadow.name",
                    "scale": "small_agent",
                    "cognition_band": "instinctive",
                    "embodiment": "wildlife",
                    "default_needs": {
                        "fear": 0,
                        "hunger": 50
                    },
                    "instinct_effect_keys": ["fire_is_dangerous"],
                    "default_policy_key": "wildlife_cautious_probe",
                    "can_learn": true,
                    "can_teach": false
                }
            ]
        })";

        auto result = JsonContentParser::parseAgents(agents_json);
        assert(result.is_ok());

        const auto& file_dto = result.value();
        assert(file_dto.agents.size() == 1);
        assert(file_dto.agents[0].can_learn == true);
        assert(file_dto.agents[0].can_teach == false);

        std::cout << "loader_test_11_agents_parse: passed" << std::endl;
    }

    // Test 12: Strict load mode refuses to build registry when validation has Error
    {
        const std::filesystem::path root = std::filesystem::temp_directory_path() / "pathfinder_content_loader_strict_test";
        const std::filesystem::path core = root / "core";
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(core / "objects");
        std::filesystem::create_directories(core / "feedbacks");

        std::ofstream(core / "manifest.json") << R"({
            "package_key": "core",
            "display_name": "test",
            "content_version": "0.1.0",
            "schema_version": "content_schema_1",
            "locale_default": "zh_cn",
            "files": [
                {"path": "objects/basic.json", "category": "objects", "required": true},
                {"path": "feedbacks/basic.json", "category": "feedbacks", "required": true}
            ]
        })";
        std::ofstream(core / "objects" / "basic.json") << R"({
            "objects": [
                {"object_key": "red_berry", "display_key": "object.red_berry.name", "description_key": "object.red_berry.desc", "kind": "food"}
            ]
        })";
        std::ofstream(core / "feedbacks" / "basic.json") << R"({
            "feedbacks": [
                {"feedback_key": "bad_feedback", "object_key": "missing_object", "action_key": "eat", "effect_key": "missing_effect"}
            ]
        })";

        ContentLoadOptions options;
        options.root_path = root.string();
        options.enabled_package_keys = {"core"};
        options.load_mode = ContentLoadMode::StrictContentRequired;

        JsonContentLoader loader;
        auto result = loader.load(options);
        assert(result.is_ok());
        assert(result.value().validation_report.hasErrors());
        assert(!result.value().registry);

        std::filesystem::remove_all(root);
        std::cout << "loader_test_12_strict_errors_skip_registry: passed" << std::endl;
    }

    std::cout << "content_loader_tests: all passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: loader, registry, validation, runtime_adapter" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "loader") {
        run_content_loader_tests();
        return 0;
    } else if (test_name == "registry") {
        run_content_registry_tests();
        return 0;
    } else if (test_name == "validation") {
        run_content_validation_tests();
        return 0;
    } else if (test_name == "runtime_adapter") {
        run_content_runtime_adapter_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
