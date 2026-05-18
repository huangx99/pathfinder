#include "pathfinder/h5_dialog/dialog_types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

static void test_smoke() {
    DialogRequestDto req;
    req.session_id = "test_session";
    req.input_text = "观察";
    req.client_turn = 1;
    auto r = req.validateBasic();
    assert(r.is_ok());
    std::cout << "smoke passed" << std::endl;
}

static void test_intent_parse_chinese() {
    // Covered in dialog_intent_test, but verify enum roundtrip here
    assert(toString(DialogIntentKind::Observe) == "observe");
    assert(dialogIntentKindFromString("observe").value() == DialogIntentKind::Observe);
    assert(toString(DialogActionKind::Eat) == "eat");
    assert(dialogActionKindFromString("eat").value() == DialogActionKind::Eat);
    assert(toString(DialogTurnDecision::RepliedOnly) == "replied_only");
    assert(dialogTurnDecisionFromString("replied_only").value() == DialogTurnDecision::RepliedOnly);
    assert(toString(DialogObjectVisibility::Visible) == "visible");
    assert(dialogObjectVisibilityFromString("visible").value() == DialogObjectVisibility::Visible);
    std::cout << "intent_parse_chinese passed" << std::endl;
}

static void test_scenario_catalog() {
    DialogScenarioObject obj;
    obj.object_key = "test_obj";
    obj.display_name = "测试对象";
    obj.player_description = "一个测试对象。";
    obj.visibility = DialogObjectVisibility::Visible;
    obj.allowed_actions = {DialogActionKind::Eat};
    auto r = obj.validateBasic();
    assert(r.is_ok());
    std::cout << "scenario_catalog passed" << std::endl;
}

static void test_wire_codec_escape() {
    DialogResponseDto resp;
    resp.session_id = "s1";
    resp.decision = DialogTurnDecision::RepliedOnly;
    resp.reply_text = "测试\"引号\\和\n换行";
    resp.state_summary_lines = {"a", "b"};
    auto r = resp.validateBasic();
    assert(r.is_ok());
    std::cout << "wire_codec_escape passed" << std::endl;
}

static void test_forbidden_key_rejected() {
    DialogRequestDto req;
    req.session_id = "test";
    req.input_text = "hidden_truth";
    auto r = req.validateBasic();
    assert(r.is_error());
    std::cout << "forbidden_key_rejected passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_smoke();
    test_intent_parse_chinese();
    test_scenario_catalog();
    test_wire_codec_escape();
    test_forbidden_key_rejected();
    std::cout << "All h5_dialog_types tests passed" << std::endl;
    return 0;
}
