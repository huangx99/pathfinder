#include "pathfinder/memory/memory_events.h"

namespace pathfinder::memory {

using pathfinder::foundation::Result;

// ============================================================
// MemoryStateChangeBuilder
// ============================================================

Result<MemoryStateChangeDraft> MemoryStateChangeBuilder::buildCreate(const MemoryRecord& record) const {
    MemoryStateChangeDraft draft;
    draft.change_key = "memory.create";
    draft.memory_id = record.memory_id;
    draft.after_record = record;
    draft.reason_keys.push_back("memory.created_from_candidate");
    return Result<MemoryStateChangeDraft>::ok(std::move(draft));
}

Result<MemoryStateChangeDraft> MemoryStateChangeBuilder::buildUpdate(
    const MemoryRecord& before,
    const MemoryRecord& after) const {
    MemoryStateChangeDraft draft;
    draft.change_key = "memory.update";
    draft.memory_id = after.memory_id;
    draft.before_record = before;
    draft.after_record = after;
    draft.reason_keys.push_back("memory.updated_from_service");
    return Result<MemoryStateChangeDraft>::ok(std::move(draft));
}

// ============================================================
// MemoryEventBuilder
// ============================================================

Result<MemoryEventDraft> MemoryEventBuilder::buildCreated(const MemoryRecord& record) const {
    MemoryEventDraft draft;
    draft.event_key = "memory.created";
    draft.memory_id = record.memory_id;
    draft.owner = record.owner;
    draft.subject = record.subject;
    draft.scope = record.scope;
    draft.lifecycle = record.lifecycle;
    draft.write_decision = MemoryWriteDecision::Created;
    return Result<MemoryEventDraft>::ok(std::move(draft));
}

Result<MemoryEventDraft> MemoryEventBuilder::buildUpdated(
    const MemoryRecord& before,
    const MemoryRecord& after) const {
    MemoryEventDraft draft;
    draft.event_key = "memory.updated";
    draft.memory_id = after.memory_id;
    draft.owner = after.owner;
    draft.subject = after.subject;
    draft.scope = after.scope;
    draft.lifecycle = after.lifecycle;
    if (before.scope != after.scope) {
        draft.write_decision = MemoryWriteDecision::Promoted;
    } else {
        draft.write_decision = MemoryWriteDecision::Reinforced;
    }
    return Result<MemoryEventDraft>::ok(std::move(draft));
}

Result<MemoryEventDraft> MemoryEventBuilder::buildForgotten(const MemoryRecord& record) const {
    MemoryEventDraft draft;
    draft.event_key = "memory.forgotten";
    draft.memory_id = record.memory_id;
    draft.owner = record.owner;
    draft.subject = record.subject;
    draft.scope = record.scope;
    draft.lifecycle = MemoryLifecycle::Forgotten;
    draft.decay_decision = MemoryDecayDecision::Forgotten;
    return Result<MemoryEventDraft>::ok(std::move(draft));
}

} // namespace pathfinder::memory
