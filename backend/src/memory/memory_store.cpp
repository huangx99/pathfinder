#include "pathfinder/memory/memory_store.h"

namespace pathfinder::memory {

using pathfinder::foundation::Result;
using pathfinder::foundation::MemoryId;

Result<void> MemoryStore::put(MemoryRecord record) {
    auto validation = record.validateBasic();
    if (validation.is_error()) {
        return validation;
    }
    records_[record.memory_id.value()] = std::move(record);
    return Result<void>::ok();
}

Result<std::optional<MemoryRecord>> MemoryStore::find(const MemoryId& memory_id) const {
    auto it = records_.find(memory_id.value());
    if (it != records_.end()) {
        return Result<std::optional<MemoryRecord>>::ok(it->second);
    }
    return Result<std::optional<MemoryRecord>>::ok(std::nullopt);
}

Result<MemoryQueryResult> MemoryStore::query(const MemoryQuery& query) const {
    auto validation = query.validateBasic();
    if (validation.is_error()) {
        return Result<MemoryQueryResult>::fail(validation.errors());
    }

    MemoryQueryResult result;
    result.query = query;

    size_t count = 0;
    for (const auto& [id, record] : records_) {
        if (count >= query.limit) break;

        // Owner filter
        if (!(record.owner == query.owner)) continue;

        // Subject filter
        if (query.subject.has_value() && !(record.subject == query.subject.value())) continue;

        // Scope filter
        if (query.scope.has_value() && record.scope != query.scope.value()) continue;

        // Lifecycle filter
        if (query.lifecycle.has_value() && record.lifecycle != query.lifecycle.value()) continue;

        // Forgotten filter
        if (!query.include_forgotten && record.lifecycle == MemoryLifecycle::Forgotten) continue;

        result.records.push_back(record);
        ++count;
    }

    return Result<MemoryQueryResult>::ok(std::move(result));
}

Result<void> MemoryStore::remove(const MemoryId& memory_id) {
    records_.erase(memory_id.value());
    return Result<void>::ok();
}

size_t MemoryStore::size() const {
    return records_.size();
}

void MemoryStore::clear() {
    records_.clear();
}

} // namespace pathfinder::memory
