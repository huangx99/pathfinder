#pragma once

#include "axmol/axmol.h"
#include "local/LocalClientRuntime.h"
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class MainScene final : public ax::Scene {
public:
    bool init() override;
    void update(float delta) override;

    CREATE_FUNC(MainScene);

private:
    struct Coord {
        int x{0};
        int y{0};
    };

    struct EntityRenderState {
        ax::Node* node{nullptr};
        std::string visual_key;
        Coord coord{0, 0};
        bool has_coord{false};
    };

    struct ActionHitBox {
        ax::Rect rect;
        pf::client::LocalCommandOptionView option;
    };

    enum class InventoryPanelTab {
        Inventory,
        Crafting,
        Knowledge,
    };

    enum class UiHitKind {
        ToggleInventoryPanel,
        CloseInventoryPanel,
        InventoryTab,
        CraftingTab,
        KnowledgeTab,
        HotbarSlot,
        InventoryItem,
        CraftOption,
    };

    struct UiHitBox {
        ax::Rect rect;
        UiHitKind kind{UiHitKind::ToggleInventoryPanel};
        int index{-1};
    };

    void bootstrapLocalRuntime();
    void renderWorld();
    void renderTerrain();
    void renderWaterAnimation(float delta);
    void renderPath();
    void renderEntities();
    void renderSelection();
    void renderUi();
    void renderInventoryBar();
    void renderInventoryPanel();
    void renderContextPanel();
    void updateHud();
    void updateSelectionPanel();
    void syncPlayerCoord();
    void startPathTo(Coord target);
    void executeNextPathStep(float delta);
    void updateWorldAutoTick(float delta);
    void pollPendingCommand();
    void submitOptionAndRender(const pf::client::LocalCommandOptionView& option, bool consumes_path_step = false);
    void finishSubmittedOption(bool ok);
    void repaintForCurrentViewport();
    bool isCoordInRenderWindow(Coord coord) const;
    bool isAdjacentToPlayer(Coord coord) const;
    std::optional<pf::client::LocalEntityView> entityAt(Coord coord) const;
    std::vector<pf::client::LocalCommandOptionView> actionsForEntity(const std::string& entity_id) const;
    bool handleActionPanelClick(const ax::Vec2& screen);
    bool handleUiClick(const ax::Vec2& screen);
    bool isUiPointBlocked(const ax::Vec2& screen) const;
    void normalizeHotbarSlots(const std::vector<pf::client::LocalInventoryItemView>& items);
    int itemIndexForHotbarSlot(int slot, const std::vector<pf::client::LocalInventoryItemView>& items) const;
    void placeInventoryItemInHotbar(int slot, int item_index, const std::vector<pf::client::LocalInventoryItemView>& items);

    ax::Node* createTerrainNode(const pf::client::LocalCellView& cell) const;
    ax::Node* createEntityNode(const pf::client::LocalEntityView& entity) const;
    ax::Node* createInventoryIcon(const pf::client::LocalInventoryItemView& item) const;
    std::string entityRenderKey(const pf::client::LocalEntityView& entity) const;
    std::string entityVisualKey(const pf::client::LocalEntityView& entity) const;
    ax::Vec2 worldToLocal(const Coord& coord) const;
    ax::Vec2 screenToLocal(const ax::Vec2& screen) const;
    Coord screenToCoord(const ax::Vec2& screen) const;
    std::vector<Coord> buildVisualPath(Coord from, Coord to) const;

    bool onMouseDown(ax::EventMouse* event);
    bool onMouseUp(ax::EventMouse* event);
    bool onMouseMove(ax::EventMouse* event);
    bool onMouseScroll(ax::EventMouse* event);

    ax::Node* world_layer_{nullptr};
    ax::Node* terrain_layer_{nullptr};
    ax::Node* water_layer_{nullptr};
    ax::Node* path_layer_{nullptr};
    ax::Node* entity_layer_{nullptr};
    ax::Node* overlay_layer_{nullptr};
    ax::Node* ui_layer_{nullptr};
    ax::Node* inventory_layer_{nullptr};
    ax::Node* context_panel_layer_{nullptr};
    ax::Label* status_label_{nullptr};
    ax::Label* selection_label_{nullptr};

    std::unique_ptr<pf::client::LocalClientRuntime> local_runtime_;
    std::unordered_map<std::string, EntityRenderState> entity_nodes_;
    std::vector<Coord> active_path_;
    std::vector<Coord> path_queue_;
    std::vector<Coord> visible_water_cells_;
    std::vector<ActionHitBox> action_hit_boxes_;
    std::vector<UiHitBox> ui_hit_boxes_;
    std::vector<ax::Rect> ui_blocking_rects_;
    std::vector<std::string> hotbar_item_keys_;
    Coord player_coord_{0, 0};
    Coord selected_coord_{0, 0};
    bool has_selection_{true};
    std::string status_text_{"正在启动本地 Runtime..."};
    float path_step_accumulator_{0.0F};
    float world_tick_accumulator_{0.0F};
    float water_anim_accumulator_{0.0F};
    int water_anim_phase_{0};
    std::future<bool> pending_command_;
    pf::client::LocalCommandOptionView pending_option_;
    bool command_pending_{false};
    bool pending_consumes_path_step_{false};
    bool pending_auto_tick_{false};
    bool player_move_animating_{false};
    float player_move_remaining_{0.0F};

    ax::Vec2 world_origin_{0.0F, 0.0F};
    ax::Vec2 camera_offset_{0.0F, 0.0F};
    ax::Vec2 mouse_down_pos_{0.0F, 0.0F};
    bool dragging_{false};
    bool mouse_down_{false};
    bool mouse_down_on_ui_{false};
    bool inventory_panel_open_{false};
    InventoryPanelTab inventory_panel_tab_{InventoryPanelTab::Inventory};
    int selected_hotbar_slot_{-1};
    int selected_inventory_index_{-1};
    int inventory_scroll_row_{0};
    float zoom_{1.0F};
    double last_click_seconds_{0.0};
    Coord last_click_coord_{0, 0};
    Coord render_min_{0, 0};
    Coord render_max_{0, 0};
};
