#pragma once

#include "pathfinder/reaction/reaction_content.h"
#include "pathfinder/reaction/reaction_resolver.h"

namespace pathfinder::reaction {

struct ReactionApplyOptions {
    std::string default_location_key{"world"};
    std::string generated_object_prefix{"obj_generated"};
    int generated_index_start{1};
};

struct ReactionApplyTrace {
    std::vector<std::string> applied_change_keys;
    std::vector<std::string> generated_object_ids;
    std::vector<std::string> consumed_object_ids;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionApplyResult {
    ReactionRuntimeWorld world;
    ReactionApplyTrace trace;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class ReactionChangeApplier {
public:
    pathfinder::foundation::Result<ReactionApplyResult> apply(
        const ReactionRuntimeWorld& world,
        const ItemCatalog& catalog,
        const ReactionResult& result,
        const ReactionApplyOptions& options = {}) const;
};

} // namespace pathfinder::reaction
