#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/pipeline/context.h"
#include "pathfinder/pipeline/result.h"
#include "pathfinder/cognition/cognition_query.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::rules;
using namespace pathfinder::pipeline;
using namespace pathfinder::state;
using namespace pathfinder::object;
using namespace pathfinder::cognition;
using namespace pathfinder::foundation;
using namespace pathfinder::event;

void run_p15_cognition_eat_feedback_flow_tests() {
    // Create initial state
    auto state = UnknownFruitFixture::createInitialState();
    assert(state.validateBasic().is_ok());

    // Verify initial cognition is empty
    assert(state.cognition_state.claims().empty());
    assert(state.cognition_state_v2.claims().empty());

    // Create eat command
    auto cmd = UnknownFruitFixture::createEatCommand();

    // Create pipeline context
    PipelineContext context;
    context.command = cmd;
    context.state_metadata = state.metadata;
    context.game_state = &state;
    context.random_seed = RandomSeed(42);

    // Execute pipeline
    RulePipeline pipeline;
    auto result = pipeline.execute(context);
    assert(result.is_ok());

    auto& pipeline_result = result.value();
    assert(pipeline_result.status == PipelineStatus::Succeeded);

    // P3 legacy cognition still has claim
    assert(state.cognition_state.claims().size() == 1);

    // P15 V2 cognition has multiple claims
    assert(state.cognition_state_v2.claims().size() >= 3);

    // Query Edibility claim
    CognitionQueryService query_service;
    CognitionSubject subject{CognitionSubjectKind::Actor, EntityId("actor_001"), std::nullopt};
    CognitionTarget target{CognitionTargetKind::ObjectDefinition, "unknown_fruit", std::nullopt, ""}; // hidden_truth: safe enum

    auto edibility_result = query_service.findEdibility(state.cognition_state_v2, subject, target);
    assert(edibility_result.is_ok());
    assert(edibility_result.value().has_value());
    auto& edibility_claim = edibility_result.value().value();
    assert(edibility_claim.confidence > 0.0);
    assert(edibility_claim.confidence_band >= CognitionConfidenceBand::Hypothesis);
    assert(edibility_claim.key.aspect == CognitionAspect::Edibility);

    // Risk claim exists
    auto risk_result = query_service.findRisk(state.cognition_state_v2, subject, target);
    assert(risk_result.is_ok());
    assert(risk_result.value().has_value());

    // ConsumptionEffect or Utility claim exists
    {
        CognitionQuery q;
        q.subject = subject;
        q.target = target;
        q.aspect = CognitionAspect::ConsumptionEffect;
        q.mode = CognitionQueryMode::ExactTarget;
        auto ce_result = query_service.query(state.cognition_state_v2, q);
        assert(ce_result.is_ok());
        bool has_consumption_effect = !ce_result.value().claims.empty();

        q.aspect = CognitionAspect::Utility;
        auto u_result = query_service.query(state.cognition_state_v2, q);
        assert(u_result.is_ok());
        bool has_utility = !u_result.value().claims.empty();

        assert(has_consumption_effect || has_utility);
    }

    std::cout << "p15_cognition_eat_feedback_flow tests passed" << std::endl;
}
