#pragma once

#include "pathfinder/memory/memory_record.h"
#include <unordered_map>
#include <vector>

namespace pathfinder::memory {

class MemoryStore {
public:
    pathfinder::foundation::Result<void> put(MemoryRecord record);
    pathfinder::foundation::Result<std::optional<MemoryRecord>> find(
        const pathfinder::foundation::MemoryId& memory_id) const;
    pathfinder::foundation::Result<MemoryQueryResult> query(
        const MemoryQuery& query) const;
    pathfinder::foundation::Result<void> remove(
        const pathfinder::foundation::MemoryId& memory_id);
    size_t size() const;
    void clear();

private:
    std::unordered_map<std::string, MemoryRecord> records_;
};

} // namespace pathfinder::memory
