#include "pathfinder/agent/record_builder.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/agent/types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/pipeline/result.h"
#include "pathfinder/state/state_change.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/event/event_stream.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;
using namespace pathfinder::state;
using namespace pathfinder::event;

// Global state for tests
static pathfinder::state::GameState g_test_state;

// Helper: build a PipelineSucceeded tick request/result pair
AgentRecordBuildRequest makePipelineSucceededRequest() {
    AgentRecordBuildRequest req;

    // Tick request
    req.tick_request.binding.agent_id = AgentId("agent_001");
    req.tick_request.binding.primary_actor_id = EntityId("actor_001");
    req.tick_request.binding.authority = AgentControlAuthority::Primary;
    req.tick_request.state = &g_test_state;
    req.tick_request.issued_tick = Tick(0);
    req.tick_request.random_seed = RandomSeed(42);
    req.tick_request.command_source = CommandSource::Ai;
    req.tick_request.options.submit_to_pipeline = false;

    // Tick result
    req.tick_result.status = AgentRuntimeStatus::PipelineSucceeded;

    // Decision
    req.tick_result.decision.decision_id = DecisionId("dec_001");
    req.tick_result.decision.agent_id = AgentId("agent_001");
    req.tick_result.decision.selected_intent.intent_type = AgentIntentType::Eat;
    req.tick_result.decision.selected_intent.action_id = ActionId("eat");
    req.tick_result.decision.selected_intent.confidence = 0.9;
    req.tick_result.decision.selected_intent.reason_key = "hungry";
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId("obj_001");
    req.tick_result.decision.selected_intent.targets.push_back(target);
    req.tick_result.decision.observation_state_version = StateVersion(1);
    req.tick_result.decision.action_id = ActionId("eat");

    // Trace
    req.tick_result.trace.selected_candidate_id = ActionId("eat_obj_001");
    req.tick_result.trace.selected_command_action_id = ActionId("eat");
    req.tick_result.trace.command_created = true;
    req.tick_result.trace.pipeline_submitted = true;
    req.tick_result.trace.phase_keys = {"observation_built", "action_space_built", "decision_made", "command_created", "pipeline_executed"};

    // Command
    CommandEnvelope env;
    env.command_id = CommandId("cmd_001");
    env.payload.action_id = ActionId("eat");
    env.source = CommandSource::Ai;
    req.tick_result.command = env;

    // Pipeline result
    PipelineResult pr;
    pr.status = PipelineStatus::Succeeded;
    StateChange change;
    change.change_id = StateChangeId("sc_001");
    change.status = StateChangeStatus::Applied;
    pr.state_changes.addChange(change);
    req.tick_result.pipeline_result = pr;

    // Hashes
    req.input_hash = HashValue(111);
    req.output_hash = HashValue(222);

    return req;
}

void test_pipeline_succeeded_build() {
    AgentRecordBuilder builder;
    auto req = makePipelineSucceededRequest();
    auto result = builder.build(req);
    if (result.is_error()) {
        std::cout << "  BUILD ERROR: " << toString(result.errors()[0]) << std::endl;
    }
    assert(result.is_ok());
    auto& rec = result.value();

    assert(rec.runtime_status == AgentRuntimeStatus::PipelineSucceeded);
    assert(rec.record_status == AgentTickRecordStatus::Recorded);
    assert(rec.agent_id == AgentId("agent_001"));
    assert(rec.actor_id == EntityId("actor_001"));
    assert(rec.tick == Tick(0));
    assert(rec.command.has_value());
    assert(rec.command.value().command_id == CommandId("cmd_001"));
    assert(rec.pipeline_status.has_value());
    assert(rec.pipeline_status.value() == PipelineStatus::Succeeded);
    assert(rec.decision_record.status == AgentDecisionRecordStatus::PipelineSucceeded);
    assert(rec.decision_record.command_id.has_value());
    assert(rec.input_hash == HashValue(111));
    assert(rec.output_hash == HashValue(222));
    assert(!rec.record_id.empty());

    std::cout << "  PASS: pipeline_succeeded_build\n";
}

void test_submit_skipped_build() {
    AgentRecordBuilder builder;
    AgentRecordBuildRequest req;

    req.tick_request.binding.agent_id = AgentId("agent_001");
    req.tick_request.binding.primary_actor_id = EntityId("actor_001");
    req.tick_request.binding.authority = AgentControlAuthority::Primary;
    req.tick_request.state = &g_test_state;
    req.tick_request.issued_tick = Tick(0);
    req.tick_request.random_seed = RandomSeed(42);
    req.tick_request.command_source = CommandSource::Ai;
    req.tick_request.options.submit_to_pipeline = false;

    req.tick_result.status = AgentRuntimeStatus::SubmitSkipped;
    req.tick_result.decision.decision_id = DecisionId("dec_001");
    req.tick_result.decision.agent_id = AgentId("agent_001");
    req.tick_result.decision.selected_intent.intent_type = AgentIntentType::Flee;
    req.tick_result.decision.selected_intent.confidence = 0.7;
    req.tick_result.decision.observation_state_version = StateVersion(1);
    req.tick_result.trace.selected_command_action_id = ActionId("flee");
    req.tick_result.trace.command_created = true;
    req.tick_result.trace.pipeline_submitted = false;
    req.tick_result.trace.phase_keys = {"observation_built", "action_space_built", "decision_made", "command_created", "submit_skipped"};

    // Command exists but not submitted
    CommandEnvelope env;
    env.command_id = CommandId("cmd_002");
    env.payload.action_id = ActionId("flee");
    env.source = CommandSource::Ai;
    req.tick_result.command = env;

    req.input_hash = HashValue(333);
    req.output_hash = HashValue(444);

    auto result = builder.build(req);
    assert(result.is_ok());
    auto& rec = result.value();

    assert(rec.runtime_status == AgentRuntimeStatus::SubmitSkipped);
    assert(rec.record_status == AgentTickRecordStatus::Recorded);
    assert(rec.decision_record.status == AgentDecisionRecordStatus::SubmitSkipped);
    assert(!rec.pipeline_status.has_value());
    assert(rec.command.has_value());

    std::cout << "  PASS: submit_skipped_build\n";
}

void test_no_decision_build() {
    AgentRecordBuilder builder;
    AgentRecordBuildRequest req;

    req.tick_request.binding.agent_id = AgentId("agent_001");
    req.tick_request.binding.primary_actor_id = EntityId("actor_001");
    req.tick_request.binding.authority = AgentControlAuthority::Primary;
    req.tick_request.state = &g_test_state;
    req.tick_request.issued_tick = Tick(0);
    req.tick_request.random_seed = RandomSeed(42);
    req.tick_request.command_source = CommandSource::Ai;
    req.tick_request.options.submit_to_pipeline = false;

    req.tick_result.status = AgentRuntimeStatus::NoDecision;
    // Empty decision_id - builder should generate one
    req.tick_result.decision.agent_id = AgentId("agent_001");
    req.tick_result.decision.observation_state_version = StateVersion(1);
    req.tick_result.trace.phase_keys = {"observation_built", "action_space_built", "no_decision"};

    req.input_hash = HashValue(555);
    req.output_hash = HashValue(666);

    auto result = builder.build(req);
    assert(result.is_ok());
    auto& rec = result.value();

    assert(rec.runtime_status == AgentRuntimeStatus::NoDecision);
    assert(rec.decision_record.status == AgentDecisionRecordStatus::NoDecision);
    assert(!rec.command.has_value());
    assert(!rec.pipeline_status.has_value());
    // decision_id should be generated
    assert(!rec.decision_record.decision_id.empty());

    std::cout << "  PASS: no_decision_build\n";
}

void test_missing_hash_returns_error() {
    AgentRecordBuilder builder;
    auto req = makePipelineSucceededRequest();
    req.input_hash = HashValue();
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: missing_hash_returns_error\n";
}

void test_record_id_deterministic() {
    AgentRecordBuilder builder;
    auto req1 = makePipelineSucceededRequest();
    auto req2 = makePipelineSucceededRequest();

    auto r1 = builder.build(req1);
    auto r2 = builder.build(req2);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().record_id == r2.value().record_id);
    std::cout << "  PASS: record_id_deterministic\n";
}

void test_selected_candidate_from_trace() {
    AgentRecordBuilder builder;
    auto req = makePipelineSucceededRequest();
    auto result = builder.build(req);
    assert(result.is_ok());
    assert(result.value().decision_record.selected_candidate_id == ActionId("eat_obj_001"));
    assert(result.value().decision_record.selected_command_action_id == ActionId("eat"));
    std::cout << "  PASS: selected_candidate_from_trace\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "Agent record builder smoke test passed" << std::endl;
        return 0;
    }

    if (test_name == "pipeline_succeeded_build") { test_pipeline_succeeded_build(); return 0; }
    if (test_name == "submit_skipped_build") { test_submit_skipped_build(); return 0; }
    if (test_name == "no_decision_build") { test_no_decision_build(); return 0; }
    if (test_name == "missing_hash_returns_error") { test_missing_hash_returns_error(); return 0; }
    if (test_name == "record_id_deterministic") { test_record_id_deterministic(); return 0; }
    if (test_name == "selected_candidate_from_trace") { test_selected_candidate_from_trace(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
