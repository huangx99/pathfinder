#pragma once

#include "pathfinder/memory/memory_summary.h"

namespace pathfinder::memory {

// ============================================================
// MemoryRecallService
// ============================================================

class MemoryRecallService {
public:
    pathfinder::foundation::Result<MemoryRecallResult> recall(
        const std::vector<MemoryRecord>& records,
        const std::vector<MemorySummary>& summaries,
        const MemoryRecallQuery& query) const;
};

} // namespace pathfinder::memory
