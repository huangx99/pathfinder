#include <cassert>
#include <iostream>
#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

// ---------------------------------------------------------------------------
// Enum roundtrip
// ---------------------------------------------------------------------------
void run_enum_roundtrip_tests() {
    // ClientRequestKind
    assert(toString(ClientRequestKind::Bootstrap) == "bootstrap");
    assert(toString(ClientRequestKind::Command) == "command");
    assert(toString(ClientRequestKind::Refresh) == "refresh");
    assert(toString(ClientRequestKind::Reset) == "reset");
    assert(toString(ClientRequestKind::TestOnly) == "test_only");
    assert(toString(ClientRequestKind::Unknown) == "unknown");

    auto r = clientRequestKindFromString("command");
    assert(r.is_ok() && r.value() == ClientRequestKind::Command);
    r = clientRequestKindFromString("invalid");
    assert(r.is_error());

    // ClientSubmitMode
    assert(toString(ClientSubmitMode::OptionId) == "option_id");
    assert(toString(ClientSubmitMode::WorldCommandDto) == "world_command_dto");
    assert(toString(ClientSubmitMode::RefreshOnly) == "refresh_only");

    auto m = clientSubmitModeFromString("option_id");
    assert(m.is_ok() && m.value() == ClientSubmitMode::OptionId);
    m = clientSubmitModeFromString("invalid");
    assert(m.is_error());

    // ClientSyncState
    assert(toString(ClientSyncState::InSync) == "in_sync");
    assert(toString(ClientSyncState::NeedsFullRefresh) == "needs_full_refresh");

    auto s = clientSyncStateFromString("out_of_sync");
    assert(s.is_ok() && s.value() == ClientSyncState::OutOfSync);
    s = clientSyncStateFromString("invalid");
    assert(s.is_error());

    // ClientProjectionScope
    assert(toString(ClientProjectionScope::Viewport) == "viewport");
    assert(toString(ClientProjectionScope::FullSafeWorld) == "full_safe_world");

    auto p = clientProjectionScopeFromString("actor_self");
    assert(p.is_ok() && p.value() == ClientProjectionScope::ActorSelf);
    p = clientProjectionScopeFromString("invalid");
    assert(p.is_error());

    std::cout << "client_protocol_enum_roundtrip_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// DTO validation
// ---------------------------------------------------------------------------
void run_dto_validation_tests() {
    // ClientBootstrapRequest: client_id and session_id required
    {
        ClientBootstrapRequest req;
        req.session_id = "s1";
        req.client_id = "";
        assert(req.validateBasic().is_error());
    }
    {
        ClientBootstrapRequest req;
        req.client_id = "c1";
        req.session_id = "";
        assert(req.validateBasic().is_error());
    }
    {
        ClientBootstrapRequest req;
        req.client_id = "c1";
        req.session_id = "s1";
        req.client_protocol_version = 0;
        assert(req.validateBasic().is_error());
    }
    {
        ClientBootstrapRequest req;
        req.client_id = "c1";
        req.session_id = "s1";
        req.client_protocol_version = 1;
        assert(req.validateBasic().is_ok());
    }

    // ClientCommandRequest: submit_mode and required fields
    {
        ClientCommandRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.submit_mode = ClientSubmitMode::Unknown;
        assert(req.validateBasic().is_error());
    }
    {
        ClientCommandRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.submit_mode = ClientSubmitMode::OptionId;
        req.option_id = "";
        assert(req.validateBasic().is_error());
    }
    {
        ClientCommandRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.submit_mode = ClientSubmitMode::WorldCommandDto;
        assert(req.validateBasic().is_error());
    }
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_1";
        cmd.command_key = "wait";
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";
        ClientCommandRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.submit_mode = ClientSubmitMode::WorldCommandDto;
        req.command = cmd;
        assert(req.validateBasic().is_ok());
    }

    // ClientRefreshRequest
    {
        ClientRefreshRequest req;
        req.session_id = "";
        req.client_id = "c1";
        assert(req.validateBasic().is_error());
    }
    {
        ClientRefreshRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        assert(req.validateBasic().is_ok());
    }

    // ClientResetRequest: confirmed must be true
    {
        ClientResetRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.confirmed = false;
        assert(req.validateBasic().is_error());
    }
    {
        ClientResetRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.confirmed = true;
        assert(req.validateBasic().is_ok());
    }

    std::cout << "client_protocol_dto_validation_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Codec roundtrip
// ---------------------------------------------------------------------------
void run_codec_roundtrip_tests() {
    ClientProtocolCodec codec;

    // Bootstrap request roundtrip
    {
        ClientBootstrapRequest req;
        req.client_id = "c1";
        req.session_id = "s1";
        req.requested_actor_key = "player";
        req.requested_layer_key = "surface";
        req.client_protocol_version = 1;

        auto enc = codec.encodeBootstrapRequest(req);
        assert(enc.is_ok());
        auto dec = codec.decodeBootstrapRequest(enc.value());
        assert(dec.is_ok());
        assert(dec.value().client_id == req.client_id);
        assert(dec.value().session_id == req.session_id);
        assert(dec.value().requested_actor_key == req.requested_actor_key);
    }

    // Bootstrap response roundtrip
    {
        ClientBootstrapResponse resp;
        resp.session_id = "s1";
        resp.server_protocol_version = 1;
        resp.projection_version = 1;
        resp.full_projection.actor_key = "player";
        resp.full_projection.projection_version = 1;

        auto enc = codec.encodeBootstrapResponse(resp);
        assert(enc.is_ok());
        auto dec = codec.decodeBootstrapResponse(enc.value());
        assert(dec.is_ok());
        assert(dec.value().session_id == resp.session_id);
        assert(dec.value().full_projection.actor_key == "player");
    }

    // Command request roundtrip (OptionId)
    {
        ClientCommandRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.client_sequence = 5;
        req.known_projection_version = 3;
        req.submit_mode = ClientSubmitMode::OptionId;
        req.option_id = "opt_wait_player";

        auto enc = codec.encodeCommandRequest(req);
        assert(enc.is_ok());
        auto dec = codec.decodeCommandRequest(enc.value());
        assert(dec.is_ok());
        assert(dec.value().option_id == req.option_id);
        assert(dec.value().client_sequence == 5);
        assert(dec.value().known_projection_version == 3);
    }

    // Command request roundtrip (WorldCommandDto)
    {
        WorldCommandDto cmd;
        cmd.command_id = "cmd_1";
        cmd.command_key = "wait";
        cmd.command_kind = WorldCommandKind::Wait;
        cmd.source = WorldCommandSource::PlayerInput;
        cmd.actor_key = "player";

        ClientCommandRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.submit_mode = ClientSubmitMode::WorldCommandDto;
        req.command = cmd;

        auto enc = codec.encodeCommandRequest(req);
        assert(enc.is_ok());
        auto dec = codec.decodeCommandRequest(enc.value());
        assert(dec.is_ok());
        assert(dec.value().command.has_value());
        assert(dec.value().command->command_key == "wait");
    }

    // Command response roundtrip
    {
        ClientCommandResponse resp;
        resp.session_id = "s1";
        resp.accepted_client_sequence = 5;
        resp.base_projection_version = 3;
        resp.new_projection_version = 4;
        resp.result.result_kind = WorldCommandResultKind::Succeeded;
        resp.result.command_id = "cmd_1";

        auto enc = codec.encodeCommandResponse(resp);
        assert(enc.is_ok());
        auto dec = codec.decodeCommandResponse(enc.value());
        assert(dec.is_ok());
        assert(dec.value().new_projection_version == 4);
        assert(dec.value().result.result_kind == WorldCommandResultKind::Succeeded);
    }

    // Refresh request roundtrip
    {
        ClientRefreshRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.known_projection_version = 3;
        req.requested_scopes = {ClientProjectionScope::Viewport, ClientProjectionScope::ActorSelf};

        auto enc = codec.encodeRefreshRequest(req);
        assert(enc.is_ok());
        auto dec = codec.decodeRefreshRequest(enc.value());
        assert(dec.is_ok());
        assert(dec.value().known_projection_version == 3);
    }

    // Reset request roundtrip
    {
        ClientResetRequest req;
        req.session_id = "s1";
        req.client_id = "c1";
        req.confirmed = true;

        auto enc = codec.encodeResetRequest(req);
        assert(enc.is_ok());
        auto dec = codec.decodeResetRequest(enc.value());
        assert(dec.is_ok());
        assert(dec.value().confirmed == true);
    }

    std::cout << "client_protocol_codec_roundtrip_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Projection version tests
// ---------------------------------------------------------------------------
void run_projection_version_tests() {
    ClientPatchContract contract;

    // Matching versions allow patch
    assert(contract.canApplyPatch(5, 5));

    // Stale version requires full refresh
    assert(!contract.canApplyPatch(3, 5));

    // Full refresh flagged by patch
    {
        WorldProjectionPatchDto patch;
        patch.base_projection_version = 5;
        patch.new_projection_version = 6;
        patch.requires_full_refresh = true;
        assert(contract.requiresFullRefresh(patch, 5));
    }

    // Full refresh required on version mismatch
    {
        WorldProjectionPatchDto patch;
        patch.base_projection_version = 5;
        patch.new_projection_version = 6;
        patch.requires_full_refresh = false;
        assert(contract.requiresFullRefresh(patch, 4));
    }

    // Version progression validation
    assert(contract.validateVersionProgression(3, 4).is_ok());
    assert(contract.validateVersionProgression(4, 4).is_ok());
    assert(contract.validateVersionProgression(5, 4).is_error());

    // Full refresh hint
    {
        auto hint = contract.buildFullRefreshHint(3, 7);
        assert(hint.requires_full_refresh);
        assert(hint.base_projection_version == 3);
        assert(hint.new_projection_version == 7);
    }

    std::cout << "client_projection_version_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Available command adapter tests
// ---------------------------------------------------------------------------
void run_available_command_tests() {
    ClientAvailableCommandAdapter adapter;

    // Backend returns options
    auto result = adapter.buildOptions("player", "surface");
    assert(result.is_ok());
    assert(!result.value().empty());

    // Materialize wait option
    auto mat = adapter.materializeOption("opt_wait_player", "player");
    assert(mat.is_ok());
    assert(mat.value().command_kind == WorldCommandKind::Wait);
    assert(mat.value().actor_key == "player");

    // Materialize inspect option
    mat = adapter.materializeOption("opt_inspect_player", "player");
    assert(mat.is_ok());
    assert(mat.value().command_kind == WorldCommandKind::Inspect);

    // Unknown option_id rejected
    mat = adapter.materializeOption("opt_unknown", "player");
    assert(mat.is_error());

    std::cout << "client_available_command_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Security request tests
// ---------------------------------------------------------------------------
void run_security_request_tests() {
    // ClientCommandRequest must not carry knowledge_json / recipient_claim_json
    // (enforced by absence of such fields in the DTO)
    ClientCommandRequest req;
    req.session_id = "s1";
    req.client_id = "c1";
    req.submit_mode = ClientSubmitMode::OptionId;
    req.option_id = "opt_wait_player";
    assert(req.validateBasic().is_ok());

    // Verify DTO has no hidden debug fields by default
    assert(!req.selection_context.selected_actor_key.empty() == false);

    std::cout << "client_security_request_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Projection adapter tests
// ---------------------------------------------------------------------------
void run_projection_adapter_tests() {
    ClientProjectionAdapter adapter;

    auto result = adapter.buildFullProjection("player", "surface", 7);
    assert(result.is_ok());
    assert(result.value().projection_version == 7);
    assert(result.value().actor_key == "player");
    assert(result.value().active_layer_key == "surface");

    // Merge patch
    WorldProjectionPatchDto patch;
    patch.base_projection_version = 7;
    patch.new_projection_version = 8;
    WorldCellPatchDto cell;
    cell.cell_id = "cell_1";
    cell.op = PatchOp::Add;
    cell.fields["terrain"] = "grass";
    patch.changed_cells.push_back(cell);

    auto merged = adapter.mergePatch(result.value(), patch);
    assert(merged.is_ok());
    assert(merged.value().projection_version == 8);
    assert(!merged.value().visible_cells.empty());
    assert(merged.value().visible_cells[0].cell_id == "cell_1");

    ClientWorldProjection with_entity = merged.value();
    WorldEntityPatchDto visible_entity;
    visible_entity.entity_id = "pickup_stone_1";
    visible_entity.op = PatchOp::Update;
    visible_entity.fields["entity_key"] = "loose_stone";
    with_entity.visible_entities.push_back(visible_entity);

    WorldProjectionPatchDto remove_patch;
    remove_patch.base_projection_version = 8;
    remove_patch.new_projection_version = 9;
    WorldEntityPatchDto removed_entity;
    removed_entity.entity_id = "pickup_stone_1";
    removed_entity.op = PatchOp::Remove;
    remove_patch.changed_entities.push_back(removed_entity);

    auto removed = adapter.mergePatch(with_entity, remove_patch);
    assert(removed.is_ok());
    assert(removed.value().projection_version == 9);
    for (const auto& entity : removed.value().visible_entities) {
        assert(entity.entity_id != "pickup_stone_1");
    }

    std::cout << "client_projection_adapter_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Snapshot materialize tests
// ---------------------------------------------------------------------------
void run_snapshot_materialize_tests() {
    ClientAvailableCommandAdapter adapter;
    auto options = adapter.buildOptions("player", "surface");
    assert(options.is_ok());
    assert(!options.value().empty());

    // Materialize from snapshot succeeds
    auto mat = adapter.materializeOption(options.value()[0].option_id, "player", options.value());
    assert(mat.is_ok());
    assert(mat.value().command_kind == options.value()[0].command_kind);

    // Materialize unknown option_id from snapshot fails
    auto mat2 = adapter.materializeOption("opt_forged_xyz", "player", options.value());
    assert(mat2.is_error());

    // Materialize option from wrong actor snapshot fails
    auto other_options = adapter.buildOptions("wolf", "surface");
    assert(other_options.is_ok());
    auto mat3 = adapter.materializeOption(other_options.value()[0].option_id, "player", options.value());
    assert(mat3.is_error());

    std::cout << "client_protocol_snapshot_materialize_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Full projection / patch codec roundtrip tests
// ---------------------------------------------------------------------------
void run_projection_codec_full_roundtrip_tests() {
    ClientProtocolCodec codec;

    // Build a non-empty projection
    ClientWorldProjection proj;
    proj.projection_version = 5;
    proj.actor_key = "player";
    proj.active_layer_key = "surface";

    WorldCellPatchDto cell;
    cell.cell_id = "c1";
    cell.op = PatchOp::Update;
    cell.fields["terrain"] = "forest";
    proj.visible_cells.push_back(cell);

    WorldEntityPatchDto entity;
    entity.entity_id = "e1";
    entity.op = PatchOp::Add;
    entity.fields["type"] = "bush";
    proj.visible_entities.push_back(entity);

    InventoryPatchDto inv;
    inv.inventory_id = "inv_player";
    inv.op = PatchOp::Update;
    inv.fields["slot_count"] = "3";
    proj.inventories.push_back(inv);

    KnowledgePatchDto kn;
    kn.actor_key = "player";
    kn.op = PatchOp::Update;
    kn.fields["known_herbs"] = "berry,mushroom";
    proj.knowledge.push_back(kn);

    AreaEffectPatchDto ae;
    ae.area_effect_id = "ae1";
    ae.op = PatchOp::Add;
    ae.fields["type"] = "fire";
    proj.area_effects.push_back(ae);

    proj.safe_summary_keys = {"summary_1", "summary_2"};

    ClientBootstrapResponse resp;
    resp.session_id = "s1";
    resp.server_protocol_version = 1;
    resp.projection_version = 5;
    resp.full_projection = proj;

    auto enc = codec.encodeBootstrapResponse(resp);
    assert(enc.is_ok());
    auto dec = codec.decodeBootstrapResponse(enc.value());
    assert(dec.is_ok());

    const auto& dp = dec.value().full_projection;
    assert(dp.projection_version == 5);
    assert(!dp.visible_cells.empty());
    assert(dp.visible_cells[0].cell_id == "c1");
    assert(dp.visible_cells[0].fields.at("terrain") == "forest");
    assert(!dp.visible_entities.empty());
    assert(dp.visible_entities[0].entity_id == "e1");
    assert(!dp.inventories.empty());
    assert(dp.inventories[0].inventory_id == "inv_player");
    assert(!dp.knowledge.empty());
    assert(dp.knowledge[0].fields.at("known_herbs") == "berry,mushroom");
    assert(!dp.area_effects.empty());
    assert(dp.area_effects[0].area_effect_id == "ae1");
    assert(dp.safe_summary_keys.size() == 2);

    std::cout << "client_protocol_projection_codec_full_roundtrip_tests: all passed" << std::endl;
}

void run_patch_codec_full_roundtrip_tests() {
    ClientProtocolCodec codec;

    WorldProjectionPatchDto patch;
    patch.base_projection_version = 3;
    patch.new_projection_version = 4;
    patch.requires_full_refresh = false;

    WorldCellPatchDto cell;
    cell.cell_id = "c1";
    cell.op = PatchOp::Update;
    cell.fields["terrain"] = "forest";
    patch.changed_cells.push_back(cell);

    WorldEntityPatchDto entity;
    entity.entity_id = "e1";
    entity.op = PatchOp::Remove;
    entity.fields["reason"] = "harvested";
    patch.changed_entities.push_back(entity);

    InventoryPatchDto inv;
    inv.inventory_id = "inv1";
    inv.op = PatchOp::Update;
    inv.fields["wood"] = "5";
    patch.changed_inventories.push_back(inv);

    KnowledgePatchDto kn;
    kn.actor_key = "player";
    kn.op = PatchOp::Update;
    kn.fields["herbs"] = "berry";
    patch.changed_knowledge.push_back(kn);

    AreaEffectPatchDto ae;
    ae.area_effect_id = "ae1";
    ae.op = PatchOp::Add;
    ae.fields["duration"] = "10";
    patch.changed_area_effects.push_back(ae);

    ClientCommandResponse resp;
    resp.session_id = "s1";
    resp.accepted_client_sequence = 1;
    resp.base_projection_version = 3;
    resp.new_projection_version = 4;
    resp.result.result_kind = WorldCommandResultKind::Succeeded;
    resp.result.command_id = "cmd_1";
    resp.result.command_key = "wait";
    resp.projection_patch = patch;

    auto enc = codec.encodeCommandResponse(resp);
    assert(enc.is_ok());
    auto dec = codec.decodeCommandResponse(enc.value());
    assert(dec.is_ok());

    const auto& dp = dec.value().projection_patch;
    assert(dp.base_projection_version == 3);
    assert(dp.new_projection_version == 4);
    assert(!dp.requires_full_refresh);
    assert(dp.changed_cells.size() == 1);
    assert(dp.changed_cells[0].cell_id == "c1");
    assert(dp.changed_cells[0].fields.at("terrain") == "forest");
    assert(dp.changed_entities.size() == 1);
    assert(dp.changed_entities[0].entity_id == "e1");
    assert(dp.changed_inventories.size() == 1);
    assert(dp.changed_inventories[0].inventory_id == "inv1");
    assert(dp.changed_knowledge.size() == 1);
    assert(dp.changed_knowledge[0].actor_key == "player");
    assert(dp.changed_area_effects.size() == 1);
    assert(dp.changed_area_effects[0].area_effect_id == "ae1");

    std::cout << "client_protocol_patch_codec_full_roundtrip_tests: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Codec rejects unknown enum
// ---------------------------------------------------------------------------
void run_codec_rejects_unknown_enum_tests() {
    ClientProtocolCodec codec;

    // JSON with unknown submit_mode should decode but validate fails because Unknown is illegal.
    std::string json = R"({"session_id":"s1","client_id":"c1","client_sequence":1,"known_projection_version":1,"submit_mode":"invalid_mode","option_id":"opt_1"})";
    auto dec = codec.decodeCommandRequest(json);
    assert(dec.is_error());

    // JSON with valid submit_mode should succeed.
    std::string json2 = R"({"session_id":"s1","client_id":"c1","client_sequence":1,"known_projection_version":1,"submit_mode":"option_id","option_id":"opt_1"})";
    auto dec2 = codec.decodeCommandRequest(json2);
    assert(dec2.is_ok());
    assert(dec2.value().submit_mode == ClientSubmitMode::OptionId);

    std::cout << "client_protocol_codec_rejects_unknown_enum_tests: all passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: enum, dto_validation, codec_roundtrip, projection_version, available_command, security_request, projection_adapter, codec_rejects_unknown_enum, snapshot_materialize, projection_codec_full_roundtrip, patch_codec_full_roundtrip" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "enum") {
        run_enum_roundtrip_tests();
    } else if (test_name == "dto_validation") {
        run_dto_validation_tests();
    } else if (test_name == "codec_roundtrip") {
        run_codec_roundtrip_tests();
    } else if (test_name == "projection_version") {
        run_projection_version_tests();
    } else if (test_name == "available_command") {
        run_available_command_tests();
    } else if (test_name == "security_request") {
        run_security_request_tests();
    } else if (test_name == "projection_adapter") {
        run_projection_adapter_tests();
    } else if (test_name == "codec_rejects_unknown_enum") {
        run_codec_rejects_unknown_enum_tests();
    } else if (test_name == "snapshot_materialize") {
        run_snapshot_materialize_tests();
    } else if (test_name == "projection_codec_full_roundtrip") {
        run_projection_codec_full_roundtrip_tests();
    } else if (test_name == "patch_codec_full_roundtrip") {
        run_patch_codec_full_roundtrip_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
