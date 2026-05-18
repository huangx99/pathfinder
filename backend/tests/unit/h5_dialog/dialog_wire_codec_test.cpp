#include "pathfinder/h5_dialog/dialog_wire_codec.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::h5_dialog;
using namespace pathfinder::foundation;

static void test_parse_request() {
    DialogWireCodec codec;
    auto r = codec.parseFormUrlEncodedRequest("session_id=s1&input_text=%E8%A7%82%E5%AF%9F&client_turn=3");
    assert(r.is_ok());
    assert(r.value().session_id == "s1");
    assert(r.value().input_text == "观察");
    assert(r.value().client_turn == 3);
    std::cout << "parse_request passed" << std::endl;
}

static void test_encode_response() {
    DialogWireCodec codec;
    DialogResponseDto resp;
    resp.session_id = "s1";
    resp.decision = DialogTurnDecision::RepliedOnly;
    resp.reply_text = "hello";
    resp.state_summary_lines = {"a", "b"};
    auto r = codec.encodeJsonResponse(resp);
    assert(r.is_ok());
    assert(r.value().find("\"session_id\":\"s1\"") != std::string::npos);
    std::cout << "encode_response passed" << std::endl;
}

static void test_chinese_escape() {
    DialogWireCodec codec;
    DialogResponseDto resp;
    resp.session_id = "s1";
    resp.decision = DialogTurnDecision::RepliedOnly;
    resp.reply_text = "测试\"引号\\和\n换行";
    auto r = codec.encodeJsonResponse(resp);
    assert(r.is_ok());
    auto json = r.value();
    assert(json.find("\\\"") != std::string::npos);
    assert(json.find("\\\\") != std::string::npos);
    assert(json.find("\\n") != std::string::npos);
    std::cout << "chinese_escape passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_parse_request();
    test_encode_response();
    test_chinese_escape();
    std::cout << "All h5_dialog_wire_codec tests passed" << std::endl;
    return 0;
}
