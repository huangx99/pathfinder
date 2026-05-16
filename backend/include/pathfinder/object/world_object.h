#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/object/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace pathfinder::object {

using pathfinder::foundation::ObjectId;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ActionId;
using pathfinder::foundation::Result;

struct ObjectDefinition {
    ObjectDefinitionId definition_id;
    ObjectCategory category = ObjectCategory::Unknown;
    std::vector<ActionId> allowed_action_ids;
    ObjectEdibleProfile edible_profile;

    Result<void> validateBasic() const;
};

struct WorldObject {
    ObjectId object_id;
    ObjectDefinitionId definition_id;
    int quantity = 1;
    ObjectRuntimeFlag flag = ObjectRuntimeFlag::None;

    Result<void> validateBasic() const;
};

class ObjectStore {
public:
    Result<void> addDefinition(ObjectDefinition def);
    Result<void> addObject(WorldObject obj);

    const ObjectDefinition* findDefinition(const ObjectDefinitionId& id) const;
    WorldObject* findObject(const ObjectId& id);
    const WorldObject* findObject(const ObjectId& id) const;

    Result<void> markConsumed(const ObjectId& id);

    const std::vector<ObjectDefinition>& definitions() const { return definitions_; }
    const std::vector<WorldObject>& objects() const { return objects_; }

    Result<void> validateBasic() const;

private:
    std::vector<ObjectDefinition> definitions_;
    std::vector<WorldObject> objects_;
};

} // namespace pathfinder::object
