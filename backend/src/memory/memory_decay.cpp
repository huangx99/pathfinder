#include "pathfinder/memory/memory_decay.h"
#include <algorithm>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;

Result<MemoryDecayResult> MemoryDecayService::applyDecay(
    const std::vector<MemoryRecord>& records,
    Tick current_tick,
    const MemoryDecayOptions& options) const {

    MemoryDecayResult result;

    auto options_validation = options.validateBasic();
    if (options_validation.is_error()) {
        result.decision = MemoryDecayDecision::Unknown;
        return Result<MemoryDecayResult>::fail(options_validation.errors());
    }

    if (records.size() > options.max_records) {
        result.decision = MemoryDecayDecision::Unknown;
        MemoryWriteIssue issue;
        issue.reason = MemoryRejectReason::TooManyRecords;
        issue.issue_key = "too_many_records_for_decay";
        result.issues.push_back(issue);
        return Result<MemoryDecayResult>::ok(std::move(result));
    }

    for (const auto& record : records) {
        // Skip Protected
        if (record.protect_from_decay || record.lifecycle == MemoryLifecycle::Protected) {
            MemoryEventDraft event;
            event.event_key = "memory.protected_skipped";
            event.memory_id = record.memory_id;
            event.owner = record.owner;
            event.subject = record.subject;
            event.scope = record.scope;
            event.lifecycle = record.lifecycle;
            event.decay_decision = MemoryDecayDecision::ProtectedSkipped;
            result.event_drafts.push_back(event);
            continue;
        }

        // Skip LongTerm unless decay_long_term is true
        if (record.scope == MemoryScope::LongTerm && !options.decay_long_term) {
            continue;
        }

        // Skip Forgotten
        if (record.lifecycle == MemoryLifecycle::Forgotten) {
            continue;
        }

        double decay_rate = 0.0;
        if (record.scope == MemoryScope::ShortTerm) {
            decay_rate = options.short_term_decay_per_tick;
        } else if (record.scope == MemoryScope::MidTerm) {
            decay_rate = options.mid_term_decay_per_tick;
        } else if (record.scope == MemoryScope::LongTerm) {
            decay_rate = options.long_term_decay_per_tick;
        }

        uint64_t elapsed = current_tick.value() >= record.last_touched_tick.value()
            ? current_tick.value() - record.last_touched_tick.value()
            : 0;
        double new_strength = record.strength - decay_rate * static_cast<double>(elapsed);
        if (new_strength < 0.0) new_strength = 0.0;

        MemoryRecord updated = record;
        updated.strength = new_strength;
        updated.strength_band = strengthToBand(new_strength);
        updated.last_touched_tick = current_tick;

        MemoryDecayDecision decision = MemoryDecayDecision::Decayed;

        if (new_strength <= options.forgotten_threshold) {
            updated.lifecycle = MemoryLifecycle::Forgotten;
            decision = MemoryDecayDecision::Forgotten;
            result.forgotten_memory_ids.push_back(updated.memory_id);

            MemoryEventDraft event;
            event.event_key = "memory.forgotten";
            event.memory_id = updated.memory_id;
            event.owner = updated.owner;
            event.subject = updated.subject;
            event.scope = updated.scope;
            event.lifecycle = updated.lifecycle;
            event.decay_decision = decision;
            result.event_drafts.push_back(event);
        } else if (new_strength <= options.fading_threshold) {
            updated.lifecycle = MemoryLifecycle::Fading;
            decision = MemoryDecayDecision::Faded;

            MemoryEventDraft event;
            event.event_key = "memory.faded";
            event.memory_id = updated.memory_id;
            event.owner = updated.owner;
            event.subject = updated.subject;
            event.scope = updated.scope;
            event.lifecycle = updated.lifecycle;
            event.decay_decision = decision;
            result.event_drafts.push_back(event);
        }

        result.updated_records.push_back(updated);

        // State change draft
        MemoryStateChangeDraft change;
        change.change_key = "memory.decay";
        change.memory_id = updated.memory_id;
        change.before_record = record;
        change.after_record = updated;
        result.state_changes.push_back(change);
    }

    bool has_faded = false;
    for (const auto& draft : result.event_drafts) {
        if (draft.decay_decision == MemoryDecayDecision::Faded) {
            has_faded = true;
            break;
        }
    }

    if (result.updated_records.empty() && result.event_drafts.empty()) {
        result.decision = MemoryDecayDecision::Unchanged;
    } else if (!result.forgotten_memory_ids.empty()) {
        result.decision = MemoryDecayDecision::Forgotten;
    } else if (has_faded) {
        result.decision = MemoryDecayDecision::Faded;
    } else {
        result.decision = MemoryDecayDecision::Decayed;
    }

    return Result<MemoryDecayResult>::ok(std::move(result));
}

} // namespace pathfinder::memory
