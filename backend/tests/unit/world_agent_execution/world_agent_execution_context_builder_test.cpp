#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/world_agent_context_builder.h"
#include "pathfinder/world_agent_execution/world_agent_execution_context_store.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using namespace pathfinder::knowledge;
using namespace pathfinder::goal_execution;
using namespace pathfinder::foundation;

// ---------------------------------------------------------------------------
// Fake Ports
// ---------------------------------------------------------------------------

class FakeVisibleWorldQueryPort : public IAgentVisibleWorldQueryPort {
public:
    void setActor(const std::string& key, int x, int y) {
        WorldActorRuntime a;
        a.actor_key = key;
        a.coord.x = x;
        a.coord.y = y;
        actors_[key] = a;
    }

    Result<WorldActorRuntime> findActor(const std::string& actor_key) const override {
        auto it = actors_.find(actor_key);
        if (it != actors_.end()) return Result<WorldActorRuntime>::ok(it->second);
        return Result<WorldActorRuntime>::fail(makeError(ErrorCode::id_not_found, "actor_not_found"));
    }

    Result<WorldRuntimeSnapshot> visibleSnapshotForActor(const std::string&) const override {
        return Result<WorldRuntimeSnapshot>::ok(WorldRuntimeSnapshot{});
    }

    Result<std::vector<VisibleResourceRef>> visibleResourcesForActor(const std::string&) const override {
        return Result<std::vector<VisibleResourceRef>>::ok(std::vector<VisibleResourceRef>{});
    }

    Result<std::vector<VisibleEntityRef>> visibleEntitiesForActor(const std::string&) const override {
        return Result<std::vector<VisibleEntityRef>>::ok(std::vector<VisibleEntityRef>{});
    }

private:
    mutable std::map<std::string, WorldActorRuntime> actors_;
};

class FakeInventoryQueryPort : public IAgentInventoryQueryPort {
public:
    Result<InventoryRuntimeSnapshot> snapshotForActorDecision(const std::string&) const override {
        return Result<InventoryRuntimeSnapshot>::ok(InventoryRuntimeSnapshot{});
    }
};

class FakeKnowledgeQueryPort : public IAgentKnowledgeQueryPort {
public:
    void setClaims(const std::string& actor_key, const std::vector<KnowledgeClaim>& claims) {
        claims_[actor_key] = claims;
    }

    Result<std::vector<KnowledgeClaim>> claimsForActor(const std::string& actor_key) const override {
        auto it = claims_.find(actor_key);
        if (it != claims_.end()) return Result<std::vector<KnowledgeClaim>>::ok(it->second);
        return Result<std::vector<KnowledgeClaim>>::ok(std::vector<KnowledgeClaim>{});
    }

private:
    mutable std::map<std::string, std::vector<KnowledgeClaim>> claims_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void run_world_agent_execution_context_builder_tests() {
    std::cout << "Running world_agent_execution context builder tests:" << std::endl;

    FakeVisibleWorldQueryPort world_port;
    FakeInventoryQueryPort inventory_port;
    FakeKnowledgeQueryPort knowledge_port;
    InMemoryWorldAgentExecutionContextStore store;

    WorldAgentContextBuilder builder(world_port, inventory_port, knowledge_port, store);

    // Actor missing
    WorldAgentTickRequest req;
    req.request_id = "r1";
    req.actor_key = "missing_npc";
    auto result = builder.build(req);
    assert(result.is_error());

    // Actor present, no knowledge
    world_port.setActor("npc_001", 1, 2);
    req.actor_key = "npc_001";
    auto result2 = builder.build(req);
    assert(result2.is_ok());
    assert(result2.value().actor_key == "npc_001");
    assert(result2.value().actor_claims.empty());

    // Player has knowledge, NPC does not
    KnowledgeClaim player_claim;
    player_claim.knowledge_id = pathfinder::foundation::KnowledgeId("k1");
    player_claim.owner.external_key = "player";
    knowledge_port.setClaims("player", {player_claim});

    auto result3 = builder.build(req);
    assert(result3.is_ok());
    assert(result3.value().actor_claims.empty());

    // NPC has knowledge
    KnowledgeClaim npc_claim;
    npc_claim.knowledge_id = pathfinder::foundation::KnowledgeId("k2");
    npc_claim.owner.external_key = "npc_001";
    knowledge_port.setClaims("npc_001", {npc_claim});

    auto result4 = builder.build(req);
    assert(result4.is_ok());
    assert(result4.value().actor_claims.size() == 1);

    std::cout << "All context builder tests passed!" << std::endl;
}
