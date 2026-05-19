#include "pathfinder/world_interaction/world_services.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include "pathfinder/h5_dialog/dialog_session.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::world_interaction;
using namespace pathfinder::h5_dialog;

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

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string name = argv[1];
    if (name == "enum") test_enum_strings_and_validation();
    else if (name == "interaction") test_interaction_service_cut_fire_torch();
    else if (name == "agent") test_agent_and_threat_services();
    else return 2;
    std::cout << "world_interaction " << name << " passed\n";
    return 0;
}
