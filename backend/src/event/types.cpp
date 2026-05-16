#include "pathfinder/event/types.h"

namespace pathfinder::event {

// EventType
std::string toString(EventType type) {
    switch (type) {
        case EventType::Unknown: return "unknown";
        case EventType::CommandAccepted: return "command_accepted";
        case EventType::CommandRejected: return "command_rejected";
        case EventType::StateChanged: return "state_changed";
        case EventType::PipelineStarted: return "pipeline_started";
        case EventType::PipelineFinished: return "pipeline_finished";
        case EventType::Debug: return "debug";
        // P3 additions
        case EventType::ActionResolved: return "action_resolved";
        case EventType::FeedbackGenerated: return "feedback_generated";
        case EventType::CognitionEvidenceAdded: return "cognition_evidence_added";
        case EventType::CognitionUpdated: return "cognition_updated";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<EventType> eventTypeFromString(const std::string& str) {
    if (str == "command_accepted") return pathfinder::foundation::Result<EventType>::ok(EventType::CommandAccepted);
    if (str == "command_rejected") return pathfinder::foundation::Result<EventType>::ok(EventType::CommandRejected);
    if (str == "state_changed") return pathfinder::foundation::Result<EventType>::ok(EventType::StateChanged);
    if (str == "pipeline_started") return pathfinder::foundation::Result<EventType>::ok(EventType::PipelineStarted);
    if (str == "pipeline_finished") return pathfinder::foundation::Result<EventType>::ok(EventType::PipelineFinished);
    if (str == "debug") return pathfinder::foundation::Result<EventType>::ok(EventType::Debug);
    // P3 additions
    if (str == "action_resolved") return pathfinder::foundation::Result<EventType>::ok(EventType::ActionResolved);
    if (str == "feedback_generated") return pathfinder::foundation::Result<EventType>::ok(EventType::FeedbackGenerated);
    if (str == "cognition_evidence_added") return pathfinder::foundation::Result<EventType>::ok(EventType::CognitionEvidenceAdded);
    if (str == "cognition_updated") return pathfinder::foundation::Result<EventType>::ok(EventType::CognitionUpdated);
    // Unknown is not valid input
    return pathfinder::foundation::Result<EventType>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown EventType: " + str
        )
    );
}

// EventVisibility
std::string toString(EventVisibility visibility) {
    switch (visibility) {
        case EventVisibility::Hidden: return "hidden";
        case EventVisibility::PlayerVisible: return "player_visible";
        case EventVisibility::TribeVisible: return "tribe_visible";
        case EventVisibility::DebugOnly: return "debug_only";
        case EventVisibility::ReplayOnly: return "replay_only";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<EventVisibility> eventVisibilityFromString(const std::string& str) {
    if (str == "hidden") return pathfinder::foundation::Result<EventVisibility>::ok(EventVisibility::Hidden);
    if (str == "player_visible") return pathfinder::foundation::Result<EventVisibility>::ok(EventVisibility::PlayerVisible);
    if (str == "tribe_visible") return pathfinder::foundation::Result<EventVisibility>::ok(EventVisibility::TribeVisible);
    if (str == "debug_only") return pathfinder::foundation::Result<EventVisibility>::ok(EventVisibility::DebugOnly);
    if (str == "replay_only") return pathfinder::foundation::Result<EventVisibility>::ok(EventVisibility::ReplayOnly);
    return pathfinder::foundation::Result<EventVisibility>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown EventVisibility: " + str
        )
    );
}

// EventImportance
std::string toString(EventImportance importance) {
    switch (importance) {
        case EventImportance::Low: return "low";
        case EventImportance::Normal: return "normal";
        case EventImportance::High: return "high";
        case EventImportance::Critical: return "critical";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<EventImportance> eventImportanceFromString(const std::string& str) {
    if (str == "low") return pathfinder::foundation::Result<EventImportance>::ok(EventImportance::Low);
    if (str == "normal") return pathfinder::foundation::Result<EventImportance>::ok(EventImportance::Normal);
    if (str == "high") return pathfinder::foundation::Result<EventImportance>::ok(EventImportance::High);
    if (str == "critical") return pathfinder::foundation::Result<EventImportance>::ok(EventImportance::Critical);
    return pathfinder::foundation::Result<EventImportance>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown EventImportance: " + str
        )
    );
}

} // namespace pathfinder::event
