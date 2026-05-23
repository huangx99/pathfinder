#include "pathfinder/world_learning/world_knowledge_learning_service.h"
#include "pathfinder/learning/learning_loop.h"
#include <algorithm>

namespace pathfinder::world_learning {

WorldKnowledgeLearningService::WorldKnowledgeLearningService(
    const pathfinder::content::ContentRegistry& content_registry,
    const pathfinder::learning::LearningLoopService& learning_service,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    const pathfinder::learning::LearningLoopOptions& options)
    : content_registry_(content_registry),
      extractor_(),
      resolver_(content_registry),
      feedback_builder_(),
      loop_bridge_(learning_service, options),
      store_port_(knowledge_repository),
      projection_bridge_(&content_registry) {}

pathfinder::foundation::Result<WorldKnowledgeLearningResult>
WorldKnowledgeLearningService::learnFromCommandResult(
    const WorldKnowledgeLearningRequest& request) {
    WorldKnowledgeLearningResult aggregate_result;
    aggregate_result.ok = true;
    aggregate_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
    aggregate_result.failure_kind = WorldLearningFailureKind::None;

    // Validate request
    auto req_val = request.validateBasic();
    if (!req_val.is_ok()) {
        aggregate_result.ok = false;
        aggregate_result.decision = WorldKnowledgeLearningDecision::Failed;
        aggregate_result.failure_kind = WorldLearningFailureKind::InvalidRequest;
        aggregate_result.warning_keys.push_back("invalid_request");
        return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(aggregate_result);
    }

    // Check command result success
    if (!isCommandResultSuccessful(request.command_result)) {
        // P49 MVP: skip failed commands (no positive learning from failure)
        aggregate_result.ok = true;
        aggregate_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
        aggregate_result.failure_kind = WorldLearningFailureKind::None;
        return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(aggregate_result);
    }

    // Extract experiences
    auto extract_result = extractor_.extract(request.command_result);
    if (extract_result.failure_kind == WorldLearningFailureKind::UnsafeExperience) {
        aggregate_result.ok = false;
        aggregate_result.decision = WorldKnowledgeLearningDecision::SkippedUnsafe;
        aggregate_result.failure_kind = WorldLearningFailureKind::UnsafeExperience;
        aggregate_result.warning_keys = extract_result.warning_keys;
        return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(aggregate_result);
    }
    if (extract_result.experiences.empty()) {
        aggregate_result.ok = true;
        aggregate_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
        aggregate_result.failure_kind = WorldLearningFailureKind::NoExperience;
        return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(aggregate_result);
    }

    bool any_learned = false;
    bool any_failure = false;
    bool any_skipped = false;

    auto working_request = request;
    std::vector<pathfinder::knowledge::KnowledgeClaim> working_claims = request.existing_actor_claims;

    for (const auto& exp : extract_result.experiences) {
        working_request.existing_actor_claims = working_claims;
        auto proc_res = processExperience(exp, working_request, aggregate_result);
        if (!proc_res.is_ok()) {
            any_failure = true;
            aggregate_result.warning_keys.push_back("process_experience_failed:" + exp.experience_id);
            continue;
        }
        auto& proc_val = proc_res.value();
        if (proc_val.decision == WorldKnowledgeLearningDecision::Learned ||
            proc_val.decision == WorldKnowledgeLearningDecision::Reinforced ||
            proc_val.decision == WorldKnowledgeLearningDecision::Revised) {
            any_learned = true;
        } else if (proc_val.decision == WorldKnowledgeLearningDecision::SkippedNoTemplate ||
                   proc_val.decision == WorldKnowledgeLearningDecision::SkippedUnsafe) {
            any_skipped = true;
        }
        if (!proc_val.ok) {
            any_failure = true;
        }
        // Merge results
        aggregate_result.drafts.insert(aggregate_result.drafts.end(),
            proc_val.drafts.begin(), proc_val.drafts.end());
        aggregate_result.learning_results.insert(aggregate_result.learning_results.end(),
            proc_val.learning_results.begin(), proc_val.learning_results.end());
        aggregate_result.learned_claims.insert(aggregate_result.learned_claims.end(),
            proc_val.learned_claims.begin(), proc_val.learned_claims.end());
        aggregate_result.knowledge_deltas.insert(aggregate_result.knowledge_deltas.end(),
            proc_val.knowledge_deltas.begin(), proc_val.knowledge_deltas.end());
        aggregate_result.events.insert(aggregate_result.events.end(),
            proc_val.events.begin(), proc_val.events.end());
        aggregate_result.state_deltas.insert(aggregate_result.state_deltas.end(),
            proc_val.state_deltas.begin(), proc_val.state_deltas.end());
        aggregate_result.warning_keys.insert(aggregate_result.warning_keys.end(),
            proc_val.warning_keys.begin(), proc_val.warning_keys.end());
        working_claims.insert(working_claims.end(),
            proc_val.learned_claims.begin(), proc_val.learned_claims.end());
    }

    if (!aggregate_result.learned_claims.empty() || !aggregate_result.knowledge_deltas.empty()) {
        auto projection = projection_bridge_.project(
            aggregate_result.learned_claims,
            aggregate_result.knowledge_deltas,
            request.actor_key,
            request.tick);
        aggregate_result.events.insert(aggregate_result.events.end(), projection.events.begin(), projection.events.end());
        aggregate_result.state_deltas.insert(aggregate_result.state_deltas.end(),
            projection.state_deltas.begin(), projection.state_deltas.end());
        aggregate_result.projection_patch = projection.patch;
    }

    if (any_learned && any_failure) {
        aggregate_result.ok = true; // partial ok per design
        aggregate_result.decision = WorldKnowledgeLearningDecision::Learned;
        aggregate_result.failure_kind = WorldLearningFailureKind::PartialFailed;
    } else if (any_learned) {
        aggregate_result.ok = true;
        aggregate_result.decision = WorldKnowledgeLearningDecision::Learned;
        aggregate_result.failure_kind = WorldLearningFailureKind::None;
    } else if (any_failure) {
        aggregate_result.ok = false;
        aggregate_result.decision = WorldKnowledgeLearningDecision::Failed;
        if (aggregate_result.failure_kind == WorldLearningFailureKind::None) {
            aggregate_result.failure_kind = WorldLearningFailureKind::LearningLoopFailed;
        }
    } else if (any_skipped) {
        aggregate_result.ok = true;
        aggregate_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
        aggregate_result.failure_kind = WorldLearningFailureKind::None;
    } else {
        aggregate_result.ok = true;
        aggregate_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
    }

    return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(aggregate_result);
}

pathfinder::foundation::Result<WorldKnowledgeLearningResult>
WorldKnowledgeLearningService::processExperience(
    const world_command::WorldExperienceDto& experience,
    const WorldKnowledgeLearningRequest& request,
    WorldKnowledgeLearningResult& /*aggregate_result*/) {
    WorldKnowledgeLearningResult single_result;
    single_result.ok = true;
    single_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;

    // 1. Resolve templates
    auto resolve_res = resolver_.resolve(experience);
    if (resolve_res.failure_kind == WorldLearningFailureKind::TemplateMissing) {
        single_result.ok = true;
        single_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
        single_result.failure_kind = WorldLearningFailureKind::TemplateMissing;
        single_result.warning_keys = resolve_res.warning_keys;
        return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(single_result);
    }
    if (resolve_res.templates.empty()) {
        single_result.ok = true;
        single_result.decision = WorldKnowledgeLearningDecision::SkippedNoTemplate;
        single_result.failure_kind = WorldLearningFailureKind::TemplateMissing;
        return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(single_result);
    }

    // 2. For each template, build feedback and run learning loop
    for (const auto& tmpl : resolve_res.templates) {
        WorldExperienceLearningDraft draft;
        draft.draft_id = experience.experience_id + ":" + tmpl.key.value();
        draft.experience = experience;
        draft.experience_kind = classifyExperienceKind(experience.command_key);
        draft.candidate_template_keys = resolve_res.matched_keys;
        draft.templates.push_back(tmpl);

        const auto* effect = content_registry_.findEffect(
            !tmpl.effect_key.empty() ? tmpl.effect_key : experience.effect_key);
        auto fb_res = feedback_builder_.build(
            experience, tmpl, effect, request.command_result.events, request.tick);
        if (fb_res.failure_kind != WorldLearningFailureKind::None) {
            single_result.warning_keys.insert(single_result.warning_keys.end(),
                fb_res.warning_keys.begin(), fb_res.warning_keys.end());
            continue;
        }

        auto bridge_memories = request.existing_memories;
        auto bridge_summaries = request.existing_summaries;
        const auto& memory_subject = fb_res.feedback.memory_subject;
        bool has_subject_summary = std::any_of(bridge_summaries.begin(), bridge_summaries.end(),
            [&](const auto& summary) { return summary.key.subject.subject_id == memory_subject.subject_id; });
        if (!has_subject_summary) {
            pathfinder::memory::MemoryEvidenceRef evidence;
            evidence.source_kind = pathfinder::memory::MemorySourceKind::DirectEvent;
            evidence.source_event_id = fb_res.feedback.source_event_ids.empty()
                ? pathfinder::foundation::EventId(experience.experience_id + ":event")
                : fb_res.feedback.source_event_ids.front();
            evidence.observed_tick = fb_res.feedback.observed_tick;
            evidence.reason_keys = fb_res.feedback.reason_keys;

            pathfinder::memory::MemoryRecord record;
            record.memory_id = pathfinder::foundation::MemoryId("mem_world_learning_" + request.actor_key + "_" + memory_subject.subject_id + "_" + experience.experience_id);
            record.owner.kind = pathfinder::memory::MemoryOwnerKind::Actor;
            record.owner.entity_id = pathfinder::foundation::EntityId(request.actor_key);
            record.subject = memory_subject;
            record.memory_kinds = {pathfinder::memory::MemoryKind::Experiment, pathfinder::memory::MemoryKind::Success};
            record.scope = pathfinder::memory::MemoryScope::MidTerm;
            record.lifecycle = pathfinder::memory::MemoryLifecycle::Active;
            record.importance = pathfinder::memory::MemoryImportance::Important;
            record.strength = 0.70;
            record.strength_band = pathfinder::memory::strengthToBand(record.strength);
            record.teaching_candidate = true;
            record.created_tick = fb_res.feedback.observed_tick;
            record.last_touched_tick = fb_res.feedback.observed_tick;
            record.evidence_refs.push_back(evidence);
            record.reason_keys = fb_res.feedback.reason_keys;
            bridge_memories.push_back(record);

            pathfinder::memory::MemorySummary summary;
            summary.summary_id = pathfinder::memory::MemorySummaryId("sum_world_learning_" + request.actor_key + "_" + memory_subject.subject_id + "_" + experience.experience_id);
            summary.key.owner = record.owner;
            summary.key.subject = memory_subject;
            summary.key.summary_kind = pathfinder::memory::MemorySummaryKind::OwnerSubjectPattern;
            summary.key.memory_kinds = {pathfinder::memory::MemoryKind::Experiment, pathfinder::memory::MemoryKind::Success};
            summary.compression_level = pathfinder::memory::MemoryCompressionLevel::LightSummary;
            summary.highest_importance = pathfinder::memory::MemoryImportance::Important;
            summary.aggregate_strength = 0.70;
            summary.aggregate_strength_band = pathfinder::memory::strengthToBand(summary.aggregate_strength);
            summary.source_memory_count = 1;
            summary.success_count = 1;
            summary.teaching_candidate_count = 1;
            summary.first_observed_tick = fb_res.feedback.observed_tick;
            summary.last_observed_tick = fb_res.feedback.observed_tick;
            summary.summarized_tick = pathfinder::foundation::Tick(request.tick);
            summary.source_memory_ids.push_back(record.memory_id);
            summary.representative_memory_ids.push_back(record.memory_id);
            summary.evidence_refs.push_back(evidence);
            summary.reason_keys = fb_res.feedback.reason_keys;
            bridge_summaries.push_back(summary);
        }

        auto loop_res = loop_bridge_.run(
            fb_res.feedback,
            request.actor_key,
            request.existing_actor_claims,
            bridge_memories,
            bridge_summaries,
            request.tick,
            draft.draft_id);

        if (loop_res.failure_kind != WorldLearningFailureKind::None) {
            single_result.warning_keys.insert(single_result.warning_keys.end(),
                loop_res.warning_keys.begin(), loop_res.warning_keys.end());
            continue;
        }

        draft.learning_inputs.push_back(pathfinder::learning::LearningLoopInput{});
        draft.learning_inputs.back().loop_key = draft.draft_id;
        draft.learning_inputs.back().feedback = fb_res.feedback;
        single_result.drafts.push_back(draft);
        single_result.learning_results.push_back(loop_res.loop_result);

        std::vector<pathfinder::knowledge::KnowledgeClaim> changed_claims;
        auto find_claim = [](const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims,
                             const pathfinder::foundation::KnowledgeId& id) -> const pathfinder::knowledge::KnowledgeClaim* {
            auto it = std::find_if(claims.begin(), claims.end(),
                [&](const auto& claim) { return claim.knowledge_id.value() == id.value(); });
            return it == claims.end() ? nullptr : &(*it);
        };
        auto contains_changed_claim = [&](const pathfinder::foundation::KnowledgeId& id) {
            return find_claim(changed_claims, id) != nullptr;
        };
        auto materially_changed = [&](const pathfinder::knowledge::KnowledgeClaim& claim) {
            const auto* existing = find_claim(request.existing_actor_claims, claim.knowledge_id);
            if (!existing) return true;
            return existing->status != claim.status ||
                   existing->confidence.support_count != claim.confidence.support_count ||
                   existing->confidence.source_summary_count != claim.confidence.source_summary_count ||
                   existing->projection_flags.usable_for_action != claim.projection_flags.usable_for_action ||
                   existing->projection_flags.usable_for_teaching != claim.projection_flags.usable_for_teaching ||
                   existing->teaching_profile.teachable != claim.teaching_profile.teachable ||
                   existing->reason_keys != claim.reason_keys;
        };

        pathfinder::learning::LearningLoopApplier applier;
        auto apply_res = applier.applyActorClaims(
            request.existing_actor_claims, loop_res.loop_result);
        if (apply_res.is_ok()) {
            for (const auto& claim : apply_res.value()) {
                if (materially_changed(claim) && !contains_changed_claim(claim.knowledge_id)) {
                    changed_claims.push_back(claim);
                }
            }
        }

        if (loop_res.loop_result.knowledge_formation_result.has_value()) {
            const auto& kf = loop_res.loop_result.knowledge_formation_result.value();
            if (kf.claim.has_value() &&
                materially_changed(kf.claim.value()) &&
                !contains_changed_claim(kf.claim->knowledge_id)) {
                changed_claims.push_back(kf.claim.value());
            }
        }

        for (const auto& claim : changed_claims) {
            auto store_res = store_port_.putClaim(claim);
            if (!store_res.is_ok()) {
                single_result.ok = false;
                single_result.decision = WorldKnowledgeLearningDecision::Failed;
                single_result.failure_kind = WorldLearningFailureKind::StoreFailed;
                single_result.warning_keys.push_back("store_failed:" + claim.knowledge_id.value());
                continue;
            }
            single_result.learned_claims.push_back(claim);
            single_result.knowledge_deltas.push_back(
                makeDelta(claim, toString(WorldKnowledgeLearningDecision::Learned)));
        }

        if (!changed_claims.empty() && single_result.failure_kind != WorldLearningFailureKind::StoreFailed) {
            single_result.ok = true;
            single_result.decision = WorldKnowledgeLearningDecision::Learned;
        }
    }

    return pathfinder::foundation::Result<WorldKnowledgeLearningResult>::ok(single_result);
}

WorldKnowledgeDelta WorldKnowledgeLearningService::makeDelta(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& decision_label) const {
    WorldKnowledgeDelta delta;
    delta.delta_id = claim.knowledge_id.value() + ":delta";
    delta.knowledge_id = claim.knowledge_id;
    delta.owner_key = claim.owner.entity_id.value();
    delta.subject_object_key = claim.subject.subject_id;
    delta.action_key = claim.predicate.action_key;
    delta.effect_key = claim.predicate.effect_key;
    delta.target_object_key = claim.subject.related_subject_ids.empty()
        ? "" : claim.subject.related_subject_ids.front();
    delta.status = pathfinder::knowledge::toString(claim.status);
    delta.decision = decision_label;
    delta.reason_keys = claim.reason_keys;
    return delta;
}

bool WorldKnowledgeLearningService::isCommandResultSuccessful(
    const world_command::WorldCommandExecutionResult& command_result) const {
    return command_result.result_kind == world_command::WorldCommandResultKind::Succeeded;
}

} // namespace pathfinder::world_learning
