#include "pathfinder/h5_dialog/dialog_intent.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

static void test_observe() {
    DialogIntentParser parser;
    auto r = parser.parse("观察");
    assert(r.is_ok());
    assert(r.value().kind == DialogIntentKind::Observe);
    std::cout << "observe passed" << std::endl;
}

static void test_eat_red_berry() {
    DialogIntentParser parser;
    auto r = parser.parse("吃红果");
    assert(r.is_ok());
    assert(r.value().kind == DialogIntentKind::TryEat);
    assert(r.value().object_key == "red_berry");
    std::cout << "eat_red_berry passed" << std::endl;
}


static void test_action_object_pairing() {
    DialogIntentParser parser;

    auto eat_stone = parser.parse("吃石片");
    assert(eat_stone.is_ok());
    assert(eat_stone.value().kind == DialogIntentKind::TryEat);
    assert(eat_stone.value().action == DialogActionKind::Eat);
    assert(eat_stone.value().object_key == "stone_flake");

    auto eat_leaf = parser.parse("吃苦叶");
    assert(eat_leaf.is_ok());
    assert(eat_leaf.value().kind == DialogIntentKind::TryEat);
    assert(eat_leaf.value().action == DialogActionKind::Eat);
    assert(eat_leaf.value().object_key == "bitter_leaf");

    auto inspect_companion = parser.parse("同伴知识");
    assert(inspect_companion.is_ok());
    assert(inspect_companion.value().kind == DialogIntentKind::InspectRecipientKnowledge);

    std::cout << "action_object_pairing passed" << std::endl;
}

static void test_teach() {
    DialogIntentParser parser;
    auto r = parser.parse("教同伴");
    assert(r.is_ok());
    assert(r.value().kind == DialogIntentKind::TeachRecipient);
    std::cout << "teach passed" << std::endl;
}

static void test_unknown() {
    DialogIntentParser parser;
    auto r = parser.parse("abcdefg");
    assert(r.is_ok());
    assert(r.value().kind == DialogIntentKind::Unknown);
    std::cout << "unknown passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_observe();
    test_eat_red_berry();
    test_action_object_pairing();
    test_teach();
    test_unknown();
    std::cout << "All h5_dialog_intent tests passed" << std::endl;
    return 0;
}
