#pragma once

#include "pathfinder/memory/memory_summary.h"

namespace pathfinder::memory {

// ============================================================
// MemoryCompressionPlanner
// ============================================================

class MemoryCompressionPlanner {
public:
    pathfinder::foundation::Result<MemoryCompressionPlan> planForOwnerSubject(
        const std::vector<MemoryRecord>& records,
        const MemorySummaryKey& key,
        const MemoryCompressionOptions& options = {}) const;
};

// ============================================================
// MemoryCompressionService
// ============================================================

class MemoryCompressionService {
public:
    pathfinder::foundation::Result<MemoryCompressionResult> compress(
        const std::vector<MemoryRecord>& records,
        const MemorySummaryKey& key,
        const MemoryCompressionOptions& options = {}) const;
};

} // namespace pathfinder::memory
