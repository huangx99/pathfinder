#include "pathfinder/replay/command_replay.h"
#include "pathfinder/replay/random_replay.h"
#include "pathfinder/replay/replay_runner.h"
#include "pathfinder/protocol/envelope.h"
#include "pathfinder/protocol/command_result_adapter.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/event/event_record.h"
#include "pathfinder/object/world_object.h"
#include "pathfinder/cognition/cognition_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::replay;
using namespace pathfinder::protocol;
using namespace pathfinder::foundation;
using namespace pathfinder::pipeline;
using namespace pathfinder::command;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "PASS: p4 integration smoke" << std::endl;
        return 0;
    }

    if (test_name == "replay_protocol_flow") {
        // Step 1: Create P3 initial state
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        // Step 2: Execute RulePipeline
        RulePipeline pipeline;
        pathfinder::state::GameState exec_state = initial_state;
        PipelineContext ctx;
        ctx.command = cmd;
        ctx.state_metadata = exec_state.metadata;
        ctx.game_state = &exec_state;
        ctx.random_seed = RandomSeed(42);
        auto pipeline_result = pipeline.execute(ctx);
        assert(pipeline_result.is_ok());
        assert(pipeline_result.value().status == PipelineStatus::Succeeded);

        // Step 2a: Verify event type sequence
        auto& events = pipeline_result.value().events.events();
        assert(events.size() == 4);
        assert(events[0].event_type == pathfinder::event::EventType::ActionResolved);
        assert(events[1].event_type == pathfinder::event::EventType::FeedbackGenerated);
        assert(events[2].event_type == pathfinder::event::EventType::StateChanged);
        assert(events[3].event_type == pathfinder::event::EventType::CognitionUpdated);

        // Step 2b: Verify state change count (hunger + consumed = 2)
        assert(pipeline_result.value().state_changes.size() == 2);

        // Step 2c: Verify final actor hunger == 60 (80 + (-20))
        auto* actor = exec_state.actor_store.findActor(
            pathfinder::foundation::EntityId("actor_001"));
        assert(actor != nullptr);
        assert(actor->hunger == 60);

        // Step 2d: Verify object consumed
        auto* obj = exec_state.object_store.findObject(
            pathfinder::foundation::ObjectId("obj_unknown_fruit_001"));
        assert(obj != nullptr);
        assert(obj->flag == pathfinder::object::ObjectRuntimeFlag::Consumed);

        // Step 2e: Verify cognition claim exists
        auto* claim = exec_state.cognition_state.findClaim(
            pathfinder::cognition::CognitionKey{
                pathfinder::foundation::EntityId("actor_001"),
                pathfinder::foundation::ObjectDefinitionId("unknown_fruit"),
                pathfinder::foundation::ActionId("eat"),
                pathfinder::cognition::CognitionEffectKind::Edible
            });
        assert(claim != nullptr);
        assert(claim->confidence > 0.0);
        assert(claim->evidence_count > 0);

        // Step 3: Generate CommandReplayRecord
        CommandReplayRecord record;
        record.record_id = ReplayRecordId("rec_p4_001");
        record.command_id = cmd.command_id;
        record.command = cmd;
        record.state_version_before = initial_state.metadata.state_version;
        record.state_version_after = exec_state.metadata.state_version;
        record.random_seed = RandomSeed(42);
        record.input_hash = HashValue::fromString("p4_input");
        record.output_hash = HashValue::fromString("p4_output");
        record.pipeline_status = pipeline_result.value().status;
        record.error_count = pipeline_result.value().errors.size();
        record.status = ReplayRecordStatus::Recorded;
        for (const auto& sc : pipeline_result.value().state_changes.changes()) {
            record.state_change_ids.push_back(sc.change_id);
        }
        for (const auto& ev : events) {
            record.event_ids.push_back(ev.event_id);
        }

        // Step 4: Append to CommandReplayLog
        CommandReplayLog cmd_log;
        auto append_result = cmd_log.append(record);
        assert(append_result.is_ok());
        assert(cmd_log.size() == 1);

        // Step 5: Append RandomReplayLog seed entry
        RandomReplayLog rand_log;
        RandomReplayEntry rand_entry;
        rand_entry.entry_id = RandomReplayEntryId("rand_p4_001");
        rand_entry.command_id = cmd.command_id;
        rand_entry.seed = RandomSeed(42);
        rand_entry.draw_index = 0;
        rand_entry.min_value = 0;
        rand_entry.max_value = 100;
        rand_entry.result_value = 0;
        rand_entry.reason = "eat_unknown_fruit_seed";
        auto rand_append = rand_log.append(rand_entry);
        assert(rand_append.is_ok());

        // Step 6: Replay
        ReplayInput replay_input;
        replay_input.initial_state = initial_state;
        replay_input.command_log = cmd_log;
        replay_input.random_log = rand_log;

        ReplayRunner runner;
        auto replay_result = runner.replayOne(replay_input);
        assert(replay_result.is_ok());
        assert(replay_result.value().report.status == ReplayCompareStatus::Match);

        // Step 6a: Compare replay state vs first execution state
        auto& replay_state = replay_result.value().replay_state;

        // Compare event type sequence
        auto& replay_events = replay_result.value().pipeline_result.events.events();
        assert(replay_events.size() == events.size());
        for (size_t i = 0; i < events.size(); ++i) {
            assert(replay_events[i].event_type == events[i].event_type);
        }

        // Compare final actor hunger
        auto* replay_actor = replay_state.actor_store.findActor(
            pathfinder::foundation::EntityId("actor_001"));
        assert(replay_actor != nullptr);
        assert(replay_actor->hunger == actor->hunger);

        // Compare object consumed
        auto* replay_obj = replay_state.object_store.findObject(
            pathfinder::foundation::ObjectId("obj_unknown_fruit_001"));
        assert(replay_obj != nullptr);
        assert(replay_obj->flag == obj->flag);

        // Compare cognition claim
        auto* replay_claim = replay_state.cognition_state.findClaim(
            pathfinder::cognition::CognitionKey{
                pathfinder::foundation::EntityId("actor_001"),
                pathfinder::foundation::ObjectDefinitionId("unknown_fruit"),
                pathfinder::foundation::ActionId("eat"),
                pathfinder::cognition::CognitionEffectKind::Edible
            });
        assert(replay_claim != nullptr);
        assert(replay_claim->confidence == claim->confidence);
        assert(replay_claim->evidence_count == claim->evidence_count);

        // Step 7: Build protocol envelope
        auto envelope = buildCommandResponseEnvelope(cmd, pipeline_result.value());
        auto env_validation = envelope.validateBasic();
        assert(env_validation.is_ok());

        // Step 8: Verify envelope properties
        assert(envelope.domain == ProtocolDomain::Command);
        assert(envelope.message_type == ProtocolMessageType::Response);
        assert(envelope.payload.payload_type == ProtocolPayloadType::CommandResult);
        assert(envelope.payload.message_key == "command.succeeded");

        // Step 9: Verify envelope doesn't expose internal state draft structures
        // The payload debug_text should contain counts, not internal structures
        assert(envelope.payload.debug_text.find("changes=") != std::string::npos);
        assert(envelope.payload.debug_text.find("events=") != std::string::npos);

        // Step 10: Verify replay result matches pipeline result
        assert(replay_result.value().pipeline_result.status == pipeline_result.value().status);
        assert(replay_result.value().pipeline_result.state_changes.size() ==
               pipeline_result.value().state_changes.size());
        assert(replay_result.value().pipeline_result.events.size() ==
               pipeline_result.value().events.size());

        std::cout << "PASS: p4 replay-protocol end-to-end flow" << std::endl;
        return 0;
    }

    return 1;
}
