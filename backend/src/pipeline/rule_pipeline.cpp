#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/rules/eat_object_resolver.h"
#include "pathfinder/state/minimal_state_applier.h"
#include "pathfinder/cognition/cognition_state.h"

namespace pathfinder::pipeline {

pathfinder::foundation::Result<PipelineResult> RulePipeline::execute(const PipelineContext& context) {
    // Validate context
    auto ctx_result = context.validateBasic();
    if (ctx_result.is_error()) {
        return pathfinder::foundation::Result<PipelineResult>::fail(
            ctx_result.errors()[0]
        );
    }

    // P3: Check if this is an eat command and we have game state
    if (context.game_state && pathfinder::rules::EatObjectResolver::canResolve(context.command)) {
        // Resolve the eat command
        pathfinder::rules::EatObjectResolveInput input{context.command, *context.game_state};
        auto resolve_result = pathfinder::rules::EatObjectResolver::resolve(input);
        if (resolve_result.is_error()) {
            PipelineResult result;
            result.status = PipelineStatus::Failed;
            result.errors.push_back(resolve_result.errors()[0]);
            return pathfinder::foundation::Result<PipelineResult>::ok(std::move(result));
        }

        auto& draft = resolve_result.value();

        // Apply state changes through MinimalStateApplier
        auto apply_result = pathfinder::state::MinimalStateApplier::apply(*context.game_state, draft.state_changes);
        if (apply_result.is_error()) {
            PipelineResult result;
            result.status = PipelineStatus::Failed;
            result.errors.push_back(apply_result.errors()[0]);
            return pathfinder::foundation::Result<PipelineResult>::ok(std::move(result));
        }

        // Apply cognition evidence
        for (const auto& evidence : draft.cognition_evidence) {
            auto cognition_result = pathfinder::cognition::CognitionUpdater::applyEvidence(context.game_state->cognition_state, evidence);
            if (cognition_result.is_error()) {
                PipelineResult result;
                result.status = PipelineStatus::Failed;
                result.errors.push_back(cognition_result.errors()[0]);
                return pathfinder::foundation::Result<PipelineResult>::ok(std::move(result));
            }
        }

        // Build result
        PipelineResult result;
        result.status = PipelineStatus::Succeeded;
        result.state_changes = std::move(draft.state_changes);

        // Add events from resolver
        for (auto& event : draft.events) {
            result.events.addEvent(std::move(event));
        }

        // Add StateChanged event
        pathfinder::event::EventRecord state_changed_event;
        state_changed_event.event_id = pathfinder::foundation::EventId("event_state_changed_" + context.command.command_id.value());
        state_changed_event.event_type = pathfinder::event::EventType::StateChanged;
        state_changed_event.command_id = context.command.command_id;
        state_changed_event.tick = context.command.issued_tick;
        state_changed_event.state_version = context.game_state->metadata.state_version;
        state_changed_event.source_id = context.command.payload.actor_id.value();
        state_changed_event.payload.payload_type = "state";
        state_changed_event.payload.message_key = "state_changed";
        state_changed_event.payload.debug_text = "State changes applied";
        state_changed_event.visibility = pathfinder::event::EventVisibility::PlayerVisible;
        state_changed_event.importance = pathfinder::event::EventImportance::Normal;
        result.events.addEvent(std::move(state_changed_event));

        // Add CognitionUpdated event
        pathfinder::event::EventRecord cognition_event;
        cognition_event.event_id = pathfinder::foundation::EventId("event_cognition_" + context.command.command_id.value());
        cognition_event.event_type = pathfinder::event::EventType::CognitionUpdated;
        cognition_event.command_id = context.command.command_id;
        cognition_event.tick = context.command.issued_tick;
        cognition_event.state_version = context.game_state->metadata.state_version;
        cognition_event.source_id = context.command.payload.actor_id.value();
        cognition_event.payload.payload_type = "cognition";
        cognition_event.payload.message_key = "cognition_updated";
        cognition_event.payload.debug_text = "Cognition evidence applied";
        cognition_event.visibility = pathfinder::event::EventVisibility::PlayerVisible;
        cognition_event.importance = pathfinder::event::EventImportance::Normal;
        result.events.addEvent(std::move(cognition_event));

        return pathfinder::foundation::Result<PipelineResult>::ok(std::move(result));
    }

    // P2: Return empty success result for unsupported commands
    PipelineResult result = PipelineResult::successEmpty();
    return pathfinder::foundation::Result<PipelineResult>::ok(std::move(result));
}

} // namespace pathfinder::pipeline
