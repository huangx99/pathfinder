#include "pathfinder/h5_dialog/dialog_turn_service.h"
#include <iostream>
#include <cassert>
#include <algorithm>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

namespace {

static bool containsText(const std::vector<std::string>& values, const std::string& text) {
    return std::any_of(values.begin(), values.end(), [&](const auto& value) {
        return value.find(text) != std::string::npos;
    });
}


static size_t totalCountText(const std::vector<std::string>& values, const std::string& text) {
    size_t count = 0;
    for (const auto& value : values) {
        size_t pos = value.find(text);
        while (pos != std::string::npos) {
            ++count;
            pos = value.find(text, pos + text.size());
        }
    }
    return count;
}

static size_t countText(const std::vector<std::string>& values, const std::string& text) {
    return static_cast<size_t>(std::count_if(values.begin(), values.end(), [&](const auto& value) {
        return value.find(text) != std::string::npos;
    }));
}

static DialogResponseDto send(DialogTurnService& svc, const std::string& session, const std::string& input) {
    DialogRequestDto req;
    req.session_id = session;
    req.input_text = input;
    req.client_turn = 1;
    auto r = svc.handle(req);
    assert(r.is_ok());
    auto resp = r.value();
    auto valid_r = resp.validateBasic();
    assert(valid_r.is_ok());
    return resp;
}

static void assertDecision(const DialogResponseDto& resp, DialogTurnDecision expected) {
    assert(resp.decision == expected);
}

} // namespace

static void test_red_berry_learning_flow() {
    DialogTurnService svc;
    std::string session = "p22_red_berry_test";

    auto observe = send(svc, session, "观察");
    assertDecision(observe, DialogTurnDecision::RepliedOnly);
    assert(observe.reply_text.find("红果") != std::string::npos);

    auto eat = send(svc, session, "吃红果");
    assertDecision(eat, DialogTurnDecision::LearningLoopRan);
    assert(containsText(eat.actor_knowledge_lines, "red_berry"));
    assert(containsText(eat.actor_knowledge_lines, "restore_hunger"));
    assert(eat.recipient_knowledge_lines.empty());
    assert(containsText(eat.debug_keys, "knowledge_formed"));
    assert(!containsText(eat.debug_keys, "knowledge_propagated"));

    auto inspect = send(svc, session, "查看知识");
    assertDecision(inspect, DialogTurnDecision::RepliedOnly);
    assert(containsText(inspect.actor_knowledge_lines, "red_berry"));
    assert(inspect.reply_text.find("你的知识：") != std::string::npos);
    assert(inspect.reply_text.find("同伴的知识：") != std::string::npos);
    assert(inspect.reply_text.find("red_berry") != std::string::npos);

    std::cout << "red_berry_learning_flow passed" << std::endl;
}

static void test_teach_companion_flow() {
    DialogTurnService svc;
    std::string empty_session = "p22_teach_empty_test";

    auto rejected = send(svc, empty_session, "教同伴");
    assertDecision(rejected, DialogTurnDecision::Rejected);
    assert(rejected.reply_text.find("还没有") != std::string::npos);

    std::string session = "p22_teach_test";
    send(svc, session, "观察");
    auto eat = send(svc, session, "吃红果");
    assertDecision(eat, DialogTurnDecision::LearningLoopRan);
    assert(eat.recipient_knowledge_lines.empty());
    assert(countText(eat.actor_knowledge_lines, "red_berry") == 1);

    auto teach = send(svc, session, "教同伴");
    assertDecision(teach, DialogTurnDecision::TeachingRan);
    assert(containsText(teach.recipient_knowledge_lines, "red_berry"));
    assert(containsText(teach.recipient_knowledge_lines, "restore_hunger"));
    assert(containsText(teach.debug_keys, "knowledge_propagated"));
    assert(countText(teach.actor_knowledge_lines, "red_berry") == 1);

    auto inspect = send(svc, session, "查看同伴");
    assertDecision(inspect, DialogTurnDecision::RepliedOnly);
    assert(containsText(inspect.recipient_knowledge_lines, "red_berry"));

    std::cout << "teach_companion_flow passed" << std::endl;
}

static void test_decayed_red_berry_revision_flow() {
    DialogTurnService svc;
    std::string session = "p22_decayed_test";

    send(svc, session, "观察");
    auto eat = send(svc, session, "吃红果");
    assertDecision(eat, DialogTurnDecision::LearningLoopRan);
    assert(containsText(eat.actor_knowledge_lines, "restore_hunger"));

    auto decayed = send(svc, session, "吃腐烂红果");
    assertDecision(decayed, DialogTurnDecision::LearningLoopRan);
    assert(containsText(decayed.actor_knowledge_lines, "poison"));
    assert(containsText(decayed.actor_knowledge_lines, "red_berry{decayed}"));
    assert(containsText(decayed.debug_keys, "knowledge_revised"));
    assert(!containsText(decayed.debug_keys, "knowledge_propagated"));

    auto red_again = send(svc, session, "吃红果");
    assertDecision(red_again, DialogTurnDecision::LearningLoopRan);
    assert(containsText(red_again.actor_knowledge_lines, "red_berry + eat -> restore_hunger"));
    assert(containsText(red_again.actor_knowledge_lines, "red_berry{decayed} + eat -> poison"));
    assert(containsText(red_again.actor_knowledge_lines, "腐烂红果"));
    assert(totalCountText(red_again.actor_knowledge_lines, "red_berry{decayed} + eat -> poison") == 1);

    auto teach = send(svc, session, "教同伴");
    assertDecision(teach, DialogTurnDecision::TeachingRan);
    assert(containsText(teach.debug_keys, "knowledge_propagated"));

    auto stone_after_teach = send(svc, session, "使用石片");
    assertDecision(stone_after_teach, DialogTurnDecision::LearningLoopRan);
    assert(containsText(stone_after_teach.actor_knowledge_lines, "stone_flake"));

    std::cout << "decayed_red_berry_revision_flow passed" << std::endl;
}

static void test_teached_companion_survives_actor_revision_flow() {
    DialogTurnService svc;
    std::string session = "p22_companion_retains_knowledge_test";

    send(svc, session, "观察");
    auto eat = send(svc, session, "吃红果");
    assertDecision(eat, DialogTurnDecision::LearningLoopRan);
    assert(containsText(eat.actor_knowledge_lines, "restore_hunger"));

    auto teach = send(svc, session, "教同伴");
    assertDecision(teach, DialogTurnDecision::TeachingRan);
    assert(containsText(teach.recipient_knowledge_lines, "red_berry"));
    assert(containsText(teach.recipient_knowledge_lines, "restore_hunger"));

    auto decayed = send(svc, session, "吃腐烂红果");
    assertDecision(decayed, DialogTurnDecision::LearningLoopRan);
    assert(containsText(decayed.actor_knowledge_lines, "red_berry"));
    assert(containsText(decayed.actor_knowledge_lines, "poison"));
    assert(containsText(decayed.actor_knowledge_lines, "red_berry{decayed}"));
    assert(containsText(decayed.actor_knowledge_lines, "腐烂红果"));
    assert(containsText(decayed.recipient_knowledge_lines, "red_berry"));
    assert(containsText(decayed.recipient_knowledge_lines, "restore_hunger"));

    auto inspect_actor = send(svc, session, "查看知识");
    assertDecision(inspect_actor, DialogTurnDecision::RepliedOnly);
    assert(containsText(inspect_actor.actor_knowledge_lines, "poison"));
    assert(containsText(inspect_actor.recipient_knowledge_lines, "restore_hunger"));
    assert(inspect_actor.reply_text.find("你的知识：") != std::string::npos);
    assert(inspect_actor.reply_text.find("同伴的知识：") != std::string::npos);
    assert(inspect_actor.reply_text.find("poison") != std::string::npos);
    assert(inspect_actor.reply_text.find("restore_hunger") != std::string::npos);

    auto inspect_recipient = send(svc, session, "查看同伴");
    assertDecision(inspect_recipient, DialogTurnDecision::RepliedOnly);
    assert(containsText(inspect_recipient.recipient_knowledge_lines, "red_berry"));
    assert(containsText(inspect_recipient.recipient_knowledge_lines, "restore_hunger"));
    assert(inspect_recipient.reply_text.find("同伴的知识：") != std::string::npos);
    assert(inspect_recipient.reply_text.find("你的知识：") != std::string::npos);
    assert(inspect_recipient.reply_text.find("red_berry") != std::string::npos);
    assert(inspect_recipient.reply_text.find("restore_hunger") != std::string::npos);

    std::cout << "teached_companion_survives_actor_revision_flow passed" << std::endl;
}



static void test_teach_after_revision_transfers_all_current_claims_flow() {
    DialogTurnService svc;
    std::string session = "p22_teach_all_current_claims_test";

    send(svc, session, "观察");
    auto red = send(svc, session, "吃红果");
    assertDecision(red, DialogTurnDecision::LearningLoopRan);
    assert(containsText(red.actor_knowledge_lines, "restore_hunger"));

    auto decayed = send(svc, session, "吃腐烂红果");
    assertDecision(decayed, DialogTurnDecision::LearningLoopRan);
    assert(containsText(decayed.actor_knowledge_lines, "restore_hunger"));
    assert(containsText(decayed.actor_knowledge_lines, "poison"));

    auto teach = send(svc, session, "教同伴");
    assertDecision(teach, DialogTurnDecision::TeachingRan);
    assert(containsText(teach.recipient_knowledge_lines, "restore_hunger"));
    assert(containsText(teach.recipient_knowledge_lines, "poison"));
    assert(totalCountText(teach.recipient_knowledge_lines, "red_berry + eat -> restore_hunger") == 1);
    assert(totalCountText(teach.recipient_knowledge_lines, "red_berry{decayed} + eat -> poison") == 1);
    assert(containsText(teach.recipient_knowledge_lines, "腐烂红果"));

    std::cout << "teach_after_revision_transfers_all_current_claims_flow passed" << std::endl;
}

static void test_action_mismatch_and_duplicate_guard_flow() {
    DialogTurnService svc;
    std::string session = "p22_action_mismatch_duplicate_guard_test";

    send(svc, session, "观察");
    auto stone = send(svc, session, "使用石片");
    assertDecision(stone, DialogTurnDecision::LearningLoopRan);
    assert(countText(stone.actor_knowledge_lines, "stone_flake") == 1);
    assert(containsText(stone.actor_knowledge_lines, "use_hint"));

    auto eat_stone = send(svc, session, "吃石片");
    assertDecision(eat_stone, DialogTurnDecision::Rejected);
    assert(eat_stone.reply_text.find("不能") != std::string::npos);

    auto bitter = send(svc, session, "吃苦叶");
    assertDecision(bitter, DialogTurnDecision::LearningLoopRan);
    assert(containsText(bitter.actor_knowledge_lines, "bitter_leaf"));
    assert(containsText(bitter.actor_knowledge_lines, "no_visible_effect"));
    assert(countText(bitter.actor_knowledge_lines, "stone_flake") == 1);
    assert(!containsText(bitter.actor_knowledge_lines, "stone_flake + eat"));

    auto red = send(svc, session, "吃红果");
    assertDecision(red, DialogTurnDecision::LearningLoopRan);
    assert(containsText(red.actor_knowledge_lines, "restore_hunger"));

    for (int i = 0; i < 8; ++i) {
        auto decayed = send(svc, session, "吃腐烂红果");
        assertDecision(decayed, DialogTurnDecision::LearningLoopRan);
        assert(containsText(decayed.actor_knowledge_lines, "poison"));
        assert(totalCountText(decayed.actor_knowledge_lines, "red_berry{decayed} + eat -> poison") == 1);
        assert(containsText(decayed.actor_knowledge_lines, "腐烂红果"));
    }

    auto inspect = send(svc, session, "查看知识");
    assertDecision(inspect, DialogTurnDecision::RepliedOnly);
    assert(totalCountText(inspect.actor_knowledge_lines, "red_berry{decayed} + eat -> poison") == 1);
    assert(totalCountText(inspect.actor_knowledge_lines, "stone_flake + use -> use_hint") == 1);
    assert(totalCountText(inspect.actor_knowledge_lines, "bitter_leaf + eat -> no_visible_effect") == 1);

    std::cout << "action_mismatch_and_duplicate_guard_flow passed" << std::endl;
}

static void test_non_red_object_flow() {
    DialogTurnService svc;
    std::string session = "p22_non_red_test";

    auto observe = send(svc, session, "观察");
    assertDecision(observe, DialogTurnDecision::RepliedOnly);
    assert(observe.reply_text.find("石片") != std::string::npos);

    auto stone = send(svc, session, "使用石片");
    assertDecision(stone, DialogTurnDecision::LearningLoopRan);
    assert(containsText(stone.actor_knowledge_lines, "stone_flake"));
    assert(containsText(stone.actor_knowledge_lines, "use_hint"));
    assert(stone.recipient_knowledge_lines.empty());

    std::string session2 = "p22_bitter_test";
    auto bitter_observe = send(svc, session2, "观察");
    assertDecision(bitter_observe, DialogTurnDecision::RepliedOnly);
    assert(bitter_observe.reply_text.find("苦叶") != std::string::npos);
    auto bitter = send(svc, session2, "吃苦叶");
    assertDecision(bitter, DialogTurnDecision::LearningLoopRan);
    assert(bitter.reply_text.find("苦叶") != std::string::npos);
    assert(bitter.reply_text.find("记忆") != std::string::npos ||
           containsText(bitter.actor_knowledge_lines, "bitter_leaf"));

    std::cout << "non_red_object_flow passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_red_berry_learning_flow();
    test_teach_companion_flow();
    test_decayed_red_berry_revision_flow();
    test_teached_companion_survives_actor_revision_flow();
    test_teach_after_revision_transfers_all_current_claims_flow();
    test_action_mismatch_and_duplicate_guard_flow();
    test_non_red_object_flow();
    std::cout << "All p22 learning story tests passed" << std::endl;
    return 0;
}
