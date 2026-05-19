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


static bool projectionContainsText(const H5PlayableResponse& response, const std::string& needle) {
    if (response.reply_text.zh_cn.find(needle) != std::string::npos) return true;
    if (response.projection.scene_title.zh_cn.find(needle) != std::string::npos) return true;
    for (const auto& line : response.projection.scene_summary) {
        if (line.zh_cn.find(needle) != std::string::npos) return true;
    }
    for (const auto& danger : response.projection.danger_hints) {
        if (danger.hint_text.zh_cn.find(needle) != std::string::npos) return true;
        for (const auto& counter : danger.countermeasure_labels) {
            if (counter.zh_cn.find(needle) != std::string::npos) return true;
        }
    }
    for (const auto& action : response.projection.action_bar) {
        if (action.label.zh_cn.find(needle) != std::string::npos || action.input_text.find(needle) != std::string::npos) return true;
    }
    return false;
}

static const pathfinder::h5_projection::ObjectCardProjection* findCard(const H5PlayableResponse& response, const std::string& object_key) {
    for (const auto& card : response.projection.object_cards) {
        if (card.object_ref_key == object_key) return &card;
    }
    return nullptr;
}

static bool cardDescriptionContains(const H5PlayableResponse& response, const std::string& object_key, const std::string& needle) {
    const auto* card = findCard(response, object_key);
    return card && card->description.zh_cn.find(needle) != std::string::npos;
}

static void test_story_first_day_projection_and_actions() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p37_story";
    bootstrap.reset = true;
    auto boot = service.bootstrap(bootstrap);
    assert(boot.is_ok());
    assert(projectionContainsText(boot.value(), "目标"));
    assert(projectionContainsText(boot.value(), "制作火把"));
    assert(projectionContainsText(boot.value(), "火把"));

    auto cut = service.handleTurn(turn("s_p37_story", 1, "用斧头砍木头"));
    assert(cut.is_ok());
    assert(cardDescriptionContains(cut.value(), "wood", "已处理木材"));

    auto fire = service.handleTurn(turn("s_p37_story", 2, "用火种点燃干草"));
    assert(fire.is_ok());
    assert(projectionContainsText(fire.value(), "点燃"));
    bool has_ignite = false;
    for (const auto& line : fire.value().projection.actor_knowledge) {
        if (line.effect_summary.zh_cn.find("点燃火源") != std::string::npos) has_ignite = true;
    }
    assert(has_ignite);

    auto torch = service.handleTurn(turn("s_p37_story", 3, "制作火把"));
    assert(torch.is_ok());
    bool has_torch_knowledge = false;
    for (const auto& line : torch.value().projection.actor_knowledge) {
        if (line.effect_summary.zh_cn.find("制作火把") != std::string::npos) has_torch_knowledge = true;
    }
    assert(has_torch_knowledge);

    service.handleTurn(turn("s_p37_story", 4, "等待一会"));
    service.handleTurn(turn("s_p37_story", 5, "等待一会"));
    auto dusk = service.handleTurn(turn("s_p37_story", 6, "等待一会"));
    assert(dusk.is_ok());
    assert(projectionContainsText(dusk.value(), "低吼") || projectionContainsText(dusk.value(), "火堆"));

    auto repel = service.handleTurn(turn("s_p37_story", 7, "用火把驱赶野兽"));
    assert(repel.is_ok());
    bool has_repel = false;
    for (const auto& line : repel.value().projection.actor_knowledge) {
        if (line.effect_summary.zh_cn.find("驱赶") != std::string::npos) has_repel = true;
    }
    assert(has_repel);
}

static void test_story_beast_use_not_torch_repel() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p37_beast_use";
    bootstrap.reset = true;
    auto boot = service.bootstrap(bootstrap);
    assert(boot.is_ok());

    auto beast = service.handleTurn(turn("s_p37_beast_use", 1, "使用靠近的野兽"));
    assert(beast.is_ok());
    for (const auto& line : beast.value().projection.actor_knowledge) {
        assert(line.effect_summary.zh_cn.find("驱赶") == std::string::npos);
        assert(line.subject_label.zh_cn.find("火把") == std::string::npos);
    }

    auto repel = service.handleTurn(turn("s_p37_beast_use", 2, "用火把驱赶野兽"));
    assert(repel.is_ok());
    assert(repel.value().reply_text.zh_cn.find("feedback_not_found") == std::string::npos);
    assert(repel.value().projection.actor_knowledge.empty());
}

static void test_p38_world_state_visible_consequences() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p38_world";
    bootstrap.reset = true;
    assert(service.bootstrap(bootstrap).is_ok());

    auto eat = service.handleTurn(turn("s_p38_world", 1, "吃红果"));
    assert(eat.is_ok());
    assert(cardDescriptionContains(eat.value(), "red_berry", "剩余：2"));
    assert(eat.value().reply_text.zh_cn.find("世界变化") != std::string::npos);
    assert(eat.value().reply_text.zh_cn.find("你消耗了一个红果") != std::string::npos);

    auto cut = service.handleTurn(turn("s_p38_world", 2, "用斧头砍木头"));
    assert(cut.is_ok());
    assert(cut.value().reply_text.zh_cn.find("木头被斧头砍开") != std::string::npos);
    assert(cardDescriptionContains(cut.value(), "wood", "已处理木材：1"));
    assert(cardDescriptionContains(cut.value(), "axe", "锋利度：2"));

    auto fire = service.handleTurn(turn("s_p38_world", 3, "用火种点燃干草"));
    assert(fire.is_ok());
    assert(fire.value().reply_text.zh_cn.find("小火堆出现") != std::string::npos);
    assert(cardDescriptionContains(fire.value(), "camp_fire", "已点燃"));
    assert(cardDescriptionContains(fire.value(), "dry_grass", "剩余：1"));

    auto torch = service.handleTurn(turn("s_p38_world", 4, "制作火把"));
    assert(torch.is_ok());
    assert(torch.value().reply_text.zh_cn.find("做出了一支火把") != std::string::npos);
    assert(cardDescriptionContains(torch.value(), "torch", "可用火把：1"));
    assert(cardDescriptionContains(torch.value(), "wood", "已处理木材") == false || cardDescriptionContains(torch.value(), "wood", "已处理木材：0") == false);
}

static void test_p38_agent_fire_avoidance_visible() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p38_agent_fire";
    bootstrap.reset = true;
    assert(service.bootstrap(bootstrap).is_ok());

    auto fire = service.handleTurn(turn("s_p38_agent_fire", 1, "用火种点燃干草"));
    assert(fire.is_ok());
    auto wait = service.handleTurn(turn("s_p38_agent_fire", 2, "等待一会"));
    assert(wait.is_ok());
    assert(projectionContainsText(wait.value(), "野兽") && projectionContainsText(wait.value(), "火"));
    assert(cardDescriptionContains(wait.value(), "beast_shadow", "退去") || cardDescriptionContains(wait.value(), "beast_shadow", "潜伏"));
}

static void test_p38_beast_progresses_without_fake_fire() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p38_beast_no_fake_fire";
    bootstrap.reset = true;
    assert(service.bootstrap(bootstrap).is_ok());

    auto wait1 = service.handleTurn(turn("s_p38_beast_no_fake_fire", 1, "等待一会"));
    assert(wait1.is_ok());
    assert(cardDescriptionContains(wait1.value(), "beast_shadow", "正在观察"));
    assert(wait1.value().reply_text.zh_cn.find("观察到火光") == std::string::npos);
    assert(wait1.value().reply_text.zh_cn.find("火光降低") == std::string::npos);

    auto wait2 = service.handleTurn(turn("s_p38_beast_no_fake_fire", 2, "等待一会"));
    assert(wait2.is_ok());
    assert(cardDescriptionContains(wait2.value(), "beast_shadow", "正在靠近"));
    assert(wait2.value().reply_text.zh_cn.find("观察到火光") == std::string::npos);
    assert(wait2.value().reply_text.zh_cn.find("火光降低") == std::string::npos);

    auto wait3 = service.handleTurn(turn("s_p38_beast_no_fake_fire", 3, "等待一会"));
    assert(wait3.is_ok());
    assert(cardDescriptionContains(wait3.value(), "beast_shadow", "正在对峙"));
    assert(wait3.value().reply_text.zh_cn.find("观察到火光") == std::string::npos);
    assert(wait3.value().reply_text.zh_cn.find("火光降低") == std::string::npos);

    auto wait4 = service.handleTurn(turn("s_p38_beast_no_fake_fire", 4, "等待一会"));
    assert(wait4.is_ok());
    assert(wait4.value().reply_text.zh_cn.find("野兽冲入营地") != std::string::npos);
    assert(wait4.value().reply_text.zh_cn.find("第一夜失败") != std::string::npos);
    assert(cardDescriptionContains(wait4.value(), "beast_shadow", "已经袭击营地"));
}

static void test_p38_resource_failure_is_readable() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p38_failure";
    bootstrap.reset = true;
    assert(service.bootstrap(bootstrap).is_ok());

    auto early = service.handleTurn(turn("s_p38_failure", 1, "制作火把"));
    assert(early.is_ok());
    assert(early.value().reply_text.zh_cn.find("feedback_not_found") == std::string::npos);
    assert(early.value().reply_text.zh_cn.find("条件") != std::string::npos || early.value().reply_text.zh_cn.find("材料") != std::string::npos);

    service.handleTurn(turn("s_p38_failure", 2, "吃苦叶"));
    service.handleTurn(turn("s_p38_failure", 3, "吃苦叶"));
    auto depleted = service.handleTurn(turn("s_p38_failure", 4, "吃苦叶"));
    assert(depleted.is_ok());
    assert(depleted.value().reply_text.zh_cn.find("feedback_not_found") == std::string::npos);
    assert(depleted.value().reply_text.zh_cn.find("没有可消耗") != std::string::npos);
}

static void test_playable_buttons_are_executable() {
    H5PlayableTurnService service;
    H5PlayableBootstrapRequest bootstrap;
    bootstrap.session_id = "s_p37_buttons";
    bootstrap.reset = true;
    auto boot = service.bootstrap(bootstrap);
    assert(boot.is_ok());

    uint64_t client_turn = 1;
    for (const auto& card : boot.value().projection.object_cards) {
        for (const auto& action : card.actions) {
            if (!action.enabled) continue;
            auto response = service.handleTurn(turn("s_p37_buttons", client_turn++, action.input_text));
            assert(response.is_ok());
            assert(response.value().reply_text.zh_cn.find("feedback_not_found") == std::string::npos);
        }
    }
    for (const auto& action : boot.value().projection.action_bar) {
        if (action.input_text == "重开") continue;
        if (action.input_text == "查看目标") continue;
        if (!action.enabled) continue;
        auto response = service.handleTurn(turn("s_p37_buttons", client_turn++, action.input_text));
        assert(response.is_ok());
        assert(response.value().reply_text.zh_cn.find("feedback_not_found") == std::string::npos);
    }
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
    else if (name == "story") test_story_first_day_projection_and_actions();
    else if (name == "story_beast_use") test_story_beast_use_not_torch_repel();
    else if (name == "buttons_executable") test_playable_buttons_are_executable();
    else if (name == "p38_world") test_p38_world_state_visible_consequences();
    else if (name == "p38_agent") test_p38_agent_fire_avoidance_visible();
    else if (name == "p38_beast_no_fake_fire") test_p38_beast_progresses_without_fake_fire();
    else if (name == "p38_failure") test_p38_resource_failure_is_readable();
    else return 2;
    std::cout << "h5_playable " << name << " passed\n";
    return 0;
}
