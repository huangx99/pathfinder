#include "pathfinder/h5_playable/playable_turn_service.h"
#include "pathfinder/h5_playable/playable_wire_codec.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace pathfinder::h5_playable;

static H5PlayableRequest turn(const std::string& session, uint64_t index, const std::string& input) {
    H5PlayableRequest request;
    request.session_id = session;
    request.client_turn = index;
    request.input_kind = PlayableInputKind::FreeText;
    request.input_text = input;
    return request;
}

static void test_enum_roundtrip() {
    assert(toString(PlayableInputKind::FreeText) == "free_text");
    assert(playableInputKindFromString("projection_action").value() == PlayableInputKind::ProjectionAction);
    assert(toString(PlayableFeedbackTone::Danger) == "danger");
    assert(playableFeedbackToneFromString("system").value() == PlayableFeedbackTone::System);
    assert(pathfinder::h5_dialog::toString(pathfinder::h5_dialog::DialogIntentKind::Wait) == "wait");
    assert(pathfinder::h5_dialog::dialogActionKindFromString("wait").value() == pathfinder::h5_dialog::DialogActionKind::Wait);
}

static void test_validation_forbidden() {
    H5PlayableRequest request = turn("s_raw_state", 1, "观察");
    assert(request.validateBasic().is_error());
    H5PlayableRequest input = turn("s_ok", 1, "查看 raw_state");
    assert(input.validateBasic().is_error());
}

static void test_bootstrap_projection() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest request;
    request.session_id = "s_p33_bootstrap";
    auto response = service.bootstrap(request);
    assert(response.is_ok());
    assert(response.value().projection.header.audience == pathfinder::h5_projection::ProjectionAudience::Player);
    assert(response.value().projection.object_cards.size() >= 4);
    assert(response.value().projection.actor_knowledge.empty());
    assert(response.value().projection.recipient_knowledge.empty());
}

static void test_learning_teaching_projection() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p33_learning";
    assert(service.bootstrap(bootstrap).is_ok());

    auto eat = service.handleTurn(turn("s_p33_learning", 1, "吃红果"));
    assert(eat.is_ok());
    assert(!eat.value().projection.actor_knowledge.empty());
    assert(eat.value().projection.recipient_knowledge.empty());
    assert(eat.value().reply_text.zh_cn.find(" -> ") == std::string::npos);

    auto teach = service.handleTurn(turn("s_p33_learning", 2, "教同伴"));
    assert(teach.is_ok());
    assert(!teach.value().projection.recipient_knowledge.empty());
}

static void test_targeted_tool_actions_projection() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p36_targeted_actions";
    bootstrap.reset = true;
    auto boot = service.bootstrap(bootstrap);
    assert(boot.is_ok());

    auto has_action = [](const H5PlayableResponse& response, const std::string& label_text) {
        for (const auto& action : response.projection.action_bar) {
            if (action.label.zh_cn == label_text && action.enabled) return true;
        }
        return false;
    };
    assert(has_action(boot.value(), "用斧头砍木头"));
    assert(has_action(boot.value(), "用磨石打磨斧头"));

    auto cut = service.handleTurn(turn("s_p36_targeted_actions", 1, "用斧头砍木头"));
    assert(cut.is_ok());
    bool saw_cut_target_knowledge = false;
    for (const auto& line : cut.value().projection.actor_knowledge) {
        if (line.subject_label.zh_cn == "斧头" &&
            line.predicate_label.zh_cn.find("对木头") != std::string::npos &&
            line.effect_summary.zh_cn.find("砍开木头") != std::string::npos) {
            saw_cut_target_knowledge = true;
        }
    }
    assert(saw_cut_target_knowledge);

    auto sharpen = service.handleTurn(turn("s_p36_targeted_actions", 2, "用磨石打磨斧头"));
    assert(sharpen.is_ok());
    bool saw_sharpen_target_knowledge = false;
    for (const auto& line : sharpen.value().projection.actor_knowledge) {
        if (line.subject_label.zh_cn == "磨石" &&
            line.predicate_label.zh_cn.find("对斧头") != std::string::npos &&
            line.effect_summary.zh_cn.find("恢复工具锋利度") != std::string::npos) {
            saw_sharpen_target_knowledge = true;
        }
    }
    assert(saw_sharpen_target_knowledge);
}

static void test_rotten_food_hint_projection() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p33_danger";
    assert(service.bootstrap(bootstrap).is_ok());
    auto rotten = service.handleTurn(turn("s_p33_danger", 1, "吃腐烂红果"));
    assert(rotten.is_ok());
    assert(!rotten.value().projection.actor_knowledge.empty());
    assert(!rotten.value().projection.danger_hints.empty());
    assert(rotten.value().reply_text.zh_cn.find("hp") == std::string::npos);
}

static void test_reset_clears_session() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p33_reset";
    assert(service.bootstrap(bootstrap).is_ok());
    assert(service.handleTurn(turn("s_p33_reset", 1, "吃红果")).is_ok());
    H5PlayableBootstrapRequest reset;
    reset.session_id = "s_p33_reset";
    reset.reset = true;
    auto response = service.bootstrap(reset);
    assert(response.is_ok());
    assert(response.value().projection.actor_knowledge.empty());
    assert(response.value().server_turn == 0);
}

static void test_wire_codec_json_safe() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p33_codec";
    assert(service.bootstrap(bootstrap).is_ok());
    auto response = service.handleTurn(turn("s_p33_codec", 1, "吃红果"));
    assert(response.is_ok());
    auto json = H5PlayableWireCodec{}.encodeResponse(response.value());
    assert(json.is_ok());
    assert(json.value().find("\"projection\"") != std::string::npos);
    assert(json.value().find(" + ") == std::string::npos);
    assert(json.value().find(" -> ") == std::string::npos);
    assert(json.value().find("raw_state") == std::string::npos);
    assert(json.value().find("hidden_truth") == std::string::npos);
}


static void test_rotten_red_berry_does_not_weaken_fresh_projection() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p33_rotten_scope";
    bootstrap.reset = true;
    assert(service.bootstrap(bootstrap).is_ok());

    auto findLine = [](const H5PlayableResponse& response, const std::string& subject, const std::string& effect_text) {
        const pathfinder::h5_projection::KnowledgeLineProjection* found = nullptr;
        for (const auto& line : response.projection.actor_knowledge) {
            if (line.subject_label.zh_cn == subject && line.effect_summary.zh_cn.find(effect_text) != std::string::npos) {
                assert(found == nullptr);
                found = &line;
            }
        }
        assert(found != nullptr);
        return found;
    };

    auto fresh1 = service.handleTurn(turn("s_p33_rotten_scope", 1, "吃红果"));
    assert(fresh1.is_ok());
    assert(findLine(fresh1.value(), "红果", "缓解饥饿")->status_key == "candidate");

    auto fresh2 = service.handleTurn(turn("s_p33_rotten_scope", 2, "吃红果"));
    assert(fresh2.is_ok());
    assert(findLine(fresh2.value(), "红果", "缓解饥饿")->status_key == "hypothesis");

    auto fresh3 = service.handleTurn(turn("s_p33_rotten_scope", 3, "吃红果"));
    assert(fresh3.is_ok());
    assert(findLine(fresh3.value(), "红果", "缓解饥饿")->status_key == "active");

    auto rotten1 = service.handleTurn(turn("s_p33_rotten_scope", 4, "吃腐烂红果"));
    assert(rotten1.is_ok());
    assert(findLine(rotten1.value(), "红果", "缓解饥饿")->status_key == "active");
    assert(findLine(rotten1.value(), "腐烂红果", "中毒")->status_key == "candidate");

    auto rotten2 = service.handleTurn(turn("s_p33_rotten_scope", 5, "吃腐烂红果"));
    assert(rotten2.is_ok());
    assert(findLine(rotten2.value(), "红果", "缓解饥饿")->status_key == "active");
    assert(findLine(rotten2.value(), "腐烂红果", "中毒")->status_key == "hypothesis");

    auto rotten3 = service.handleTurn(turn("s_p33_rotten_scope", 6, "吃腐烂红果"));
    assert(rotten3.is_ok());
    assert(findLine(rotten3.value(), "红果", "缓解饥饿")->status_key == "active");
    assert(findLine(rotten3.value(), "腐烂红果", "中毒")->status_key == "hypothesis");

    auto wait = service.handleTurn(turn("s_p33_rotten_scope", 7, "等待一会"));
    assert(wait.is_ok());
    assert(wait.value().reply_text.zh_cn.find("短时间") != std::string::npos);

    auto rotten4 = service.handleTurn(turn("s_p33_rotten_scope", 8, "吃腐烂红果"));
    assert(rotten4.is_ok());
    assert(findLine(rotten4.value(), "红果", "缓解饥饿")->status_key == "active");
    assert(findLine(rotten4.value(), "腐烂红果", "中毒")->status_key == "candidate");

    auto rotten5 = service.handleTurn(turn("s_p33_rotten_scope", 9, "吃腐烂红果"));
    assert(rotten5.is_ok());
    assert(findLine(rotten5.value(), "腐烂红果", "中毒")->status_key == "hypothesis");

    auto rotten6 = service.handleTurn(turn("s_p33_rotten_scope", 10, "吃腐烂红果"));
    assert(rotten6.is_ok());
    assert(findLine(rotten6.value(), "腐烂红果", "中毒")->status_key == "active");
}

static std::string readFile(const std::string& path) {
    for (const auto& candidate : {path, "../" + path, "../../" + path, "../../../" + path}) {
        std::ifstream file(candidate);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }
    return "";
}

static void test_frontend_static_gate() {
    const auto html = readFile("frontend/h5_playable/index.html");
    const auto js = readFile("frontend/h5_playable/app.js");
    assert(html.find("viewport") != std::string::npos);
    assert(html.find("object-list") != std::string::npos);
    assert(html.find("actor-knowledge") != std::string::npos);
    assert(js.find("projection.object_cards") != std::string::npos);
    assert(js.find("projection.actor_knowledge") != std::string::npos);
    assert(js.find("[actionBar, objectList]") != std::string::npos);
    assert(js.find("red_berry") == std::string::npos);
    assert(js.find("restore_hunger") == std::string::npos);
    assert(js.find("poison") == std::string::npos);
    assert(js.find(" -> ") == std::string::npos);
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string name = argv[1];
    if (name == "enum") test_enum_roundtrip();
    else if (name == "validation") test_validation_forbidden();
    else if (name == "bootstrap") test_bootstrap_projection();
    else if (name == "learning") test_learning_teaching_projection();
    else if (name == "targeted_actions") test_targeted_tool_actions_projection();
    else if (name == "danger") test_rotten_food_hint_projection();
    else if (name == "reset") test_reset_clears_session();
    else if (name == "codec") test_wire_codec_json_safe();
    else if (name == "rotten_scope") test_rotten_red_berry_does_not_weaken_fresh_projection();
    else if (name == "frontend_gate") test_frontend_static_gate();
    else return 2;
    std::cout << "h5_playable " << name << " passed\n";
    return 0;
}
