#include "pathfinder/event/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::event;
using namespace pathfinder::foundation;

void run_event_types_tests() {
    // Test EventType toString
    assert(toString(EventType::Unknown) == "unknown");
    assert(toString(EventType::CommandAccepted) == "command_accepted");
    assert(toString(EventType::CommandRejected) == "command_rejected");
    assert(toString(EventType::StateChanged) == "state_changed");
    assert(toString(EventType::PipelineStarted) == "pipeline_started");
    assert(toString(EventType::PipelineFinished) == "pipeline_finished");
    assert(toString(EventType::Debug) == "debug");
    // P3 additions
    assert(toString(EventType::ActionResolved) == "action_resolved");
    assert(toString(EventType::FeedbackGenerated) == "feedback_generated");
    assert(toString(EventType::CognitionEvidenceAdded) == "cognition_evidence_added");
    assert(toString(EventType::CognitionUpdated) == "cognition_updated");

    // Test EventType fromString
    {
        auto result = eventTypeFromString("command_accepted");
        assert(result.is_ok());
        assert(result.value() == EventType::CommandAccepted);
    }
    {
        auto result = eventTypeFromString("unknown");
        assert(result.is_error());
    }
    // P3 additions fromString
    {
        auto result = eventTypeFromString("action_resolved");
        assert(result.is_ok());
        assert(result.value() == EventType::ActionResolved);
    }
    {
        auto result = eventTypeFromString("feedback_generated");
        assert(result.is_ok());
        assert(result.value() == EventType::FeedbackGenerated);
    }
    {
        auto result = eventTypeFromString("cognition_evidence_added");
        assert(result.is_ok());
        assert(result.value() == EventType::CognitionEvidenceAdded);
    }
    {
        auto result = eventTypeFromString("cognition_updated");
        assert(result.is_ok());
        assert(result.value() == EventType::CognitionUpdated);
    }

    // Test EventVisibility toString
    assert(toString(EventVisibility::Hidden) == "hidden");
    assert(toString(EventVisibility::PlayerVisible) == "player_visible");
    assert(toString(EventVisibility::TribeVisible) == "tribe_visible");
    assert(toString(EventVisibility::DebugOnly) == "debug_only");
    assert(toString(EventVisibility::ReplayOnly) == "replay_only");

    // Test EventVisibility fromString
    {
        auto result = eventVisibilityFromString("player_visible");
        assert(result.is_ok());
        assert(result.value() == EventVisibility::PlayerVisible);
    }
    {
        auto result = eventVisibilityFromString("invalid");
        assert(result.is_error());
    }

    // Test EventImportance toString
    assert(toString(EventImportance::Low) == "low");
    assert(toString(EventImportance::Normal) == "normal");
    assert(toString(EventImportance::High) == "high");
    assert(toString(EventImportance::Critical) == "critical");

    // Test EventImportance fromString
    {
        auto result = eventImportanceFromString("high");
        assert(result.is_ok());
        assert(result.value() == EventImportance::High);
    }
    {
        auto result = eventImportanceFromString("invalid");
        assert(result.is_error());
    }

    std::cout << "event_types tests passed" << std::endl;
}
