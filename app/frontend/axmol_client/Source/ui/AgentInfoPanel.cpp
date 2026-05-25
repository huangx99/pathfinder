#include "ui/AgentInfoPanel.h"
#include "ui/UiStyle.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace pf::ui {
namespace {
constexpr float kWidth = 280.0F;
constexpr float kHeight = 570.0F;

std::string displayKey(const std::string& key) {
    if (key == "red_berry") return "红果";
    if (key == "decayed_red_berry") return "腐烂红果";
    if (key == "bitter_leaf") return "苦叶";
    if (key == "stone_flake") return "石片";
    if (key == "wood") return "木头";
    if (key == "eat") return "吃";
    if (key == "use") return "使用";
    if (key == "restore_hunger") return "缓解饥饿";
    if (key == "poison") return "不适/中毒";
    if (key == "use_hint") return "发现用途";
    if (key == "active") return "已确认";
    if (key == "hypothesis") return "假设";
    if (key == "candidate") return "候选";
    return key;
}

std::string percentText(double value) {
    return std::to_string(static_cast<int>(std::round(std::clamp(value, 0.0, 100.0)))) + "%";
}
} // namespace

AgentInfoPanel* AgentInfoPanel::create() {
    auto* panel = new AgentInfoPanel();
    if (panel && panel->init()) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool AgentInfoPanel::init() {
    if (!Node::init()) return false;
    setContentSize(ax::Size(kWidth, kHeight));
    setAgent(nullptr);
    return true;
}

void AgentInfoPanel::setAgent(const pathfinder::v3_sandbox::V3AgentView* agent) {
    removeAllChildren();
    addChild(panelBackground(getContentSize(), color(8, 13, 26, 0.94F)), 0);

    auto* title = panelLabel(agent ? agent->name : "选择小人", 22.0F, ax::Vec2(kWidth - 28.0F, 30.0F));
    title->setTextColor(ax::Color32(250, 204, 21, 255));
    title->setPosition(14.0F, kHeight - 16.0F);
    addChild(title, 2);

    float y = kHeight - 58.0F;
    if (!agent) {
        addLine("点击地图上的小人查看详情。", y, 26.0F, 15.0F);
        addSectionTitle("可查看", y);
        addLine("人物状态", y);
        addLine("背包", y);
        addLine("知识", y);
        addLine("执行历史", y);
        return;
    }

    addLine("位置：(" + std::to_string(agent->x) + "," + std::to_string(agent->y) + ")", y, 22.0F, 14.0F);
    addLine("当前：" + shorten(humanizeDebugText(agent->current_intent), 22), y, 38.0F, 14.0F);
    addMeter("生命", agent->health, color(34, 197, 94, 0.95F), y);
    addMeter("饥饿", agent->hunger, color(251, 191, 36, 0.95F), y);

    addSectionTitle("背包", y);
    if (agent->inventory.empty()) {
        addLine("空", y);
    } else {
        int shown = 0;
        for (const auto& item : agent->inventory) {
            addLine(shorten(item.display_name + " ×" + std::to_string(item.quantity), 24), y, 24.0F, 13.0F);
            if (++shown >= 5) break;
        }
    }

    addSectionTitle("知识", y);
    if (agent->knowledge.empty()) {
        addLine("未知", y);
    } else {
        int shown = 0;
        for (const auto& claim : agent->knowledge) {
            const auto text = displayKey(claim.subject_object_key) + " → " + displayKey(claim.action_key) + " → " + displayKey(claim.effect_key) + " · " + displayKey(claim.status);
            addLine(shorten(text, 25), y, 28.0F, 13.0F);
            if (++shown >= 6) break;
        }
    }

    addSectionTitle("执行历史", y);
    if (agent->recent_memory.empty()) {
        addLine("暂无", y);
    } else {
        int shown = 0;
        for (auto it = agent->recent_memory.rbegin(); it != agent->recent_memory.rend() && shown < 6; ++it, ++shown) {
            addLine(shorten(humanizeDebugText(*it), 25), y, 30.0F, 13.0F);
        }
    }
}

void AgentInfoPanel::addSectionTitle(const std::string& text, float& y) {
    y -= 8.0F;
    auto* section = panelLabel(text, 15.0F, ax::Vec2(kWidth - 28.0F, 22.0F));
    section->setTextColor(ax::Color32(147, 197, 253, 255));
    section->setPosition(14.0F, y);
    addChild(section, 2);
    y -= 26.0F;
}

void AgentInfoPanel::addLine(const std::string& text, float& y, float height, float size) {
    if (y < 18.0F) return;
    auto* line = panelLabel(text, size, ax::Vec2(kWidth - 28.0F, height));
    line->setPosition(14.0F, y);
    addChild(line, 2);
    y -= height;
}

void AgentInfoPanel::addMeter(const std::string& label_text, double value, const ax::Color& fill, float& y) {
    if (y < 38.0F) return;
    auto* title = panelLabel(label_text + " " + percentText(value), 13.0F, ax::Vec2(74.0F, 18.0F));
    title->setPosition(14.0F, y);
    addChild(title, 2);

    const float bar_x = 90.0F;
    const float bar_y = y - 1.0F;
    const float bar_w = 170.0F;
    const float bar_h = 12.0F;
    const float ratio = static_cast<float>(std::clamp(value, 0.0, 100.0) / 100.0);
    auto* bar = ax::DrawNode::create();
    bar->drawSolidRect(ax::Vec2(bar_x, bar_y), ax::Vec2(bar_x + bar_w, bar_y + bar_h), color(30, 41, 59, 0.95F));
    bar->drawSolidRect(ax::Vec2(bar_x, bar_y), ax::Vec2(bar_x + bar_w * ratio, bar_y + bar_h), fill);
    bar->drawRect(ax::Vec2(bar_x, bar_y), ax::Vec2(bar_x + bar_w, bar_y + bar_h), color(148, 163, 184, 0.65F));
    addChild(bar, 1);
    y -= 26.0F;
}

} // namespace pf::ui
