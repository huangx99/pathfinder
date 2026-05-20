#include "pathfinder/world_interaction/world_services.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include "pathfinder/h5_dialog/dialog_session.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::world_interaction;
using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;
using namespace pathfinder::knowledge;

static DialogSessionState freshState(const std::string& session_id) {
    InMemoryDialogSessionStore store;
    auto result = store.loadOrCreate(session_id);
    assert(result.is_ok());
    return result.value();
}

static DialogScenario scenario() {
    DialogScenarioCatalog catalog;
    auto result = catalog.defaultScenario();
    assert(result.is_ok());
    return result.value();
}

static KnowledgeClaim companionClaim(const std::string& subject, const std::string& action, const std::string& effect, const std::string& target = {}) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId("knowledge_companion_" + subject + "_" + effect);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId("companion");
    claim.owner.external_key = "companion";
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject;
    claim.subject.subject_type_key = "world_object";
    if (!target.empty()) claim.subject.related_subject_ids.push_back(target);
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = action;
    claim.predicate.effect_key = effect;
    claim.confidence.confidence = 0.7;
    claim.confidence.band = KnowledgeConfidenceBand::Reliable;
    claim.status = KnowledgeStatus::Shared;
    claim.projection_flags.usable_by_ai = true;
    claim.projection_flags.usable_for_action = true;
    return claim;
}

static void test_enum_strings_and_validation() {
    assert(toString(WorldObjectInstanceKind::ResourceStack) == "resource_stack");
    assert(toString(WorldChangeKind::ObjectGenerated) == "object_generated");
    assert(toString(InteractionFailureKind::ToolUnavailable) == "tool_unavailable");
    assert(toString(AgentAutonomyActionKind::AvoidHazard) == "avoid_hazard");
    assert(toString(ThreatEventPhase::Approaching) == "approaching");

    WorldChange invalid;
    assert(invalid.validateBasic().is_error());

    WorldChange valid;
    valid.change_id = "change.test";
    valid.kind = WorldChangeKind::ObjectQuantityChanged;
    valid.target_instance_id = "red_berry";
    valid.quantity_delta = -1;
    valid.player_summary_zh_cn = "红果减少了。";
    assert(valid.validateBasic().is_ok());
}

static void test_interaction_service_cut_fire_torch() {
    auto sc = scenario();
    auto state = freshState("world_interaction_cut_fire");
    WorldSnapshotAdapter adapter;
    WorldInteractionService service;
    WorldChangeApplier applier;

    auto snapshot_result = adapter.fromDialogSession(sc, state);
    assert(snapshot_result.is_ok());
    auto snapshot = snapshot_result.value();

    WorldInteractionInput cut;
    cut.interaction_id = "test.cut";
    cut.object_key = "axe";
    cut.target_object_key = "wood";
    cut.action = DialogActionKind::Use;
    cut.effect_key = "cut_wood";
    cut.feedback_key = "axe_use_cut_wood";
    auto cut_result = service.resolve(snapshot, cut, InteractionResolveOptions{});
    assert(cut_result.is_ok());
    assert(cut_result.value().ok);
    assert(!cut_result.value().changes.empty());
    auto after_cut = applier.apply(snapshot, cut_result.value().changes);
    assert(after_cut.is_ok());
    snapshot = after_cut.value();
    assert(snapshot.objects_by_key["wood"].quantity == 1);
    assert(snapshot.objects_by_key["wood"].numeric_states["processed"] == 1.0);
    assert(snapshot.objects_by_key["axe"].numeric_states["sharpness"] == 2.0);

    WorldInteractionInput fire;
    fire.interaction_id = "test.fire";
    fire.object_key = "fire_seed";
    fire.target_object_key = "dry_grass";
    fire.action = DialogActionKind::Use;
    fire.effect_key = "ignite_fire";
    fire.feedback_key = "fire_seed_ignite_dry_grass";
    auto fire_result = service.resolve(snapshot, fire, InteractionResolveOptions{});
    assert(fire_result.is_ok());
    assert(fire_result.value().ok);
    auto after_fire = applier.apply(snapshot, fire_result.value().changes);
    assert(after_fire.is_ok());
    snapshot = after_fire.value();
    assert(snapshot.objects_by_key["dry_grass"].quantity == 1);
    assert(snapshot.objects_by_key["camp_fire"].quantity == 1);
    assert(snapshot.objects_by_key["beast_shadow"].numeric_states["observed_fire"] == 1.0);

    WorldInteractionInput torch;
    torch.interaction_id = "test.torch";
    torch.object_key = "wood";
    torch.target_object_key = "fire_seed";
    torch.action = DialogActionKind::Use;
    torch.effect_key = "make_torch";
    torch.feedback_key = "wood_make_torch";
    auto torch_result = service.resolve(snapshot, torch, InteractionResolveOptions{});
    assert(torch_result.is_ok());
    assert(torch_result.value().ok);
    auto after_torch = applier.apply(snapshot, torch_result.value().changes);
    assert(after_torch.is_ok());
    snapshot = after_torch.value();
    assert(snapshot.objects_by_key["torch"].quantity == 1);
    assert(snapshot.objects_by_key["wood"].numeric_states["processed"] == 0.0);
}

static void test_agent_and_threat_services() {
    auto sc = scenario();
    auto state = freshState("world_interaction_agent");
    WorldSnapshotAdapter adapter;
    WorldChangeApplier applier;
    ThreatEventBridge threat_bridge;
    AgentAutonomyService agent_service;

    auto snapshot = adapter.fromDialogSession(sc, state).value();
    snapshot.objects_by_key["camp_fire"].quantity = 1;
    snapshot.objects_by_key["camp_fire"].numeric_states["quantity"] = 1.0;
    auto threat_changes = threat_bridge.progressThreats(snapshot, ThreatProgressInput{"beast_shadow", 50.0, 8});
    assert(threat_changes.is_ok());
    auto after_threat = applier.apply(snapshot, threat_changes.value());
    assert(after_threat.is_ok());
    snapshot = after_threat.value();
    assert(snapshot.threats_by_key["beast_shadow"].level == 50.0);

    AgentAutonomyRequest beast;
    beast.request_key = "test.beast";
    beast.agent_actor_key = "beast_shadow";
    beast.trigger_key = "threat_progress";
    beast.target_threat_key = "beast_shadow";
    beast.required_knowledge_effect_key = "fire_is_dangerous";
    beast.observed_object_keys = {"fire"};
    auto beast_result = agent_service.tryAct(snapshot, beast);
    assert(beast_result.is_ok());
    assert(beast_result.value().executed);
    assert(beast_result.value().action_kind == AgentAutonomyActionKind::AvoidHazard);
    assert(!beast_result.value().changes.empty());
}

static void test_threat_reenters_after_flank_waits() {
    auto sc = scenario();
    auto state = freshState("world_interaction_threat_reentry");
    WorldSnapshotAdapter adapter;
    WorldChangeApplier applier;
    ThreatEventBridge threat_bridge;

    auto snapshot = adapter.fromDialogSession(sc, state).value();
    snapshot.threats_by_key["beast_shadow"].level = 0.0;
    snapshot.threats_by_key["beast_shadow"].resolved = true;
    snapshot.threats_by_key["beast_shadow"].active = false;
    snapshot.threats_by_key["beast_shadow"].phase = ThreatEventPhase::Resolved;
    snapshot.objects_by_key["beast_shadow"].state_tags.push_back("resolved");
    snapshot.objects_by_key["beast_shadow"].numeric_states["flank_waits"] = 0.0;

    ThreatProgressInput input;
    input.threat_key = "beast_shadow";
    input.reentry_interval_waits = 3;
    input.reentry_level = 50.0;

    for (int wait = 1; wait <= 2; ++wait) {
        snapshot.turn_index++;
        auto changes = threat_bridge.progressThreats(snapshot, input);
        assert(changes.is_ok());
        assert(changes.value().size() == 1);
        assert(changes.value().front().kind == WorldChangeKind::ObjectStateChanged);
        auto applied = applier.apply(snapshot, changes.value());
        assert(applied.is_ok());
        snapshot = applied.value();
        assert(snapshot.threats_by_key["beast_shadow"].resolved);
        assert(snapshot.objects_by_key["beast_shadow"].numeric_states["flank_waits"] == static_cast<double>(wait));
    }

    snapshot.turn_index++;
    auto reentry_changes = threat_bridge.progressThreats(snapshot, input);
    assert(reentry_changes.is_ok());
    assert(reentry_changes.value().size() == 2);
    bool has_reentry = false;
    for (const auto& change : reentry_changes.value()) {
        if (change.target_threat_key.has_value() && *change.target_threat_key == "beast_shadow") {
            has_reentry = true;
            assert(change.player_summary_zh_cn.find("重新靠近") != std::string::npos);
        }
    }
    assert(has_reentry);
    auto applied = applier.apply(snapshot, reentry_changes.value());
    assert(applied.is_ok());
    snapshot = applied.value();
    assert(!snapshot.threats_by_key["beast_shadow"].resolved);
    assert(snapshot.threats_by_key["beast_shadow"].level == 50.0);
    assert(snapshot.objects_by_key["beast_shadow"].numeric_states["flank_waits"] == 0.0);
    bool has_flanking_probe = false;
    for (const auto& tag : snapshot.objects_by_key["beast_shadow"].state_tags) {
        if (tag == "flanking_probe") has_flanking_probe = true;
    }
    assert(has_flanking_probe);

    snapshot.objects_by_key["camp_fire"].quantity = 1;
    snapshot.objects_by_key["camp_fire"].numeric_states["quantity"] = 1.0;
    AgentAutonomyService agent_service;
    AgentAutonomyRequest beast;
    beast.request_key = "test.beast.flanking.probe";
    beast.agent_actor_key = "beast_shadow";
    beast.trigger_key = "threat_progress";
    beast.target_threat_key = "beast_shadow";
    beast.required_knowledge_effect_key = "fire_is_dangerous";
    auto beast_result = agent_service.tryAct(snapshot, beast);
    assert(beast_result.is_ok());
    assert(!beast_result.value().executed);
    assert(beast_result.value().summary_zh_cn.find("普通火光不足") != std::string::npos);
}

static void test_companion_cannot_use_pioneer_owned_torch() {
    auto sc = scenario();
    auto state = freshState("world_interaction_companion_no_shared_torch");
    WorldSnapshotAdapter adapter;
    AgentAutonomyService agent_service;

    auto snapshot = adapter.fromDialogSession(sc, state).value();
    auto& torch = snapshot.objects_by_key["torch"];
    torch.quantity = 1;
    torch.numeric_states["quantity"] = 1.0;
    torch.owner_actor_key = "pioneer";
    torch.actor_quantities.clear();
    torch.actor_quantities["pioneer"] = 1;
    snapshot.threats_by_key["beast_shadow"].level = 70.0;
    snapshot.threats_by_key["beast_shadow"].active = true;
    snapshot.threats_by_key["beast_shadow"].resolved = false;
    snapshot.threats_by_key["beast_shadow"].phase = ThreatEventPhase::Approaching;

    auto& companion = snapshot.actors_by_key["companion"];
    companion.known_claims = {companionClaim("torch", "use", "repel_beast", "beast_shadow")};
    companion.known_effect_keys = {"repel_beast"};

    AgentAutonomyRequest request;
    request.request_key = "test.companion.no.shared.torch";
    request.agent_actor_key = "companion";
    request.trigger_key = "threat_progress";
    request.target_threat_key = "beast_shadow";
    request.required_knowledge_effect_key = "repel_beast";
    auto result = agent_service.tryAct(snapshot, request);
    assert(result.is_ok());
    assert(!result.value().executed);
    assert(result.value().summary_zh_cn.find("不能凭空使用你的物品") != std::string::npos);
}

static void test_companion_uses_p40_to_make_missing_torch() {
    auto sc = scenario();
    auto state = freshState("world_interaction_companion_p40_torch");
    WorldSnapshotAdapter adapter;
    WorldChangeApplier applier;
    AgentAutonomyService agent_service;

    auto snapshot = adapter.fromDialogSession(sc, state).value();
    snapshot.objects_by_key["torch"].quantity = 0;
    snapshot.objects_by_key["torch"].numeric_states["quantity"] = 0.0;
    snapshot.objects_by_key["wood"].numeric_states["processed"] = 1.0;
    snapshot.objects_by_key["camp_fire"].quantity = 1;
    snapshot.objects_by_key["camp_fire"].numeric_states["quantity"] = 1.0;
    snapshot.objects_by_key["camp_fire"].owner_actor_key.clear();
    snapshot.objects_by_key["camp_fire"].actor_quantities.clear();
    snapshot.threats_by_key["beast_shadow"].level = 70.0;
    snapshot.threats_by_key["beast_shadow"].active = true;
    snapshot.threats_by_key["beast_shadow"].resolved = false;
    snapshot.threats_by_key["beast_shadow"].phase = ThreatEventPhase::Approaching;

    auto& companion = snapshot.actors_by_key["companion"];
    companion.known_claims = {
        companionClaim("axe", "use", "cut_wood"),
        companionClaim("fire_seed", "use", "ignite_fire"),
        companionClaim("wood", "use", "make_torch"),
        companionClaim("torch", "use", "repel_beast", "beast_shadow")
    };
    companion.known_effect_keys = {"cut_wood", "ignite_fire", "make_torch", "repel_beast"};

    AgentAutonomyRequest request;
    request.request_key = "test.companion.p40.make_torch";
    request.agent_actor_key = "companion";
    request.trigger_key = "threat_progress";
    request.target_threat_key = "beast_shadow";
    request.required_knowledge_effect_key = "repel_beast";
    auto result = agent_service.tryAct(snapshot, request);
    assert(result.is_ok());
    assert(result.value().executed);
    assert(result.value().summary_zh_cn.find("制作火把") != std::string::npos);

    auto applied = applier.apply(snapshot, result.value().changes);
    assert(applied.is_ok());
    auto torch = applied.value().objects_by_key["torch"];
    assert(torch.quantity == 1);
    assert(torch.actor_quantities["companion"] == 1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string name = argv[1];
    if (name == "enum") test_enum_strings_and_validation();
    else if (name == "interaction") test_interaction_service_cut_fire_torch();
    else if (name == "agent") test_agent_and_threat_services();
    else if (name == "threat_reentry") test_threat_reenters_after_flank_waits();
    else if (name == "companion_no_shared_torch") test_companion_cannot_use_pioneer_owned_torch();
    else if (name == "companion_p40_torch") test_companion_uses_p40_to_make_missing_torch();
    else return 2;
    std::cout << "world_interaction " << name << " passed\n";
    return 0;
}
