#include "pathfinder/object/world_object.h"

namespace pathfinder::object {

Result<void> ObjectDefinition::validateBasic() const {
    if (definition_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "ObjectDefinition: definition_id is empty"));
    }
    return Result<void>::ok();
}

Result<void> WorldObject::validateBasic() const {
    if (object_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "WorldObject: object_id is empty"));
    }
    if (definition_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "WorldObject: definition_id is empty"));
    }
    if (quantity < 0) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "WorldObject: quantity must be >= 0"));
    }
    return Result<void>::ok();
}

Result<void> ObjectStore::addDefinition(ObjectDefinition def) {
    auto vr = def.validateBasic();
    if (vr.is_error()) return vr;
    for (const auto& existing : definitions_) {
        if (existing.definition_id == def.definition_id) {
            return Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "ObjectStore: duplicate definition_id"));
        }
    }
    definitions_.push_back(std::move(def));
    return Result<void>::ok();
}

Result<void> ObjectStore::addObject(WorldObject obj) {
    auto vr = obj.validateBasic();
    if (vr.is_error()) return vr;
    for (const auto& existing : objects_) {
        if (existing.object_id == obj.object_id) {
            return Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "ObjectStore: duplicate object_id"));
        }
    }
    objects_.push_back(std::move(obj));
    return Result<void>::ok();
}

const ObjectDefinition* ObjectStore::findDefinition(const ObjectDefinitionId& id) const {
    for (const auto& d : definitions_) {
        if (d.definition_id == id) return &d;
    }
    return nullptr;
}

WorldObject* ObjectStore::findObject(const ObjectId& id) {
    for (auto& o : objects_) {
        if (o.object_id == id) return &o;
    }
    return nullptr;
}

const WorldObject* ObjectStore::findObject(const ObjectId& id) const {
    for (const auto& o : objects_) {
        if (o.object_id == id) return &o;
    }
    return nullptr;
}

Result<void> ObjectStore::markConsumed(const ObjectId& id) {
    auto* obj = findObject(id);
    if (!obj) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "ObjectStore: object not found for markConsumed"));
    }
    obj->flag = ObjectRuntimeFlag::Consumed;
    return Result<void>::ok();
}

Result<void> ObjectStore::validateBasic() const {
    for (const auto& d : definitions_) {
        auto r = d.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& o : objects_) {
        auto r = o.validateBasic();
        if (r.is_error()) return r;
        if (!findDefinition(o.definition_id)) {
            return Result<void>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                    "ObjectStore: object references non-existent definition"));
        }
    }
    return Result<void>::ok();
}

} // namespace pathfinder::object
