#pragma once

#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::event {

// Event types
enum class EventType {
    Unknown,
    CommandAccepted,
    CommandRejected,
    StateChanged,
    PipelineStarted,
    PipelineFinished,
    Debug,
    // P3 additions
    ActionResolved,
    FeedbackGenerated,
    CognitionEvidenceAdded,
    CognitionUpdated
};

std::string toString(EventType type);
pathfinder::foundation::Result<EventType> eventTypeFromString(const std::string& str);

// Event visibility
enum class EventVisibility {
    Hidden,
    PlayerVisible,
    TribeVisible,
    DebugOnly,
    ReplayOnly
};

std::string toString(EventVisibility visibility);
pathfinder::foundation::Result<EventVisibility> eventVisibilityFromString(const std::string& str);

// Event importance
enum class EventImportance {
    Low,
    Normal,
    High,
    Critical
};

std::string toString(EventImportance importance);
pathfinder::foundation::Result<EventImportance> eventImportanceFromString(const std::string& str);

} // namespace pathfinder::event
