#include "pathfinder/h5_dialog/dialog_presenter.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;
using namespace pathfinder::learning;
using namespace pathfinder::knowledge;
using namespace pathfinder::memory;
using namespace pathfinder::cognition;

static DialogSessionState makeTestSession() {
    DialogSessionState state;
    state.session_id = "test_session";
    state.scenario_key = "p22_minimal";
    state.turn_index = 2;
    state.current_tick = Tick(2);
    state.actor.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, EntityId("pioneer"), {}, {}, "pioneer"};
    state.actor.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, EntityId("pioneer"), {}, "pioneer"};
    state.actor.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, EntityId("pioneer"), std::nullopt};
    state.actor.display_key = "pioneer";
    state.visible_object_keys = {"red_berry", "decayed_red_berry", "bitter_leaf", "stone_flake"};
    return state;
}

static void test_chinese_response() {
    DialogPresenter presenter;
    auto state = makeTestSession();

    // Test observe response
    auto resp_r = presenter.buildObserveResponse(state);
    assert(resp_r.is_ok());
    auto resp = resp_r.value();
    assert(!resp.reply_text.empty());
    assert(resp.decision == DialogTurnDecision::RepliedOnly);
    assert(!resp.quick_actions.empty());

    // Test inspect response
    auto insp_r = presenter.buildInspectResponse(state, false);
    assert(insp_r.is_ok());
    auto insp = insp_r.value();
    assert(!insp.reply_text.empty());

    // Test help response
    auto help_r = presenter.buildHelpResponse(state);
    assert(help_r.is_ok());
    auto help = help_r.value();
    assert(help.reply_text.find("观察") != std::string::npos);

    std::cout << "chinese_response passed" << std::endl;
}


static void test_knowledge_lines_have_chinese_notes() {
    DialogPresenter presenter;
    auto state = makeTestSession();

    KnowledgeClaim claim;
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "red_berry";
    claim.subject.subject_type_key = "object";
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = "eat";
    claim.predicate.effect_key = "restore_hunger";
    claim.status = KnowledgeStatus::Candidate;
    state.actor_claims.push_back(claim);

    auto insp_r = presenter.buildInspectResponse(state, false);
    assert(insp_r.is_ok());
    auto insp = insp_r.value();
    assert(!insp.actor_knowledge_lines.empty());
    assert(insp.actor_knowledge_lines.front().find("red_berry + eat -> restore_hunger [candidate]") != std::string::npos);
    assert(insp.actor_knowledge_lines.front().find("红果") != std::string::npos);
    assert(insp.actor_knowledge_lines.front().find("缓解饥饿") != std::string::npos);
    assert(insp.actor_knowledge_lines.front().find("候选判断") != std::string::npos);
    assert(insp.reply_text.find("红果") != std::string::npos);

    std::cout << "knowledge_lines_have_chinese_notes passed" << std::endl;
}


static void test_decayed_condition_has_player_friendly_name() {
    DialogPresenter presenter;
    auto state = makeTestSession();

    KnowledgeClaim claim;
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "red_berry";
    claim.subject.subject_type_key = "object";
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = "eat";
    claim.predicate.effect_key = "poison";
    claim.status = KnowledgeStatus::Active;
    KnowledgeCondition condition;
    condition.condition_key = "decayed";
    claim.conditions.push_back(condition);
    state.actor_claims.push_back(claim);

    auto insp_r = presenter.buildInspectResponse(state, false);
    assert(insp_r.is_ok());
    auto insp = insp_r.value();
    assert(!insp.actor_knowledge_lines.empty());
    assert(insp.actor_knowledge_lines.front().find("red_berry{decayed} + eat -> poison [active]") != std::string::npos);
    assert(insp.actor_knowledge_lines.front().find("腐烂红果") != std::string::npos);
    assert(insp.actor_knowledge_lines.front().find("中毒") != std::string::npos);
    assert(insp.reply_text.find("腐烂红果") != std::string::npos);

    std::cout << "decayed_condition_has_player_friendly_name passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_chinese_response();
    test_knowledge_lines_have_chinese_notes();
    test_decayed_condition_has_player_friendly_name();
    std::cout << "All h5_dialog_presenter tests passed" << std::endl;
    return 0;
}
