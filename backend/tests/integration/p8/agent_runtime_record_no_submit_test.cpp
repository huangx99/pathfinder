#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/policy.h"
#include "pathfinder/agent/record_builder.h"
#include "pathfinder/agent/replay_bridge.h"
#include "pathfinder/agent/record_types.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/state/actor_state.h"
#include "pathfinder/replay/command_replay.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::command;
using namespace pathfinder::replay;

void run_p8_no_submit_record() {
    // === Test 1: Fire threat -> SubmitSkipped ===
    {
        GameState state;
        state.metadata.state_id = GameStateId(std::string("state_fire"));
        state.metadata.state_version = StateVersion(1);
        state.metadata.current_tick = Tick(0);

        ActorSurvivalState actor;
        actor.actor_id = EntityId(std::string("actor_001"));
        actor.hunger = 50;
        actor.health = 100;
        state.actor_store.addActor(actor);

        AgentRuntimeOptions opts;
        opts.submit_to_pipeline = true;
        opts.submit_action_allowlist.push_back(ActionId(std::string("eat")));

        AgentTickRequest req;
        req.binding.agent_id = AgentId(std::string("agent_001"));
        req.binding.primary_actor_id = EntityId(std::string("actor_001"));
        req.binding.authority = AgentControlAuthority::Primary;
        req.state = &state;
        req.issued_tick = Tick(0);
        req.random_seed = RandomSeed(42);
        req.command_source = CommandSource::Ai;
        req.options = opts;

        ObservedThreatSeed fire_seed;
        fire_seed.source_id = EntityId(std::string("fire_001"));
        fire_seed.threat_type = AgentThreatType::Fire;
        fire_seed.severity = 0.95;
        fire_seed.confidence = 0.99;
        req.visibility.threat_seeds.push_back(fire_seed);

        FirstSupportedPolicy policy;
        AgentRuntime runtime(policy);
        auto tick_result = runtime.tickOne(req);
        assert(tick_result.is_ok());
        assert(tick_result.value().status == AgentRuntimeStatus::SubmitSkipped);
        std::cout << "  PASS: p8_fire_submit_skipped\n";

        // Build record
        AgentRecordBuildRequest build_req;
        build_req.tick_request = req;
        build_req.tick_result = tick_result.value();
        build_req.input_hash = HashValue(111);
        build_req.output_hash = HashValue(222);

        AgentRecordBuilder builder;
        auto record_result = builder.build(build_req);
        assert(record_result.is_ok());
        auto& tick_record = record_result.value();
        assert(tick_record.validateBasic().is_ok());
        assert(tick_record.runtime_status == AgentRuntimeStatus::SubmitSkipped);
        assert(tick_record.decision_record.status == AgentDecisionRecordStatus::SubmitSkipped);
        std::cout << "  PASS: p8_fire_record_valid\n";

        // Bridge should fail (no pipeline submission)
        AgentReplayBridge bridge;
        auto bridge_result = bridge.toCommandReplayRecord(tick_record);
        assert(bridge_result.is_error());
        std::cout << "  PASS: p8_fire_bridge_fails\n";

        // Lock check: has agent record but no command record
        AgentReplayLockChecker checker;
        CommandReplayLog empty_log;
        auto lock_result = checker.check(tick_record, empty_log);
        assert(lock_result.has_agent_record);
        assert(!lock_result.has_command_record);
        assert(!lock_result.can_replay_without_policy);
        std::cout << "  PASS: p8_fire_lock_check\n";
    }

    // === Test 2: Social threat -> NoDecision ===
    {
        GameState state;
        state.metadata.state_id = GameStateId(std::string("state_social"));
        state.metadata.state_version = StateVersion(1);
        state.metadata.current_tick = Tick(0);

        ActorSurvivalState actor;
        actor.actor_id = EntityId(std::string("actor_001"));
        actor.hunger = 50;
        actor.health = 100;
        state.actor_store.addActor(actor);

        AgentRuntimeOptions opts;
        opts.submit_to_pipeline = true;
        opts.submit_action_allowlist.push_back(ActionId(std::string("eat")));

        AgentTickRequest req;
        req.binding.agent_id = AgentId(std::string("agent_001"));
        req.binding.primary_actor_id = EntityId(std::string("actor_001"));
        req.binding.authority = AgentControlAuthority::Primary;
        req.state = &state;
        req.issued_tick = Tick(0);
        req.random_seed = RandomSeed(42);
        req.command_source = CommandSource::Ai;
        req.options = opts;

        ObservedThreatSeed social_seed;
        social_seed.source_id = EntityId(std::string("pack_001"));
        social_seed.threat_type = AgentThreatType::Social;
        social_seed.severity = 0.7;
        social_seed.confidence = 0.8;
        req.visibility.threat_seeds.push_back(social_seed);

        FirstSupportedPolicy policy;
        AgentRuntime runtime(policy);
        auto tick_result = runtime.tickOne(req);
        assert(tick_result.is_ok());
        assert(tick_result.value().status == AgentRuntimeStatus::NoDecision);
        std::cout << "  PASS: p8_social_no_decision\n";

        // Build record
        AgentRecordBuildRequest build_req;
        build_req.tick_request = req;
        build_req.tick_result = tick_result.value();
        build_req.input_hash = HashValue(333);
        build_req.output_hash = HashValue(444);

        AgentRecordBuilder builder;
        auto record_result = builder.build(build_req);
        assert(record_result.is_ok());
        auto& tick_record = record_result.value();
        assert(tick_record.validateBasic().is_ok());
        assert(tick_record.runtime_status == AgentRuntimeStatus::NoDecision);
        assert(tick_record.decision_record.status == AgentDecisionRecordStatus::NoDecision);
        assert(!tick_record.command.has_value());
        std::cout << "  PASS: p8_social_record_valid\n";

        // Bridge should fail (no command)
        AgentReplayBridge bridge;
        auto bridge_result = bridge.toCommandReplayRecord(tick_record);
        assert(bridge_result.is_error());
        std::cout << "  PASS: p8_social_bridge_fails\n";

        // Lock check
        AgentReplayLockChecker checker;
        CommandReplayLog empty_log;
        auto lock_result = checker.check(tick_record, empty_log);
        assert(lock_result.has_agent_record);
        assert(!lock_result.has_command_record);
        assert(!lock_result.can_replay_without_policy);
        std::cout << "  PASS: p8_social_lock_check\n";
    }

    std::cout << "P8 no-submit / no-decision record tests passed!\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P8 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "p8_no_submit_record") {
        run_p8_no_submit_record();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
