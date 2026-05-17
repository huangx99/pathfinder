#include "pathfinder/agent/history_query.h"
#include <algorithm>
#include <sstream>

namespace pathfinder::agent {

// --- ID Helper ---

AgentHistoryQueryId makeAgentHistoryQueryId(
    const std::optional<AgentId>& agent_id,
    const std::optional<pathfinder::foundation::Tick>& tick_from,
    const std::optional<pathfinder::foundation::Tick>& tick_to,
    size_t limit) {
    std::ostringstream oss;
    oss << "agent_history_query_";
    if (agent_id.has_value()) {
        oss << agent_id.value().value();
    } else {
        oss << "all";
    }
    oss << "_";
    if (tick_from.has_value()) {
        oss << tick_from.value().value();
    } else {
        oss << "0";
    }
    oss << "_";
    if (tick_to.has_value()) {
        oss << tick_to.value().value();
    } else {
        oss << "0";
    }
    oss << "_" << limit;
    return AgentHistoryQueryId(oss.str());
}

// --- AgentHistorySortOrder ---

std::string toString(AgentHistorySortOrder order) {
    switch (order) {
        case AgentHistorySortOrder::Unknown: return "unknown";
        case AgentHistorySortOrder::TickAscending: return "tick_ascending";
        case AgentHistorySortOrder::TickDescending: return "tick_descending";
    }
    return "unknown";
}

pathfinder::foundation::Result<AgentHistorySortOrder> agentHistorySortOrderFromString(const std::string& str) {
    if (str == "unknown") return pathfinder::foundation::Result<AgentHistorySortOrder>::ok(AgentHistorySortOrder::Unknown);
    if (str == "tick_ascending") return pathfinder::foundation::Result<AgentHistorySortOrder>::ok(AgentHistorySortOrder::TickAscending);
    if (str == "tick_descending") return pathfinder::foundation::Result<AgentHistorySortOrder>::ok(AgentHistorySortOrder::TickDescending);
    return pathfinder::foundation::Result<AgentHistorySortOrder>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent.history_sort_order_invalid",
            "Unknown AgentHistorySortOrder: " + str));
}

// --- AgentHistoryProjectionMode ---

std::string toString(AgentHistoryProjectionMode mode) {
    switch (mode) {
        case AgentHistoryProjectionMode::Unknown: return "unknown";
        case AgentHistoryProjectionMode::Public: return "public";
        case AgentHistoryProjectionMode::Debug: return "debug";
        case AgentHistoryProjectionMode::Training: return "training";
    }
    return "unknown";
}

pathfinder::foundation::Result<AgentHistoryProjectionMode> agentHistoryProjectionModeFromString(const std::string& str) {
    if (str == "unknown") return pathfinder::foundation::Result<AgentHistoryProjectionMode>::ok(AgentHistoryProjectionMode::Unknown);
    if (str == "public") return pathfinder::foundation::Result<AgentHistoryProjectionMode>::ok(AgentHistoryProjectionMode::Public);
    if (str == "debug") return pathfinder::foundation::Result<AgentHistoryProjectionMode>::ok(AgentHistoryProjectionMode::Debug);
    if (str == "training") return pathfinder::foundation::Result<AgentHistoryProjectionMode>::ok(AgentHistoryProjectionMode::Training);
    return pathfinder::foundation::Result<AgentHistoryProjectionMode>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent.history_projection_mode_invalid",
            "Unknown AgentHistoryProjectionMode: " + str));
}

// --- AgentHistoryQueryOptions ---

pathfinder::foundation::Result<void> AgentHistoryQueryOptions::validateBasic() const {
    // query_id must be valid
    if (query_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "agent.query_id_missing"));
    }

    // sort_order must not be Unknown
    if (sort_order == AgentHistorySortOrder::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent.sort_order_unknown"));
    }

    // projection_mode must not be Unknown
    if (projection_mode == AgentHistoryProjectionMode::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "agent.projection_mode_unknown"));
    }

    // limit must be >0 and <=1000
    if (limit == 0 || limit > 1000) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "agent.limit_out_of_range",
                "limit must be >0 and <=1000"));
    }

    // tick_from > tick_to is invalid
    if (tick_from.has_value() && tick_to.has_value() && tick_from.value() > tick_to.value()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "agent.tick_range_invalid",
                "tick_from must not be greater than tick_to"));
    }

    // Public mode must not include debug trace
    if (projection_mode == AgentHistoryProjectionMode::Public && include_debug_trace) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "agent.public_no_debug_trace",
                "Public mode must not include debug trace"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentHistoryQueryResult ---

pathfinder::foundation::Result<void> AgentHistoryQueryResult::validateBasic() const {
    // query_id must be valid
    if (query_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "agent.query_result_id_missing"));
    }

    // Each record must validate
    for (const auto& record : records) {
        auto r = record.validateBasic();
        if (r.is_error()) return r;
    }

    // records.size <= total_matched
    if (records.size() > total_matched) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "agent.records_exceed_total",
                "records.size() must not exceed total_matched"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentHistoryQueryService ---

pathfinder::foundation::Result<AgentHistoryQueryResult> AgentHistoryQueryService::query(
    const AgentTickLog& log,
    const AgentHistoryQueryOptions& options) const {
    // Validate options
    auto vr = options.validateBasic();
    if (vr.is_error()) return pathfinder::foundation::Result<AgentHistoryQueryResult>::fail(vr.errors());

    // Filter records
    std::vector<AgentTickRecord> filtered;
    for (const auto& record : log.records()) {
        // Filter by agent_id
        if (options.agent_id.has_value() && record.agent_id != options.agent_id.value()) {
            continue;
        }
        // Filter by tick_from
        if (options.tick_from.has_value() && record.tick < options.tick_from.value()) {
            continue;
        }
        // Filter by tick_to
        if (options.tick_to.has_value() && record.tick > options.tick_to.value()) {
            continue;
        }
        filtered.push_back(record);
    }

    size_t total_matched = filtered.size();

    // Sort
    if (options.sort_order == AgentHistorySortOrder::TickAscending) {
        std::sort(filtered.begin(), filtered.end(),
            [](const AgentTickRecord& a, const AgentTickRecord& b) { return a.tick < b.tick; });
    } else if (options.sort_order == AgentHistorySortOrder::TickDescending) {
        std::sort(filtered.begin(), filtered.end(),
            [](const AgentTickRecord& a, const AgentTickRecord& b) { return a.tick > b.tick; });
    }

    // Apply limit
    bool truncated = false;
    if (filtered.size() > options.limit) {
        filtered.resize(options.limit);
        truncated = true;
    }

    AgentHistoryQueryResult result;
    result.query_id = options.query_id;
    result.records = std::move(filtered);
    result.total_matched = total_matched;
    result.truncated = truncated;

    return pathfinder::foundation::Result<AgentHistoryQueryResult>::ok(std::move(result));
}

} // namespace pathfinder::agent
