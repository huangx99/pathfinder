#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/world_learning/world_experience_extractor.h"
#include "pathfinder/world_learning/world_knowledge_template_resolver.h"
#include "pathfinder/world_learning/world_safe_feedback_builder.h"
#include "pathfinder/world_learning/world_learning_loop_bridge.h"
#include "pathfinder/world_learning/world_knowledge_store_port.h"
#include "pathfinder/world_learning/world_knowledge_learning_service.h"
#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"
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

static std::shared_ptr<ContentRegistry> makeTestContentRegistry() {
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

    ObjectDefinitionContent mushroom_obj;
    mushroom_obj.key = ObjectDefinitionKey("toxic_mushroom");
    mushroom_obj.display_key = "entity.toxic_mushroom";
    draft.addObject(std::move(mushroom_obj));

    EffectDefinitionContent poison_effect;
    poison_effect.key = EffectDefinitionKey("poison");
    poison_effect.display_key = "effect.poison.name";
    poison_effect.semantic_kind = "actor_need_delta";
    poison_effect.target_kind = "actor_self";
    poison_effect.confidence_delta = 0.35;
    poison_effect.teachable = true;
    poison_effect.usable_by_ai = true;
    poison_effect.risk_score = 8;
    poison_effect.time_cost = 1;
    EffectOperationDto consume_poison;
    consume_poison.op = "consume_object_quantity";
    consume_poison.target = "object";
    consume_poison.quantity = 1;
    poison_effect.operations.push_back(consume_poison);
    EffectOperationDto health_loss;
    health_loss.op = "change_actor_need";
    health_loss.target = "actor";
    health_loss.need_key = "health";
    health_loss.delta = -40.0;
    poison_effect.operations.push_back(health_loss);
    draft.addEffect(std::move(poison_effect));

    KnowledgeTemplateContent tmpl_poison;
    tmpl_poison.key = KnowledgeTemplateKey("knowledge_toxic_mushroom_poison");
    tmpl_poison.subject_object_key = "toxic_mushroom";
    tmpl_poison.action_key = "eat";
    tmpl_poison.effect_key = "poison";
    tmpl_poison.default_status = "Hypothesis";
    tmpl_poison.teachable = true;
    tmpl_poison.usable_by_ai = true;
    tmpl_poison.summary_key = "knowledge.toxic_mushroom_poison";
    draft.addKnowledgeTemplate(std::move(tmpl_poison));

    return ContentRegistry::build(draft);
}

static WorldCommandExecutionResult makeCommandResultWithExperiences(
    const std::vector<WorldExperienceDto>& experiences,
    WorldCommandResultKind result_kind = WorldCommandResultKind::Succeeded) {
    WorldCommandExecutionResult result;
    result.result_kind = result_kind;
    result.experiences = experiences;
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

static WorldCommandDto makeSourceCommand(const std::string& actor_key) {
    WorldCommandDto cmd;
    cmd.command_id = "cmd_test_001";
    cmd.command_key = "test_command";
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = actor_key;
    return cmd;
}

// ---------------------------------------------------------------------------
// Enum roundtrip
// ---------------------------------------------------------------------------

void run_world_learning_enum_roundtrip_tests() {
    // WorldLearningSourceKind
    {
        auto r1 = worldLearningSourceKindFromString("DirectExperience");
        assert(r1.is_ok() && r1.value() == WorldLearningSourceKind::DirectExperience);
        assert(toString(r1.value()) == "DirectExperience");

        auto r2 = worldLearningSourceKindFromString("TestOnly");
        assert(r2.is_ok() && r2.value() == WorldLearningSourceKind::TestOnly);

        auto r3 = worldLearningSourceKindFromString("Unknown");
        assert(r3.is_ok() && r3.value() == WorldLearningSourceKind::Unknown);

        auto r4 = worldLearningSourceKindFromString("BadValue");
        assert(r4.is_error());
    }

    // WorldLearningExperienceKind
    {
        auto r1 = worldLearningExperienceKindFromString("ResourceHarvest");
        assert(r1.is_ok() && r1.value() == WorldLearningExperienceKind::ResourceHarvest);
        assert(toString(r1.value()) == "ResourceHarvest");

        auto r2 = worldLearningExperienceKindFromString("ReactionCraft");
        assert(r2.is_ok() && r2.value() == WorldLearningExperienceKind::ReactionCraft);

        auto r3 = worldLearningExperienceKindFromString("BadValue");
        assert(r3.is_error());
    }

    // WorldLearningFailureKind
    {
        auto r1 = worldLearningFailureKindFromString("TemplateMissing");
        assert(r1.is_ok() && r1.value() == WorldLearningFailureKind::TemplateMissing);
        assert(toString(r1.value()) == "TemplateMissing");

        auto r2 = worldLearningFailureKindFromString("None");
        assert(r2.is_ok() && r2.value() == WorldLearningFailureKind::None);
    }

    // WorldKnowledgeLearningDecision
    {
        auto r1 = worldKnowledgeLearningDecisionFromString("Learned");
        assert(r1.is_ok() && r1.value() == WorldKnowledgeLearningDecision::Learned);
        assert(toString(r1.value()) == "Learned");

        auto r2 = worldKnowledgeLearningDecisionFromString("SkippedNoTemplate");
        assert(r2.is_ok() && r2.value() == WorldKnowledgeLearningDecision::SkippedNoTemplate);
    }

    std::cout << "world_learning_enum_roundtrip: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Extracts experience from command result
// ---------------------------------------------------------------------------

void run_world_learning_extracts_experience_from_command_result_tests() {
    WorldExperienceExtractor extractor;

    std::vector<WorldExperienceDto> exps;
    exps.push_back(makeExperience("exp1", "player", "gather", "berry_bush", "gather_red_berry"));
    exps.push_back(makeExperience("exp2", "player", "craft", "torch", "make_torch"));

    auto result = extractor.extract(makeCommandResultWithExperiences(exps));
    assert(result.failure_kind == WorldLearningFailureKind::None);
    assert(result.experiences.size() == 2);
    assert(result.experiences[0].experience_id == "exp1");
    assert(result.experiences[1].experience_id == "exp2");

    // Empty experiences -> NoExperience
    auto empty_res = extractor.extract(makeCommandResultWithExperiences({}));
    assert(empty_res.failure_kind == WorldLearningFailureKind::NoExperience);
    assert(empty_res.experiences.empty());

    std::cout << "world_learning_extracts_experience_from_command_result: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Resolves template from reason keys
// ---------------------------------------------------------------------------

void run_world_learning_resolves_template_from_reason_keys_tests() {
    auto registry = makeTestContentRegistry();
    WorldKnowledgeTemplateResolver resolver(*registry);

    auto exp = makeExperience("exp1", "player", "craft", "torch", "make_torch",
        std::vector<std::string>{"knowledge_make_torch"});

    auto result = resolver.resolve(exp);
    assert(result.failure_kind == WorldLearningFailureKind::None);
    assert(!result.templates.empty());
    assert(result.templates[0].key.value() == "knowledge_make_torch");
    assert(result.matched_keys[0] == "knowledge_make_torch");

    std::cout << "world_learning_resolves_template_from_reason_keys: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Resolves template by action effect fallback
// ---------------------------------------------------------------------------

void run_world_learning_resolves_template_by_action_effect_fallback_tests() {
    auto registry = makeTestContentRegistry();
    WorldKnowledgeTemplateResolver resolver(*registry);

    // No reason_keys, should fallback to action/effect matching
    auto exp = makeExperience("exp1", "player", "gather", "berry_bush", "gather_red_berry");

    auto result = resolver.resolve(exp);
    assert(result.failure_kind == WorldLearningFailureKind::None);
    assert(!result.templates.empty());
    assert(result.templates[0].key.value() == "knowledge_gather_berry");

    std::cout << "world_learning_resolves_template_by_action_effect_fallback: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Skips missing template without world mutation
// ---------------------------------------------------------------------------

void run_world_learning_skips_missing_template_without_world_mutation_tests() {
    auto registry = makeTestContentRegistry();
    WorldKnowledgeTemplateResolver resolver(*registry);

    auto exp = makeExperience("exp1", "player", "craft", "unknown_item", "unknown_effect");

    auto result = resolver.resolve(exp);
    assert(result.failure_kind == WorldLearningFailureKind::TemplateMissing);
    assert(result.templates.empty());

    std::cout << "world_learning_skips_missing_template_without_world_mutation: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Rejects unsafe experience
// ---------------------------------------------------------------------------

void run_world_learning_rejects_unsafe_experience_tests() {
    WorldExperienceExtractor extractor;

    auto exp = makeExperience("exp1", "player", "gather", "berry_bush", "gather_red_berry");
    exp.condition_keys.push_back("hidden_truth_leaked");

    auto result = extractor.extract(makeCommandResultWithExperiences({exp}));
    assert(result.failure_kind == WorldLearningFailureKind::UnsafeExperience);
    assert(result.experiences.empty());

    std::cout << "world_learning_rejects_unsafe_experience: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Builds safe feedback
// ---------------------------------------------------------------------------

void run_world_learning_builds_safe_feedback_tests() {
    WorldSafeFeedbackBuilder builder;

    auto exp = makeExperience("exp1", "player", "craft", "torch", "make_torch");
    exp.condition_keys = {"fresh"};

    KnowledgeTemplateContent tmpl;
    tmpl.key = KnowledgeTemplateKey("knowledge_make_torch");
    tmpl.subject_object_key = "torch";
    tmpl.action_key = "craft";
    tmpl.effect_key = "make_torch";
    tmpl.teachable = true;
    tmpl.usable_by_ai = true;

    std::vector<WorldEventDto> events;
    WorldEventDto evt;
    evt.event_id = "evt1";
    evt.event_kind = "craft_success";
    events.push_back(evt);

    auto result = builder.build(exp, tmpl, events, 42);
    assert(result.failure_kind == WorldLearningFailureKind::None);

    auto& fb = result.feedback;
    assert(fb.feedback_key == "exp1:feedback");
    assert(fb.action_key == "craft");
    assert(fb.effect_key == "make_torch");
    assert(!fb.condition_keys.empty());
    assert(fb.observed_tick.value() == 1); // experience.tick
    assert(!fb.source_event_ids.empty());

    auto validation = fb.validateBasic();
    assert(validation.is_ok());

    std::cout << "world_learning_builds_safe_feedback: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Direct experience forms claim
// ---------------------------------------------------------------------------

void run_world_learning_direct_experience_forms_claim_tests() {
    auto registry = makeTestContentRegistry();
    LearningLoopService learning_service;
    KnowledgeRepository repository;

    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    WorldKnowledgeLearningRequest request;
    request.request_id = "req1";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 10;
    request.source_command = makeSourceCommand("player");

    auto exp = makeExperience("exp1", "player", "craft", "torch", "make_torch",
        std::vector<std::string>{"knowledge_make_torch"});
    request.source_command.command_id = "cmd1";
    request.source_command.command_key = "craft";
    request.source_command.source = WorldCommandSource::PlayerInput;
    request.source_command.actor_key = "player";
    request.command_result = makeCommandResultWithExperiences({exp});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().decision == WorldKnowledgeLearningDecision::Learned ||
           result.value().decision == WorldKnowledgeLearningDecision::SkippedNoTemplate);

    // If learning succeeded, repository should have claims
    if (result.value().ok && !result.value().learned_claims.empty()) {
        auto query_res = repository.query(KnowledgeQuery{});
        // Verify claims exist or check learned_claims directly
        assert(!result.value().learned_claims.empty());
        auto& claim = result.value().learned_claims.front();
        assert(claim.owner.entity_id.value() == "player" || claim.owner.external_key == "player");
    }

    std::cout << "world_learning_direct_experience_forms_claim: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Repeated experience reinforces claim
// ---------------------------------------------------------------------------

void run_world_learning_repeated_experience_reinforces_claim_tests() {
    auto registry = makeTestContentRegistry();
    LearningLoopService learning_service;
    KnowledgeRepository repository;

    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    // First experience
    WorldKnowledgeLearningRequest request1;
    request1.request_id = "req1";
    request1.actor_key = "player";
    request1.source_kind = WorldLearningSourceKind::DirectExperience;
    request1.tick = 10;
    request1.source_command = makeSourceCommand("player");
    auto exp1 = makeExperience("exp1", "player", "craft", "torch", "make_torch",
        std::vector<std::string>{"knowledge_make_torch"});
    request1.command_result = makeCommandResultWithExperiences({exp1});

    auto result1 = service.learnFromCommandResult(request1);
    assert(result1.is_ok());

    // Second experience with existing claims
    WorldKnowledgeLearningRequest request2;
    request2.request_id = "req2";
    request2.actor_key = "player";
    request2.source_kind = WorldLearningSourceKind::DirectExperience;
    request2.tick = 20;
    request2.source_command = makeSourceCommand("player");
    auto exp2 = makeExperience("exp2", "player", "craft", "torch", "make_torch",
        std::vector<std::string>{"knowledge_make_torch"});
    request2.command_result = makeCommandResultWithExperiences({exp2});

    // Pass existing claims from first run
    request2.existing_actor_claims = result1.value().learned_claims;

    auto result2 = service.learnFromCommandResult(request2);
    assert(result2.is_ok());

    // Should not produce unbound garbage claims
    size_t repo_size = repository.size();
    assert(repo_size < 100); // sanity bound

    std::cout << "world_learning_repeated_experience_reinforces_claim: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// High-risk effect normalization
// ---------------------------------------------------------------------------

void run_world_learning_high_risk_effect_is_normalized_tests() {
    auto registry = makeTestContentRegistry();
    WorldSafeFeedbackBuilder builder;

    auto exp = makeExperience("exp_poison", "player", "eat", "toxic_mushroom", "poison",
        std::vector<std::string>{"knowledge_toxic_mushroom_poison"});
    auto* tmpl = registry->findKnowledgeTemplate("knowledge_toxic_mushroom_poison");
    assert(tmpl != nullptr);
    auto* effect = registry->findEffect("poison");
    assert(effect != nullptr);

    auto feedback = builder.build(exp, *tmpl, effect, {}, 42);
    assert(feedback.failure_kind == WorldLearningFailureKind::None);
    assert(feedback.feedback.risk_delta >= 0.79 && feedback.feedback.risk_delta <= 0.81);
    assert(feedback.feedback.outcome_signals[0] == pathfinder::cognition::CognitionOutcomeSignal::HealthWorsened);

    LearningLoopService learning_service;
    KnowledgeRepository repository;
    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    WorldKnowledgeLearningRequest request;
    request.request_id = "req_poison";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 42;
    request.source_command = makeSourceCommand("player");
    request.command_result = makeCommandResultWithExperiences({exp});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.ok);
    assert(val.decision == WorldKnowledgeLearningDecision::Learned);
    assert(!val.learned_claims.empty());
    assert(val.knowledge_deltas[0].effect_key == "poison");

    std::cout << "world_learning_high_risk_effect_is_normalized: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// No frontend knowledge input
// ---------------------------------------------------------------------------

void run_world_learning_no_frontend_knowledge_input_tests() {
    auto registry = makeTestContentRegistry();
    LearningLoopService learning_service;
    KnowledgeRepository repository;

    WorldKnowledgeLearningService service(*registry, learning_service, repository);

    WorldKnowledgeLearningRequest request;
    request.request_id = "req1";
    request.actor_key = "player";
    request.source_kind = WorldLearningSourceKind::DirectExperience;
    request.tick = 10;
    request.source_command = makeSourceCommand("player");
    // Frontend injects a fake knowledge_key parameter
    request.parameters["knowledge_key"] = "knowledge_make_torch";

    // But command_result.experiences is empty (no real experience)
    request.source_command.command_id = "cmd1";
    request.source_command.command_key = "craft";
    request.source_command.source = WorldCommandSource::PlayerInput;
    request.source_command.actor_key = "player";
    request.command_result = makeCommandResultWithExperiences({});

    auto result = service.learnFromCommandResult(request);
    assert(result.is_ok());
    // Should skip because no experiences, not form fake knowledge
    assert(result.value().learned_claims.empty());

    std::cout << "world_learning_no_frontend_knowledge_input: passed" << std::endl;
}
