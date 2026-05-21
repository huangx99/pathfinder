#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::foundation;

void run_world_agent_execution_enum_roundtrip_tests() {
    std::cout << "Running world_agent_execution enum roundtrip tests:" << std::endl;

    // WorldAgentExecutionDecision
    assert(toString(WorldAgentExecutionDecision::Unknown) == "unknown");
    assert(toString(WorldAgentExecutionDecision::NoopNoGoal) == "noop_no_goal");
    assert(toString(WorldAgentExecutionDecision::IssuedCommand) == "issued_command");
    assert(toString(WorldAgentExecutionDecision::TestOnly) == "test_only");

    auto d1 = worldAgentExecutionDecisionFromString("unknown");
    assert(d1.is_ok() && d1.value() == WorldAgentExecutionDecision::Unknown);
    auto d2 = worldAgentExecutionDecisionFromString("noop_no_goal");
    assert(d2.is_ok() && d2.value() == WorldAgentExecutionDecision::NoopNoGoal);
    auto d_bad = worldAgentExecutionDecisionFromString("not_a_real_value");
    assert(d_bad.is_error());

    // WorldAgentExecutionFailureKind
    assert(toString(WorldAgentExecutionFailureKind::None) == "none");
    assert(toString(WorldAgentExecutionFailureKind::KnowledgeMissing) == "knowledge_missing");
    assert(toString(WorldAgentExecutionFailureKind::TestOnly) == "test_only");

    auto f1 = worldAgentExecutionFailureKindFromString("none");
    assert(f1.is_ok() && f1.value() == WorldAgentExecutionFailureKind::None);
    auto f2 = worldAgentExecutionFailureKindFromString("actor_missing");
    assert(f2.is_ok() && f2.value() == WorldAgentExecutionFailureKind::ActorMissing);
    auto f_bad = worldAgentExecutionFailureKindFromString("invalid");
    assert(f_bad.is_error());

    // WorldAgentGoalSourceKind
    assert(toString(WorldAgentGoalSourceKind::InternalNeed) == "internal_need");
    assert(toString(WorldAgentGoalSourceKind::ExternalInterrupt) == "external_interrupt");

    auto g1 = worldAgentGoalSourceKindFromString("internal_need");
    assert(g1.is_ok() && g1.value() == WorldAgentGoalSourceKind::InternalNeed);
    auto g_bad = worldAgentGoalSourceKindFromString("invalid");
    assert(g_bad.is_error());

    // WorldAgentStepCommandKind
    assert(toString(WorldAgentStepCommandKind::Move) == "move");
    assert(toString(WorldAgentStepCommandKind::Gather) == "gather");
    assert(toString(WorldAgentStepCommandKind::TestOnly) == "test_only");

    auto s1 = worldAgentStepCommandKindFromString("move");
    assert(s1.is_ok() && s1.value() == WorldAgentStepCommandKind::Move);
    auto s2 = worldAgentStepCommandKindFromString("craft");
    assert(s2.is_ok() && s2.value() == WorldAgentStepCommandKind::Craft);
    auto s_bad = worldAgentStepCommandKindFromString("invalid");
    assert(s_bad.is_error());

    // Forbidden keys
    assert(containsWorldAgentExecutionForbiddenKey("hidden_truth"));
    assert(!containsWorldAgentExecutionForbiddenKey("safe_summary"));
    assert(containsWorldAgentExecutionForbiddenKey("some_hidden_truth_here"));

    std::cout << "All enum roundtrip tests passed!" << std::endl;
}
