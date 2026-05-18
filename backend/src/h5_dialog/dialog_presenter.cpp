#include "pathfinder/h5_dialog/dialog_presenter.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <sstream>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::learning::LearningLoopResult;
using pathfinder::learning::LearningLoopDecision;
using pathfinder::learning::LearningLoopStage;
using pathfinder::learning::LearningStepDecision;
using pathfinder::learning::LearningDebugReport;
using pathfinder::knowledge::toString;

namespace {

std::string actionDescription(DialogActionKind action, const std::string& object_name) {
    switch (action) {
        case DialogActionKind::Eat:
            return "你吃了" + object_name + "。";
        case DialogActionKind::Use:
            return "你使用了" + object_name + "。";
        default:
            return "你尝试了某个动作。";
    }
}

std::string effectDescription(const std::string& effect_key) {
    if (effect_key == "restore_hunger") return "身体的需求有所缓解。";
    if (effect_key == "poison") return "你感到不适，健康状况变差了。";
    if (effect_key == "no_visible_effect") return "似乎没有什么明显的效果。";
    if (effect_key == "use_hint") return "你感觉到这个工具有某种用途。";
    return "你感知到了一些变化。";
}

std::vector<DialogQuickActionDto> defaultQuickActions() {
    std::vector<DialogQuickActionDto> actions;
    auto add = [&](const std::string& label, const std::string& input, DialogIntentKind kind) {
        DialogQuickActionDto qa;
        qa.label_text = label;
        qa.input_text = input;
        qa.intent_kind = kind;
        actions.push_back(qa);
    };
    add("观察", "观察", DialogIntentKind::Observe);
    add("吃红果", "吃红果", DialogIntentKind::TryEat);
    add("教同伴", "教同伴", DialogIntentKind::TeachRecipient);
    add("查看知识", "查看知识", DialogIntentKind::InspectActorKnowledge);
    add("查看同伴", "查看同伴", DialogIntentKind::InspectRecipientKnowledge);
    add("吃腐烂红果", "吃腐烂红果", DialogIntentKind::TryEat);
    add("帮助", "帮助", DialogIntentKind::Help);
    add("重开", "重开", DialogIntentKind::Restart);
    return actions;
}

bool hasCondition(const pathfinder::knowledge::KnowledgeClaim& claim, const std::string& condition_key) {
    for (const auto& condition : claim.conditions) {
        if (condition.condition_key == condition_key) {
            return true;
        }
    }
    return false;
}

std::string conditionDebugSuffix(const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (claim.conditions.empty()) {
        return "";
    }
    std::string suffix = "{";
    for (size_t i = 0; i < claim.conditions.size(); ++i) {
        if (i > 0) suffix += ",";
        suffix += claim.conditions[i].condition_key;
    }
    suffix += "}";
    return suffix;
}

std::string objectChineseName(const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (claim.subject.subject_id == "red_berry" && hasCondition(claim, "decayed")) return "腐烂红果";
    if (claim.subject.subject_id == "red_berry") return "红果";
    if (claim.subject.subject_id == "decayed_red_berry") return "腐烂红果";
    if (claim.subject.subject_id == "bitter_leaf") return "苦叶";
    if (claim.subject.subject_id == "stone_flake") return "石片";
    return claim.subject.subject_id;
}

std::string actionChineseName(const std::string& action_key) {
    if (action_key == "eat") return "吃";
    if (action_key == "use") return "使用";
    if (action_key == "observe") return "观察";
    if (action_key == "teach") return "传授";
    return action_key;
}

std::string effectChineseName(const std::string& effect_key) {
    if (effect_key == "restore_hunger") return "可以缓解饥饿";
    if (effect_key == "poison") return "会让身体不适或中毒";
    if (effect_key == "no_visible_effect") return "暂时没有明显效果";
    if (effect_key == "use_hint") return "可能有工具用途";
    return effect_key;
}

std::string statusChineseName(pathfinder::knowledge::KnowledgeStatus status) {
    switch (status) {
        case pathfinder::knowledge::KnowledgeStatus::Candidate: return "候选判断，还不稳定";
        case pathfinder::knowledge::KnowledgeStatus::Hypothesis: return "经验假设，需要继续验证";
        case pathfinder::knowledge::KnowledgeStatus::Active: return "已确认的有效知识";
        case pathfinder::knowledge::KnowledgeStatus::Teachable: return "可稳定传授的知识";
        case pathfinder::knowledge::KnowledgeStatus::Shared: return "已经分享过的知识";
        case pathfinder::knowledge::KnowledgeStatus::Operational: return "可直接用于行动的知识";
        case pathfinder::knowledge::KnowledgeStatus::Deprecated: return "旧知识，已不推荐使用";
        case pathfinder::knowledge::KnowledgeStatus::Disproven: return "已被否定的知识";
        case pathfinder::knowledge::KnowledgeStatus::Unknown: return "未知状态";
        case pathfinder::knowledge::KnowledgeStatus::TestOnly: return "测试状态";
    }
    return "未知状态";
}

std::string knowledgeChineseNote(const pathfinder::knowledge::KnowledgeClaim& claim) {
    std::string note = objectChineseName(claim);
    note += "：" + actionChineseName(claim.predicate.action_key);
    note += "后" + effectChineseName(claim.predicate.effect_key);
    note += "；" + statusChineseName(claim.status);
    return note;
}

std::string knowledgeSummaryLine(const pathfinder::knowledge::KnowledgeClaim& claim) {
    std::string line = claim.subject.subject_id + conditionDebugSuffix(claim);
    line += " + " + claim.predicate.action_key;
    line += " -> " + claim.predicate.effect_key;
    line += " [" + toString(claim.status) + "]";
    line += "（" + knowledgeChineseNote(claim) + "）";
    return line;
}

bool shouldShowKnowledgeLine(const pathfinder::knowledge::KnowledgeClaim& claim) {
    return claim.status != pathfinder::knowledge::KnowledgeStatus::Deprecated &&
           claim.status != pathfinder::knowledge::KnowledgeStatus::Disproven;
}

std::vector<std::string> knowledgeLines(const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    std::vector<std::string> lines;
    for (const auto& claim : claims) {
        if (shouldShowKnowledgeLine(claim)) {
            lines.push_back(knowledgeSummaryLine(claim));
        }
    }
    return lines;
}

void appendKnowledgeBlock(std::ostringstream& oss,
                          const std::string& title,
                          const std::vector<std::string>& lines) {
    oss << "\n" << title;
    if (lines.empty()) {
        oss << "（还没有什么知识）";
        return;
    }
    for (const auto& line : lines) {
        oss << "\n- " << line;
    }
}

} // namespace

Result<DialogResponseDto> DialogPresenter::buildObserveResponse(const DialogSessionState& state) const {
    DialogResponseDto resp;
    resp.session_id = state.session_id;
    resp.decision = DialogTurnDecision::RepliedOnly;

    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    auto scenario = scenario_r.value();

    std::ostringstream oss;
    oss << scenario.welcome_text << "\n\n";
    oss << "你能看见的东西：";
    for (size_t i = 0; i < state.visible_object_keys.size(); ++i) {
        if (i > 0) oss << "、";
        auto obj_r = catalog.findObject(scenario, state.visible_object_keys[i]);
        if (obj_r.is_ok()) {
            oss << obj_r.value()->display_name;
        }
    }
    oss << "。";
    resp.reply_text = oss.str();

    resp.state_summary_lines.push_back("当前主体：先驱者");
    resp.state_summary_lines.push_back("同伴：同伴");
    resp.quick_actions = defaultQuickActions();
    return Result<DialogResponseDto>::ok(resp);
}

Result<DialogResponseDto> DialogPresenter::buildLearningResponse(
    const DialogSessionState& state,
    const DialogIntent& intent,
    const LearningLoopResult& result,
    const LearningDebugReport& report) const {

    DialogResponseDto resp;
    resp.session_id = state.session_id;

    if (result.decision == LearningLoopDecision::Completed || result.decision == LearningLoopDecision::PartialCompleted) {
        resp.decision = intent.kind == DialogIntentKind::TeachRecipient ? DialogTurnDecision::TeachingRan : DialogTurnDecision::LearningLoopRan;
    } else if (result.decision == LearningLoopDecision::RoutedToRevision) {
        resp.decision = DialogTurnDecision::LearningLoopRan;
    } else {
        resp.decision = DialogTurnDecision::Failed;
    }

    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    auto scenario = scenario_r.value();

    std::string object_name = intent.object_key;
    auto obj_r = catalog.findObject(scenario, intent.object_key);
    if (obj_r.is_ok()) object_name = obj_r.value()->display_name;

    std::ostringstream oss;

    // 1. 行动确认
    if (intent.kind == DialogIntentKind::TeachRecipient) {
        oss << "你把知道的事情告诉了同伴。";
    } else {
        oss << actionDescription(intent.action, object_name);
    }
    oss << "\n";

    // 2. 感知反馈
    if (intent.kind != DialogIntentKind::TeachRecipient) {
        auto fb_r = catalog.findFeedback(scenario, intent.object_key, intent.action);
        if (fb_r.is_ok()) {
            oss << effectDescription(fb_r.value().effect_key) << "\n";
        }
    }

    // 3. 学习过程
    bool has_formed = false;
    bool has_propagated = false;
    bool has_revised = false;
    for (const auto& trace : result.traces) {
        if (trace.decision == LearningStepDecision::Executed) {
            if (trace.stage == LearningLoopStage::KnowledgeFormed) has_formed = true;
            if (trace.stage == LearningLoopStage::KnowledgePropagated) has_propagated = true;
            if (trace.stage == LearningLoopStage::KnowledgeRevised) has_revised = true;
        } else if (trace.decision == LearningStepDecision::Routed &&
                   trace.stage == LearningLoopStage::KnowledgeRevised) {
            has_revised = true;
        }
    }

    if (intent.kind == DialogIntentKind::TeachRecipient) {
        if (has_propagated) {
            oss << "同伴听取了你的讲述，并形成了自己的认知。\n";
        } else {
            oss << "同伴没有从你这里获得新的信息。\n";
        }
    } else {
        if (has_formed) {
            oss << "先驱者把这次经历写入记忆，并形成了一个新的知识。\n";
        } else if (result.memory_write_result.has_value() && !result.memory_write_result.value().created_records.empty()) {
            oss << "先驱者把这次经历写入了记忆。\n";
        }
        if (has_revised) {
            oss << "旧知识因为新经历被修正了。\n";
        }
    }

    resp.actor_knowledge_lines = knowledgeLines(state.actor_claims);
    resp.recipient_knowledge_lines = knowledgeLines(state.recipient_claims);
    appendKnowledgeBlock(oss, "你现在知道：", resp.actor_knowledge_lines);
    appendKnowledgeBlock(oss, "同伴现在知道：", resp.recipient_knowledge_lines);
    resp.reply_text = oss.str();

    // 5. Debug keys
    if (has_formed) resp.debug_keys.push_back("knowledge_formed");
    if (has_propagated) resp.debug_keys.push_back("knowledge_propagated");
    if (has_revised) resp.debug_keys.push_back("knowledge_revised_routed");
    if (result.decision == LearningLoopDecision::Completed) {
        resp.debug_keys.push_back("learning_loop_completed");
    }

    resp.state_summary_lines.push_back("回合：" + std::to_string(state.turn_index));
    resp.quick_actions = defaultQuickActions();

    return Result<DialogResponseDto>::ok(resp);
}

Result<DialogResponseDto> DialogPresenter::buildInspectResponse(
    const DialogSessionState& state,
    bool inspect_recipient) const {

    DialogResponseDto resp;
    resp.session_id = state.session_id;
    resp.decision = DialogTurnDecision::RepliedOnly;

    resp.actor_knowledge_lines = knowledgeLines(state.actor_claims);
    resp.recipient_knowledge_lines = knowledgeLines(state.recipient_claims);

    std::ostringstream oss;
    if (inspect_recipient) {
        appendKnowledgeBlock(oss, "同伴的知识：", resp.recipient_knowledge_lines);
        appendKnowledgeBlock(oss, "你的知识：", resp.actor_knowledge_lines);
    } else {
        appendKnowledgeBlock(oss, "你的知识：", resp.actor_knowledge_lines);
        appendKnowledgeBlock(oss, "同伴的知识：", resp.recipient_knowledge_lines);
    }
    resp.reply_text = oss.str();

    resp.state_summary_lines.push_back("回合：" + std::to_string(state.turn_index));
    resp.quick_actions = defaultQuickActions();
    return Result<DialogResponseDto>::ok(resp);
}

Result<DialogResponseDto> DialogPresenter::buildRejectedResponse(
    const DialogSessionState& state,
    const DialogIntent& intent,
    const std::string& reason_key) const {

    DialogResponseDto resp;
    resp.session_id = state.session_id;
    resp.decision = DialogTurnDecision::Rejected;

    if (reason_key == "object_not_visible") {
        resp.reply_text = "你看不见那个东西。";
    } else if (reason_key == "action_not_allowed") {
        resp.reply_text = "你不能对那个东西做这个动作。";
    } else if (reason_key == "unknown_intent") {
        resp.reply_text = "我不明白你想做什么。试试输入“帮助”查看可用指令。";
    } else if (reason_key == "no_teachable_knowledge") {
        resp.reply_text = "你还没有可以教给同伴的知识。先尝试一个东西，再回来教学。";
    } else if (reason_key == "learning_loop_failed") {
        resp.reply_text = "这次行动没有形成有效结果，已有知识不会被覆盖。";
    } else {
        resp.reply_text = "这个动作无法进行：" + reason_key;
    }

    resp.state_summary_lines.push_back("回合：" + std::to_string(state.turn_index));
    resp.quick_actions = defaultQuickActions();
    return Result<DialogResponseDto>::ok(resp);
}

Result<DialogResponseDto> DialogPresenter::buildHelpResponse(const DialogSessionState& state) const {
    DialogResponseDto resp;
    resp.session_id = state.session_id;
    resp.decision = DialogTurnDecision::RepliedOnly;
    resp.reply_text =
        "可用指令：\n"
        "观察 / 看看周围 — 查看环境中的东西\n"
        "吃红果 / 吃一口红果 — 尝试吃红果\n"
        "吃腐烂红果 — 尝试吃腐烂红果\n"
        "吃苦叶 — 尝试吃苦叶\n"
        "使用石片 / 用石片 — 尝试使用石片\n"
        "查看知识 / 我知道什么 — 查看你已形成的知识\n"
        "查看同伴 / 同伴知道什么 — 查看同伴的知识\n"
        "告诉同伴 / 教同伴 — 把知识教给同伴\n"
        "再吃一口 / 继续吃 — 重复上一轮动作\n"
        "重开 / 重新开始 — 重置当前会话";
    resp.state_summary_lines.push_back("回合：" + std::to_string(state.turn_index));
    resp.quick_actions = defaultQuickActions();
    return Result<DialogResponseDto>::ok(resp);
}

Result<DialogResponseDto> DialogPresenter::buildRestartResponse(const DialogSessionState& state) const {
    DialogResponseDto resp;
    resp.session_id = state.session_id;
    resp.decision = DialogTurnDecision::RepliedOnly;
    resp.reply_text = "会话已重置。一切从头开始。";
    resp.state_summary_lines.push_back("回合：0");
    resp.quick_actions = defaultQuickActions();
    return Result<DialogResponseDto>::ok(resp);
}

} // namespace pathfinder::h5_dialog
