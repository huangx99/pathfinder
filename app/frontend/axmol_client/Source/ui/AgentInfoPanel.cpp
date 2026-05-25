#include "ui/AgentInfoPanel.h"
#include "ui/UiStyle.h"

#include "axmol/ui/UIScrollView.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace pf::ui {
namespace {
constexpr float kWidth = 280.0F;
constexpr float kHeight = 570.0F;
constexpr float kScrollX = 12.0F;
constexpr float kScrollY = 14.0F;
constexpr float kScrollWidth = kWidth - 24.0F;
constexpr float kScrollHeight = kHeight - 72.0F;
constexpr float kInnerHeight = 980.0F;

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
    if (key == "no_visible_effect") return "无明显效果";
    if (key == "active") return "已确认";
    if (key == "hypothesis") return "假设";
    if (key == "candidate") return "候选";
    return humanizeDebugText(key);
}

std::string percentText(double value) {
    return std::to_string(static_cast<int>(std::round(std::clamp(value, 0.0, 100.0)))) + "%";
}

ax::ui::ScrollView* makeScrollView() {
    auto* scroll = ax::ui::ScrollView::create();
    scroll->setContentSize(ax::Size(kScrollWidth, kScrollHeight));
    scroll->setInnerContainerSize(ax::Size(kScrollWidth, kInnerHeight));
    scroll->setDirection(ax::ui::ScrollView::Direction::VERTICAL);
    scroll->setBounceEnabled(true);
    scroll->setScrollBarEnabled(true);
    scroll->setScrollBarWidth(4.0F);
    scroll->setScrollBarColor(ax::Color32(148, 163, 184, 255));
    scroll->setScrollBarOpacity(180);
    scroll->setScrollBarPositionFromCorner(ax::Vec2(2.0F, 2.0F));
    return scroll;
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

    has_current_agent_ = agent != nullptr;
    if (agent) current_agent_ = *agent;
    if (!agent) selected_inventory_key_.clear();

    auto* title = panelLabel(agent ? agent->name : "选择小人", 22.0F, ax::Vec2(kWidth - 28.0F, 30.0F));
    title->setTextColor(ax::Color32(250, 204, 21, 255));
    title->setPosition(14.0F, kHeight - 16.0F);
    addChild(title, 2);

    auto* scroll = makeScrollView();
    scroll->setPosition(ax::Vec2(kScrollX, kScrollY));
    addChild(scroll, 2);
    LayoutTarget target{scroll, kScrollWidth};
    float y = kInnerHeight - 12.0F;

    if (!agent) {
        addLine("点击地图上的小人查看详情。", y, target, 26.0F, 15.0F);
        addSectionTitle("可查看", y, target);
        addLine("人物状态", y, target);
        addLine("背包", y, target);
        addLine("知识", y, target);
        addLine("执行历史", y, target);
        return;
    }

    addLine("位置：(" + std::to_string(agent->x) + "," + std::to_string(agent->y) + ")", y, target, 22.0F, 14.0F);
    addLine("当前：" + shorten(humanizeDebugText(agent->current_intent), 24), y, target, 42.0F, 14.0F);
    addMeter("生命", agent->health, color(34, 197, 94, 0.95F), y, target);
    addMeter("饥饿", agent->hunger, color(251, 191, 36, 0.95F), y, target);

    addSectionTitle((inventory_expanded_ ? "▼ " : "▶ ") + std::string("背包"), y, target, [this]() {
        inventory_expanded_ = !inventory_expanded_;
        if (has_current_agent_) setAgent(&current_agent_);
    });
    if (inventory_expanded_) {
        if (agent->inventory.empty()) {
            addLine("空", y, target);
        } else {
            for (const auto& item : agent->inventory) {
                const bool selected = item.object_key == selected_inventory_key_;
                addLine((selected ? "● " : "○ ") + shorten(item.display_name + " ×" + std::to_string(item.quantity), 22), y, target, 26.0F, 13.0F, [this, key = item.object_key]() {
                    selected_inventory_key_ = selected_inventory_key_ == key ? std::string{} : key;
                    if (has_current_agent_) setAgent(&current_agent_);
                });
            }
        }
    }

    addSectionTitle((knowledge_expanded_ ? "▼ " : "▶ ") + std::string("知识"), y, target, [this]() {
        knowledge_expanded_ = !knowledge_expanded_;
        if (has_current_agent_) setAgent(&current_agent_);
    });
    if (knowledge_expanded_) {
        int shown = 0;
        for (const auto& claim : agent->knowledge) {
            if (!selected_inventory_key_.empty() && claim.subject_object_key != selected_inventory_key_) continue;
            const auto text = displayKey(claim.subject_object_key) + " → " + displayKey(claim.action_key) + " → " + displayKey(claim.effect_key) + " · " + displayKey(claim.status);
            addLine(shorten(text, 28), y, target, 28.0F, 13.0F);
            ++shown;
        }
        if (shown == 0) addLine(selected_inventory_key_.empty() ? "未知" : "这个物品还没有知识", y, target);
    }

    addSectionTitle("执行历史", y, target);
    if (agent->recent_memory.empty()) {
        addLine("暂无", y, target);
    } else {
        for (auto it = agent->recent_memory.rbegin(); it != agent->recent_memory.rend(); ++it) {
            addLine(shorten(humanizeDebugText(*it), 30), y, target, 30.0F, 13.0F);
        }
    }
}

void AgentInfoPanel::addSectionTitle(const std::string& text, float& y, LayoutTarget target, const std::function<void()>& on_click) {
    y -= 8.0F;
    addLine(text, y, target, 24.0F, 15.0F, on_click);
}

void AgentInfoPanel::addLine(const std::string& text, float& y, LayoutTarget target, float height, float size, const std::function<void()>& on_click) {
    if (y < 18.0F) return;
    auto* line = panelLabel(text, size, ax::Vec2(target.width - 8.0F, height));
    if (on_click) {
        line->setPosition(ax::Vec2::ZERO);
        auto* item = ax::MenuItemLabel::create(line, [on_click](ax::Object*) { on_click(); });
        item->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
        item->setPosition(4.0F, y);
        auto* menu = ax::Menu::create(item, nullptr);
        menu->setPosition(ax::Vec2::ZERO);
        target.parent->addChild(menu, 2);
    } else {
        line->setPosition(4.0F, y);
        target.parent->addChild(line, 2);
    }
    y -= height;
}

void AgentInfoPanel::addMeter(const std::string& label_text, double value, const ax::Color& fill, float& y, LayoutTarget target) {
    if (y < 38.0F) return;
    auto* title = panelLabel(label_text + " " + percentText(value), 13.0F, ax::Vec2(74.0F, 18.0F));
    title->setPosition(4.0F, y);
    target.parent->addChild(title, 2);

    const float bar_x = 80.0F;
    const float bar_y = y - 1.0F;
    const float bar_w = target.width - 92.0F;
    const float bar_h = 12.0F;
    const float ratio = static_cast<float>(std::clamp(value, 0.0, 100.0) / 100.0);
    auto* bar = ax::DrawNode::create();
    bar->drawSolidRect(ax::Vec2(bar_x, bar_y), ax::Vec2(bar_x + bar_w, bar_y + bar_h), color(30, 41, 59, 0.95F));
    bar->drawSolidRect(ax::Vec2(bar_x, bar_y), ax::Vec2(bar_x + bar_w * ratio, bar_y + bar_h), fill);
    bar->drawRect(ax::Vec2(bar_x, bar_y), ax::Vec2(bar_x + bar_w, bar_y + bar_h), color(148, 163, 184, 0.65F));
    target.parent->addChild(bar, 1);
    y -= 26.0F;
}

} // namespace pf::ui
