#include "pathfinder/h5_dialog/dialog_intent.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

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

std::string detectObjectKey(const std::string& text) {
    if (text.find("制作火把") != std::string::npos || text.find("做火把") != std::string::npos) return "wood";
    const bool mentions_torch = text.find("火把") != std::string::npos;
    const bool mentions_beast = text.find("野兽") != std::string::npos || text.find("低吼") != std::string::npos || text.find("影子") != std::string::npos;
    const bool explicitly_repels = text.find("驱赶") != std::string::npos || text.find("赶走") != std::string::npos || text.find("吓退") != std::string::npos;
    if (text.find("腐烂红果") != std::string::npos) return "decayed_red_berry";
    if (text.find("红果") != std::string::npos) return "red_berry";
    if (text.find("磨石") != std::string::npos || text.find("打磨") != std::string::npos || text.find("磨一下") != std::string::npos) return "whetstone";
    if (text.find("斧头") != std::string::npos || text.find("斧子") != std::string::npos) return "axe";
    if (text.find("苦叶") != std::string::npos) return "bitter_leaf";
    if (text.find("石片") != std::string::npos) return "stone_flake";
    if ((mentions_torch && mentions_beast) || explicitly_repels) return "torch";
    if (text.find("火把") != std::string::npos) return "torch";
    if (mentions_beast) return "beast_shadow";
    if (text.find("火种") != std::string::npos || text.find("点燃") != std::string::npos || text.find("火源") != std::string::npos || text.find("生火") != std::string::npos) return "fire_seed";
    if (text.find("干草") != std::string::npos || text.find("草") != std::string::npos) return "dry_grass";
    if (text.find("木头") != std::string::npos || text.find("木材") != std::string::npos || text.find("制作火把") != std::string::npos) return "wood";
    return "";
}

std::string detectTargetObjectKey(const std::string& text, const std::string& object_key) {
    if (object_key == "axe" && (text.find("木头") != std::string::npos || text.find("木材") != std::string::npos || text.find("砍") != std::string::npos)) return "wood";
    if (object_key == "whetstone" && (text.find("斧头") != std::string::npos || text.find("斧子") != std::string::npos || text.find("打磨") != std::string::npos || text.find("磨") != std::string::npos)) return "axe";
    if (object_key == "fire_seed" && (text.find("干草") != std::string::npos || text.find("草") != std::string::npos || text.find("火源") != std::string::npos || text.find("生火") != std::string::npos || text.find("点燃") != std::string::npos)) return "dry_grass";
    if (object_key == "wood" && (text.find("火把") != std::string::npos || text.find("制作") != std::string::npos)) return "fire_seed";
    if (object_key == "torch" && (text.find("野兽") != std::string::npos || text.find("驱赶") != std::string::npos || text.find("影子") != std::string::npos || text.find("低吼") != std::string::npos)) return "beast_shadow";
    return "";
}

DialogActionKind defaultActionForObject(const std::string& object_key) {
    if (object_key == "stone_flake" || object_key == "axe" || object_key == "whetstone" || object_key == "wood" || object_key == "dry_grass" || object_key == "fire_seed" || object_key == "torch" || object_key == "beast_shadow") return DialogActionKind::Use;
    if (!object_key.empty()) return DialogActionKind::Eat;
    return DialogActionKind::Unknown;
}

DialogIntentKind kindFromAction(DialogActionKind action) {
    if (action == DialogActionKind::Eat) return DialogIntentKind::TryEat;
    if (action == DialogActionKind::Use) return DialogIntentKind::TryUse;
    return DialogIntentKind::Unknown;
}

DialogActionKind detectLearningAction(const std::string& text, const std::string& object_key) {
    if (containsAny(text, {"吃", "咬", "吞"})) return DialogActionKind::Eat;
    if (containsAny(text, {"使用", "用", "拿", "试用", "砍", "打磨", "磨"})) return DialogActionKind::Use;
    if (containsAny(text, {"尝试", "试试"})) return defaultActionForObject(object_key);
    return defaultActionForObject(object_key);
}

} // namespace

Result<DialogIntent> DialogIntentParser::parse(const std::string& input_text) const {
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

    const auto object_key = detectObjectKey(trimmed);
    if (!object_key.empty()) {
        intent.object_key = object_key;
        intent.target_object_key = detectTargetObjectKey(trimmed, object_key);
        intent.action = detectLearningAction(trimmed, object_key);
        intent.kind = kindFromAction(intent.action);
        return Result<DialogIntent>::ok(intent);
    }

    intent.kind = DialogIntentKind::Unknown;
    intent.action = DialogActionKind::Unknown;
    return Result<DialogIntent>::ok(intent);
}

} // namespace pathfinder::h5_dialog
