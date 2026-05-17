#include "pathfinder/agent/history_query.h"
#include "pathfinder/pipeline/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::pipeline;
using namespace pathfinder::command;

// --- Helpers ---

AgentDecisionRecord makeTestDecisionRecord(const std::string& agent_id_str, Tick tick) {
    AgentDecisionRecord rec;
    rec.record_id = AgentDecisionRecordId("agent_decision_record_dec_" + agent_id_str + "_" + std::to_string(tick.value()));
    rec.decision_id = DecisionId("dec_" + agent_id_str + "_" + std::to_string(tick.value()));
    rec.agent_id = AgentId(agent_id_str);
    rec.actor_id = EntityId("actor_" + agent_id_str);
    rec.decision_tick = tick;
    rec.observation_state_version = StateVersion(1);
    rec.status = AgentDecisionRecordStatus::PipelineSucceeded;
    rec.confidence = 0.8;
    rec.command_id = CommandId("cmd_" + agent_id_str + "_" + std::to_string(tick.value()));
    return rec;
}

AgentTickRecord makeTestTickRecord(const std::string& agent_id_str, Tick tick) {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId(agent_id_str), tick, StateVersion(1));
    rec.agent_id = AgentId(agent_id_str);
    rec.actor_id = EntityId("actor_" + agent_id_str);
    rec.tick = tick;
    rec.state_version_before = StateVersion(1);
    rec.state_version_after = StateVersion(2);
    rec.random_seed = RandomSeed(42);
    rec.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(111);
    rec.output_hash = HashValue(222);
    rec.pipeline_status = PipelineStatus::Succeeded;

    // Set command envelope
    CommandEnvelope env;
    env.command_id = CommandId("cmd_" + agent_id_str + "_" + std::to_string(tick.value()));
    env.payload.action_id = ActionId("eat");
    env.source = CommandSource::Ai;
    rec.command = env;

    rec.decision_record = makeTestDecisionRecord(agent_id_str, tick);
    return rec;
}

AgentTickLog makeTestLog() {
    AgentTickLog log;
    // agent_001: ticks 1, 3, 5
    log.append(makeTestTickRecord("agent_001", Tick(1)));
    log.append(makeTestTickRecord("agent_001", Tick(3)));
    log.append(makeTestTickRecord("agent_001", Tick(5)));
    // agent_002: ticks 2, 4
    log.append(makeTestTickRecord("agent_002", Tick(2)));
    log.append(makeTestTickRecord("agent_002", Tick(4)));
    return log;
}

AgentHistoryQueryOptions makeValidOptions() {
    AgentHistoryQueryOptions opts;
    opts.query_id = makeAgentHistoryQueryId(std::nullopt, std::nullopt, std::nullopt, 100);
    opts.sort_order = AgentHistorySortOrder::TickAscending;
    opts.projection_mode = AgentHistoryProjectionMode::Public;
    opts.limit = 100;
    return opts;
}

// --- Enum roundtrip tests ---

void test_sort_order_roundtrip() {
    auto orders = {
        AgentHistorySortOrder::Unknown,
        AgentHistorySortOrder::TickAscending,
        AgentHistorySortOrder::TickDescending
    };
    for (auto o : orders) {
        auto str = toString(o);
        auto back = agentHistorySortOrderFromString(str);
        assert(back.is_ok());
        if (o != AgentHistorySortOrder::Unknown) {
            assert(back.value() == o);
        }
    }
    auto invalid = agentHistorySortOrderFromString("invalid");
    assert(invalid.is_error());
    std::cout << "  PASS: sort_order_roundtrip\n";
}

void test_projection_mode_roundtrip() {
    auto modes = {
        AgentHistoryProjectionMode::Unknown,
        AgentHistoryProjectionMode::Public,
        AgentHistoryProjectionMode::Debug,
        AgentHistoryProjectionMode::Training
    };
    for (auto m : modes) {
        auto str = toString(m);
        auto back = agentHistoryProjectionModeFromString(str);
        assert(back.is_ok());
        if (m != AgentHistoryProjectionMode::Unknown) {
            assert(back.value() == m);
        }
    }
    auto invalid = agentHistoryProjectionModeFromString("invalid");
    assert(invalid.is_error());
    std::cout << "  PASS: projection_mode_roundtrip\n";
}

// --- ID helper tests ---

void test_query_id_helper() {
    auto id = makeAgentHistoryQueryId(AgentId("agent_001"), Tick(1), Tick(10), 50);
    assert(!id.empty());
    assert(id.value() == "agent_history_query_agent_001_1_10_50");

    auto id_all = makeAgentHistoryQueryId(std::nullopt, std::nullopt, std::nullopt, 100);
    assert(!id_all.empty());
    assert(id_all.value() == "agent_history_query_all_0_0_100");

    std::cout << "  PASS: query_id_helper\n";
}

// --- QueryOptions validation tests ---

void test_valid_options_passes() {
    auto opts = makeValidOptions();
    assert(opts.validateBasic().is_ok());
    std::cout << "  PASS: valid_options_passes\n";
}

void test_empty_query_id_fails() {
    auto opts = makeValidOptions();
    opts.query_id = AgentHistoryQueryId();
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: empty_query_id_fails\n";
}

void test_unknown_sort_order_fails() {
    auto opts = makeValidOptions();
    opts.sort_order = AgentHistorySortOrder::Unknown;
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: unknown_sort_order_fails\n";
}

void test_unknown_projection_mode_fails() {
    auto opts = makeValidOptions();
    opts.projection_mode = AgentHistoryProjectionMode::Unknown;
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: unknown_projection_mode_fails\n";
}

void test_limit_zero_fails() {
    auto opts = makeValidOptions();
    opts.limit = 0;
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: limit_zero_fails\n";
}

void test_limit_over_1000_fails() {
    auto opts = makeValidOptions();
    opts.limit = 1001;
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: limit_over_1000_fails\n";
}

void test_tick_from_greater_than_to_fails() {
    auto opts = makeValidOptions();
    opts.tick_from = Tick(10);
    opts.tick_to = Tick(5);
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: tick_from_greater_than_to_fails\n";
}

void test_public_debug_trace_fails() {
    auto opts = makeValidOptions();
    opts.projection_mode = AgentHistoryProjectionMode::Public;
    opts.include_debug_trace = true;
    assert(opts.validateBasic().is_error());
    std::cout << "  PASS: public_debug_trace_fails\n";
}

// --- QueryService tests ---

void test_filter_by_agent_id() {
    auto log = makeTestLog();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    opts.agent_id = AgentId("agent_001");
    auto result = service.query(log, opts);
    assert(result.is_ok());
    assert(result.value().records.size() == 3);
    assert(result.value().total_matched == 3);
    for (const auto& r : result.value().records) {
        assert(r.agent_id == AgentId("agent_001"));
    }
    std::cout << "  PASS: filter_by_agent_id\n";
}

void test_filter_by_tick_range() {
    auto log = makeTestLog();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    opts.tick_from = Tick(2);
    opts.tick_to = Tick(4);
    auto result = service.query(log, opts);
    assert(result.is_ok());
    assert(result.value().records.size() == 3); // agent_001@3, agent_002@2, agent_002@4
    assert(result.value().total_matched == 3);
    for (const auto& r : result.value().records) {
        assert(r.tick.value() >= 2 && r.tick.value() <= 4);
    }
    std::cout << "  PASS: filter_by_tick_range\n";
}

void test_sort_ascending() {
    auto log = makeTestLog();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    opts.sort_order = AgentHistorySortOrder::TickAscending;
    auto result = service.query(log, opts);
    assert(result.is_ok());
    for (size_t i = 1; i < result.value().records.size(); ++i) {
        assert(result.value().records[i].tick >= result.value().records[i - 1].tick);
    }
    std::cout << "  PASS: sort_ascending\n";
}

void test_sort_descending() {
    auto log = makeTestLog();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    opts.sort_order = AgentHistorySortOrder::TickDescending;
    auto result = service.query(log, opts);
    assert(result.is_ok());
    for (size_t i = 1; i < result.value().records.size(); ++i) {
        assert(result.value().records[i].tick <= result.value().records[i - 1].tick);
    }
    std::cout << "  PASS: sort_descending\n";
}

void test_limit_truncates() {
    auto log = makeTestLog();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    opts.limit = 2;
    auto result = service.query(log, opts);
    assert(result.is_ok());
    assert(result.value().records.size() == 2);
    assert(result.value().total_matched == 5);
    assert(result.value().truncated == true);
    std::cout << "  PASS: limit_truncates\n";
}

void test_no_match_empty_result() {
    auto log = makeTestLog();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    opts.agent_id = AgentId("nonexistent");
    auto result = service.query(log, opts);
    assert(result.is_ok());
    assert(result.value().records.empty());
    assert(result.value().total_matched == 0);
    assert(result.value().truncated == false);
    std::cout << "  PASS: no_match_empty_result\n";
}

void test_query_does_not_modify_log() {
    auto log = makeTestLog();
    size_t original_size = log.size();
    AgentHistoryQueryService service;
    auto opts = makeValidOptions();
    auto result = service.query(log, opts);
    assert(result.is_ok());
    assert(log.size() == original_size);
    std::cout << "  PASS: query_does_not_modify_log\n";
}

// --- main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "PASS: agent_history_query smoke" << std::endl;
        return 0;
    }

    // Enum roundtrip
    if (test_name == "sort_order_roundtrip") { test_sort_order_roundtrip(); return 0; }
    if (test_name == "projection_mode_roundtrip") { test_projection_mode_roundtrip(); return 0; }

    // ID helper
    if (test_name == "query_id_helper") { test_query_id_helper(); return 0; }

    // Options validation
    if (test_name == "valid_options_passes") { test_valid_options_passes(); return 0; }
    if (test_name == "empty_query_id_fails") { test_empty_query_id_fails(); return 0; }
    if (test_name == "unknown_sort_order_fails") { test_unknown_sort_order_fails(); return 0; }
    if (test_name == "unknown_projection_mode_fails") { test_unknown_projection_mode_fails(); return 0; }
    if (test_name == "limit_zero_fails") { test_limit_zero_fails(); return 0; }
    if (test_name == "limit_over_1000_fails") { test_limit_over_1000_fails(); return 0; }
    if (test_name == "tick_from_greater_than_to_fails") { test_tick_from_greater_than_to_fails(); return 0; }
    if (test_name == "public_debug_trace_fails") { test_public_debug_trace_fails(); return 0; }

    // QueryService
    if (test_name == "filter_by_agent_id") { test_filter_by_agent_id(); return 0; }
    if (test_name == "filter_by_tick_range") { test_filter_by_tick_range(); return 0; }
    if (test_name == "sort_ascending") { test_sort_ascending(); return 0; }
    if (test_name == "sort_descending") { test_sort_descending(); return 0; }
    if (test_name == "limit_truncates") { test_limit_truncates(); return 0; }
    if (test_name == "no_match_empty_result") { test_no_match_empty_result(); return 0; }
    if (test_name == "query_does_not_modify_log") { test_query_does_not_modify_log(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
