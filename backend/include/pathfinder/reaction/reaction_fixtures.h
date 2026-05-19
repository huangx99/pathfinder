#pragma once

#include "pathfinder/reaction/reaction_resolver.h"

namespace pathfinder::reaction::fixtures {

ReactionObjectRef fireSource(bool burning = true);
ReactionObjectRef dryBranch();
ReactionObjectRef wetBranch();
ReactionObjectRef waterPortion();

ObjectReactionRule fireDryBranchToTorchRule();
ObjectReactionRule waterExtinguishFireRule();

ReactionInputSet fireBranchInput(const ReactionObjectRef& branch);
ReactionInputSet waterFireInput();
std::vector<ObjectReactionRule> coreP28Rules();

} // namespace pathfinder::reaction::fixtures
