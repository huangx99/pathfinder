#pragma once

#include "pathfinder/memory/memory_record.h"

namespace pathfinder::memory {

class MemoryDecayService {
public:
    pathfinder::foundation::Result<MemoryDecayResult> applyDecay(
        const std::vector<MemoryRecord>& records,
        pathfinder::foundation::Tick current_tick,
        const MemoryDecayOptions& options = {}) const;
};

} // namespace pathfinder::memory
