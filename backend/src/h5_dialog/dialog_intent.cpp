// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#include "pathfinder/h5_dialog/dialog_intent.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

std::string trim(const std::string& s) {
    auto start = s.begin();
    auto end = s.end();
    while (start != end && std::isspace(*start)) ++start;
    while (end != start && std::isspace(*(end - 1))) --end;
    return std::string(start, end);
}

std::string toLower(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (unsigned char c : s) r.push_back(static_cast<char>(std::tolower(c)));
    return r;
}

bool containsAny(const std::string& text, const std::vector<std::string>& keywords) {
    for (const auto& keyword : keywords) {
        if (text.find(keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool aliasesMatch(const std::string& text, const DialogScenarioObject& object) {
    if (!object.display_name.empty() && text.find(object.display_name) != std::string::npos) return true;
    for (const auto& alias : object.input_aliases) {
        if (!alias.empty() && text.find(alias) != std::string::npos) return true;
    }
    return false;
}

const DialogScenarioActionTemplate* matchingScenarioAction(const std::string& text, const DialogScenario& scenario) {
    const DialogScenarioActionTemplate* best = nullptr;
    size_t best_length = 0;
    for (const auto& action : scenario.suggested_action_templates) {
        if (action.input_text.empty() || action.object_key.empty()) continue;
        if (text.find(action.input_text) == std::string::npos) continue;
        if (action.input_text.size() >= best_length) {
            best = &action;
            best_length = action.input_text.size();
        }
    }
    return best;
}

const DialogScenarioObject* findScenarioObject(const DialogScenario& scenario, const std::string& object_key) {
    auto it = std::find_if(scenario.objects.begin(), scenario.objects.end(), [&](const auto& object) { return object.object_key == object_key; });
    return it == scenario.objects.end() ? nullptr : &*it;
}

std::string detectObjectKey(const std::string& text, const DialogScenario& scenario) {
    for (const auto& feedback : scenario.feedbacks) {
        if (feedback.target_object_key.empty()) continue;
        const auto* source = findScenarioObject(scenario, feedback.object_key);
        const auto* target = findScenarioObject(scenario, feedback.target_object_key);
        if (source != nullptr && target != nullptr && aliasesMatch(text, *source) && aliasesMatch(text, *target)) return feedback.object_key;
    }
    std::string best_key;
    size_t best_length = 0;
    for (const auto& object : scenario.objects) {
        if (!aliasesMatch(text, object)) continue;
        size_t candidate_length = object.display_name.size();
        for (const auto& alias : object.input_aliases) {
            if (text.find(alias) != std::string::npos) candidate_length = std::max(candidate_length, alias.size());
        }
        if (candidate_length >= best_length) {
            best_key = object.object_key;
            best_length = candidate_length;
        }
    }
    return best_key;
}

bool textMentionsObject(const std::string& text, const DialogScenarioObject& object) {
    return aliasesMatch(text, object);
}

std::string detectTargetObjectKey(const std::string& text, const std::string& object_key, DialogActionKind action, const DialogScenario& scenario) {
    for (const auto& feedback : scenario.feedbacks) {
        if (feedback.object_key != object_key || feedback.action != action || feedback.target_object_key.empty()) continue;
        auto it = std::find_if(scenario.objects.begin(), scenario.objects.end(), [&](const auto& object) { return object.object_key == feedback.target_object_key; });
        if (it != scenario.objects.end() && textMentionsObject(text, *it)) return feedback.target_object_key;
    }
    return "";
}

DialogActionKind defaultActionForObject(const std::string& object_key, const DialogScenario& scenario) {
    auto it = std::find_if(scenario.objects.begin(), scenario.objects.end(), [&](const auto& object) { return object.object_key == object_key; });
    if (it != scenario.objects.end()) {
        if (it->default_action != DialogActionKind::Unknown) return it->default_action;
        if (!it->allowed_actions.empty()) return it->allowed_actions.front();
    }
    return DialogActionKind::Unknown;
}

DialogIntentKind kindFromAction(DialogActionKind action) {
    if (action == DialogActionKind::Eat) return DialogIntentKind::TryEat;
    if (action == DialogActionKind::Use) return DialogIntentKind::TryUse;
    return DialogIntentKind::Unknown;
}

DialogActionKind detectLearningAction(const std::string& text, const std::string& object_key, const DialogScenario& scenario) {
    if (containsAny(text, {"吃", "咬", "吞"})) return DialogActionKind::Eat;
    if (containsAny(text, {"使用", "用", "拿", "试用", "砍", "打磨", "磨"})) return DialogActionKind::Use;
    if (containsAny(text, {"尝试", "试试"})) return defaultActionForObject(object_key, scenario);
    return defaultActionForObject(object_key, scenario);
}

} // namespace

Result<DialogIntent> DialogIntentParser::parse(const std::string& input_text) const {
    DialogScenarioCatalog catalog;
    auto scenario = catalog.defaultScenario();
    if (scenario.is_error()) return Result<DialogIntent>::fail(scenario.errors());
    return parseWithScenario(input_text, scenario.value());
}

Result<DialogIntent> DialogIntentParser::parseWithScenario(const std::string& input_text, const DialogScenario& scenario) const {
    auto trimmed = trim(input_text);
    if (trimmed.empty()) {
        return Result<DialogIntent>::fail(makeError(ErrorCode::validation_failed, "empty input"));
    }

    DialogIntent intent;
    intent.normalized_text = trimmed;
    intent.recipient_key = "companion";

    const auto lowered = toLower(trimmed);
    auto textContainsAny = [&](const std::vector<std::string>& keywords) {
        return containsAny(trimmed, keywords) || containsAny(lowered, keywords);
    };

    if (textContainsAny({"重开", "重新开始", "重置"})) {
        intent.kind = DialogIntentKind::Restart;
        intent.action = DialogActionKind::Restart;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"帮助", "指令", "说明", "怎么用"})) {
        intent.kind = DialogIntentKind::Help;
        intent.action = DialogActionKind::Observe;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"等待", "休息", "过一会", "等一会", "等一下"})) {
        intent.kind = DialogIntentKind::Wait;
        intent.action = DialogActionKind::Wait;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"查看同伴", "同伴知道什么", "同伴知识"})) {
        intent.kind = DialogIntentKind::InspectRecipientKnowledge;
        intent.action = DialogActionKind::Inspect;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"查看知识", "我知道什么", "我的知识", "知识"})) {
        intent.kind = DialogIntentKind::InspectActorKnowledge;
        intent.action = DialogActionKind::Inspect;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"告诉同伴", "教同伴", "教学", "传授"})) {
        intent.kind = DialogIntentKind::TeachRecipient;
        intent.action = DialogActionKind::Teach;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"观察", "看看周围", "看一下", "环顾"})) {
        intent.kind = DialogIntentKind::Observe;
        intent.action = DialogActionKind::Observe;
        return Result<DialogIntent>::ok(intent);
    }

    if (textContainsAny({"再吃一口", "继续吃", "再来一次", "重复"})) {
        intent.kind = DialogIntentKind::RepeatLastAction;
        intent.action = DialogActionKind::Unknown;
        return Result<DialogIntent>::ok(intent);
    }

    if (const auto* action_template = matchingScenarioAction(trimmed, scenario)) {
        intent.object_key = action_template->object_key;
        intent.target_object_key = action_template->target_object_key;
        intent.action = DialogActionKind::Use;
        intent.kind = DialogIntentKind::TryUse;
        intent.reason_keys.push_back("scenario_action_template:" + action_template->action_key);
        return Result<DialogIntent>::ok(intent);
    }

    const auto object_key = detectObjectKey(trimmed, scenario);
    if (!object_key.empty()) {
        intent.object_key = object_key;
        intent.action = detectLearningAction(trimmed, object_key, scenario);
        intent.target_object_key = detectTargetObjectKey(trimmed, object_key, intent.action, scenario);
        intent.kind = kindFromAction(intent.action);
        return Result<DialogIntent>::ok(intent);
    }

    intent.kind = DialogIntentKind::Unknown;
    intent.action = DialogActionKind::Unknown;
    return Result<DialogIntent>::ok(intent);
}

} // namespace pathfinder::h5_dialog
