#include <cassert>
#include <iostream>
#include "pathfinder/content/content_validation.h"
#include "pathfinder/content/content_types.h"

using namespace pathfinder::content;
using pathfinder::config::ConfigValidationSeverity;

void run_content_validation_tests() {
    // Test 1: Structural validation - empty draft passes
    {
        ContentDraftRegistry draft;
        ContentStructuralValidator validator;
        auto report = validator.validate(draft);
        assert(!report.hasErrors());
        assert(!report.hasFatal());

        std::cout << "validation_test_1_empty_draft: passed" << std::endl;
    }

    // Test 2: Structural validation - invalid object key
    {
        ContentDraftRegistry draft;
        ObjectDefinitionContent obj;
        obj.key = ObjectDefinitionKey("");  // empty key
        obj.display_key = "";
        obj.kind = "";
        draft.addObject(std::move(obj));

        ContentStructuralValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_2_invalid_object: passed" << std::endl;
    }

    // Test 3: Structural validation - invalid effect operation op
    {
        ContentDraftRegistry draft;
        EffectDefinitionContent eff;
        eff.key = EffectDefinitionKey("test_effect");
        eff.display_key = "test";
        EffectOperationDto op;
        op.op = "invalid_operation_name";
        eff.operations.push_back(op);
        draft.addEffect(std::move(eff));

        ContentStructuralValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_3_invalid_op: passed" << std::endl;
    }

    // Test 4: Structural validation - valid effect operations pass
    {
        ContentDraftRegistry draft;
        EffectDefinitionContent eff;
        eff.key = EffectDefinitionKey("test_effect");
        eff.display_key = "test";
        EffectOperationDto op;
        op.op = "consume_object_quantity";
        op.target = "$object";
        eff.operations.push_back(op);
        draft.addEffect(std::move(eff));

        ContentStructuralValidator validator;
        auto report = validator.validate(draft);
        assert(!report.hasErrors());

        std::cout << "validation_test_4_valid_op: passed" << std::endl;
    }

    // Test 4b: Structural validation rejects duplicate definition keys
    {
        ContentDraftRegistry draft;
        ObjectDefinitionContent obj1;
        obj1.key = ObjectDefinitionKey("red_berry");
        obj1.display_key = "object.red_berry.name";
        obj1.kind = "food";
        draft.addObject(std::move(obj1));

        ObjectDefinitionContent obj2;
        obj2.key = ObjectDefinitionKey("red_berry");
        obj2.display_key = "object.red_berry.name";
        obj2.kind = "food";
        draft.addObject(std::move(obj2));

        ContentStructuralValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_4b_duplicate_keys: passed" << std::endl;
    }

    // Test 5: Reference validation - feedback references unknown object
    {
        ContentDraftRegistry draft;

        // Only register red_berry object
        ObjectDefinitionContent obj;
        obj.key = ObjectDefinitionKey("red_berry");
        obj.kind = "food";
        obj.display_key = "test";
        draft.addObject(std::move(obj));

        // Feedback references non-existent object
        FeedbackDefinitionContent fb;
        fb.key = FeedbackDefinitionKey("test_fb");
        fb.object_key = "nonexistent_object";
        fb.action_key = "eat";
        fb.effect_key = "unknown_effect";
        draft.addFeedback(std::move(fb));

        ContentReferenceValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_5_ref_unknown_object: passed" << std::endl;
    }

    // Test 6: Reference validation - valid references pass
    {
        ContentDraftRegistry draft;

        ObjectDefinitionContent obj;
        obj.key = ObjectDefinitionKey("red_berry");
        obj.kind = "food";
        obj.display_key = "test";
        draft.addObject(std::move(obj));

        EffectDefinitionContent eff;
        eff.key = EffectDefinitionKey("restore_hunger");
        eff.display_key = "test";
        draft.addEffect(std::move(eff));

        FeedbackDefinitionContent fb;
        fb.key = FeedbackDefinitionKey("test_fb");
        fb.object_key = "red_berry";
        fb.action_key = "eat";
        fb.effect_key = "restore_hunger";
        draft.addFeedback(std::move(fb));

        ContentReferenceValidator validator;
        auto report = validator.validate(draft);
        assert(!report.hasErrors());

        std::cout << "validation_test_6_valid_references: passed" << std::endl;
    }

    // Test 7: Reference validation - reaction input/output references
    {
        ContentDraftRegistry draft;

        ObjectDefinitionContent wood;
        wood.key = ObjectDefinitionKey("wood");
        wood.kind = "material";
        wood.display_key = "test";
        draft.addObject(std::move(wood));

        EffectDefinitionContent make_torch;
        make_torch.key = EffectDefinitionKey("make_torch");
        make_torch.display_key = "test";
        draft.addEffect(std::move(make_torch));

        ReactionDefinitionContent reaction;
        reaction.key = ReactionDefinitionKey("test_reaction");
        reaction.effect_key = "make_torch";

        // Input references unknown object
        ReactionInputDto input;
        input.object_key = "nonexistent";
        reaction.inputs.push_back(input);

        // Output references unknown object
        ReactionOutputDto output;
        output.object_key = "nonexistent_output";
        reaction.outputs.push_back(output);

        draft.addReaction(std::move(reaction));

        ContentReferenceValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_7_ref_reaction: passed" << std::endl;
    }

    // Test 8: Reference validation - threat countermeasure references unknown effect
    {
        ContentDraftRegistry draft;

        AgentTemplateContent agent;
        agent.key = AgentTemplateKey("beast");
        agent.scale = "small_agent";
        draft.addAgent(std::move(agent));

        ThreatDefinitionContent threat;
        threat.key = ThreatDefinitionKey("beast_threat");
        threat.agent_key = "beast";
        ThreatCountermeasureDto cm;
        cm.effect_key = "nonexistent_effect";
        cm.level_delta = -100;
        threat.countermeasures.push_back(cm);
        draft.addThreat(std::move(threat));

        ContentReferenceValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_8_ref_threat: passed" << std::endl;
    }

    // Test 9: Reference validation - scenario initial object references
    {
        ContentDraftRegistry draft;

        ScenarioDefinitionContent scenario;
        scenario.key = ScenarioDefinitionKey("test_scenario");
        ScenarioInitialObjectDto init_obj;
        init_obj.object_key = "nonexistent_object";
        scenario.initial_objects.push_back(init_obj);
        draft.addScenario(std::move(scenario));

        ContentReferenceValidator validator;
        auto report = validator.validate(draft);
        assert(report.hasErrors());

        std::cout << "validation_test_9_ref_scenario: passed" << std::endl;
    }

    // Test 10: Reference validation - scenario default agent knowledge references
    {
        ContentDraftRegistry draft;

        ScenarioDefinitionContent scenario;
        scenario.key = ScenarioDefinitionKey("test_scenario");
        scenario.default_agent_knowledge = {"missing_knowledge"};
        draft.addScenario(std::move(scenario));

        ContentReferenceValidator validator;
        auto report = validator.validate(draft);
        bool found_agent_knowledge_issue = false;
        for (const auto& issue : report.issues()) {
            if (issue.json_path && *issue.json_path == "$.default_agent_knowledge[*]") {
                found_agent_knowledge_issue = true;
            }
        }
        assert(found_agent_knowledge_issue);

        std::cout << "validation_test_10_ref_agent_knowledge: passed" << std::endl;
    }

    std::cout << "content_validation_tests: all passed" << std::endl;
}
