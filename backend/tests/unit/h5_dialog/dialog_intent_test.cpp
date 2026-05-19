#include "pathfinder/h5_dialog/dialog_intent.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
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

    auto use_beast = parser.parse("使用靠近的野兽");
    assert(use_beast.is_ok());
    assert(use_beast.value().kind == DialogIntentKind::TryUse);
    assert(use_beast.value().action == DialogActionKind::Use);
    assert(use_beast.value().object_key == "beast_shadow");
    assert(use_beast.value().target_object_key.empty());

    auto repel_beast = parser.parse("用火把驱赶野兽");
    assert(repel_beast.is_ok());
    assert(repel_beast.value().kind == DialogIntentKind::TryUse);
    assert(repel_beast.value().action == DialogActionKind::Use);
    assert(repel_beast.value().object_key == "torch");
    assert(repel_beast.value().target_object_key == "beast_shadow");

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

static void test_scenario_alias_driven_object_detection() {
    DialogScenario scenario;
    scenario.scenario_key = "test_alias_scene";
    scenario.display_name = "测试场景";
    scenario.welcome_text = "测试";
    DialogScenarioObject crystal;
    crystal.object_key = "glow_crystal";
    crystal.display_name = "发光晶体";
    crystal.player_description = "测试物品";
    crystal.visibility = DialogObjectVisibility::Visible;
    crystal.allowed_actions = {DialogActionKind::Use};
    crystal.default_action = DialogActionKind::Use;
    crystal.input_aliases = {"晶体", "发光石"};
    scenario.objects.push_back(crystal);

    DialogIntentParser parser;
    auto result = parser.parseWithScenario("用发光石", scenario);
    assert(result.is_ok());
    assert(result.value().kind == DialogIntentKind::TryUse);
    assert(result.value().object_key == "glow_crystal");
    std::cout << "scenario_alias_driven_object_detection passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_observe();
    test_eat_red_berry();
    test_action_object_pairing();
    test_teach();
    test_unknown();
    test_scenario_alias_driven_object_detection();
    std::cout << "All h5_dialog_intent tests passed" << std::endl;
    return 0;
}
