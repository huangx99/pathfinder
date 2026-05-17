#include "pathfinder/agent/training_sample.h"
#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/agent/types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/command/target.h"
#include "pathfinder/pipeline/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;

// --- Helper: make a PipelineSucceeded AgentTickRecord ---
AgentTickRecord makePipelineSucceededRecord() {
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
    rec.decision_record.selected_candidate_id = ActionId("eat_candidate");
    rec.decision_record.selected_command_action_id = ActionId("eat");
    rec.decision_record.selected_intent_type = AgentIntentType::Eat;
    rec.decision_record.confidence = 0.9;

    rec.phase_keys.push_back("observation_built");
    rec.phase_keys.push_back("action_space_built");

    return rec;
}

// --- Enum Roundtrip Tests ---

void test_sample_status_roundtrip() {
    auto values = {
        AgentTrainingSampleStatus::Unknown,
        AgentTrainingSampleStatus::Draft,
        AgentTrainingSampleStatus::ReplayLocked,
        AgentTrainingSampleStatus::Skipped,
        AgentTrainingSampleStatus::NoDecision,
        AgentTrainingSampleStatus::Invalid
    };
    for (auto v : values) {
        auto str = toString(v);
        auto result = agentTrainingSampleStatusFromString(str);
        assert(result.is_ok());
        assert(result.value() == v);
    }
    assert(agentTrainingSampleStatusFromString("Garbage").is_error());
    std::cout << "  PASS: sample_status_roundtrip\n";
}

void test_reward_status_roundtrip() {
    auto values = {
        AgentRewardDraftStatus::Unknown,
        AgentRewardDraftStatus::NotComputed,
        AgentRewardDraftStatus::PlaceholderOnly
    };
    for (auto v : values) {
        auto str = toString(v);
        auto result = agentRewardDraftStatusFromString(str);
        assert(result.is_ok());
        assert(result.value() == v);
    }
    assert(agentRewardDraftStatusFromString("Garbage").is_error());
    std::cout << "  PASS: reward_status_roundtrip\n";
}

void test_done_status_roundtrip() {
    auto values = {
        AgentDoneDraftStatus::Unknown,
        AgentDoneDraftStatus::NotComputed,
        AgentDoneDraftStatus::PlaceholderOnly
    };
    for (auto v : values) {
        auto str = toString(v);
        auto result = agentDoneDraftStatusFromString(str);
        assert(result.is_ok());
        assert(result.value() == v);
    }
    assert(agentDoneDraftStatusFromString("Garbage").is_error());
    std::cout << "  PASS: done_status_roundtrip\n";
}

// --- ID Helper Tests ---

void test_sample_id_deterministic() {
    auto id1 = makeAgentTrainingSampleId(AgentId("agent_001"), Tick(0), StateVersion(1));
    auto id2 = makeAgentTrainingSampleId(AgentId("agent_001"), Tick(0), StateVersion(1));
    assert(id1 == id2);
    assert(id1.value() == "agent_training_sample_agent_001_0_1");
    std::cout << "  PASS: sample_id_deterministic\n";
}

// --- Validation Tests ---

void test_valid_sample_passes() {
    auto rec = makePipelineSucceededRecord();
    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    assert(result.value().validateBasic().is_ok());
    std::cout << "  PASS: valid_sample_passes\n";
}

void test_missing_sample_id_fails() {
    AgentTrainingSampleDraft sample;
    sample.source_record_id = AgentTickRecordId("test");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: missing_sample_id_fails\n";
}

void test_missing_source_record_id_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: missing_source_record_id_fails\n";
}

void test_unknown_status_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Unknown;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: unknown_status_fails\n";
}

void test_result_unknown_runtime_status_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::Unknown;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: result_unknown_runtime_status_fails\n";
}

void test_empty_input_hash_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: empty_input_hash_fails\n";
}

void test_empty_output_hash_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: empty_output_hash_fails\n";
}

void test_reward_status_unknown_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::Unknown;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: reward_status_unknown_fails\n";
}

void test_done_status_unknown_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::Unknown;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    sample.action.command_id = CommandId("cmd_001");
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: done_status_unknown_fails\n";
}

void test_pipeline_succeeded_requires_command_id() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::Draft;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;
    // no command_id
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: pipeline_succeeded_requires_command_id\n";
}

void test_no_decision_allows_no_command_id() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::NoDecision;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::NoDecision;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::NoDecision;
    sample.trace.reason_keys.push_back("no_decision_no_command");
    assert(sample.validateBasic().is_ok());
    std::cout << "  PASS: no_decision_allows_no_command_id\n";
}

void test_no_decision_trace_no_reason_fails() {
    AgentTrainingSampleDraft sample;
    sample.sample_id = AgentTrainingSampleId("test_sample");
    sample.source_record_id = AgentTickRecordId("test_record");
    sample.status = AgentTrainingSampleStatus::NoDecision;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.observation_hash = HashValue(333);
    sample.result.runtime_status = AgentRuntimeStatus::NoDecision;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.action.decision_status = AgentDecisionRecordStatus::NoDecision;
    // no reason_keys or phase_keys in trace
    assert(sample.validateBasic().is_error());
    std::cout << "  PASS: no_decision_trace_no_reason_fails\n";
}

// --- Builder Tests ---

void test_builder_pipeline_succeeded_draft() {
    auto rec = makePipelineSucceededRecord();
    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& sample = result.value();
    assert(sample.status == AgentTrainingSampleStatus::Draft);
    assert(sample.observation.agent_id == AgentId("agent_001"));
    assert(sample.observation.observation_hash == HashValue(333));
    assert(sample.action.decision_id.has_value());
    assert(sample.action.command_id.has_value());
    assert(sample.result.runtime_status == AgentRuntimeStatus::PipelineSucceeded);
    assert(sample.result.reward_status == AgentRewardDraftStatus::NotComputed);
    assert(sample.result.done_status == AgentDoneDraftStatus::NotComputed);
    assert(sample.result.state_change_count == 0);
    assert(sample.result.event_count == 0);
    std::cout << "  PASS: builder_pipeline_succeeded_draft\n";
}

void test_builder_replay_locked_sample() {
    auto rec = makePipelineSucceededRecord();

    AgentReplayLockEntry lock_entry;
    lock_entry.agent_record_id = rec.record_id;
    lock_entry.agent_id = rec.agent_id;
    lock_entry.tick = rec.tick;
    lock_entry.state_version_before = rec.state_version_before;
    lock_entry.state_version_after = rec.state_version_after;
    lock_entry.command_id = rec.decision_record.command_id;
    lock_entry.replay_record_id = pathfinder::replay::ReplayRecordId("agent_replay_cmd_001");
    lock_entry.status = AgentReplayLockStatus::Locked;
    lock_entry.reasons.push_back(AgentReplayLockReason::CommandRecordMatched);

    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.replay_lock_entry = lock_entry;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& sample = result.value();
    assert(sample.status == AgentTrainingSampleStatus::ReplayLocked);
    assert(sample.trace.replay_locked);
    assert(sample.trace.replay_lock_status == AgentReplayLockStatus::Locked);
    std::cout << "  PASS: builder_replay_locked_sample\n";
}

void test_builder_submit_skipped_sample() {
    auto rec = makePipelineSucceededRecord();
    rec.runtime_status = AgentRuntimeStatus::SubmitSkipped;
    rec.pipeline_status = std::nullopt;
    rec.command = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::SubmitSkipped;
    rec.decision_record.command_id = std::nullopt;

    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& sample = result.value();
    assert(sample.status == AgentTrainingSampleStatus::Skipped);
    assert(sample.result.reward_status == AgentRewardDraftStatus::NotComputed);
    assert(sample.result.done_status == AgentDoneDraftStatus::NotComputed);
    std::cout << "  PASS: builder_submit_skipped_sample\n";
}

void test_builder_no_decision_sample() {
    auto rec = makePipelineSucceededRecord();
    rec.runtime_status = AgentRuntimeStatus::NoDecision;
    rec.pipeline_status = std::nullopt;
    rec.command = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_record.command_id = std::nullopt;
    rec.decision_record.reason_key = "no_food_nearby";

    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& sample = result.value();
    assert(sample.status == AgentTrainingSampleStatus::NoDecision);
    assert(sample.result.reward_status == AgentRewardDraftStatus::NotComputed);
    assert(sample.result.done_status == AgentDoneDraftStatus::NotComputed);
    std::cout << "  PASS: builder_no_decision_sample\n";
}

void test_builder_missing_observation_hash_fails() {
    auto rec = makePipelineSucceededRecord();
    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    // observation_hash empty

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: builder_missing_observation_hash_fails\n";
}

void test_builder_sample_id_deterministic() {
    auto rec = makePipelineSucceededRecord();
    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto r1 = builder.build(req);
    auto r2 = builder.build(req);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().sample_id == r2.value().sample_id);
    std::cout << "  PASS: builder_sample_id_deterministic\n";
}

void test_builder_state_counts_preserved() {
    auto rec = makePipelineSucceededRecord();
    StateChangeId sc_id("sc_001");
    rec.state_change_ids.push_back(sc_id);
    EventId ev_id("ev_001");
    rec.event_ids.push_back(ev_id);
    rec.error_count = 2;

    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    assert(result.value().result.state_change_count == 1);
    assert(result.value().result.event_count == 1);
    assert(result.value().result.error_count == 2);
    std::cout << "  PASS: builder_state_counts_preserved\n";
}

void test_trace_replay_locked_consistency() {
    // replay_locked=true with replay_lock_status=Locked should pass
    {
        AgentTraceSampleDraft trace;
        trace.replay_locked = true;
        trace.replay_lock_status = AgentReplayLockStatus::Locked;
        assert(trace.validateBasic().is_ok());
    }
    // replay_locked=true with replay_lock_status=Unknown should fail
    {
        AgentTraceSampleDraft trace;
        trace.replay_locked = true;
        trace.replay_lock_status = AgentReplayLockStatus::Unknown;
        assert(trace.validateBasic().is_error());
    }
    // replay_locked=false with any status should pass
    {
        AgentTraceSampleDraft trace;
        trace.replay_locked = false;
        trace.replay_lock_status = AgentReplayLockStatus::ExplainedOnly;
        assert(trace.validateBasic().is_ok());
    }
    std::cout << "  PASS: trace_replay_locked_consistency\n";
}

void test_trace_phase_keys_not_in_reason_keys() {
    auto rec = makePipelineSucceededRecord();
    rec.decision_record.phase_keys.clear();
    rec.decision_record.phase_keys.push_back("observation_built");
    rec.decision_record.phase_keys.push_back("action_space_built");
    rec.decision_record.reason_key = "";  // no reason key
    rec.decision_record.rejected_reason_keys.clear();

    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& sample = result.value();

    // phase_keys should contain tick record + decision record phase keys
    assert(sample.trace.phase_keys.size() >= 2);
    // reason_keys should NOT contain phase keys
    for (const auto& rk : sample.trace.reason_keys) {
        assert(rk != "observation_built");
        assert(rk != "action_space_built");
    }
    std::cout << "  PASS: trace_phase_keys_not_in_reason_keys\n";
}

void test_trace_rejected_reason_keys_in_reason_keys() {
    auto rec = makePipelineSucceededRecord();
    rec.decision_record.reason_key = "low_confidence";
    rec.decision_record.rejected_reason_keys.push_back("too_far");
    rec.decision_record.rejected_reason_keys.push_back("too_dangerous");

    AgentTrainingSampleBuildRequest req;
    req.record = rec;
    req.observation_hash = HashValue(333);

    AgentTrainingSampleDraftBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& sample = result.value();

    // reason_keys should contain reason_key and rejected_reason_keys
    bool found_reason = false;
    bool found_too_far = false;
    bool found_too_dangerous = false;
    for (const auto& rk : sample.trace.reason_keys) {
        if (rk == "low_confidence") found_reason = true;
        if (rk == "too_far") found_too_far = true;
        if (rk == "too_dangerous") found_too_dangerous = true;
    }
    assert(found_reason);
    assert(found_too_far);
    assert(found_too_dangerous);
    std::cout << "  PASS: trace_rejected_reason_keys_in_reason_keys\n";
}

// --- Main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "Agent training sample smoke test passed" << std::endl;
        return 0;
    }

    // Enum roundtrip
    if (test_name == "sample_status_roundtrip") { test_sample_status_roundtrip(); return 0; }
    if (test_name == "reward_status_roundtrip") { test_reward_status_roundtrip(); return 0; }
    if (test_name == "done_status_roundtrip") { test_done_status_roundtrip(); return 0; }

    // ID helper
    if (test_name == "sample_id_deterministic") { test_sample_id_deterministic(); return 0; }

    // Validation tests
    if (test_name == "valid_sample_passes") { test_valid_sample_passes(); return 0; }
    if (test_name == "missing_sample_id_fails") { test_missing_sample_id_fails(); return 0; }
    if (test_name == "missing_source_record_id_fails") { test_missing_source_record_id_fails(); return 0; }
    if (test_name == "unknown_status_fails") { test_unknown_status_fails(); return 0; }
    if (test_name == "result_unknown_runtime_status_fails") { test_result_unknown_runtime_status_fails(); return 0; }
    if (test_name == "empty_input_hash_fails") { test_empty_input_hash_fails(); return 0; }
    if (test_name == "empty_output_hash_fails") { test_empty_output_hash_fails(); return 0; }
    if (test_name == "reward_status_unknown_fails") { test_reward_status_unknown_fails(); return 0; }
    if (test_name == "done_status_unknown_fails") { test_done_status_unknown_fails(); return 0; }
    if (test_name == "pipeline_succeeded_requires_command_id") { test_pipeline_succeeded_requires_command_id(); return 0; }
    if (test_name == "no_decision_allows_no_command_id") { test_no_decision_allows_no_command_id(); return 0; }
    if (test_name == "no_decision_trace_no_reason_fails") { test_no_decision_trace_no_reason_fails(); return 0; }

    // Builder tests
    if (test_name == "builder_pipeline_succeeded_draft") { test_builder_pipeline_succeeded_draft(); return 0; }
    if (test_name == "builder_replay_locked_sample") { test_builder_replay_locked_sample(); return 0; }
    if (test_name == "builder_submit_skipped_sample") { test_builder_submit_skipped_sample(); return 0; }
    if (test_name == "builder_no_decision_sample") { test_builder_no_decision_sample(); return 0; }
    if (test_name == "builder_missing_observation_hash_fails") { test_builder_missing_observation_hash_fails(); return 0; }
    if (test_name == "builder_sample_id_deterministic") { test_builder_sample_id_deterministic(); return 0; }
    if (test_name == "builder_state_counts_preserved") { test_builder_state_counts_preserved(); return 0; }
    if (test_name == "trace_replay_locked_consistency") { test_trace_replay_locked_consistency(); return 0; }
    if (test_name == "trace_phase_keys_not_in_reason_keys") { test_trace_phase_keys_not_in_reason_keys(); return 0; }
    if (test_name == "trace_rejected_reason_keys_in_reason_keys") { test_trace_rejected_reason_keys_in_reason_keys(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
