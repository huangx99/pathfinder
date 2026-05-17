#pragma once

#include "pathfinder/memory/memory_record.h"

namespace pathfinder::memory {

// ============================================================
// MemoryStateChangeBuilder
// ============================================================

class MemoryStateChangeBuilder {
public:
    pathfinder::foundation::Result<MemoryStateChangeDraft> buildCreate(
        const MemoryRecord& record) const;

    pathfinder::foundation::Result<MemoryStateChangeDraft> buildUpdate(
        const MemoryRecord& before,
        const MemoryRecord& after) const;
};

// ============================================================
// MemoryEventBuilder
// ============================================================

class MemoryEventBuilder {
public:
    pathfinder::foundation::Result<MemoryEventDraft> buildCreated(
        const MemoryRecord& record) const;

    pathfinder::foundation::Result<MemoryEventDraft> buildUpdated(
        const MemoryRecord& before,
        const MemoryRecord& after) const;

    pathfinder::foundation::Result<MemoryEventDraft> buildForgotten(
        const MemoryRecord& record) const;
};

} // namespace pathfinder::memory
