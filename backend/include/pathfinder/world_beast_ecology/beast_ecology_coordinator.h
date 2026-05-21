#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include "pathfinder/world_beast_ecology/beast_perception_builder.h"
#include "pathfinder/world_beast_ecology/beast_instinct_gate.h"
#include "pathfinder/world_beast_ecology/beast_decision_policy.h"
#include "pathfinder/world_beast_ecology/beast_command_compiler.h"
#include "pathfinder/world_beast_ecology/beast_interrupt_projector.h"
#include "pathfinder/world_beast_ecology/beast_projection_bridge.h"

namespace pathfinder::world_beast_ecology {

class BeastEcologyCoordinator {
public:
    BeastEcologyCoordinator(
        BeastPerceptionBuilder& perception_builder,
        BeastInstinctGate& instinct_gate,
        BeastDecisionPolicy& decision_policy,
        BeastCommandCompiler& command_compiler,
        BeastInterruptProjector& interrupt_projector,
        BeastProjectionBridge& projection_bridge,
        IBeastProfileQueryPort& profile_port,
        IBeastWorldQueryPort& world_port,
        IBeastKnowledgeQueryPort& knowledge_port,
        IBeastCommandPipelinePort& pipeline_port);

    pathfinder::foundation::Result<BeastTickResult> tick(const BeastTickRequest& request);

private:
    BeastPerceptionBuilder& perception_builder_;
    BeastInstinctGate& instinct_gate_;
    BeastDecisionPolicy& decision_policy_;
    BeastCommandCompiler& command_compiler_;
    BeastInterruptProjector& interrupt_projector_;
    BeastProjectionBridge& projection_bridge_;
    IBeastProfileQueryPort& profile_port_;
    IBeastWorldQueryPort& world_port_;
    IBeastKnowledgeQueryPort& knowledge_port_;
    IBeastCommandPipelinePort& pipeline_port_;

    pathfinder::foundation::Result<BeastTickResult> makeFailureResult(
        const BeastTickRequest& request,
        const std::vector<std::string>& reason_keys);
};

} // namespace pathfinder::world_beast_ecology
