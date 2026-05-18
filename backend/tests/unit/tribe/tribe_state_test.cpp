#include "pathfinder/tribe/tribe_state.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static TribeState makeState() {
    TribeState state;
    state.tribe_id = TribeId("tribe_alpha");
    state.display_key = "tribe_alpha";
    state.morale.overall = 0.5;
    state.trust.leader_trust = 0.5;
    state.trust.member_trust = 0.5;
    state.trust.teaching_fairness = 0.5;
    state.split_risk.risk = 0.15;
    state.split_risk.resource_pressure = 0.0;
    state.split_risk.trust_pressure = 0.5;
    state.split_risk.knowledge_conflict_pressure = 0.0;
    state.split_risk.casualty_pressure = 0.0;
    state.split_risk.cohesion_state = TribeCohesionState::Stable;
    return state;
}

static TribeMemberEvent addMember(const std::string& id, TribeMemberRole role) {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::MemberAdded;
    event.member_id = EntityId(id);
    event.role = role;
    event.trust = 0.6;
    event.morale = 0.6;
    event.reason_keys = {"member_joined"};
    return event;
}

static void test_state_validate() {
    TribeState state = makeState();
    state.members.push_back(TribeMember{EntityId("member_a"), TribeMemberRole::Pioneer, true, 0.6, 0.6, {}, Tick(1), Tick(1), {"created"}});
    state.population_total = 1;
    state.active_population = 1;
    state.shared_knowledge_ids = {KnowledgeId("know_red_berry")};
    assert(state.validateBasic().is_ok());

    state.population_total = 2;
    assert(state.validateBasic().is_error());
    std::cout << "tribe_state_validate passed" << std::endl;
}

static void test_projection_safe_summary() {
    TribeState state = makeState();
    state.members.push_back(TribeMember{EntityId("member_a"), TribeMemberRole::Pioneer, true, 0.6, 0.6, {}, Tick(1), Tick(1), {"created"}});
    state.population_total = 1;
    state.active_population = 1;
    state.shared_knowledge_ids = {KnowledgeId("know_red_berry"), KnowledgeId("know_blue_fruit")};

    TribeProjectionBuilder builder;
    auto result = builder.build(state);
    assert(result.is_ok());
    auto projection = result.value();
    assert(projection.shared_knowledge_count == 2);
    assert(projection.population_summary.find("population_total=1") != std::string::npos);
    assert(!containsTribeForbiddenKey(projection.role_summary_lines));
    assert(projection.validateBasic().is_ok());
    std::cout << "tribe_projection_safe_summary passed" << std::endl;
}

static void test_resolver_member_join() {
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_alpha");
    input.input_tick = Tick(10);
    input.member_events = {addMember("pioneer", TribeMemberRole::Pioneer), addMember("companion", TribeMemberRole::Learner)};
    input.reason_keys = {"tribe_creation"};

    TribeStateResolver resolver;
    auto result = resolver.resolve(TribeState{}, input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.validateBasic().is_ok());
    assert(dto.updated_state.population_total == 2);
    assert(dto.updated_state.active_population == 2);
    assert(dto.projection.population_summary.find("population_total=2") != std::string::npos);
    std::cout << "tribe_resolver_member_join passed" << std::endl;
}

static void test_resolver_role_change() {
    TribeStateInput create;
    create.tribe_id = TribeId("tribe_alpha");
    create.input_tick = Tick(10);
    create.member_events = {addMember("companion", TribeMemberRole::Learner)};
    TribeStateResolver resolver;
    auto created = resolver.resolve(TribeState{}, create).value().updated_state;

    TribeMemberEvent change;
    change.kind = TribeStateChangeKind::MemberRoleChanged;
    change.member_id = EntityId("companion");
    change.role = TribeMemberRole::Gatherer;
    change.reason_keys = {"division_of_labor"};
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_alpha");
    input.input_tick = Tick(11);
    input.member_events = {change};

    auto result = resolver.resolve(created, input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.updated_state.members[0].role == TribeMemberRole::Gatherer);
    assert(std::any_of(dto.state_changes.begin(), dto.state_changes.end(), [](const auto& draft) {
        return draft.kind == TribeStateChangeKind::MemberRoleChanged;
    }));
    assert(dto.projection.role_summary_lines[0].find("gatherer=1") != std::string::npos);
    std::cout << "tribe_resolver_role_change passed" << std::endl;
}

static void test_resolver_morale_trust() {
    TribeState state = makeState();
    state.population_total = 0;
    state.active_population = 0;
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_alpha");
    input.input_tick = Tick(12);
    input.morale_delta = 0.2;
    input.trust_delta = -0.2;
    input.reason_keys = {"recent_success", "leader_trust_decreased"};

    TribeStateResolver resolver;
    auto result = resolver.resolve(state, input);
    assert(result.is_ok());
    auto dto = result.value();
    assert(dto.updated_state.morale.overall > 0.5);
    assert(dto.updated_state.trust.leader_trust < 0.5);
    assert(std::any_of(dto.trace.steps.begin(), dto.trace.steps.end(), [](const auto& step) {
        return step.find("morale_changed") != std::string::npos;
    }));
    assert(std::any_of(dto.trace.steps.begin(), dto.trace.steps.end(), [](const auto& step) {
        return step.find("trust_changed") != std::string::npos;
    }));
    std::cout << "tribe_resolver_morale_trust passed" << std::endl;
}

static void test_resolver_split_risk() {
    TribeState state = makeState();
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_alpha");
    input.input_tick = Tick(13);
    input.resource_pressure = 0.8;
    input.knowledge_conflict_pressure = 0.6;
    input.casualty_pressure = 0.5;
    input.trust_delta = -0.4;
    input.reason_keys = {"pressure_summary"};

    TribeStateResolver resolver;
    auto first = resolver.resolve(state, input);
    auto second = resolver.resolve(state, input);
    assert(first.is_ok());
    assert(second.is_ok());
    assert(first.value().updated_state.split_risk.risk == second.value().updated_state.split_risk.risk);
    assert(first.value().updated_state.split_risk.cohesion_state == TribeCohesionState::Fracturing);
    assert(std::any_of(first.value().trace.steps.begin(), first.value().trace.steps.end(), [](const auto& step) {
        return step.find("split_risk_changed") != std::string::npos;
    }));
    std::cout << "tribe_resolver_split_risk passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "state_validate") test_state_validate();
    else if (arg == "projection_safe_summary") test_projection_safe_summary();
    else if (arg == "resolver_member_join") test_resolver_member_join();
    else if (arg == "resolver_role_change") test_resolver_role_change();
    else if (arg == "resolver_morale_trust") test_resolver_morale_trust();
    else if (arg == "resolver_split_risk") test_resolver_split_risk();
    else {
        test_state_validate();
        test_projection_safe_summary();
        test_resolver_member_join();
        test_resolver_role_change();
        test_resolver_morale_trust();
        test_resolver_split_risk();
    }
    return 0;
}
