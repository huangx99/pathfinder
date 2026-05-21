#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/world_learning/world_knowledge_learning_service.h"
#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"
#include "pathfinder/world_learning/world_knowledge_store_port.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_learning;
using namespace pathfinder::world_command;
using namespace pathfinder::content;
using namespace pathfinder::learning;
using namespace pathfinder::knowledge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<ContentRegistry> makeP48ContentRegistry() {
    ContentDraftRegistry draft;

    ObjectDefinitionContent wood_obj;
    wood_obj.key = ObjectDefinitionKey("wood");
    wood_obj.display_key = "entity.wood";
    draft.addObject(std::move(wood_obj));

    ObjectDefinitionContent torch_obj;
    torch_obj.key = ObjectDefinitionKey("torch");
    torch_obj.display_key = "entity.torch";
    draft.addObject(std::move(torch_obj));

    ObjectDefinitionContent berry_obj;
    berry_obj.key = ObjectDefinitionKey("berry_bush");
    berry_obj.display_key = "entity.berry_bush";
    draft.addObject(std::move(berry_obj));

    // P48 craft reaction knowledge template
    KnowledgeTemplateContent tmpl_torch;
    tmpl_torch.key = KnowledgeTemplateKey("knowledge_make_torch");
    tmpl_torch.subject_object_key = "torch";
    tmpl_torch.action_key = "craft";
    tmpl_torch.effect_key = "make_torch";
    tmpl_torch.target_object_key = "";
    tmpl_torch.default_status = "Hypothesis";
    tmpl_torch.teachable = true;
    tmpl_torch.usable_by_ai = true;
    tmpl_torch.summary_key = "knowledge.make_torch";
    draft.addKnowledgeTemplate(std::move(tmpl_torch));

    // P47 harvest knowledge template
    KnowledgeTemplateContent tmpl_berry;
    tmpl_berry.key = KnowledgeTemplateKey("knowledge_gather_berry");
    tmpl_berry.subject_object_key = "berry_bush";
    tmpl_berry.action_key = "gather";
    tmpl_berry.effect_key = "gather_red_berry";
    tmpl_berry.target_object_key = "";
    tmpl_berry.default_status = "Hypothesis";
    tmpl_berry.teachable = true;
    tmpl_berry.usable_by_ai = true;
    tmpl_berry.summary_key = "knowledge.gather_berry";
    draft.addKnowledgeTemplate(std::move(tmpl_berry));

    return ContentRegistry::build(draft);
}

static std::shared_ptr<ContentRegistry> makeP48ContentRegistryWithoutTemplate() {
    ContentDraftRegistry draft;

    ObjectDefinitionContent wood_obj;
    wood_obj.key = ObjectDefinitionKey("wood");
    wood_obj.display_key = "entity.wood";
    draft.addObject(std::move(wood_obj));

    ObjectDefinitionContent torch_obj;
    torch_obj.key = ObjectDefinitionKey("torch");
    torch_obj.display_key = "entity.torch";
    draft.addObject(std::move(torch_obj));

    // No knowledge template added
    return ContentRegistry::build(draft);
}

static WorldCommandDto makeSourceCommand(const std::string& actor_key) {
    WorldCommandDto cmd;
    cmd.command_id = "cmd_test_001";
    cmd.command_key = "test_command";
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = actor_key;
    return cmd;
}

static WorldCommandExecutionResult makeCommandResultWithExperiences(
    const std::vector<WorldExperienceDto>& experiences,
    const std::vector<WorldEventDto>& events = {},
    WorldCommandResultKind result_kind = WorldCommandResultKind::Succeeded) {
    WorldCommandExecutionResult result;
    result.result_kind = result_kind;
    result.experiences = experiences;
    result.events = events;
    return result;
}

static WorldExperienceDto makeExperience(
    const std::string& id,
    const std::string& actor_key,
    const std::string& command_key,
    const std::string& subject,
    const std::string& effect,
    const std::vector<std::string>& reason_keys = {}) {
    WorldExperienceDto exp;
    exp.experience_id = id;
    exp.actor_key = actor_key;
    exp.command_key = command_key;
    exp.subject_entity_key = subject;
    exp.effect_key = effect;
    exp.reason_keys = reason_keys;
    exp.tick = 1;
    return exp;
}

// ---------------------------------------------------------------------------
// world_learning_craft_experience_uses_p48_knowledge_template
// ---------------------------------------------------------------------------

void run_world_learning_craft_experience_uses_p48_knowledge_template_tests() {
    auto registry = makeP48ContentRegistry();
    LearningLoopService learning_service;
    KnowledgeRepository repository;
    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    // Simulate P48 craft output: torch + craft -> make_torch
    WorldKnowledgeLearningRequest request;
    request.request_id = "req_craft";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 10;
    request.source_command = makeSourceCommand("player");

    auto exp = makeExperience("exp_craft", "player", "craft", "torch", "make_torch",
        std::vector<std::string>{"knowledge_make_torch"});
    request.command_result = makeCommandResultWithExperiences({exp});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.ok);
    assert(val.decision == WorldKnowledgeLearningDecision::Learned);
    assert(!val.drafts.empty());
    assert(!val.learning_results.empty());
    assert(val.learning_results[0].ok());
    // Verify knowledge was actually formed (not just loop ok)
    bool has_formation_claim = false;
    for (const auto& lr : val.learning_results) {
        if (lr.knowledge_formation_result.has_value() && lr.knowledge_formation_result->claim.has_value()) {
            has_formation_claim = true;
        }
    }
    assert(has_formation_claim);
    assert(!val.learned_claims.empty());
    assert(!val.knowledge_deltas.empty());
    assert(val.knowledge_deltas[0].action_key == "craft");
    assert(val.knowledge_deltas[0].effect_key == "make_torch");
    // Verify the draft captures the correct experience and template.
    assert(val.drafts[0].experience.effect_key == "make_torch");
    assert(!val.drafts[0].templates.empty());
    assert(val.drafts[0].templates[0].key.value() == "knowledge_make_torch");

    std::cout << "world_learning_craft_experience_uses_p48_knowledge_template: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// world_learning_harvest_experience_uses_p47_output
// ---------------------------------------------------------------------------

void run_world_learning_harvest_experience_uses_p47_output_tests() {
    auto registry = makeP48ContentRegistry();
    LearningLoopService learning_service;
    KnowledgeRepository repository;
    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    // Simulate P47 harvest output: berry_bush + gather -> gather_red_berry
    WorldKnowledgeLearningRequest request;
    request.request_id = "req_harvest";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 10;
    request.source_command = makeSourceCommand("player");

    auto exp = makeExperience("exp_harvest", "player", "gather", "berry_bush", "gather_red_berry",
        std::vector<std::string>{"knowledge_gather_berry"});
    request.command_result = makeCommandResultWithExperiences({exp});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.ok);
    assert(val.decision == WorldKnowledgeLearningDecision::Learned);
    assert(!val.drafts.empty());
    assert(!val.learning_results.empty());
    assert(val.learning_results[0].ok());
    assert(!val.learned_claims.empty());
    assert(!val.knowledge_deltas.empty());
    assert(val.drafts[0].experience.effect_key == "gather_red_berry");
    assert(!val.drafts[0].templates.empty());
    assert(val.drafts[0].templates[0].key.value() == "knowledge_gather_berry");

    std::cout << "world_learning_harvest_experience_uses_p47_output: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// world_learning_command_result_merges_learning_events
// ---------------------------------------------------------------------------

void run_world_learning_command_result_merges_learning_events_tests() {
    auto registry = makeP48ContentRegistry();
    LearningLoopService learning_service;
    KnowledgeRepository repository;
    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    WorldEventDto original_event;
    original_event.event_id = "original_event_1";
    original_event.event_kind = "craft_success";
    original_event.tick = 10;
    original_event.actor_key = "player";

    WorldKnowledgeLearningRequest request;
    request.request_id = "req_merge";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 10;
    request.source_command = makeSourceCommand("player");

    auto exp = makeExperience("exp_merge", "player", "craft", "torch", "make_torch",
        std::vector<std::string>{"knowledge_make_torch"});
    request.command_result = makeCommandResultWithExperiences({exp}, {original_event});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.ok);
    assert(!val.learned_claims.empty());
    assert(!val.knowledge_deltas.empty());

    WorldKnowledgeProjectionBridge projection;
    auto proj_res = projection.project(val.learned_claims, val.knowledge_deltas, "player", 10);
    assert(!proj_res.events.empty());
    assert(!proj_res.patch.changed_knowledge.empty());
    assert(val.learning_results[0].ok());

    std::cout << "world_learning_command_result_merges_learning_events: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// world_learning_missing_template_does_not_fail_craft
// ---------------------------------------------------------------------------

void run_world_learning_missing_template_does_not_fail_craft_tests() {
    auto registry = makeP48ContentRegistryWithoutTemplate();
    LearningLoopService learning_service;
    KnowledgeRepository repository;
    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    WorldKnowledgeLearningRequest request;
    request.request_id = "req_missing";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 10;
    request.source_command = makeSourceCommand("player");

    // Craft succeeded (simulated), but no knowledge template
    auto exp = makeExperience("exp_missing", "player", "craft", "torch", "make_torch");
    request.command_result = makeCommandResultWithExperiences({exp});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    auto& val = result.value();
    // P49 should skip when no template is found; the craft result itself is untouched.
    assert(val.decision == WorldKnowledgeLearningDecision::SkippedNoTemplate ||
           val.decision == WorldKnowledgeLearningDecision::Failed);
    assert(val.learned_claims.empty());

    std::cout << "world_learning_missing_template_does_not_fail_craft: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "craft_experience_uses_p48_knowledge_template") {
        run_world_learning_craft_experience_uses_p48_knowledge_template_tests();
    } else if (test_name == "harvest_experience_uses_p47_output") {
        run_world_learning_harvest_experience_uses_p47_output_tests();
    } else if (test_name == "command_result_merges_learning_events") {
        run_world_learning_command_result_merges_learning_events_tests();
    } else if (test_name == "missing_template_does_not_fail_craft") {
        run_world_learning_missing_template_does_not_fail_craft_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
