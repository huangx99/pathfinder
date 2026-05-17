#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/agent/types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/command/target.h"
#include "pathfinder/pipeline/types.h"
#include "pathfinder/replay/command_replay.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;
using namespace pathfinder::replay;

// --- Helper: make a valid PipelineSucceeded AgentTickRecord ---
AgentTickRecord makeValidTickRecord() {
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

    rec.decision_record.record_id = makeAgentDecisionRecordId(DecisionId("dec_001"));
    rec.decision_record.decision_id = DecisionId("dec_001");
    rec.decision_record.agent_id = AgentId("agent_001");
    rec.decision_record.actor_id = EntityId("actor_001");
    rec.decision_record.decision_tick = Tick(0);
    rec.decision_record.observation_state_version = StateVersion(1);
    rec.decision_record.status = AgentDecisionRecordStatus::PipelineSucceeded;
    rec.decision_record.command_id = CommandId("cmd_001");
    rec.decision_record.confidence = 0.9;

    return rec;
}

// --- Helper: make a valid CommandReplayRecord ---
CommandReplayRecord makeValidReplayRecord() {
    CommandReplayRecord rr;
    rr.record_id = ReplayRecordId("agent_replay_cmd_001");
    rr.command_id = CommandId("cmd_001");
    rr.state_version_before = StateVersion(1);
    rr.state_version_after = StateVersion(2);
    rr.random_seed = RandomSeed(42);
    rr.input_hash = HashValue(111);
    rr.output_hash = HashValue(222);
    rr.pipeline_status = PipelineStatus::Succeeded;
    rr.error_count = 0;
    rr.status = ReplayRecordStatus::Recorded;

    CommandEnvelope env;
    env.command_id = CommandId("cmd_001");
    env.payload.action_id = ActionId("eat");
    env.payload.actor_id = EntityId("actor_001");
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId("obj_001");
    env.payload.targets.push_back(target);
    env.source = CommandSource::Ai;
    rr.command = env;

    return rr;
}

// --- Enum Roundtrip Tests ---

void test_lock_status_roundtrip() {
    auto values = {
        AgentReplayLockStatus::Unknown,
        AgentReplayLockStatus::Locked,
        AgentReplayLockStatus::ExplainedOnly,
        AgentReplayLockStatus::Broken,
        AgentReplayLockStatus::Invalid
    };
    for (auto v : values) {
        auto str = toString(v);
        auto result = agentReplayLockStatusFromString(str);
        assert(result.is_ok());
        assert(result.value() == v);
    }
    assert(agentReplayLockStatusFromString("Garbage").is_error());
    std::cout << "  PASS: lock_status_roundtrip\n";
}

void test_lock_reason_roundtrip() {
    auto values = {
        AgentReplayLockReason::Unknown,
        AgentReplayLockReason::CommandRecordMatched,
        AgentReplayLockReason::NoCommandExpected,
        AgentReplayLockReason::CommandRecordMissing,
        AgentReplayLockReason::CommandIdMismatch,
        AgentReplayLockReason::AgentRecordInvalid,
        AgentReplayLockReason::CommandRecordInvalid
    };
    for (auto v : values) {
        auto str = toString(v);
        auto result = agentReplayLockReasonFromString(str);
        assert(result.is_ok());
        assert(result.value() == v);
    }
    assert(agentReplayLockReasonFromString("Garbage").is_error());
    std::cout << "  PASS: lock_reason_roundtrip\n";
}

// --- AgentReplayLockEntry Tests ---

void test_locked_entry_passes() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.command_id = CommandId("cmd_001");
    entry.replay_record_id = ReplayRecordId("agent_replay_cmd_001");
    entry.status = AgentReplayLockStatus::Locked;
    entry.reasons.push_back(AgentReplayLockReason::CommandRecordMatched);
    assert(entry.validateBasic().is_ok());
    std::cout << "  PASS: locked_entry_passes\n";
}

void test_locked_entry_missing_command_id_fails() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.replay_record_id = ReplayRecordId("agent_replay_cmd_001");
    entry.status = AgentReplayLockStatus::Locked;
    assert(entry.validateBasic().is_error());
    std::cout << "  PASS: locked_entry_missing_command_id_fails\n";
}

void test_locked_entry_missing_replay_record_id_fails() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.command_id = CommandId("cmd_001");
    entry.status = AgentReplayLockStatus::Locked;
    assert(entry.validateBasic().is_error());
    std::cout << "  PASS: locked_entry_missing_replay_record_id_fails\n";
}

void test_explained_only_no_command_passes() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.status = AgentReplayLockStatus::ExplainedOnly;
    entry.reasons.push_back(AgentReplayLockReason::NoCommandExpected);
    assert(entry.validateBasic().is_ok());
    std::cout << "  PASS: explained_only_no_command_passes\n";
}

void test_broken_entry_requires_reason() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.status = AgentReplayLockStatus::Broken;
    assert(entry.validateBasic().is_error());
    std::cout << "  PASS: broken_entry_requires_reason\n";
}

void test_unknown_status_fails() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.status = AgentReplayLockStatus::Unknown;
    assert(entry.validateBasic().is_error());
    std::cout << "  PASS: unknown_status_fails\n";
}

void test_empty_agent_record_id_fails() {
    AgentReplayLockEntry entry;
    entry.agent_id = AgentId("agent_001");
    entry.status = AgentReplayLockStatus::ExplainedOnly;
    assert(entry.validateBasic().is_error());
    std::cout << "  PASS: empty_agent_record_id_fails\n";
}

void test_empty_agent_id_fails() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.status = AgentReplayLockStatus::ExplainedOnly;
    assert(entry.validateBasic().is_error());
    std::cout << "  PASS: empty_agent_id_fails\n";
}

// --- AgentReplayLockSet Tests ---

void test_valid_lock_set_passes() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.command_id = CommandId("cmd_001");
    entry.replay_record_id = ReplayRecordId("agent_replay_cmd_001");
    entry.status = AgentReplayLockStatus::Locked;
    entry.reasons.push_back(AgentReplayLockReason::CommandRecordMatched);

    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = makeAgentReplayLockSetId(Tick(0), Tick(0), 1);
    lock_set.source_hash = HashValue(999);
    lock_set.entries.push_back(entry);

    assert(lock_set.validateBasic().is_ok());
    std::cout << "  PASS: valid_lock_set_passes\n";
}

void test_empty_lock_set_fails() {
    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = makeAgentReplayLockSetId(Tick(0), Tick(0), 0);
    lock_set.source_hash = HashValue(999);
    assert(lock_set.validateBasic().is_error());
    std::cout << "  PASS: empty_lock_set_fails\n";
}

void test_all_replayable_without_policy() {
    AgentReplayLockEntry locked;
    locked.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    locked.agent_id = AgentId("agent_001");
    locked.tick = Tick(0);
    locked.state_version_before = StateVersion(1);
    locked.state_version_after = StateVersion(2);
    locked.command_id = CommandId("cmd_001");
    locked.replay_record_id = ReplayRecordId("agent_replay_cmd_001");
    locked.status = AgentReplayLockStatus::Locked;

    AgentReplayLockEntry explained;
    explained.agent_record_id = makeAgentTickRecordId(AgentId("agent_002"), Tick(1), StateVersion(3));
    explained.agent_id = AgentId("agent_002");
    explained.tick = Tick(1);
    explained.state_version_before = StateVersion(3);
    explained.state_version_after = StateVersion(4);
    explained.status = AgentReplayLockStatus::ExplainedOnly;
    explained.reasons.push_back(AgentReplayLockReason::NoCommandExpected);

    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = makeAgentReplayLockSetId(Tick(0), Tick(1), 2);
    lock_set.source_hash = HashValue(999);
    lock_set.entries.push_back(locked);
    lock_set.entries.push_back(explained);

    assert(lock_set.allReplayableWithoutPolicy());
    std::cout << "  PASS: all_replayable_without_policy\n";
}

void test_has_broken_entry() {
    AgentReplayLockEntry broken;
    broken.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    broken.agent_id = AgentId("agent_001");
    broken.tick = Tick(0);
    broken.state_version_before = StateVersion(1);
    broken.state_version_after = StateVersion(2);
    broken.command_id = CommandId("cmd_001");
    broken.status = AgentReplayLockStatus::Broken;
    broken.reasons.push_back(AgentReplayLockReason::CommandRecordMissing);

    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = makeAgentReplayLockSetId(Tick(0), Tick(0), 1);
    lock_set.source_hash = HashValue(999);
    lock_set.entries.push_back(broken);

    assert(lock_set.hasBrokenEntry());
    assert(!lock_set.allReplayableWithoutPolicy());
    std::cout << "  PASS: has_broken_entry\n";
}

// --- ID Helper Tests ---

void test_lock_set_empty_source_hash_fails() {
    AgentReplayLockEntry entry;
    entry.agent_record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(0);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.command_id = CommandId("cmd_001");
    entry.replay_record_id = ReplayRecordId("agent_replay_cmd_001");
    entry.status = AgentReplayLockStatus::Locked;
    entry.reasons.push_back(AgentReplayLockReason::CommandRecordMatched);

    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = makeAgentReplayLockSetId(Tick(0), Tick(0), 1);
    // source_hash empty
    lock_set.entries.push_back(entry);

    assert(lock_set.validateBasic().is_error());
    std::cout << "  PASS: lock_set_empty_source_hash_fails\n";
}

void test_lock_set_id_helper() {
    auto id = makeAgentReplayLockSetId(Tick(5), Tick(10), 3);
    assert(id.value() == "agent_replay_lock_set_5_10_3");
    assert(!id.empty());
    std::cout << "  PASS: lock_set_id_helper\n";
}

// --- Builder Tests ---

void test_builder_matching_command_locked() {
    auto rec = makeValidTickRecord();
    AgentTickLog log;
    log.append(rec);

    CommandReplayLog cmd_log;
    cmd_log.append(makeValidReplayRecord());

    AgentReplayLockBuildRequest req;
    req.agent_log = &log;
    req.command_log = &cmd_log;
    req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& lock_set = result.value();
    assert(lock_set.entries.size() == 1);
    assert(lock_set.entries[0].status == AgentReplayLockStatus::Locked);
    assert(lock_set.entries[0].replay_record_id.has_value());
    assert(lock_set.allReplayableWithoutPolicy());
    std::cout << "  PASS: builder_matching_command_locked\n";
}

void test_builder_missing_command_broken() {
    auto rec = makeValidTickRecord();
    AgentTickLog log;
    log.append(rec);

    CommandReplayLog cmd_log;  // empty

    AgentReplayLockBuildRequest req;
    req.agent_log = &log;
    req.command_log = &cmd_log;
    req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& lock_set = result.value();
    assert(lock_set.entries.size() == 1);
    assert(lock_set.entries[0].status == AgentReplayLockStatus::Broken);
    assert(lock_set.hasBrokenEntry());
    std::cout << "  PASS: builder_missing_command_broken\n";
}

void test_builder_submit_skipped_explained_only() {
    auto rec = makeValidTickRecord();
    rec.runtime_status = AgentRuntimeStatus::SubmitSkipped;
    rec.pipeline_status = std::nullopt;
    rec.command = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::SubmitSkipped;
    rec.decision_record.command_id = std::nullopt;

    AgentTickLog log;
    log.append(rec);

    CommandReplayLog cmd_log;

    AgentReplayLockBuildRequest req;
    req.agent_log = &log;
    req.command_log = &cmd_log;
    req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& lock_set = result.value();
    assert(lock_set.entries.size() == 1);
    assert(lock_set.entries[0].status == AgentReplayLockStatus::ExplainedOnly);
    assert(lock_set.allReplayableWithoutPolicy());
    std::cout << "  PASS: builder_submit_skipped_explained_only\n";
}

void test_builder_no_decision_explained_only() {
    auto rec = makeValidTickRecord();
    rec.runtime_status = AgentRuntimeStatus::NoDecision;
    rec.pipeline_status = std::nullopt;
    rec.command = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_record.command_id = std::nullopt;
    rec.decision_record.reason_key = "no_food_nearby";

    AgentTickLog log;
    log.append(rec);

    CommandReplayLog cmd_log;

    AgentReplayLockBuildRequest req;
    req.agent_log = &log;
    req.command_log = &cmd_log;
    req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& lock_set = result.value();
    assert(lock_set.entries.size() == 1);
    assert(lock_set.entries[0].status == AgentReplayLockStatus::ExplainedOnly);
    assert(lock_set.allReplayableWithoutPolicy());
    std::cout << "  PASS: builder_no_decision_explained_only\n";
}

void test_builder_source_hash_empty_fails() {
    AgentTickLog log;
    CommandReplayLog cmd_log;

    AgentReplayLockBuildRequest req;
    req.agent_log = &log;
    req.command_log = &cmd_log;
    // source_hash empty

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: builder_source_hash_empty_fails\n";
}

void test_builder_null_agent_log_fails() {
    CommandReplayLog cmd_log;

    AgentReplayLockBuildRequest req;
    req.agent_log = nullptr;
    req.command_log = &cmd_log;
    req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: builder_null_agent_log_fails\n";
}

void test_builder_empty_agent_log_fails() {
    AgentTickLog log;
    CommandReplayLog cmd_log;

    AgentReplayLockBuildRequest req;
    req.agent_log = &log;
    req.command_log = &cmd_log;
    req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: builder_empty_agent_log_fails\n";
}

// --- Main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "Agent replay lock smoke test passed" << std::endl;
        return 0;
    }

    // Enum roundtrip
    if (test_name == "lock_status_roundtrip") { test_lock_status_roundtrip(); return 0; }
    if (test_name == "lock_reason_roundtrip") { test_lock_reason_roundtrip(); return 0; }

    // Entry tests
    if (test_name == "locked_entry_passes") { test_locked_entry_passes(); return 0; }
    if (test_name == "locked_entry_missing_command_id_fails") { test_locked_entry_missing_command_id_fails(); return 0; }
    if (test_name == "locked_entry_missing_replay_record_id_fails") { test_locked_entry_missing_replay_record_id_fails(); return 0; }
    if (test_name == "explained_only_no_command_passes") { test_explained_only_no_command_passes(); return 0; }
    if (test_name == "broken_entry_requires_reason") { test_broken_entry_requires_reason(); return 0; }
    if (test_name == "unknown_status_fails") { test_unknown_status_fails(); return 0; }
    if (test_name == "empty_agent_record_id_fails") { test_empty_agent_record_id_fails(); return 0; }
    if (test_name == "empty_agent_id_fails") { test_empty_agent_id_fails(); return 0; }

    // Set tests
    if (test_name == "valid_lock_set_passes") { test_valid_lock_set_passes(); return 0; }
    if (test_name == "empty_lock_set_fails") { test_empty_lock_set_fails(); return 0; }
    if (test_name == "all_replayable_without_policy") { test_all_replayable_without_policy(); return 0; }
    if (test_name == "has_broken_entry") { test_has_broken_entry(); return 0; }

    // ID helper
    if (test_name == "lock_set_id_helper") { test_lock_set_id_helper(); return 0; }

    // Builder tests
    if (test_name == "builder_matching_command_locked") { test_builder_matching_command_locked(); return 0; }
    if (test_name == "builder_missing_command_broken") { test_builder_missing_command_broken(); return 0; }
    if (test_name == "builder_submit_skipped_explained_only") { test_builder_submit_skipped_explained_only(); return 0; }
    if (test_name == "builder_no_decision_explained_only") { test_builder_no_decision_explained_only(); return 0; }
    if (test_name == "builder_source_hash_empty_fails") { test_builder_source_hash_empty_fails(); return 0; }
    if (test_name == "builder_null_agent_log_fails") { test_builder_null_agent_log_fails(); return 0; }
    if (test_name == "builder_empty_agent_log_fails") { test_builder_empty_agent_log_fails(); return 0; }
    if (test_name == "lock_set_empty_source_hash_fails") { test_lock_set_empty_source_hash_fails(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
