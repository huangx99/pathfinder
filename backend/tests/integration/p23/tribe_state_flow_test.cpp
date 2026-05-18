#include "pathfinder/tribe/tribe_state.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static TribeMemberEvent addMember(const std::string& id, TribeMemberRole role) {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::MemberAdded;
    event.member_id = EntityId(id);
    event.role = role;
    event.trust = 0.65;
    event.morale = 0.65;
    event.reason_keys = {"safe_member_join"};
    return event;
}

static TribeState createTribe() {
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_p23");
    input.input_tick = Tick(20);
    input.member_events = {addMember("pioneer", TribeMemberRole::Pioneer), addMember("companion", TribeMemberRole::Learner)};
    input.knowledge_events = {TribeKnowledgeEvent{KnowledgeId("know_red_berry"), {"shared_learning"}}};
    input.reason_keys = {"tribe_creation"};
    TribeStateResolver resolver;
    auto result = resolver.resolve(TribeState{}, input);
    assert(result.is_ok());
    assert(result.value().validateBasic().is_ok());
    return result.value().updated_state;
}

static void test_tribe_creation_flow() {
    TribeState state = createTribe();
    assert(state.population_total == 2);
    assert(state.active_population == 2);
    assert(state.members[0].role == TribeMemberRole::Pioneer);
    assert(state.members[1].role == TribeMemberRole::Learner);
    assert(state.shared_knowledge_ids.size() == 1);
    std::cout << "p23_tribe_creation_flow passed" << std::endl;
}

static void test_tribe_role_and_projection_flow() {
    TribeState state = createTribe();
    TribeMemberEvent change;
    change.kind = TribeStateChangeKind::MemberRoleChanged;
    change.member_id = EntityId("companion");
    change.role = TribeMemberRole::Gatherer;
    change.reason_keys = {"role_assignment"};

    TribeStateInput input;
    input.tribe_id = TribeId("tribe_p23");
    input.input_tick = Tick(21);
    input.member_events = {change};

    TribeStateResolver resolver;
    auto result = resolver.resolve(state, input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.updated_state.members[1].role == TribeMemberRole::Gatherer);
    assert(std::any_of(dto.state_changes.begin(), dto.state_changes.end(), [](const auto& draft) {
        return draft.kind == TribeStateChangeKind::MemberRoleChanged;
    }));
    assert(std::any_of(dto.projection.role_summary_lines.begin(), dto.projection.role_summary_lines.end(), [](const auto& line) {
        return line == "gatherer=1";
    }));
    std::cout << "p23_tribe_role_and_projection_flow passed" << std::endl;
}

static void test_split_risk_deterministic_flow() {
    TribeState state = createTribe();
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_p23");
    input.input_tick = Tick(22);
    input.resource_pressure = 0.9;
    input.knowledge_conflict_pressure = 0.8;
    input.casualty_pressure = 0.7;
    input.trust_delta = -0.5;
    input.reason_keys = {"pressure_summary"};

    TribeStateResolver resolver;
    auto first = resolver.resolve(state, input);
    auto second = resolver.resolve(state, input);
    assert(first.is_ok());
    assert(second.is_ok());
    auto first_risk = first.value().updated_state.split_risk;
    auto second_risk = second.value().updated_state.split_risk;
    assert(first_risk.risk == second_risk.risk);
    assert(first_risk.cohesion_state == second_risk.cohesion_state);
    assert(first_risk.cohesion_state == TribeCohesionState::Fracturing);
    assert(std::any_of(first.value().trace.steps.begin(), first.value().trace.steps.end(), [](const auto& step) {
        return step.find("split_risk_changed") != std::string::npos;
    }));
    std::cout << "p23_split_risk_deterministic_flow passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "tribe_creation_flow") test_tribe_creation_flow();
    else if (arg == "tribe_role_and_projection_flow") test_tribe_role_and_projection_flow();
    else if (arg == "split_risk_deterministic_flow") test_split_risk_deterministic_flow();
    else {
        test_tribe_creation_flow();
        test_tribe_role_and_projection_flow();
        test_split_risk_deterministic_flow();
    }
    return 0;
}
