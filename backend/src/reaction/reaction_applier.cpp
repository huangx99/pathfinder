#include "pathfinder/reaction/reaction_applier.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::ObjectId;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static bool containsValue(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

static void addUnique(std::vector<std::string>& values, const std::string& value) {
    if (!containsValue(values, value)) values.push_back(value);
}

static void removeValue(std::vector<std::string>& values, const std::string& value) {
    values.erase(std::remove(values.begin(), values.end(), value), values.end());
}

static Result<ObjectId> nextGeneratedId(const ReactionRuntimeWorld& world, const ReactionApplyOptions& options, int offset) {
    if (options.generated_object_prefix.empty() || containsReactionForbiddenKey(options.generated_object_prefix)) {
        return Result<ObjectId>::fail(makeError(ErrorCode::validation_failed, "ReactionApplyOptions generated_object_prefix invalid"));
    }
    if (options.generated_index_start < 0) {
        return Result<ObjectId>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionApplyOptions generated_index_start invalid"));
    }
    int index = options.generated_index_start + offset;
    while (true) {
        ObjectId id(options.generated_object_prefix + "_" + std::to_string(index));
        if (!isValidIdString(id.value())) {
            return Result<ObjectId>::fail(makeError(ErrorCode::id_invalid_format, "ReactionApplyOptions generated object id invalid"));
        }
        if (!world.findObject(id)) return Result<ObjectId>::ok(std::move(id));
        ++index;
    }
}

Result<void> ReactionApplyTrace::validateBasic() const {
    if (containsReactionForbiddenKey(applied_change_keys) || containsReactionForbiddenKey(generated_object_ids) || containsReactionForbiddenKey(consumed_object_ids)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionApplyTrace contains forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> ReactionApplyResult::validateBasic() const {
    auto world_valid = world.validateBasic();
    if (world_valid.is_error()) return world_valid;
    return trace.validateBasic();
}

Result<ReactionApplyResult> ReactionChangeApplier::apply(
    const ReactionRuntimeWorld& world,
    const ItemCatalog& catalog,
    const ReactionResult& result,
    const ReactionApplyOptions& options) const {

    auto world_valid = world.validateBasic();
    if (world_valid.is_error()) return Result<ReactionApplyResult>::fail(world_valid.errors());
    auto catalog_valid = catalog.validateBasic();
    if (catalog_valid.is_error()) return Result<ReactionApplyResult>::fail(catalog_valid.errors());
    auto result_valid = result.validateBasic();
    if (result_valid.is_error()) return Result<ReactionApplyResult>::fail(result_valid.errors());
    if (containsReactionForbiddenKey(options.default_location_key)) {
        return Result<ReactionApplyResult>::fail(makeError(ErrorCode::validation_failed, "ReactionApplyOptions default_location_key forbidden"));
    }

    ReactionApplyResult apply_result;
    apply_result.world = world;
    int produced_count = 0;

    for (const auto& change : result.object_changes) {
        apply_result.trace.applied_change_keys.push_back(toString(change.change_kind));

        if (change.change_kind == ReactionOutputKind::ProduceObject) {
            auto id = nextGeneratedId(apply_result.world, options, produced_count);
            if (id.is_error()) return Result<ReactionApplyResult>::fail(id.errors());
            const int quantity = change.quantity_delta > 0 ? change.quantity_delta : 1;
            auto object = catalog.instantiate(id.value(), change.to_definition_id, options.default_location_key, quantity);
            if (object.is_error()) return Result<ReactionApplyResult>::fail(object.errors());
            auto add = apply_result.world.addObject(std::move(object.value()));
            if (add.is_error()) return Result<ReactionApplyResult>::fail(add.errors());
            apply_result.trace.generated_object_ids.push_back(id.value().value());
            ++produced_count;
            continue;
        }

        auto* object = apply_result.world.findObject(change.object_id);
        if (!object) return Result<ReactionApplyResult>::fail(makeError(ErrorCode::id_not_found, "ReactionChangeApplier target object not found"));

        switch (change.change_kind) {
            case ReactionOutputKind::TransformObject: {
                const auto* item = catalog.find(change.to_definition_id);
                if (!item) return Result<ReactionApplyResult>::fail(makeError(ErrorCode::id_definition_not_found, "ReactionChangeApplier transform definition missing"));
                object->definition_id = item->definition_id;
                object->object_key = item->object_key;
                object->category = item->category;
                object->tag_keys = item->tag_keys;
                object->state_keys = item->default_state_keys;
                object->resource_values = item->default_resource_values;
                if (change.quantity_delta != 0) object->quantity += change.quantity_delta;
                break;
            }
            case ReactionOutputKind::ConsumeObject: {
                if (change.quantity_delta == 0) object->quantity = 0;
                else object->quantity += change.quantity_delta;
                if (object->quantity < 0) object->quantity = 0;
                if (object->quantity == 0) {
                    addUnique(object->state_keys, "consumed");
                    apply_result.trace.consumed_object_ids.push_back(object->object_id.value());
                }
                break;
            }
            case ReactionOutputKind::AddState:
                addUnique(object->state_keys, change.state_key);
                break;
            case ReactionOutputKind::RemoveState:
                removeValue(object->state_keys, change.state_key);
                break;
            case ReactionOutputKind::ResourceDelta: {
                if (!std::isfinite(change.resource_delta)) {
                    return Result<ReactionApplyResult>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionChangeApplier resource delta invalid"));
                }
                object->resource_values[change.resource_key] += change.resource_delta;
                break;
            }
            case ReactionOutputKind::FeedbackOnly:
            case ReactionOutputKind::KnowledgeOnly:
                break;
            case ReactionOutputKind::Unknown:
            case ReactionOutputKind::TestOnly:
                return Result<ReactionApplyResult>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionChangeApplier change kind invalid"));
        }

        auto object_valid = object->validateBasic();
        if (object_valid.is_error()) return Result<ReactionApplyResult>::fail(object_valid.errors());
    }

    auto valid = apply_result.validateBasic();
    if (valid.is_error()) return Result<ReactionApplyResult>::fail(valid.errors());
    return Result<ReactionApplyResult>::ok(std::move(apply_result));
}

} // namespace pathfinder::reaction
