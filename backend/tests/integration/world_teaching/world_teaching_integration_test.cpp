#include "pathfinder/world_teaching/teach_command_handler.h"
#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/world_teaching/iworld_actor_query_port.h"
#include "pathfinder/world_teaching/world_teaching_owner_resolver.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>

using namespace pathfinder::world_teaching;
using namespace pathfinder::knowledge;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

// ---------------------------------------------------------------------------
// Fake Actor Query Port
// ---------------------------------------------------------------------------

class FakeWorldActorQueryPort : public IWorldActorQueryPort {
public:
    void setActor(const std::string& key, int x, int y, const std::string& layer = "surface") {
        WorldActorRuntime actor;
        actor.actor_key = key;
        actor.coord.x = x;
        actor.coord.y = y;
        actor.coord.layer_key = layer;
        actors_[key] = actor;
    }

    std::optional<WorldActorRuntime> findActor(const std::string& actor_key) const override {
        auto it = actors_.find(actor_key);
        if (it != actors_.end()) return it->second;
        return std::nullopt;
    }

private:
    mutable std::map<std::string, WorldActorRuntime> actors_;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static KnowledgeClaim makeTestClaim(
    const std::string& id,
    const std::string& owner_key,
    const std::string& subject,
    const std::string& action,
    const std::string& effect,
    bool teachable,
    KnowledgeStatus status,
    bool usable_for_action,
    KnowledgeRelationType relation = KnowledgeRelationType::HasEffect) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId(owner_key);
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject;
    claim.predicate.relation_type = relation;
    claim.predicate.action_key = action;
    claim.predicate.effect_key = effect;
    claim.status = status;
    claim.teaching_profile.teachable = teachable;
    if (teachable) {
        claim.teaching_profile.teaching_message_key = "test.teaching." + id;
    }
    claim.projection_flags.usable_for_action = usable_for_action;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.8;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);

    if (status == KnowledgeStatus::Active || status == KnowledgeStatus::Teachable || status == KnowledgeStatus::Shared || status == KnowledgeStatus::Operational) {
        KnowledgeEvidence ev;
        ev.evidence_id = KnowledgeEvidenceId("evd_" + id);
        ev.evidence_kind = KnowledgeEvidenceKind::DirectExperience;
        ev.source_summary_id = pathfinder::memory::MemorySummaryId("sum_" + id);
        ev.supports_claim = true;
        ev.confidence_delta = 0.1;
        ev.reliability = 1.0;
        ev.observed_tick = Tick(1);
        claim.evidence_refs.push_back(ev);
    }

    return claim;
}

static WorldCommandDto makeTeachCommand(
    const std::string& teacher,
    const std::string& recipient,
    const std::string& knowledge_id) {
    WorldCommandDto cmd;
    cmd.command_id = "cmd_teach_001";
    cmd.command_kind = WorldCommandKind::Teach;
    cmd.command_key = "Teach";
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = teacher;
    cmd.target.target_kind = WorldCommandTargetKind::Actor;
    cmd.target.target_actor_key = recipient;
    cmd.target.knowledge_key = knowledge_id;
    cmd.context.issued_tick = 10;
    return cmd;
}

// ---------------------------------------------------------------------------
// teach_command_player_to_npc
// ---------------------------------------------------------------------------

void run_teach_command_player_to_npc_tests() {
    KnowledgeRepository repo;
    auto claim = makeTestClaim("k1", "player", "berry_bush", "gather", "gather_item", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    FakeWorldActorQueryPort port;
    port.setActor("player", 0, 0);
    port.setActor("npc_001", 1, 0);

    auto handler = createTeachCommandHandler(repo, std::make_unique<FakeWorldActorQueryPort>(port));

    WorldCommandDto cmd = makeTeachCommand("player", "npc_001", "k1");
    WorldCommandContext ctx(cmd);

    auto result = handler->execute(ctx, cmd);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.result_kind == WorldCommandResultKind::Succeeded);

    // Events
    bool has_knowledge_taught = false;
    for (const auto& evt : val.events) {
        if (evt.event_kind == "knowledge_taught") {
            has_knowledge_taught = true;
            assert(evt.actor_key == "player");
            assert(evt.target_entity_id == "npc_001");
        }
    }
    assert(has_knowledge_taught);

    // State deltas
    bool has_recipient_delta = false;
    for (const auto& sd : val.state_deltas) {
        if (sd.delta_kind == "recipient_knowledge_delta") {
            has_recipient_delta = true;
            assert(sd.target_ref == "npc_001");
        }
    }
    assert(has_recipient_delta);

    // Projection patch
    assert(val.projection_patch_override.has_value());
    assert(!val.projection_patch_override->changed_knowledge.empty());
    bool found_npc_patch = false;
    for (const auto& kp : val.projection_patch_override->changed_knowledge) {
        if (kp.actor_key == "npc_001") {
            found_npc_patch = true;
        }
    }
    assert(found_npc_patch);

    // Repository: npc_001 should have a claim
    WorldActorKnowledgeOwnerResolver resolver;
    auto npc_owner = resolver.resolve("npc_001");
    auto npc_claims = repo.listByOwner(npc_owner);
    assert(npc_claims.is_ok());
    assert(!npc_claims.value().empty());

    std::cout << "teach_command_player_to_npc: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// teach_command_npc_to_player
// ---------------------------------------------------------------------------

void run_teach_command_npc_to_player_tests() {
    KnowledgeRepository repo;
    auto claim = makeTestClaim("k1", "npc_001", "berry_bush", "gather", "gather_item", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    FakeWorldActorQueryPort port;
    port.setActor("npc_001", 0, 0);
    port.setActor("player", 1, 0);

    auto handler = createTeachCommandHandler(repo, std::make_unique<FakeWorldActorQueryPort>(port));

    WorldCommandDto cmd = makeTeachCommand("npc_001", "player", "k1");
    WorldCommandContext ctx(cmd);

    auto result = handler->execute(ctx, cmd);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.result_kind == WorldCommandResultKind::Succeeded);

    // Repository: player should have a claim
    WorldActorKnowledgeOwnerResolver resolver;
    auto player_owner = resolver.resolve("player");
    auto player_claims = repo.listByOwner(player_owner);
    assert(player_claims.is_ok());
    assert(!player_claims.value().empty());

    std::cout << "teach_command_npc_to_player: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// teach_command_out_of_range_no_patch
// ---------------------------------------------------------------------------

void run_teach_command_out_of_range_no_patch_tests() {
    KnowledgeRepository repo;
    auto claim = makeTestClaim("k1", "player", "berry_bush", "gather", "gather_item", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    FakeWorldActorQueryPort port;
    port.setActor("player", 0, 0);
    port.setActor("npc_001", 5, 0);

    auto handler = createTeachCommandHandler(repo, std::make_unique<FakeWorldActorQueryPort>(port));

    WorldCommandDto cmd = makeTeachCommand("player", "npc_001", "k1");
    WorldCommandContext ctx(cmd);

    auto result = handler->execute(ctx, cmd);
    assert(result.is_ok());
    auto& val = result.value();
    assert(val.result_kind == WorldCommandResultKind::Blocked);

    bool has_out_of_range = false;
    for (const auto& reason : val.failure_reason_keys) {
        if (reason.find("OutOfRange") != std::string::npos) {
            has_out_of_range = true;
        }
    }
    assert(has_out_of_range);

    // No changed knowledge
    if (val.projection_patch_override.has_value()) {
        assert(val.projection_patch_override->changed_knowledge.empty());
    }

    // Repository unchanged for npc
    WorldActorKnowledgeOwnerResolver resolver;
    auto npc_owner = resolver.resolve("npc_001");
    auto npc_claims = repo.listByOwner(npc_owner);
    assert(npc_claims.is_ok());
    assert(npc_claims.value().empty());

    std::cout << "teach_command_out_of_range_no_patch: passed" << std::endl;
}


// ---------------------------------------------------------------------------
// teach_command_missing_actor_query_port_blocks
// ---------------------------------------------------------------------------

void run_teach_command_missing_actor_query_port_blocks_tests() {
    KnowledgeRepository repo;
    auto claim = makeTestClaim("k1", "player", "berry_bush", "gather", "gather_item", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    auto handler = createTeachCommandHandler(repo, nullptr);

    WorldCommandDto cmd = makeTeachCommand("player", "npc_001", "k1");
    WorldCommandContext ctx(cmd);

    auto result = handler->execute(ctx, cmd);
    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Blocked);

    WorldActorKnowledgeOwnerResolver resolver;
    auto npc_owner = resolver.resolve("npc_001");
    auto npc_claims = repo.listByOwner(npc_owner);
    assert(npc_claims.is_ok());
    assert(npc_claims.value().empty());

    std::cout << "teach_command_missing_actor_query_port_blocks: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// teach_handler_registers_in_registry
// ---------------------------------------------------------------------------

void run_teach_handler_registers_in_registry_tests() {
    KnowledgeRepository repo;
    FakeWorldActorQueryPort port;
    auto handler = createTeachCommandHandler(repo, std::make_unique<FakeWorldActorQueryPort>(port));

    WorldCommandHandlerRegistry registry;
    auto reg_result = registry.registerHandler(handler);
    assert(reg_result.is_ok());
    assert(registry.handlerCount() > 0);

    auto found = registry.findByKind(WorldCommandKind::Teach);
    assert(found != nullptr);
    assert(found->kind() == WorldCommandKind::Teach);

    std::cout << "teach_handler_registers_in_registry: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// teach_then_action_gate
// ---------------------------------------------------------------------------

void run_teach_then_action_gate_tests() {
    KnowledgeRepository repo;
    auto claim = makeTestClaim("k1", "player", "berry_bush", "gather", "gather_item", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    FakeWorldActorQueryPort port;
    port.setActor("player", 0, 0);
    port.setActor("npc_001", 1, 0);

    // Before teaching: gate should block
    NpcBasicActionKnowledgeGate gate;
    auto before = gate.check("npc_001", "berry_bush", "gather", "gather_item", "", true, false, repo);
    assert(!before.allowed);
    assert(before.decision == NpcActionKnowledgeGateDecision::BlockedNoKnowledge);

    // Teach
    auto handler = createTeachCommandHandler(repo, std::make_unique<FakeWorldActorQueryPort>(port));
    WorldCommandDto cmd = makeTeachCommand("player", "npc_001", "k1");
    WorldCommandContext ctx(cmd);
    auto result = handler->execute(ctx, cmd);
    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Succeeded);

    // After teaching: gate should allow
    auto after = gate.check("npc_001", "berry_bush", "gather", "gather_item", "", true, false, repo);
    assert(after.allowed);
    assert(after.decision == NpcActionKnowledgeGateDecision::AllowedByKnowledge);

    std::cout << "teach_then_action_gate: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Scan helpers
// ---------------------------------------------------------------------------

static std::filesystem::path getProjectRoot() {
    std::filesystem::path p(__FILE__);
    // __FILE__ is at backend/tests/integration/world_teaching/...
    // Go up 5 levels to project root
    for (int i = 0; i < 5; ++i) p = p.parent_path();
    return p;
}

static bool directoryContainsRegex(const std::filesystem::path& dir, const std::regex& re) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::ifstream f(entry.path());
        if (!f) continue;
        std::string line;
        while (std::getline(f, line)) {
            if (std::regex_search(line, re)) {
                std::cout << "MATCH: " << entry.path().string() << ": " << line << std::endl;
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// no_h5_dependency_scan
// ---------------------------------------------------------------------------

void run_no_h5_dependency_scan_tests() {
    auto root = getProjectRoot();
    std::regex re("h5|dialog|playable", std::regex::icase);
    bool found = false;
    found |= directoryContainsRegex(root / "backend" / "include" / "pathfinder" / "world_teaching", re);
    found |= directoryContainsRegex(root / "backend" / "src" / "world_teaching", re);
    assert(!found);
    std::cout << "no_h5_dependency_scan: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// no_content_hardcode_scan
// ---------------------------------------------------------------------------

void run_no_content_hardcode_scan_tests() {
    auto root = getProjectRoot();
    std::regex re("red_berry|torch|wood|beast|wolf|mushroom", std::regex::icase);
    bool found = false;
    found |= directoryContainsRegex(root / "backend" / "include" / "pathfinder" / "world_teaching", re);
    found |= directoryContainsRegex(root / "backend" / "src" / "world_teaching", re);
    assert(!found);
    std::cout << "no_content_hardcode_scan: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "teach_command_player_to_npc") {
        run_teach_command_player_to_npc_tests();
    } else if (test_name == "teach_command_npc_to_player") {
        run_teach_command_npc_to_player_tests();
    } else if (test_name == "teach_command_out_of_range_no_patch") {
        run_teach_command_out_of_range_no_patch_tests();
    } else if (test_name == "teach_command_missing_actor_query_port_blocks") {
        run_teach_command_missing_actor_query_port_blocks_tests();
    } else if (test_name == "teach_handler_registers_in_registry") {
        run_teach_handler_registers_in_registry_tests();
    } else if (test_name == "teach_then_action_gate") {
        run_teach_then_action_gate_tests();
    } else if (test_name == "no_h5_dependency_scan") {
        run_no_h5_dependency_scan_tests();
    } else if (test_name == "no_content_hardcode_scan") {
        run_no_content_hardcode_scan_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
