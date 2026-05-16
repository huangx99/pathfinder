#include "pathfinder/event/event_stream.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::event;
using namespace pathfinder::foundation;

void run_event_stream_tests() {
    // Test empty EventStream is valid
    {
        EventStream stream;
        assert(stream.empty());
        assert(stream.size() == 0);
        auto result = stream.validateBasic();
        assert(result.is_ok());
    }

    // Test adding valid event
    {
        EventStream stream;
        EventRecord record;
        record.event_id = EventId("event_001");
        record.event_type = EventType::CommandAccepted;
        record.tick = Tick(100);
        record.state_version = StateVersion(1);
        record.payload.payload_type = "action";
        record.visibility = EventVisibility::PlayerVisible;
        record.importance = EventImportance::Normal;

        auto result = stream.addEvent(std::move(record));
        assert(result.is_ok());
        assert(!stream.empty());
        assert(stream.size() == 1);
    }

    // Test adding invalid event fails
    {
        EventStream stream;
        EventRecord record;
        // Missing event_id
        record.event_type = EventType::Debug;
        record.payload.payload_type = "debug";

        auto result = stream.addEvent(std::move(record));
        assert(result.is_error());
        assert(stream.empty());
    }

    // Test InMemoryEventStore
    {
        InMemoryEventStore store;
        assert(store.size() == 0);

        EventRecord record;
        record.event_id = EventId("event_store_001");
        record.event_type = EventType::PipelineStarted;
        record.tick = Tick(500);
        record.state_version = StateVersion(5);
        record.payload.payload_type = "pipeline";
        record.visibility = EventVisibility::DebugOnly;
        record.importance = EventImportance::Low;

        auto result = store.append(std::move(record));
        assert(result.is_ok());
        assert(store.size() == 1);
    }

    // Test appending stream to store
    {
        InMemoryEventStore store;
        EventStream stream;

        EventRecord record1;
        record1.event_id = EventId("event_stream_001");
        record1.event_type = EventType::StateChanged;
        record1.tick = Tick(600);
        record1.state_version = StateVersion(6);
        record1.payload.payload_type = "state";
        record1.visibility = EventVisibility::PlayerVisible;
        record1.importance = EventImportance::Normal;
        stream.addEvent(std::move(record1));

        EventRecord record2;
        record2.event_id = EventId("event_stream_002");
        record2.event_type = EventType::PipelineFinished;
        record2.tick = Tick(601);
        record2.state_version = StateVersion(6);
        record2.payload.payload_type = "pipeline";
        record2.visibility = EventVisibility::DebugOnly;
        record2.importance = EventImportance::Low;
        stream.addEvent(std::move(record2));

        auto result = store.append(stream);
        assert(result.is_ok());
        assert(store.size() == 2);
    }

    // Test clear
    {
        InMemoryEventStore store;
        EventRecord record;
        record.event_id = EventId("event_clear_001");
        record.event_type = EventType::Debug;
        record.tick = Tick(700);
        record.state_version = StateVersion(7);
        record.payload.payload_type = "debug";
        record.visibility = EventVisibility::DebugOnly;
        record.importance = EventImportance::Low;
        store.append(std::move(record));

        assert(store.size() == 1);
        store.clear();
        assert(store.size() == 0);
    }

    std::cout << "event_stream tests passed" << std::endl;
}
