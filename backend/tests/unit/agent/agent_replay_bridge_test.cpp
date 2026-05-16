#include "pathfinder/agent/replay_bridge.h"
#include "pathfinder/agent/record_builder.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/agent/types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/pipeline/result.h"
#include "pathfinder/state/state_change.h"
#include "pathfinder/event/event_stream.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;
using namespace pathfinder::replay;
using namespace pathfinder::state;
using namespace pathfinder::event;

// Helper: build a PipelineSucceeded AgentTickRecord
AgentTickRecord makePipelineSucceededTickRecord() {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    rec.agent_id = AgentId("agent_001");
    rec.actor_id = EntityId("actor_001");
    rec.tick = Tick(0);
    rec.state_version_before = StateVersion(1);
    rec.state_version_after = StateVersion(2);
    rec.random_seed = RandomSeed(42);
    rec.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(111);
    rec.output_hash = HashValue(222);
    rec.pipeline_status = PipelineStatus::Succeeded;

    CommandEnvelope env;
    env.command_id = CommandId("cmd_001");
    env.payload.action_id = ActionId("eat");
    env.payload.actor_id = EntityId("actor_001");
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId("obj_001");
    env.payload.targets.push_back(target);
    env.source = CommandSource::Ai;
    rec.command = env;

    rec.decision_record.record_id = AgentDecisionRecordId("agent_decision_record_dec_001");
    rec.decision_record.decision_id = DecisionId("dec_001");
    rec.decision_record.agent_id = AgentId("agent_001");
    rec.decision_record.actor_id = EntityId("actor_001");
    rec.decision_record.decision_tick = Tick(0);
    rec.decision_record.observation_state_version = StateVersion(1);
    rec.decision_record.status = AgentDecisionRecordStatus::PipelineSucceeded;
    rec.decision_record.command_id = CommandId("cmd_001");
    rec.decision_record.confidence = 0.9;

    StateChangeId sc_id("sc_001");
    rec.state_change_ids.push_back(sc_id);

    EventId ev_id("ev_001");
    rec.event_ids.push_back(ev_id);

    return rec;
}

void test_pipeline_succeeded_to_replay_record() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    auto result = bridge.toCommandReplayRecord(rec);
    if (result.is_error()) {
        std::cout << "  BRIDGE ERROR: " << toString(result.errors()[0]) << std::endl;
    }
    assert(result.is_ok());
    auto& rr = result.value();

    assert(rr.command_id == CommandId("cmd_001"));
    assert(rr.command.command_id == CommandId("cmd_001"));
    assert(rr.state_version_before == StateVersion(1));
    assert(rr.state_version_after == StateVersion(2));
    assert(rr.random_seed == RandomSeed(42));
    assert(rr.input_hash == HashValue(111));
    assert(rr.output_hash == HashValue(222));
    assert(rr.pipeline_status == PipelineStatus::Succeeded);
    assert(rr.state_change_ids.size() == 1);
    assert(rr.event_ids.size() == 1);
    assert(rr.status == ReplayRecordStatus::Recorded);
    assert(rr.record_id.value() == "agent_replay_cmd_001");

    std::cout << "  PASS: pipeline_succeeded_to_replay_record\n";
}

void test_command_id_matches() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    auto result = bridge.toCommandReplayRecord(rec);
    assert(result.is_ok());
    assert(result.value().command_id == rec.command.value().command_id);
    std::cout << "  PASS: command_id_matches\n";
}

void test_state_change_ids_preserved() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    auto result = bridge.toCommandReplayRecord(rec);
    assert(result.is_ok());
    assert(result.value().state_change_ids == rec.state_change_ids);
    assert(result.value().event_ids == rec.event_ids);
    std::cout << "  PASS: state_change_ids_preserved\n";
}

void test_submit_skipped_fails() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    rec.runtime_status = AgentRuntimeStatus::SubmitSkipped;
    rec.pipeline_status = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::SubmitSkipped;
    auto result = bridge.toCommandReplayRecord(rec);
    assert(result.is_error());
    std::cout << "  PASS: submit_skipped_fails\n";
}

void test_no_decision_fails() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    rec.runtime_status = AgentRuntimeStatus::NoDecision;
    rec.command = std::nullopt;
    rec.pipeline_status = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_record.command_id = std::nullopt;
    auto result = bridge.toCommandReplayRecord(rec);
    assert(result.is_error());
    std::cout << "  PASS: no_decision_fails\n";
}

void test_append_to_log() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    CommandReplayLog log;
    auto result = bridge.appendCommandReplayRecord(rec, log);
    assert(result.is_ok());
    assert(log.size() == 1);
    assert(log.findByCommandId(CommandId("cmd_001")).has_value());
    std::cout << "  PASS: append_to_log\n";
}

void test_append_duplicate_returns_error() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    CommandReplayLog log;
    assert(bridge.appendCommandReplayRecord(rec, log).is_ok());
    assert(bridge.appendCommandReplayRecord(rec, log).is_error());
    assert(log.size() == 1);
    std::cout << "  PASS: append_duplicate_returns_error\n";
}

// --- ReplayLockChecker tests ---

void test_lock_check_with_matching_command() {
    AgentReplayBridge bridge;
    auto rec = makePipelineSucceededTickRecord();
    CommandReplayLog log;
    bridge.appendCommandReplayRecord(rec, log);

    AgentReplayLockChecker checker;
    auto result = checker.check(rec, log);
    assert(result.has_agent_record);
    assert(result.has_command_record);
    assert(result.can_replay_without_policy);
    std::cout << "  PASS: lock_check_with_matching_command\n";
}

void test_lock_check_missing_command() {
    auto rec = makePipelineSucceededTickRecord();
    CommandReplayLog log;  // empty

    AgentReplayLockChecker checker;
    auto result = checker.check(rec, log);
    assert(result.has_agent_record);
    assert(!result.has_command_record);
    assert(!result.can_replay_without_policy);
    std::cout << "  PASS: lock_check_missing_command\n";
}

void test_lock_check_no_decision() {
    auto rec = makePipelineSucceededTickRecord();
    rec.runtime_status = AgentRuntimeStatus::NoDecision;
    rec.command = std::nullopt;
    rec.decision_record.command_id = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::NoDecision;
    CommandReplayLog log;

    AgentReplayLockChecker checker;
    auto result = checker.check(rec, log);
    assert(result.has_agent_record);
    assert(!result.has_command_record);
    assert(!result.can_replay_without_policy);
    // Check reason key
    bool found = false;
    for (const auto& key : result.reason_keys) {
        if (key == "no_decision_no_command") found = true;
    }
    assert(found);
    std::cout << "  PASS: lock_check_no_decision\n";
}

void test_lock_check_submit_skipped() {
    auto rec = makePipelineSucceededTickRecord();
    rec.runtime_status = AgentRuntimeStatus::SubmitSkipped;
    rec.pipeline_status = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::SubmitSkipped;
    rec.decision_record.command_id = std::nullopt;
    CommandReplayLog log;

    AgentReplayLockChecker checker;
    auto result = checker.check(rec, log);
    assert(result.has_agent_record);
    assert(!result.has_command_record);
    assert(!result.can_replay_without_policy);
    bool found = false;
    for (const auto& key : result.reason_keys) {
        if (key == "submit_skipped_no_command") found = true;
    }
    assert(found);
    std::cout << "  PASS: lock_check_submit_skipped\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "Agent replay bridge smoke test passed" << std::endl;
        return 0;
    }

    if (test_name == "pipeline_succeeded_to_replay_record") { test_pipeline_succeeded_to_replay_record(); return 0; }
    if (test_name == "command_id_matches") { test_command_id_matches(); return 0; }
    if (test_name == "state_change_ids_preserved") { test_state_change_ids_preserved(); return 0; }
    if (test_name == "submit_skipped_fails") { test_submit_skipped_fails(); return 0; }
    if (test_name == "no_decision_fails") { test_no_decision_fails(); return 0; }
    if (test_name == "append_to_log") { test_append_to_log(); return 0; }
    if (test_name == "append_duplicate_returns_error") { test_append_duplicate_returns_error(); return 0; }
    if (test_name == "lock_check_with_matching_command") { test_lock_check_with_matching_command(); return 0; }
    if (test_name == "lock_check_missing_command") { test_lock_check_missing_command(); return 0; }
    if (test_name == "lock_check_no_decision") { test_lock_check_no_decision(); return 0; }
    if (test_name == "lock_check_submit_skipped") { test_lock_check_submit_skipped(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
