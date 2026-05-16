#include "pathfinder/event/event_record.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::event;
using namespace pathfinder::foundation;

void run_event_record_tests() {
    // Test valid EventRecord
    {
        EventRecord record;
        record.event_id = EventId("event_001");
        record.event_type = EventType::CommandAccepted;
        record.tick = Tick(100);
        record.state_version = StateVersion(1);
        record.payload.payload_type = "action";
        record.payload.message_key = "cmd.accepted";
        record.visibility = EventVisibility::PlayerVisible;
        record.importance = EventImportance::Normal;

        auto result = record.validateBasic();
        assert(result.is_ok());
    }

    // Test missing event_id fails
    {
        EventRecord record;
        record.event_type = EventType::CommandAccepted;
        record.payload.payload_type = "action";

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test Unknown event_type fails
    {
        EventRecord record;
        record.event_id = EventId("event_002");
        record.event_type = EventType::Unknown;
        record.payload.payload_type = "action";

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test empty payload_type fails
    {
        EventRecord record;
        record.event_id = EventId("event_003");
        record.event_type = EventType::Debug;

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test invalid event_id format fails
    {
        EventRecord record;
        record.event_id = EventId("invalid id!");
        record.event_type = EventType::CommandAccepted;
        record.payload.payload_type = "action";

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test invalid command_id format fails
    {
        EventRecord record;
        record.event_id = EventId("event_bad_cmd");
        record.event_type = EventType::CommandAccepted;
        record.command_id = CommandId("invalid id!");
        record.payload.payload_type = "action";

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test invalid state_change_id format fails
    {
        EventRecord record;
        record.event_id = EventId("event_bad_sc");
        record.event_type = EventType::StateChanged;
        record.tick = Tick(100);
        record.state_version = StateVersion(1);
        record.state_change_ids.push_back(StateChangeId("invalid id!"));
        record.payload.payload_type = "state";

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test invalid target_id format fails
    {
        EventRecord record;
        record.event_id = EventId("event_bad_target");
        record.event_type = EventType::StateChanged;
        record.tick = Tick(100);
        record.state_version = StateVersion(1);
        record.target_ids.push_back(TargetId("invalid id!"));
        record.payload.payload_type = "state";

        auto result = record.validateBasic();
        assert(result.is_error());
    }

    // Test optional command_id
    {
        EventRecord record;
        record.event_id = EventId("event_004");
        record.event_type = EventType::StateChanged;
        record.tick = Tick(200);
        record.state_version = StateVersion(2);
        record.payload.payload_type = "state";
        record.visibility = EventVisibility::DebugOnly;
        record.importance = EventImportance::Low;

        // No command_id - should be ok for system events
        auto result = record.validateBasic();
        assert(result.is_ok());
    }

    // Test multiple state_change_ids
    {
        EventRecord record;
        record.event_id = EventId("event_005");
        record.event_type = EventType::StateChanged;
        record.tick = Tick(300);
        record.state_version = StateVersion(3);
        record.state_change_ids.push_back(StateChangeId("sc_001"));
        record.state_change_ids.push_back(StateChangeId("sc_002"));
        record.payload.payload_type = "state";
        record.visibility = EventVisibility::PlayerVisible;
        record.importance = EventImportance::Normal;

        auto result = record.validateBasic();
        assert(result.is_ok());
        assert(record.state_change_ids.size() == 2);
    }

    std::cout << "event_record tests passed" << std::endl;
}
