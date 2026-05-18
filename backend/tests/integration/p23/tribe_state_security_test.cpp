#include "pathfinder/tribe/tribe_state.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::tribe;
using namespace pathfinder::foundation;

static TribeStateInput baseInput() {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::MemberAdded;
    event.member_id = EntityId("companion");
    event.role = TribeMemberRole::Learner;
    event.reason_keys = {"safe_join"};

    TribeStateInput input;
    input.tribe_id = TribeId("tribe_secure");
    input.input_tick = Tick(30);
    input.member_events = {event};
    input.reason_keys = {"safe_summary"};
    return input;
}

static void test_forbidden_keys() {
    auto input = baseInput();
    input.reason_keys = {"raw_state"};
    assert(input.validateBasic().is_error());

    input = baseInput();
    input.member_events[0].reason_keys = {"hidden_truth"};
    assert(input.validateBasic().is_error());

    input = baseInput();
    input.knowledge_events = {TribeKnowledgeEvent{KnowledgeId("know_red_berry"), {"GameState"}}};
    assert(input.validateBasic().is_error());
    std::cout << "p23_security_forbidden_keys passed" << std::endl;
}

static void test_rejects_random_split() {
    auto input = baseInput();
    input.reason_keys = {"random_split"};
    TribeStateResolver resolver;
    auto result = resolver.resolve(TribeState{}, input);
    assert(result.is_error());

    input = baseInput();
    input.reason_keys = {"probability_split"};
    result = resolver.resolve(TribeState{}, input);
    assert(result.is_error());
    std::cout << "p23_security_rejects_random_split passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "forbidden_keys") test_forbidden_keys();
    else if (arg == "rejects_random_split") test_rejects_random_split();
    else {
        test_forbidden_keys();
        test_rejects_random_split();
    }
    return 0;
}
