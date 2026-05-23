#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <set>
#include <sstream>

namespace pathfinder::world_reaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorDetail;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using namespace pathfinder::world_command;
using namespace pathfinder::content;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

WorldReactionService::WorldReactionService(
    const ContentRegistry& content_registry,
    IWorldRuntime& world_runtime,
    IInventoryRuntime& inventory_runtime,
    IWorldEntityLocationPort& location_port)
    : content_registry_(content_registry)
    , world_runtime_(world_runtime)
    , inventory_runtime_(inventory_runtime)
    , location_port_(location_port) {}

// ---------------------------------------------------------------------------
// Public: execute = validate + apply
// ---------------------------------------------------------------------------

Result<WorldReactionResult> WorldReactionService::execute(const WorldReactionRequest& request) {
    auto draft_res = validate(request);
    if (draft_res.is_error()) {
        WorldReactionResult result;
        result.ok = false;
        const auto& errors = draft_res.errors();
        if (!errors.empty()) {
            const std::string& mk = errors[0].message_key;
            if (mk == "invalid_request")       result.failure_kind = WorldReactionFailureKind::InvalidRequest;
            else if (mk == "actor_missing")    result.failure_kind = WorldReactionFailureKind::ActorMissing;
            else if (mk == "reaction_missing") result.failure_kind = WorldReactionFailureKind::ReactionMissing;
            else if (mk == "reaction_invalid") result.failure_kind = WorldReactionFailureKind::ReactionInvalid;
            else if (mk == "action_mismatch")  result.failure_kind = WorldReactionFailureKind::ActionMismatch;
            else if (mk == "inventory_missing")result.failure_kind = WorldReactionFailureKind::InventoryMissing;
            else if (mk == "input_missing")    result.failure_kind = WorldReactionFailureKind::InputMissing;
            else if (mk == "input_state_mismatch") result.failure_kind = WorldReactionFailureKind::InputStateMismatch;
            else if (mk == "quantity_insufficient") result.failure_kind = WorldReactionFailureKind::QuantityInsufficient;
            else if (mk == "output_invalid")   result.failure_kind = WorldReactionFailureKind::OutputInvalid;
            else if (mk == "output_policy_invalid") result.failure_kind = WorldReactionFailureKind::OutputPolicyInvalid;
            else if (mk == "runtime_conflict") result.failure_kind = WorldReactionFailureKind::RuntimeConflict;
            else                               result.failure_kind = WorldReactionFailureKind::ReactionInvalid;
        } else {
            result.failure_kind = WorldReactionFailureKind::RuntimeConflict;
        }
        return Result<WorldReactionResult>::ok(std::move(result));
    }
    auto draft = std::move(draft_res.value());
    return apply(draft);
}

// ---------------------------------------------------------------------------
// Public: validate
// ---------------------------------------------------------------------------

Result<WorldReactionDraft> WorldReactionService::validate(const WorldReactionRequest& request) const {
    // 1. Basic request validation
    auto basic_res = request.validateBasic();
    if (basic_res.is_error()) {
        return Result<WorldReactionDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "invalid_request",
                      "Request validation failed"));
    }

    WorldReactionDraft draft;
    draft.draft_id = request.reaction_command_id + "_draft";
    draft.request = request;

    // 2. Actor must exist
    auto actor_res = world_runtime_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<WorldReactionDraft>::fail(
            makeError(ErrorCode::id_not_found, "actor_missing",
                      "Actor not found: " + request.actor_key));
    }
    const auto* actor = actor_res.value();

    // 3. Reaction must exist in ContentRegistry
    const auto* reaction = content_registry_.findReaction(request.reaction_key);
    if (reaction == nullptr) {
        return Result<WorldReactionDraft>::fail(
            makeError(ErrorCode::id_not_found, "reaction_missing",
                      "Reaction not found: " + request.reaction_key));
    }
    draft.reaction = *reaction;
    draft.effect_key = reaction->effect_key;
    draft.knowledge_template_keys = reaction->knowledge_templates;

    // 4. Action kind must match reaction action_key (if reaction specifies one)
    // P48 MVP: Craft command is the universal crafting entry point and can drive
    // reactions with action_key: craft, use, combine.
    if (!reaction->action_key.empty()) {
        std::string expected_action = toString(request.action_kind);
        bool action_ok = (expected_action == reaction->action_key);
        // Craft command accepts craft/use/combine reactions
        if (!action_ok && request.action_kind == WorldReactionActionKind::Craft) {
            action_ok = (reaction->action_key == "craft" ||
                         reaction->action_key == "use" ||
                         reaction->action_key == "combine");
        }
        if (!action_ok) {
            return Result<WorldReactionDraft>::fail(
                makeError(ErrorCode::command_invalid_argument, "action_mismatch",
                          "Action mismatch: expected " + reaction->action_key + ", got " + expected_action));
        }
    }

    // 5. Resolve inputs (search actor inventory, same cell, adjacent cells)
    auto input_res = resolveInputs(request, draft);
    if (input_res.is_error()) {
        return Result<WorldReactionDraft>::fail(input_res.errors()[0]);
    }

    // 6. Build consume drafts
    auto consume_res = buildConsumeDrafts(request, draft);
    if (consume_res.is_error()) {
        return Result<WorldReactionDraft>::fail(consume_res.errors()[0]);
    }

    // 7. Build output drafts
    auto output_res = buildOutputDrafts(request, draft);
    if (output_res.is_error()) {
        return Result<WorldReactionDraft>::fail(output_res.errors()[0]);
    }

    return Result<WorldReactionDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// Private: resolveInputs
// ---------------------------------------------------------------------------

Result<void> WorldReactionService::resolveInputs(
    const WorldReactionRequest& request,
    WorldReactionDraft& draft) const {

    std::set<std::string> seen_object_keys;
    const auto* actor = world_runtime_.findActor(request.actor_key).value();

    for (const auto& input : draft.reaction.inputs) {
        // P48 MVP: reject duplicate input object_key
        if (seen_object_keys.count(input.object_key)) {
            return Result<void>::fail(
                makeError(ErrorCode::state_change_invalid, "reaction_invalid",
                          "Duplicate input object_key: " + input.object_key));
        }
        seen_object_keys.insert(input.object_key);

        int required_quantity = (input.min > 0) ? input.min : ((input.quantity > 0) ? input.quantity : 1);
        std::string required_state = input.state;

        WorldReactionInputProof proof;
        proof.object_key = input.object_key;
        proof.required_state = required_state;
        proof.required_quantity = required_quantity;

        bool found = false;
        bool insufficient = false;
        bool state_mismatch = false;

        // 1. Search ActorInventory
        if (!found) {
            InventoryOwnerRef owner{InventoryOwnerKind::Actor, request.actor_key};
            auto items_res = inventory_runtime_.queryItems(owner, input.object_key);
            if (items_res.is_ok() && !items_res.value().empty()) {
                for (const auto& item : items_res.value()) {
                    if (!required_state.empty()) {
                        bool has_state = false;
                        for (const auto& sk : item.state_keys) {
                            if (sk == required_state) { has_state = true; break; }
                        }
                        if (!has_state) {
                            state_mismatch = true;
                            continue;
                        }
                    }
                    if (item.quantity >= required_quantity) {
                        proof.available_quantity = item.quantity;
                        proof.source_kind = WorldReactionInputSourceKind::ActorInventory;
                        proof.entity_id = item.entity_id;
                        proof.inventory_id = "inv_actor_" + request.actor_key;
                        proof.state_keys = item.state_keys;
                        found = true;
                        break;
                    } else {
                        insufficient = true;
                    }
                }
            }
        }

        // 2. Search SameCellMap
        if (!found) {
            auto cell_res = world_runtime_.findCell(actor->coord);
            if (cell_res.is_ok()) {
                const auto* cell = cell_res.value();
                std::vector<std::string> candidate_ids = cell->entity_ids;
                std::sort(candidate_ids.begin(), candidate_ids.end());
                for (const auto& eid : candidate_ids) {
                    auto entity_res = world_runtime_.findEntity(eid);
                    if (entity_res.is_error()) continue;
                    const auto* entity = entity_res.value();
                    if (entity->entity_key != input.object_key) continue;
                    if (entity->location_kind != WorldEntityLocationKind::OnMap) continue;
                    if (!required_state.empty()) {
                        bool has_state = false;
                        for (const auto& sk : entity->state_keys) {
                            if (sk == required_state) { has_state = true; break; }
                        }
                        if (!has_state) {
                            state_mismatch = true;
                            continue;
                        }
                    }
                    if (entity->quantity >= required_quantity) {
                        proof.available_quantity = entity->quantity;
                        proof.source_kind = WorldReactionInputSourceKind::SameCellMap;
                        proof.entity_id = entity->entity_id;
                        proof.coord = actor->coord;
                        proof.state_keys = entity->state_keys;
                        found = true;
                        break;
                    } else {
                        insufficient = true;
                    }
                }
            }
        }

        // 3. Search AdjacentMap (stable order: cell_id, then entity_id)
        if (!found) {
            std::vector<WorldCellCoord> adjacent = {
                WorldCellCoord{actor->coord.x + 1, actor->coord.y, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x - 1, actor->coord.y, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x, actor->coord.y + 1, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x, actor->coord.y - 1, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x + 1, actor->coord.y + 1, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x - 1, actor->coord.y + 1, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x + 1, actor->coord.y - 1, actor->coord.layer_key},
                WorldCellCoord{actor->coord.x - 1, actor->coord.y - 1, actor->coord.layer_key},
            };
            std::sort(adjacent.begin(), adjacent.end(),
                      [](const WorldCellCoord& a, const WorldCellCoord& b) {
                          return a.cellId() < b.cellId();
                      });
            for (const auto& adj : adjacent) {
                auto cell_res = world_runtime_.findCell(adj);
                if (cell_res.is_error()) continue;
                const auto* cell = cell_res.value();
                std::vector<std::string> candidate_ids = cell->entity_ids;
                std::sort(candidate_ids.begin(), candidate_ids.end());
                for (const auto& eid : candidate_ids) {
                    auto entity_res = world_runtime_.findEntity(eid);
                    if (entity_res.is_error()) continue;
                    const auto* entity = entity_res.value();
                    if (entity->entity_key != input.object_key) continue;
                    if (entity->location_kind != WorldEntityLocationKind::OnMap) continue;
                    if (!required_state.empty()) {
                        bool has_state = false;
                        for (const auto& sk : entity->state_keys) {
                            if (sk == required_state) { has_state = true; break; }
                        }
                        if (!has_state) {
                            state_mismatch = true;
                            continue;
                        }
                    }
                    if (entity->quantity >= required_quantity) {
                        proof.available_quantity = entity->quantity;
                        proof.source_kind = WorldReactionInputSourceKind::AdjacentMap;
                        proof.entity_id = entity->entity_id;
                        proof.coord = adj;
                        proof.state_keys = entity->state_keys;
                        found = true;
                        break;
                    } else {
                        insufficient = true;
                    }
                }
                if (found) break;
            }
        }

        if (!found) {
            if (state_mismatch) {
                return Result<void>::fail(
                    makeError(ErrorCode::precondition_environment_mismatch, "input_state_mismatch",
                              "Input state mismatch: " + input.object_key));
            }
            if (insufficient) {
                return Result<void>::fail(
                    makeError(ErrorCode::precondition_insufficient_cost, "quantity_insufficient",
                              "Quantity insufficient: " + input.object_key));
            }
            return Result<void>::fail(
                makeError(ErrorCode::precondition_insufficient_cost, "input_missing",
                          "Input missing: " + input.object_key));
        }

        draft.input_proofs.push_back(std::move(proof));
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Private: findInputInActorInventory
// ---------------------------------------------------------------------------

bool WorldReactionService::findInputInActorInventory(
    const std::string& actor_key,
    const std::string& object_key,
    const std::string& required_state,
    int required_quantity,
    WorldReactionInputProof& out_proof) const {

    InventoryOwnerRef owner{InventoryOwnerKind::Actor, actor_key};
    auto items_res = inventory_runtime_.queryItems(owner, object_key);
    if (items_res.is_error() || items_res.value().empty()) {
        return false;
    }

    for (const auto& item : items_res.value()) {
        // State check
        if (!required_state.empty()) {
            bool has_state = false;
            for (const auto& sk : item.state_keys) {
                if (sk == required_state) { has_state = true; break; }
            }
            if (!has_state) continue;
        }

        // Quantity check
        if (item.quantity >= required_quantity) {
            out_proof.available_quantity = item.quantity;
            out_proof.source_kind = WorldReactionInputSourceKind::ActorInventory;
            out_proof.entity_id = item.entity_id;
            out_proof.inventory_id = "inv_actor_" + actor_key;
            out_proof.state_keys = item.state_keys;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Private: findInputOnMapAtCoord
// ---------------------------------------------------------------------------

bool WorldReactionService::findInputOnMapAtCoord(
    const WorldCellCoord& coord,
    const std::string& object_key,
    const std::string& required_state,
    int required_quantity,
    WorldReactionInputProof& out_proof) const {

    auto cell_res = world_runtime_.findCell(coord);
    if (cell_res.is_error()) return false;
    const auto* cell = cell_res.value();

    // Collect candidate entity_ids and sort for stability
    std::vector<std::string> candidate_ids = cell->entity_ids;
    std::sort(candidate_ids.begin(), candidate_ids.end());

    for (const auto& eid : candidate_ids) {
        auto entity_res = world_runtime_.findEntity(eid);
        if (entity_res.is_error()) continue;
        const auto* entity = entity_res.value();

        if (entity->entity_key != object_key) continue;
        if (entity->location_kind != WorldEntityLocationKind::OnMap) continue;

        // State check
        if (!required_state.empty()) {
            bool has_state = false;
            for (const auto& sk : entity->state_keys) {
                if (sk == required_state) { has_state = true; break; }
            }
            if (!has_state) continue;
        }

        // Quantity check
        if (entity->quantity >= required_quantity) {
            out_proof.available_quantity = entity->quantity;
            out_proof.source_kind = WorldReactionInputSourceKind::SameCellMap;
            out_proof.entity_id = entity->entity_id;
            out_proof.coord = coord;
            out_proof.state_keys = entity->state_keys;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Private: buildConsumeDrafts
// ---------------------------------------------------------------------------

Result<void> WorldReactionService::buildConsumeDrafts(
    const WorldReactionRequest& request,
    WorldReactionDraft& draft) const {

    (void)request;

    for (const auto& consume : draft.reaction.consume) {
        // Find matching proof
        const WorldReactionInputProof* matched_proof = nullptr;
        for (const auto& proof : draft.input_proofs) {
            if (proof.object_key == consume.object_key) {
                matched_proof = &proof;
                break;
            }
        }
        if (matched_proof == nullptr) {
            return Result<void>::fail(
                makeError(ErrorCode::state_change_invalid, "reaction_invalid",
                          "Consume references missing input: " + consume.object_key));
        }

        // State match check on consume
        if (!consume.state.empty()) {
            bool has_state = false;
            for (const auto& sk : matched_proof->state_keys) {
                if (sk == consume.state) { has_state = true; break; }
            }
            if (!has_state) {
                return Result<void>::fail(
                    makeError(ErrorCode::precondition_environment_mismatch, "input_state_mismatch",
                              "Consume state mismatch for: " + consume.object_key));
            }
        }

        // Delta must be negative
        if (consume.delta >= 0) {
            return Result<void>::fail(
                makeError(ErrorCode::state_change_invalid, "reaction_invalid",
                          "Consume delta must be negative for: " + consume.object_key));
        }

        int consume_qty = std::abs(consume.delta);
        if (consume_qty > matched_proof->available_quantity) {
            return Result<void>::fail(
                makeError(ErrorCode::precondition_insufficient_cost, "quantity_insufficient",
                          "Insufficient quantity to consume: " + consume.object_key));
        }

        WorldReactionConsumeDraft cd;
        cd.object_key = consume.object_key;
        cd.entity_id = matched_proof->entity_id;
        cd.quantity = consume_qty;
        cd.source_kind = matched_proof->source_kind;
        cd.inventory_id = matched_proof->inventory_id;
        cd.coord = matched_proof->coord;
        draft.consume_drafts.push_back(std::move(cd));
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Private: buildOutputDrafts
// ---------------------------------------------------------------------------

Result<void> WorldReactionService::buildOutputDrafts(
    const WorldReactionRequest& request,
    WorldReactionDraft& draft) const {

    for (size_t i = 0; i < draft.reaction.outputs.size(); ++i) {
        const auto& output = draft.reaction.outputs[i];

        if (output.object_key.empty()) {
            return Result<void>::fail(
                makeError(ErrorCode::command_invalid_argument, "output_invalid",
                          "Output object_key is empty"));
        }
        if (output.quantity_delta <= 0) {
            return Result<void>::fail(
                makeError(ErrorCode::command_invalid_argument, "output_invalid",
                          "Output quantity_delta must be positive"));
        }

        const auto* obj = content_registry_.findObject(output.object_key);
        if (obj == nullptr) {
            return Result<void>::fail(
                makeError(ErrorCode::id_not_found, "output_invalid",
                          "Output object not found: " + output.object_key));
        }

        WorldReactionOutputDraft od;
        od.entity_id = request.reaction_command_id + "_" + output.object_key + "_" + std::to_string(i);
        od.entity_key = output.object_key;
        od.display_name_key = obj->display_key;
        od.quantity = output.quantity_delta;
        od.numeric_states = obj->default_numeric;
        od.stackable = od.numeric_states.empty();
        od.stack_key = od.stackable ? output.object_key + ":crafted" : output.object_key + ":crafted:" + od.entity_id;
        od.state_keys = {"crafted"};
        od.tag_keys = {"reaction_output"};
        od.location_policy = request.output_location_policy;
        draft.output_drafts.push_back(std::move(od));
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Public: apply
// ---------------------------------------------------------------------------

Result<WorldReactionResult> WorldReactionService::apply(WorldReactionDraft& draft) {
    WorldReactionResult result;
    result.ok = false;
    result.failure_kind = WorldReactionFailureKind::None;

    // 1. Re-verify actor still exists
    auto actor_res = world_runtime_.findActor(draft.request.actor_key);
    if (actor_res.is_error()) {
        result.failure_kind = WorldReactionFailureKind::RuntimeConflict;
        return Result<WorldReactionResult>::ok(std::move(result));
    }
    const auto* actor = actor_res.value();

    // 2. Re-verify inputs before consuming
    for (const auto& cd : draft.consume_drafts) {
        if (cd.source_kind == WorldReactionInputSourceKind::ActorInventory) {
            auto inv_res = inventory_runtime_.findInventory(cd.inventory_id);
            if (inv_res.is_error()) {
                result.failure_kind = WorldReactionFailureKind::RuntimeConflict;
                return Result<WorldReactionResult>::ok(std::move(result));
            }
            bool found = false;
            for (const auto& entry : inv_res.value()->entries) {
                if (entry.entity_id == cd.entity_id && entry.quantity >= cd.quantity) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.failure_kind = WorldReactionFailureKind::QuantityInsufficient;
                return Result<WorldReactionResult>::ok(std::move(result));
            }
        } else {
            auto entity_res = world_runtime_.findEntity(cd.entity_id);
            if (entity_res.is_error() || entity_res.value()->quantity < cd.quantity) {
                result.failure_kind = WorldReactionFailureKind::QuantityInsufficient;
                return Result<WorldReactionResult>::ok(std::move(result));
            }
        }
    }

    // 3. Apply consumes
    auto consume_apply_res = applyConsumes(draft, result);
    if (consume_apply_res.is_error()) {
        result.failure_kind = WorldReactionFailureKind::ConsumeFailed;
        return Result<WorldReactionResult>::ok(std::move(result));
    }

    // 4. Apply spawns
    auto spawn_apply_res = applySpawns(draft, result);
    if (spawn_apply_res.is_error()) {
        result.failure_kind = WorldReactionFailureKind::SpawnFailed;
        return Result<WorldReactionResult>::ok(std::move(result));
    }

    // 5. Build events
    WorldEventDto success_event;
    success_event.event_id = draft.request.reaction_command_id + "_crafted";
    success_event.event_kind = "WorldReactionCrafted";
    success_event.tick = 0; // caller sets if available
    success_event.title_text = "制作成功";
    success_event.body_text = "Reaction succeeded: " + draft.request.reaction_key;
    success_event.actor_key = draft.request.actor_key;
    result.events.push_back(std::move(success_event));

    // Build state deltas
    for (const auto& cd : draft.consume_drafts) {
        WorldStateDeltaDto delta;
        delta.delta_id = draft.request.reaction_command_id + "_consume_" + cd.object_key;
        delta.delta_kind = "item_consumed";
        delta.target_ref = cd.entity_id;
        delta.op = PatchOp::Update;
        delta.fields["quantity_delta"] = std::to_string(-cd.quantity);
        delta.fields["object_key"] = cd.object_key;
        result.state_deltas.push_back(std::move(delta));
    }
    for (const auto& od : draft.output_drafts) {
        WorldStateDeltaDto delta;
        delta.delta_id = draft.request.reaction_command_id + "_spawn_" + od.entity_key;
        delta.delta_kind = "item_created";
        delta.target_ref = od.entity_id;
        delta.op = PatchOp::Add;
        delta.fields["quantity"] = std::to_string(od.quantity);
        delta.fields["object_key"] = od.entity_key;
        delta.fields["location_policy"] = toString(od.location_policy);
        result.state_deltas.push_back(std::move(delta));
    }

    // Build experience
    if (!draft.output_drafts.empty()) {
        WorldExperienceDto exp;
        exp.experience_id = draft.request.reaction_command_id + "_experience";
        exp.actor_key = draft.request.actor_key;
        exp.command_key = "craft";
        exp.subject_entity_key = draft.output_drafts[0].entity_key;
        exp.target_entity_key = draft.request.reaction_key;
        exp.effect_key = draft.effect_key;
        exp.reason_keys.push_back(draft.request.reaction_key);
        for (const auto& kt : draft.knowledge_template_keys) {
            exp.reason_keys.push_back(kt);
        }
        result.experiences.push_back(std::move(exp));
    }

    result.ok = true;
    return Result<WorldReactionResult>::ok(std::move(result));
}

// ---------------------------------------------------------------------------
// Private: applyConsumes
// ---------------------------------------------------------------------------

Result<void> WorldReactionService::applyConsumes(WorldReactionDraft& draft, WorldReactionResult& result) {
    for (auto& cd : draft.consume_drafts) {
        if (cd.source_kind == WorldReactionInputSourceKind::ActorInventory) {
            InventoryTransferRequest req;
            req.transfer_id = draft.request.reaction_command_id + "_consume_" + cd.object_key;
            req.transfer_kind = InventoryTransferKind::ConsumeToNowhere;
            req.actor_key = draft.request.actor_key;
            req.entity_id = cd.entity_id;
            req.quantity = cd.quantity;
            req.from.inventory_id = cd.inventory_id;

            auto transfer_res = inventory_runtime_.transfer(req);
            if (transfer_res.is_error() || !transfer_res.value().ok) {
                return Result<void>::fail(
                    makeError(ErrorCode::state_change_invalid, "consume_failed",
                              "Failed to consume: " + cd.object_key));
            }
            auto& tresult = transfer_res.value();
            for (const auto& id : tresult.changed_inventory_ids) {
                if (std::find(result.changed_inventory_ids.begin(), result.changed_inventory_ids.end(), id) == result.changed_inventory_ids.end()) {
                    result.changed_inventory_ids.push_back(id);
                }
            }
            for (const auto& id : tresult.changed_entity_ids) {
                if (std::find(result.changed_entity_ids.begin(), result.changed_entity_ids.end(), id) == result.changed_entity_ids.end()) {
                    result.changed_entity_ids.push_back(id);
                }
            }
        } else {
            // OnMap consume via location port
            auto entity_res = world_runtime_.findEntity(cd.entity_id);
            if (entity_res.is_error()) {
                return Result<void>::fail(
                    makeError(ErrorCode::id_not_found, "consume_failed",
                              "Entity no longer exists: " + cd.entity_id));
            }
            const auto* entity = entity_res.value();
            if (cd.quantity < entity->quantity) {
                int new_qty = entity->quantity - cd.quantity;
                auto set_res = location_port_.setEntityStackData(cd.entity_id, new_qty, entity->stack_key, entity->stackable);
                if (set_res.is_error()) {
                    return Result<void>::fail(set_res.errors()[0]);
                }
            } else {
                auto destroy_res = location_port_.destroyEntity(cd.entity_id);
                if (destroy_res.is_error()) {
                    return Result<void>::fail(destroy_res.errors()[0]);
                }
            }
            if (std::find(result.changed_entity_ids.begin(), result.changed_entity_ids.end(), cd.entity_id) == result.changed_entity_ids.end()) {
                result.changed_entity_ids.push_back(cd.entity_id);
            }
            if (cd.coord.has_value()) {
                std::string cell_id = cd.coord.value().cellId();
                if (std::find(result.changed_cell_ids.begin(), result.changed_cell_ids.end(), cell_id) == result.changed_cell_ids.end()) {
                    result.changed_cell_ids.push_back(cell_id);
                }
            }
        }
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Private: applySpawns
// ---------------------------------------------------------------------------

Result<void> WorldReactionService::applySpawns(WorldReactionDraft& draft, WorldReactionResult& result) {
    auto actor_res = world_runtime_.findActor(draft.request.actor_key);
    if (actor_res.is_error()) {
        return Result<void>::fail(actor_res.errors()[0]);
    }
    const auto* actor = actor_res.value();

    for (auto& od : draft.output_drafts) {
        if (od.location_policy == WorldReactionOutputLocationPolicy::ActorInventory) {
            InventoryTransferRequest req;
            req.transfer_id = draft.request.reaction_command_id + "_spawn_" + od.entity_key;
            req.transfer_kind = InventoryTransferKind::SpawnToInventory;
            req.actor_key = draft.request.actor_key;
            req.entity_id = od.entity_id;
            req.entity_key = od.entity_key;
            req.quantity = od.quantity;
            req.parameters["stack_key"] = od.stack_key;
            req.parameters["stackable"] = od.stackable ? "true" : "false";
            req.parameters["display_name_key"] = od.display_name_key;
            if (!od.state_keys.empty()) {
                std::stringstream ss;
                for (size_t i = 0; i < od.state_keys.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << od.state_keys[i];
                }
                req.parameters["state_keys"] = ss.str();
            }
            if (!od.tag_keys.empty()) {
                std::stringstream ss;
                for (size_t i = 0; i < od.tag_keys.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << od.tag_keys[i];
                }
                req.parameters["tag_keys"] = ss.str();
            }
            if (!od.numeric_states.empty()) {
                std::stringstream ss;
                size_t i = 0;
                for (const auto& [key, value] : od.numeric_states) {
                    if (i++ > 0) ss << ",";
                    ss << key << "=" << value;
                }
                req.parameters["numeric_states"] = ss.str();
            }

            auto transfer_res = inventory_runtime_.transfer(req);
            if (transfer_res.is_error() || !transfer_res.value().ok) {
                return Result<void>::fail(
                    makeError(ErrorCode::state_change_invalid, "spawn_failed",
                              "Failed to spawn to inventory: " + od.entity_key));
            }
            auto& tresult = transfer_res.value();
            for (const auto& id : tresult.changed_inventory_ids) {
                if (std::find(result.changed_inventory_ids.begin(), result.changed_inventory_ids.end(), id) == result.changed_inventory_ids.end()) {
                    result.changed_inventory_ids.push_back(id);
                }
            }
            for (const auto& id : tresult.changed_entity_ids) {
                if (std::find(result.changed_entity_ids.begin(), result.changed_entity_ids.end(), id) == result.changed_entity_ids.end()) {
                    result.changed_entity_ids.push_back(id);
                }
            }
        } else if (od.location_policy == WorldReactionOutputLocationPolicy::DropOnMap) {
            auto spawn_res = location_port_.spawnEntityOnMap(
                od.entity_id,
                od.entity_key,
                od.display_name_key,
                actor->coord,
                od.quantity,
                od.stack_key,
                od.stackable,
                od.state_keys,
                od.numeric_states,
                od.tag_keys);
            if (spawn_res.is_error()) {
                return Result<void>::fail(spawn_res.errors()[0]);
            }
            if (std::find(result.changed_entity_ids.begin(), result.changed_entity_ids.end(), od.entity_id) == result.changed_entity_ids.end()) {
                result.changed_entity_ids.push_back(od.entity_id);
            }
            std::string cell_id = actor->coord.cellId();
            if (std::find(result.changed_cell_ids.begin(), result.changed_cell_ids.end(), cell_id) == result.changed_cell_ids.end()) {
                result.changed_cell_ids.push_back(cell_id);
            }
        } else {
            return Result<void>::fail(
                makeError(ErrorCode::command_invalid_argument, "output_policy_invalid",
                          "Unknown output location policy"));
        }
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_reaction
