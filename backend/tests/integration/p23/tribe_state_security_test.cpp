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

static void test_tribe_id_empty_rejected() {
    TribeStateInput input = baseInput();
    input.tribe_id = TribeId("");
    assert(input.validateBasic().is_error());
    std::cout << "p23_security_tribe_id_empty_rejected passed" << std::endl;
}

static void test_member_id_empty_rejected() {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::MemberAdded;
    event.member_id = EntityId("");
    event.role = TribeMemberRole::Learner;
    event.reason_keys = {"test"};
    assert(event.validateBasic().is_error());
    std::cout << "p23_security_member_id_empty_rejected passed" << std::endl;
}

static void test_ratio_out_of_range_rejected() {
    TribeStateInput input = baseInput();
    input.resource_pressure = 1.5;
    assert(input.validateBasic().is_error());

    input = baseInput();
    input.resource_pressure = -0.5;
    assert(input.validateBasic().is_error());

    input = baseInput();
    input.safety_pressure = 2.0;
    assert(input.validateBasic().is_error());

    input = baseInput();
    input.knowledge_conflict_pressure = 1.1;
    assert(input.validateBasic().is_error());

    input = baseInput();
    input.casualty_pressure = -0.1;
    assert(input.validateBasic().is_error());
    std::cout << "p23_security_ratio_out_of_range_rejected passed" << std::endl;
}

static void test_active_population_exceeds_total_rejected() {
    TribeState state;
    state.tribe_id = TribeId("tribe_test");
    state.display_key = "tribe_test";
    state.population_total = 1;
    state.active_population = 3;
    auto result = state.validateBasic();
    assert(result.is_error());
    std::cout << "p23_security_active_population_exceeds_total_rejected passed" << std::endl;
}

static void test_duplicate_member_id_rejected() {
    TribeState state;
    state.tribe_id = TribeId("tribe_test");
    state.display_key = "tribe_test";
    state.members.push_back(TribeMember{EntityId("member_a"), TribeMemberRole::Pioneer, true, 0.5, 0.5, {}, Tick(1), Tick(1), {"test"}});
    state.members.push_back(TribeMember{EntityId("member_a"), TribeMemberRole::Learner, true, 0.5, 0.5, {}, Tick(1), Tick(1), {"test"}});
    state.population_total = 2;
    state.active_population = 2;
    assert(state.validateBasic().is_error());
    std::cout << "p23_security_duplicate_member_id_rejected passed" << std::endl;
}

static void test_duplicate_knowledge_id_rejected() {
    TribeState state;
    state.tribe_id = TribeId("tribe_test");
    state.display_key = "tribe_test";
    state.shared_knowledge_ids = {KnowledgeId("know_a"), KnowledgeId("know_a")};
    assert(state.validateBasic().is_error());
    std::cout << "p23_security_duplicate_knowledge_id_rejected passed" << std::endl;
}

static void test_test_only_enum_rejected() {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::TestOnly;
    event.member_id = EntityId("member_x");
    event.role = TribeMemberRole::Learner;
    assert(event.validateBasic().is_error());

    event.kind = TribeStateChangeKind::MemberAdded;
    event.role = TribeMemberRole::TestOnly;
    assert(event.validateBasic().is_error());

    TribeStateChangeDraft draft;
    draft.kind = TribeStateChangeKind::TestOnly;
    draft.summary_key = "test_draft";
    assert(draft.validateBasic().is_error());

    TribeMember member;
    member.member_id = EntityId("member_b");
    member.role = TribeMemberRole::TestOnly;
    assert(member.validateBasic().is_error());

    TribeSplitRisk risk;
    risk.resource_pressure = 0.1;
    risk.trust_pressure = 0.1;
    risk.knowledge_conflict_pressure = 0.1;
    risk.casualty_pressure = 0.1;
    risk.risk = 0.2;
    risk.cohesion_state = TribeCohesionState::TestOnly;
    assert(risk.validateBasic().is_error());
    std::cout << "p23_security_test_only_enum_rejected passed" << std::endl;
}

static void test_trust_changed_missing_trust_rejected() {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::TrustChanged;
    event.member_id = EntityId("member_x");
    event.reason_keys = {"test"};
    assert(event.validateBasic().is_error());
    std::cout << "p23_security_trust_changed_missing_trust_rejected passed" << std::endl;
}

static void test_morale_changed_missing_morale_rejected() {
    TribeMemberEvent event;
    event.kind = TribeStateChangeKind::MoraleChanged;
    event.member_id = EntityId("member_x");
    event.reason_keys = {"test"};
    assert(event.validateBasic().is_error());
    std::cout << "p23_security_morale_changed_missing_morale_rejected passed" << std::endl;
}

static void test_duplicate_member_events_rejected() {
    TribeStateInput input;
    input.tribe_id = TribeId("tribe_test");
    input.input_tick = Tick(10);
    TribeMemberEvent e1;
    e1.kind = TribeStateChangeKind::MemberAdded;
    e1.member_id = EntityId("member_a");
    e1.role = TribeMemberRole::Pioneer;
    e1.reason_keys = {"test"};
    TribeMemberEvent e2;
    e2.kind = TribeStateChangeKind::MemberRoleChanged;
    e2.member_id = EntityId("member_a");
    e2.role = TribeMemberRole::Learner;
    e2.reason_keys = {"test"};
    input.member_events = {e1, e2};
    assert(input.validateBasic().is_error());
    std::cout << "p23_security_duplicate_member_events_rejected passed" << std::endl;
}

static void test_duplicate_knowledge_events_rejected() {
    TribeStateInput input = baseInput();
    input.knowledge_events = {
        TribeKnowledgeEvent{KnowledgeId("know_a"), {"test"}},
        TribeKnowledgeEvent{KnowledgeId("know_a"), {"test"}},
    };
    assert(input.validateBasic().is_error());
    std::cout << "p23_security_duplicate_knowledge_events_rejected passed" << std::endl;
}

static void test_draft_validation_rejected() {
    // MemberAdded without member_id
    TribeStateChangeDraft draft;
    draft.kind = TribeStateChangeKind::MemberAdded;
    draft.new_role = TribeMemberRole::Pioneer;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // MemberAdded without new_role
    draft.kind = TribeStateChangeKind::MemberAdded;
    draft.member_id = EntityId("member_x");
    draft.new_role = std::nullopt;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // MemberRemoved without member_id
    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::MemberRemoved;
    draft.old_role = TribeMemberRole::Learner;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // MemberRemoved without old_role
    draft.kind = TribeStateChangeKind::MemberRemoved;
    draft.member_id = EntityId("member_x");
    draft.old_role = std::nullopt;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // MemberRoleChanged without member_id
    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::MemberRoleChanged;
    draft.old_role = TribeMemberRole::Learner;
    draft.new_role = TribeMemberRole::Gatherer;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // MemberRoleChanged without old_role
    draft.kind = TribeStateChangeKind::MemberRoleChanged;
    draft.member_id = EntityId("member_x");
    draft.old_role = std::nullopt;
    draft.new_role = TribeMemberRole::Gatherer;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // MemberRoleChanged without new_role
    draft.kind = TribeStateChangeKind::MemberRoleChanged;
    draft.member_id = EntityId("member_x");
    draft.old_role = TribeMemberRole::Learner;
    draft.new_role = std::nullopt;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // TrustChanged with out-of-range values
    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::TrustChanged;
    draft.old_value = 0.5;
    draft.new_value = 1.5;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // KnowledgeLinked with negative new_value
    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::KnowledgeLinked;
    draft.new_value = -1.0;
    draft.summary_key = "test";
    assert(draft.validateBasic().is_error());

    // Valid drafts pass
    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::MemberAdded;
    draft.member_id = EntityId("member_x");
    draft.new_role = TribeMemberRole::Pioneer;
    draft.summary_key = "member_added";
    assert(draft.validateBasic().is_ok());

    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::TrustChanged;
    draft.old_value = 0.5;
    draft.new_value = 0.7;
    draft.summary_key = "trust_changed";
    assert(draft.validateBasic().is_ok());

    draft = TribeStateChangeDraft{};
    draft.kind = TribeStateChangeKind::KnowledgeLinked;
    draft.new_value = 3.0;
    draft.summary_key = "knowledge_linked";
    assert(draft.validateBasic().is_ok());

    std::cout << "p23_security_draft_validation_rejected passed" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::string arg = argc > 1 ? argv[1] : "all";
    if (arg == "forbidden_keys") test_forbidden_keys();
    else if (arg == "rejects_random_split") test_rejects_random_split();
    else if (arg == "tribe_id_empty_rejected") test_tribe_id_empty_rejected();
    else if (arg == "member_id_empty_rejected") test_member_id_empty_rejected();
    else if (arg == "ratio_out_of_range_rejected") test_ratio_out_of_range_rejected();
    else if (arg == "active_population_exceeds_total_rejected") test_active_population_exceeds_total_rejected();
    else if (arg == "duplicate_member_id_rejected") test_duplicate_member_id_rejected();
    else if (arg == "duplicate_knowledge_id_rejected") test_duplicate_knowledge_id_rejected();
    else if (arg == "test_only_enum_rejected") test_test_only_enum_rejected();
    else if (arg == "trust_changed_missing_trust_rejected") test_trust_changed_missing_trust_rejected();
    else if (arg == "morale_changed_missing_morale_rejected") test_morale_changed_missing_morale_rejected();
    else if (arg == "duplicate_member_events_rejected") test_duplicate_member_events_rejected();
    else if (arg == "duplicate_knowledge_events_rejected") test_duplicate_knowledge_events_rejected();
    else if (arg == "draft_validation_rejected") test_draft_validation_rejected();
    else {
        test_forbidden_keys();
        test_rejects_random_split();
        test_tribe_id_empty_rejected();
        test_member_id_empty_rejected();
        test_ratio_out_of_range_rejected();
        test_active_population_exceeds_total_rejected();
        test_duplicate_member_id_rejected();
        test_duplicate_knowledge_id_rejected();
        test_test_only_enum_rejected();
        test_trust_changed_missing_trust_rejected();
        test_morale_changed_missing_morale_rejected();
        test_duplicate_member_events_rejected();
        test_duplicate_knowledge_events_rejected();
        test_draft_validation_rejected();
    }
    return 0;
}
