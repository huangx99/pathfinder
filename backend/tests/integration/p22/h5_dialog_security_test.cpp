#include "pathfinder/h5_dialog/dialog_turn_service.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

static DialogResponseDto send(const std::string& input) {
    DialogTurnService svc;
    DialogRequestDto req;
    req.session_id = "sec_test";
    req.input_text = input;
    req.client_turn = 1;
    auto r = svc.handle(req);
    assert(r.is_ok());
    auto resp = r.value();
    auto valid_r = resp.validateBasic();
    assert(valid_r.is_ok());
    return resp;
}

static void test_forbidden_input() {
    auto resp = send("hidden_truth");
    assert(resp.decision == DialogTurnDecision::Rejected);
    std::cout << "forbidden_input passed" << std::endl;
}

static void test_hidden_truth() {
    auto resp = send("raw_state");
    assert(resp.decision == DialogTurnDecision::Rejected);
    std::cout << "hidden_truth passed" << std::endl;
}

static void test_script_input_rejected() {
    auto resp = send("<script>alert(1)</script>");
    assert(resp.decision == DialogTurnDecision::Rejected);
    assert(resp.reply_text.find("<script>") == std::string::npos);
    std::cout << "script_input_rejected passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_forbidden_input();
    test_hidden_truth();
    test_script_input_rejected();
    std::cout << "All p22 security tests passed" << std::endl;
    return 0;
}
