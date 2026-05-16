#pragma once

#include "pathfinder/event/event_record.h"
#include <vector>

namespace pathfinder::event {

// Event stream - ordered collection of events
class EventStream {
public:
    // Add an event to the stream
    pathfinder::foundation::Result<void> addEvent(EventRecord event);

    // Check if empty
    bool empty() const { return events_.empty(); }

    // Get size
    size_t size() const { return events_.size(); }

    // Get all events
    const std::vector<EventRecord>& events() const { return events_; }

    // Validate all events
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<EventRecord> events_;
};

// In-memory event store
class InMemoryEventStore {
public:
    // Append a single event
    pathfinder::foundation::Result<void> append(EventRecord event);

    // Append a stream of events
    pathfinder::foundation::Result<void> append(const EventStream& stream);

    // Get total size
    size_t size() const { return events_.size(); }

    // Get all events
    const std::vector<EventRecord>& allEvents() const { return events_; }

    // Clear all events
    void clear() { events_.clear(); }

private:
    std::vector<EventRecord> events_;
};

} // namespace pathfinder::event
