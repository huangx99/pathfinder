#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/event/types.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::event {

// Event payload (minimal)
struct EventPayload {
    std::string payload_type;
    std::string message_key;
    std::string debug_text;
};

// Event record
struct EventRecord {
    pathfinder::foundation::EventId event_id;
    EventType event_type;
    std::optional<pathfinder::foundation::CommandId> command_id;
    std::optional<std::string> correlation_id;
    pathfinder::foundation::Tick tick;
    pathfinder::foundation::StateVersion state_version;
    std::vector<pathfinder::foundation::StateChangeId> state_change_ids;
    std::optional<std::string> source_id;
    std::vector<pathfinder::foundation::TargetId> target_ids;
    EventPayload payload;
    EventVisibility visibility;
    EventImportance importance;

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::event
