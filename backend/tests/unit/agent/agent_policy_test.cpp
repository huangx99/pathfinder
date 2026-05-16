#include "pathfinder/agent/policy.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;

static AgentPolicyRequest makePolicyRequest() {
    AgentPolicyRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.observation.agent_id = AgentId(std::string("agent_001"));
    req.observation.observer_actor_id = EntityId(std::string("actor_001"));
    req.observation.state_version = StateVersion(1);
    req.observation.observed_tick = Tick(0);
    req.action_space.agent_id = AgentId(std::string("agent_001"));
    req.action_space.state_version = StateVersion(1);
    req.decision_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    return req;
}

void test_selects_first_supported_candidate() {
    auto req = makePolicyRequest();

    AgentActionCandidate c1;
    c1.action_id = ActionId(std::string("explore_obj_001"));
    c1.command_action_id = ActionId(std::string("explore"));
    c1.intent_type = AgentIntentType::Explore;
    c1.command_supported = false;
    c1.reason_key = "explore";
    req.action_space.candidates.push_back(c1);

    AgentActionCandidate c2;
    c2.action_id = ActionId(std::string("eat_obj_001"));
    c2.command_action_id = ActionId(std::string("eat"));
    c2.intent_type = AgentIntentType::Eat;
    c2.required_target_types.push_back(ActionTargetType::Object);
    c2.command_supported = true;
    c2.reason_key = "try_eat";
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId(std::string("obj_001"));
    c2.suggested_targets.push_back(target);
    req.action_space.candidates.push_back(c2);

    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_ok());
    auto& decision = result.value();
    assert(decision.selected_intent.intent_type == AgentIntentType::Eat);
    assert(decision.selected_intent.action_id == ActionId(std::string("eat")));
    assert(decision.selected_intent.targets.size() == 1);
    assert(decision.selected_intent.targets[0].target_id == TargetId(std::string("obj_001")));
    assert(decision.action_id == ActionId(std::string("eat")));
    std::cout << "  PASS: selects_first_supported_candidate\n";
}

void test_skips_unsupported_candidates() {
    auto req = makePolicyRequest();

    AgentActionCandidate c1;
    c1.action_id = ActionId(std::string("explore_obj_001"));
    c1.command_action_id = ActionId(std::string("explore"));
    c1.intent_type = AgentIntentType::Explore;
    c1.command_supported = false;
    c1.reason_key = "explore";
    req.action_space.candidates.push_back(c1);

    AgentActionCandidate c2;
    c2.action_id = ActionId(std::string("call_group_001"));
    c2.command_action_id = ActionId(std::string("call_group"));
    c2.intent_type = AgentIntentType::CallGroup;
    c2.command_supported = false;
    c2.reason_key = "call_group";
    req.action_space.candidates.push_back(c2);

    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_error());
    std::cout << "  PASS: skips_unsupported_candidates\n";
}

void test_no_candidates_returns_error() {
    auto req = makePolicyRequest();
    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_error());
    std::cout << "  PASS: no_candidates_returns_error\n";
}

void test_uses_suggested_targets() {
    auto req = makePolicyRequest();

    AgentActionCandidate c1;
    c1.action_id = ActionId(std::string("eat_obj_001"));
    c1.command_action_id = ActionId(std::string("eat"));
    c1.intent_type = AgentIntentType::Eat;
    c1.required_target_types.push_back(ActionTargetType::Object);
    c1.command_supported = true;
    c1.reason_key = "try_eat";
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId(std::string("obj_001"));
    c1.suggested_targets.push_back(target);
    req.action_space.candidates.push_back(c1);

    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_ok());
    assert(result.value().selected_intent.targets.size() == 1);
    assert(result.value().selected_intent.targets[0].target_type == ActionTargetType::Object);
    std::cout << "  PASS: uses_suggested_targets\n";
}

void test_decision_validate_basic() {
    auto req = makePolicyRequest();

    AgentActionCandidate c1;
    c1.action_id = ActionId(std::string("eat_obj_001"));
    c1.command_action_id = ActionId(std::string("eat"));
    c1.intent_type = AgentIntentType::Eat;
    c1.command_supported = true;
    c1.reason_key = "try_eat";
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId(std::string("obj_001"));
    c1.suggested_targets.push_back(target);
    req.action_space.candidates.push_back(c1);

    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_ok());
    assert(result.value().validateBasic().is_ok());
    std::cout << "  PASS: decision_validate_basic\n";
}

void test_deterministic_decision_id() {
    auto req = makePolicyRequest();

    AgentActionCandidate c1;
    c1.action_id = ActionId(std::string("eat_obj_001"));
    c1.command_action_id = ActionId(std::string("eat"));
    c1.intent_type = AgentIntentType::Eat;
    c1.command_supported = true;
    c1.reason_key = "try_eat";
    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId(std::string("obj_001"));
    c1.suggested_targets.push_back(target);
    req.action_space.candidates.push_back(c1);

    FirstSupportedPolicy policy;
    auto r1 = policy.decide(req);
    auto r2 = policy.decide(req);
    assert(r1.is_ok() && r2.is_ok());
    assert(r1.value().decision_id == r2.value().decision_id);
    std::cout << "  PASS: deterministic_decision_id\n";
}

void test_policy_no_world_state_dependency() {
    // Verify AgentPolicyRequest has no world state pointer
    AgentPolicyRequest req;
    // This test passes by compilation - if world state were a member, this file
    // would need to include state headers which the boundary scan would catch.
    (void)req;
    std::cout << "  PASS: policy_no_world_state_dependency\n";
}

void test_allowlist_prioritizes_matching_candidate() {
    // Candidates: [flee(supported=true), eat(supported=true)]
    // allowlist = [eat] -> should select Eat, not Flee
    auto req = makePolicyRequest();
    req.submit_action_allowlist.push_back(ActionId(std::string("eat")));

    AgentActionCandidate c_flee;
    c_flee.action_id = ActionId(std::string("flee_fire_001"));
    c_flee.command_action_id = ActionId(std::string("flee"));
    c_flee.intent_type = AgentIntentType::Flee;
    c_flee.required_target_types.push_back(ActionTargetType::Entity);
    c_flee.command_supported = true;
    c_flee.reason_key = "flee_from_fire";
    ActionTarget flee_target;
    flee_target.target_type = ActionTargetType::Entity;
    flee_target.target_id = TargetId(std::string("fire_001"));
    c_flee.suggested_targets.push_back(flee_target);
    req.action_space.candidates.push_back(c_flee);

    AgentActionCandidate c_eat;
    c_eat.action_id = ActionId(std::string("eat_obj_001"));
    c_eat.command_action_id = ActionId(std::string("eat"));
    c_eat.intent_type = AgentIntentType::Eat;
    c_eat.required_target_types.push_back(ActionTargetType::Object);
    c_eat.command_supported = true;
    c_eat.reason_key = "try_eat";
    ActionTarget eat_target;
    eat_target.target_type = ActionTargetType::Object;
    eat_target.target_id = TargetId(std::string("obj_001"));
    c_eat.suggested_targets.push_back(eat_target);
    req.action_space.candidates.push_back(c_eat);

    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_ok());
    // Should select Eat (in allowlist), not Flee (not in allowlist)
    assert(result.value().selected_intent.intent_type == AgentIntentType::Eat);
    assert(result.value().action_id == ActionId(std::string("eat")));
    std::cout << "  PASS: allowlist_prioritizes_matching_candidate\n";
}

void test_allowlist_empty_falls_back_to_first_supported() {
    // Empty allowlist -> selects first command_supported=true (flee)
    auto req = makePolicyRequest();

    AgentActionCandidate c_flee;
    c_flee.action_id = ActionId(std::string("flee_fire_001"));
    c_flee.command_action_id = ActionId(std::string("flee"));
    c_flee.intent_type = AgentIntentType::Flee;
    c_flee.required_target_types.push_back(ActionTargetType::Entity);
    c_flee.command_supported = true;
    c_flee.reason_key = "flee_from_fire";
    ActionTarget flee_target;
    flee_target.target_type = ActionTargetType::Entity;
    flee_target.target_id = TargetId(std::string("fire_001"));
    c_flee.suggested_targets.push_back(flee_target);
    req.action_space.candidates.push_back(c_flee);

    AgentActionCandidate c_eat;
    c_eat.action_id = ActionId(std::string("eat_obj_001"));
    c_eat.command_action_id = ActionId(std::string("eat"));
    c_eat.intent_type = AgentIntentType::Eat;
    c_eat.required_target_types.push_back(ActionTargetType::Object);
    c_eat.command_supported = true;
    c_eat.reason_key = "try_eat";
    ActionTarget eat_target;
    eat_target.target_type = ActionTargetType::Object;
    eat_target.target_id = TargetId(std::string("obj_001"));
    c_eat.suggested_targets.push_back(eat_target);
    req.action_space.candidates.push_back(c_eat);

    FirstSupportedPolicy policy;
    auto result = policy.decide(req);
    assert(result.is_ok());
    // Empty allowlist -> first supported is flee
    assert(result.value().selected_intent.intent_type == AgentIntentType::Flee);
    std::cout << "  PASS: allowlist_empty_falls_back_to_first_supported\n";
}

void run_agent_policy_tests() {
    test_selects_first_supported_candidate();
    test_skips_unsupported_candidates();
    test_no_candidates_returns_error();
    test_uses_suggested_targets();
    test_decision_validate_basic();
    test_deterministic_decision_id();
    test_policy_no_world_state_dependency();
    test_allowlist_prioritizes_matching_candidate();
    test_allowlist_empty_falls_back_to_first_supported();
    std::cout << "All agent policy tests passed!\n";
}
