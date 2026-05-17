#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/training_sample.h"
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
using namespace pathfinder::replay;

// Helper: construct a SubmitSkipped AgentTickRecord directly (no AgentRuntime)
AgentTickRecord makeSubmitSkippedRecord() {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    rec.agent_id = AgentId("agent_001");
    rec.actor_id = EntityId("actor_001");
    rec.tick = Tick(0);
    rec.state_version_before = StateVersion(1);
    rec.state_version_after = StateVersion(1);
    rec.random_seed = RandomSeed(42);
    rec.runtime_status = AgentRuntimeStatus::SubmitSkipped;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(111);
    rec.output_hash = HashValue(222);

    CommandEnvelope env;
    env.command_id = CommandId("cmd_001");
    env.payload.action_id = ActionId("flee");
    env.payload.actor_id = EntityId("actor_001");
    env.source = CommandSource::Ai;
    rec.command = env;

    rec.decision_record.record_id = makeAgentDecisionRecordId(DecisionId("dec_001"));
    rec.decision_record.decision_id = DecisionId("dec_001");
    rec.decision_record.agent_id = AgentId("agent_001");
    rec.decision_record.actor_id = EntityId("actor_001");
    rec.decision_record.decision_tick = Tick(0);
    rec.decision_record.observation_state_version = StateVersion(1);
    rec.decision_record.status = AgentDecisionRecordStatus::SubmitSkipped;
    rec.decision_record.command_id = CommandId("cmd_001");
    rec.decision_record.selected_candidate_id = ActionId("flee_candidate");
    rec.decision_record.selected_command_action_id = ActionId("flee");
    rec.decision_record.selected_intent_type = AgentIntentType::AvoidRisk;
    rec.decision_record.confidence = 0.8;
    rec.decision_record.reason_key = "submit_action_not_in_allowlist";
    rec.decision_record.phase_keys.push_back("observation_built");
    rec.decision_record.phase_keys.push_back("action_space_built");
    rec.decision_record.phase_keys.push_back("decision_made");

    rec.phase_keys.push_back("observation_built");
    rec.phase_keys.push_back("action_space_built");
    rec.phase_keys.push_back("decision_made");

    return rec;
}

// Helper: construct a NoDecision AgentTickRecord directly (no AgentRuntime)
AgentTickRecord makeNoDecisionRecord() {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId("agent_002"), Tick(1), StateVersion(3));
    rec.agent_id = AgentId("agent_002");
    rec.actor_id = EntityId("actor_002");
    rec.tick = Tick(1);
    rec.state_version_before = StateVersion(3);
    rec.state_version_after = StateVersion(3);
    rec.random_seed = RandomSeed(43);
    rec.runtime_status = AgentRuntimeStatus::NoDecision;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(333);
    rec.output_hash = HashValue(444);

    rec.decision_record.record_id = makeAgentDecisionRecordId(DecisionId("dec_002"));
    rec.decision_record.decision_id = DecisionId("dec_002");
    rec.decision_record.agent_id = AgentId("agent_002");
    rec.decision_record.actor_id = EntityId("actor_002");
    rec.decision_record.decision_tick = Tick(1);
    rec.decision_record.observation_state_version = StateVersion(3);
    rec.decision_record.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_record.confidence = 0.0;
    rec.decision_record.reason_key = "no_supported_candidate";
    rec.decision_record.phase_keys.push_back("observation_built");
    rec.decision_record.phase_keys.push_back("action_space_built");

    rec.phase_keys.push_back("observation_built");
    rec.phase_keys.push_back("action_space_built");

    return rec;
}

void run_p9_no_decision_training_draft() {
    // === Test 1: SubmitSkipped -> ExplainedOnly -> Skipped sample ===
    {
        auto tick_record = makeSubmitSkippedRecord();
        assert(tick_record.validateBasic().is_ok());
        std::cout << "  PASS: p9_submit_skipped_record_valid\n";

        // Build ReplayLockSet
        AgentTickLog tick_log;
        tick_log.append(tick_record);

        CommandReplayLog cmd_log;

        AgentReplayLockBuildRequest lock_req;
        lock_req.agent_log = &tick_log;
        lock_req.command_log = &cmd_log;
        lock_req.source_hash = HashValue(777);

        AgentReplayLockSetBuilder lock_builder;
        auto lock_result = lock_builder.build(lock_req);
        assert(lock_result.is_ok());
        auto& lock_set = lock_result.value();
        assert(lock_set.entries[0].status == AgentReplayLockStatus::ExplainedOnly);
        assert(lock_set.allReplayableWithoutPolicy());
        std::cout << "  PASS: p9_submit_skipped_explained_only\n";

        // Build TrainingSampleDraft
        AgentTrainingSampleBuildRequest sample_req;
        sample_req.record = tick_record;
        sample_req.replay_lock_entry = lock_set.entries[0];
        sample_req.observation_hash = HashValue(555);

        AgentTrainingSampleDraftBuilder sample_builder;
        auto sample_result = sample_builder.build(sample_req);
        assert(sample_result.is_ok());
        auto& sample = sample_result.value();
        assert(sample.validateBasic().is_ok());
        assert(sample.status == AgentTrainingSampleStatus::Skipped);
        assert(sample.result.reward_status == AgentRewardDraftStatus::NotComputed);
        assert(sample.result.done_status == AgentDoneDraftStatus::NotComputed);
        // SubmitSkipped trace must have reason_keys or phase_keys
        assert(!sample.trace.reason_keys.empty() || !sample.trace.phase_keys.empty());
        std::cout << "  PASS: p9_submit_skipped_sample\n";
    }

    // === Test 2: NoDecision -> ExplainedOnly -> NoDecision sample ===
    {
        auto tick_record = makeNoDecisionRecord();
        assert(tick_record.validateBasic().is_ok());
        std::cout << "  PASS: p9_no_decision_record_valid\n";

        // Build ReplayLockSet
        AgentTickLog tick_log;
        tick_log.append(tick_record);

        CommandReplayLog cmd_log;

        AgentReplayLockBuildRequest lock_req;
        lock_req.agent_log = &tick_log;
        lock_req.command_log = &cmd_log;
        lock_req.source_hash = HashValue(777);

        AgentReplayLockSetBuilder lock_builder;
        auto lock_result = lock_builder.build(lock_req);
        assert(lock_result.is_ok());
        auto& lock_set = lock_result.value();
        assert(lock_set.entries[0].status == AgentReplayLockStatus::ExplainedOnly);
        assert(lock_set.allReplayableWithoutPolicy());
        std::cout << "  PASS: p9_no_decision_explained_only\n";

        // Build TrainingSampleDraft
        AgentTrainingSampleBuildRequest sample_req;
        sample_req.record = tick_record;
        sample_req.replay_lock_entry = lock_set.entries[0];
        sample_req.observation_hash = HashValue(666);

        AgentTrainingSampleDraftBuilder sample_builder;
        auto sample_result = sample_builder.build(sample_req);
        assert(sample_result.is_ok());
        auto& sample = sample_result.value();
        assert(sample.validateBasic().is_ok());
        assert(sample.status == AgentTrainingSampleStatus::NoDecision);
        assert(sample.result.reward_status == AgentRewardDraftStatus::NotComputed);
        assert(sample.result.done_status == AgentDoneDraftStatus::NotComputed);
        // NoDecision trace must have reason_keys or phase_keys
        assert(!sample.trace.reason_keys.empty() || !sample.trace.phase_keys.empty());
        std::cout << "  PASS: p9_no_decision_sample\n";
    }

    std::cout << "P9 no-decision / skipped training draft test passed!\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P9 no-decision training draft smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "p9_no_decision_training_draft") {
        run_p9_no_decision_training_draft();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
