#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/world_agent_execution_context_store.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::goal_execution;
using namespace pathfinder::foundation;

void run_world_agent_execution_context_store_tests() {
    std::cout << "Running world_agent_execution context store tests:" << std::endl;

    InMemoryWorldAgentExecutionContextStore store;

    // Empty store returns nullopt
    auto found = store.findByActor("npc_001");
    assert(found.is_ok());
    assert(!found.value().has_value());

    // Put and retrieve
    ExecutionContext ctx;
    ctx.context_id = "ctx_1";
    ctx.actor_key = "npc_001";
    ctx.last_update_tick = 42;

    auto save_result = store.saveContext(ctx);
    assert(save_result.is_ok());

    auto found2 = store.findByActor("npc_001");
    assert(found2.is_ok());
    assert(found2.value().has_value());
    assert(found2.value().value().context_id == "ctx_1");
    assert(found2.value().value().last_update_tick == 42);

    // Clear
    auto clear_result = store.clearActor("npc_001");
    assert(clear_result.is_ok());

    auto found3 = store.findByActor("npc_001");
    assert(found3.is_ok());
    assert(!found3.value().has_value());

    std::cout << "All context store tests passed!" << std::endl;
}
