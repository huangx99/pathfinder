#include "pathfinder/event/event_stream.h"

namespace pathfinder::event {

// EventStream
pathfinder::foundation::Result<void> EventStream::addEvent(EventRecord event) {
    auto result = event.validateBasic();
    if (result.is_error()) {
        return result;
    }
    events_.push_back(std::move(event));
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> EventStream::validateBasic() const {
    for (const auto& event : events_) {
        auto result = event.validateBasic();
        if (result.is_error()) {
            return result;
        }
    }
    return pathfinder::foundation::Result<void>::ok();
}

// InMemoryEventStore
pathfinder::foundation::Result<void> InMemoryEventStore::append(EventRecord event) {
    auto result = event.validateBasic();
    if (result.is_error()) {
        return result;
    }
    events_.push_back(std::move(event));
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> InMemoryEventStore::append(const EventStream& stream) {
    auto result = stream.validateBasic();
    if (result.is_error()) {
        return result;
    }
    for (const auto& event : stream.events()) {
        events_.push_back(event);
    }
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::event
