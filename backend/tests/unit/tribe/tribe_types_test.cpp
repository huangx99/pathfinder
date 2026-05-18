#include "pathfinder/tribe/tribe_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static void test_role_enum_roundtrip() {
    assert(toString(TribeMemberRole::Pioneer) == "pioneer");
    assert(tribeMemberRoleFromString("pioneer").value() == TribeMemberRole::Pioneer);
    assert(tribeMemberRoleFromString("gatherer").value() == TribeMemberRole::Gatherer);
    assert(tribeMemberRoleFromString("invalid").is_error());
    std::cout << "tribe_role_enum_roundtrip passed" << std::endl;
}

static void test_cohesion_enum_roundtrip() {
    assert(toString(TribeCohesionState::Stable) == "stable");
    assert(tribeCohesionStateFromString("stable").value() == TribeCohesionState::Stable);
    assert(tribeCohesionStateFromString("fracturing").value() == TribeCohesionState::Fracturing);
    assert(tribeCohesionStateFromString("invalid").is_error());
    assert(cohesionStateForRisk(0.24) == TribeCohesionState::Stable);
    assert(cohesionStateForRisk(0.25) == TribeCohesionState::Watchful);
    assert(cohesionStateForRisk(0.45) == TribeCohesionState::Tense);
    assert(cohesionStateForRisk(0.65) == TribeCohesionState::Fracturing);
    assert(cohesionStateForRisk(0.85) == TribeCohesionState::Splintered);
    std::cout << "tribe_cohesion_enum_roundtrip passed" << std::endl;
}

static void test_member_validate() {
    TribeMember member;
    member.member_id = EntityId("member_a");
    member.role = TribeMemberRole::Learner;
    member.trust = 0.7;
    member.morale = 0.6;
    member.known_knowledge_ids = {KnowledgeId("know_red_berry")};
    member.reason_keys = {"safe_summary"};
    assert(member.validateBasic().is_ok());

    member.role = TribeMemberRole::Unknown;
    assert(member.validateBasic().is_error());
    member.role = TribeMemberRole::Learner;
    member.trust = 1.5;
    assert(member.validateBasic().is_error());
    member.trust = 0.7;
    member.reason_keys = {"raw_state"};
    assert(member.validateBasic().is_error());
    std::cout << "tribe_member_validate passed" << std::endl;
}

static void test_split_risk_validate() {
    TribeSplitRisk risk;
    risk.resource_pressure = 0.6;
    risk.trust_pressure = 0.5;
    risk.knowledge_conflict_pressure = 0.4;
    risk.casualty_pressure = 0.2;
    risk.risk = 0.46;
    risk.cohesion_state = TribeCohesionState::Tense;
    assert(risk.validateBasic().is_ok());

    risk.cohesion_state = TribeCohesionState::Stable;
    assert(risk.validateBasic().is_error());
    std::cout << "tribe_split_risk_validate passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "role_enum_roundtrip") test_role_enum_roundtrip();
    else if (arg == "cohesion_enum_roundtrip") test_cohesion_enum_roundtrip();
    else if (arg == "member_validate") test_member_validate();
    else if (arg == "split_risk_validate") test_split_risk_validate();
    else {
        test_role_enum_roundtrip();
        test_cohesion_enum_roundtrip();
        test_member_validate();
        test_split_risk_validate();
    }
    return 0;
}
