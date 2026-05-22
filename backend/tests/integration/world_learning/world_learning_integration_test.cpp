#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/world_learning/world_knowledge_learning_service.h"
#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"
#include "pathfinder/world_learning/world_knowledge_store_port.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_reaction/craft_command_handler.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_learning;
using namespace pathfinder::world_command;
using namespace pathfinder::world_reaction;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
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
// ext-style new item reaction must go Command -> Experience -> Knowledge
// ---------------------------------------------------------------------------

static std::shared_ptr<ContentRegistry> makeExtStyleIncenseRegistry() {
    ContentDraftRegistry draft;

    ObjectDefinitionContent herb_obj;
    herb_obj.key = ObjectDefinitionKey("test_ext_herb");
    herb_obj.display_key = "entity.test_ext_herb";
    draft.addObject(std::move(herb_obj));

    ObjectDefinitionContent incense_obj;
    incense_obj.key = ObjectDefinitionKey("test_ext_incense");
    incense_obj.display_key = "entity.test_ext_incense";
    draft.addObject(std::move(incense_obj));

    ReactionDefinitionContent reaction;
    reaction.key = ReactionDefinitionKey("test_ext_herb_make_incense");
    reaction.inputs = {
        ReactionInputDto{"test_ext_herb", "fresh", 1, 0}
    };
    reaction.action_key = "craft";
    reaction.effect_key = "make_incense";
    reaction.outputs = {
        ReactionOutputDto{"test_ext_incense", 1}
    };
    reaction.consume = {
        ReactionConsumeDto{"test_ext_herb", "fresh", -1}
    };
    reaction.summary_key = "reaction.test_ext_make_incense.summary";
    reaction.knowledge_templates = {"knowledge_test_ext_make_incense"};
    draft.addReaction(std::move(reaction));

    KnowledgeTemplateContent tmpl;
    tmpl.key = KnowledgeTemplateKey("knowledge_test_ext_make_incense");
    tmpl.subject_object_key = "test_ext_incense";
    tmpl.action_key = "craft";
    tmpl.effect_key = "make_incense";
    tmpl.target_object_key = "";
    tmpl.default_status = "Hypothesis";
    tmpl.teachable = true;
    tmpl.usable_by_ai = true;
    tmpl.summary_key = "knowledge.test_ext_make_incense";
    draft.addKnowledgeTemplate(std::move(tmpl));

    return ContentRegistry::build(draft);
}

static void spawnExtHerbToInventory(WorldInventoryRuntime& inventory_runtime) {
    InventoryTransferRequest req;
    req.transfer_id = "setup_test_ext_herb";
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = "player";
    req.entity_key = "test_ext_herb";
    req.quantity = 1;
    req.parameters["stack_key"] = "test_ext_herb:fresh";
    req.parameters["state_keys"] = "fresh";
    auto res = inventory_runtime.transfer(req);
    assert(res.is_ok());
    assert(res.value().ok);
}

void run_world_learning_ext_item_reaction_command_to_knowledge_tests() {
    auto registry = makeExtStyleIncenseRegistry();

    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();
    spawnExtHerbToInventory(inventory_runtime);

    WorldReactionService reaction_service(*registry, world_runtime, inventory_runtime, world_runtime);
    auto craft_handler = createCraftCommandHandler(reaction_service, world_runtime, inventory_runtime);

    WorldCommandHandlerRegistry command_registry;
    auto reg_res = command_registry.registerHandler(craft_handler);
    assert(reg_res.is_ok());
    WorldCommandDispatcher dispatcher(command_registry);
    WorldCommandPipeline pipeline(dispatcher);

    WorldCommandDto craft_cmd;
    craft_cmd.command_id = "cmd_test_ext_make_incense";
    craft_cmd.command_key = "craft";
    craft_cmd.command_kind = WorldCommandKind::Craft;
    craft_cmd.source = WorldCommandSource::PlayerInput;
    craft_cmd.actor_key = "player";
    craft_cmd.target.recipe_key = "test_ext_herb_make_incense";
    craft_cmd.context.issued_tick = 21;

    auto command_res = pipeline.execute("session_ext_test", craft_cmd);
    assert(command_res.is_ok());
    const auto& command_response = command_res.value();
    assert(command_response.result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!command_response.experiences.empty());
    assert(command_response.experiences[0].command_key == "craft");
    assert(command_response.experiences[0].subject_entity_key == "test_ext_incense");
    assert(command_response.experiences[0].effect_key == "make_incense");
    assert(command_response.experiences[0].reason_keys.size() >= 2);

    bool has_template_reason = false;
    for (const auto& reason : command_response.experiences[0].reason_keys) {
        if (reason == "knowledge_test_ext_make_incense") {
            has_template_reason = true;
        }
    }
    assert(has_template_reason);

    auto inv_res = inventory_runtime.queryItems(InventoryOwnerRef{InventoryOwnerKind::Actor, "player"}, "test_ext_incense");
    assert(inv_res.is_ok());
    assert(!inv_res.value().empty());
    assert(inv_res.value()[0].quantity == 1);

    WorldCommandExecutionResult execution_result;
    execution_result.result_kind = command_response.result.result_kind;
    execution_result.events = command_response.event_feed;
    execution_result.experiences = command_response.experiences;
    execution_result.state_deltas = command_response.result.state_deltas;

    LearningLoopService learning_service;
    KnowledgeRepository repository;
    WorldKnowledgeLearningService learning(*registry, learning_service, repository);

    WorldKnowledgeLearningRequest learning_request;
    learning_request.request_id = "req_test_ext_make_incense";
    learning_request.actor_key = "player";
    learning_request.source_kind = WorldLearningSourceKind::DirectExperience;
    learning_request.tick = 21;
    learning_request.source_command = craft_cmd;
    learning_request.command_result = execution_result;

    auto learning_res = learning.learnFromCommandResult(learning_request);
    assert(learning_res.is_ok());
    const auto& learned = learning_res.value();
    assert(learned.ok);
    assert(learned.decision == WorldKnowledgeLearningDecision::Learned);
    assert(!learned.learned_claims.empty());
    assert(!learned.knowledge_deltas.empty());
    assert(learned.knowledge_deltas[0].action_key == "craft");
    assert(learned.knowledge_deltas[0].effect_key == "make_incense");
    assert(!learned.drafts.empty());
    assert(!learned.drafts[0].templates.empty());
    assert(learned.drafts[0].templates[0].key.value() == "knowledge_test_ext_make_incense");

    std::cout << "world_learning_ext_item_reaction_command_to_knowledge: passed" << std::endl;
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

    if (test_name == "ext_item_reaction_command_to_knowledge") {
        run_world_learning_ext_item_reaction_command_to_knowledge_tests();
    } else if (test_name == "craft_experience_uses_p48_knowledge_template") {
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
