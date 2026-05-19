#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <algorithm>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

DialogScenario buildDefaultScenario() {
    DialogScenario scenario;
    scenario.scenario_key = "p22_minimal";
    scenario.display_name = "先驱者：初醒";
    scenario.welcome_text = "你在一片陌生环境中醒来。你能看见几个东西：红果、腐烂红果、苦叶、石片、斧头、木头、磨石。你不知道它们有什么用，只能通过尝试形成认知。旁边有一个同伴，他不知道你学到了什么，除非你教他。";

    // red_berry
    DialogScenarioObject red_berry;
    red_berry.object_key = "red_berry";
    red_berry.display_name = "红果";
    red_berry.player_description = "一枚红色的小果实。你不知道吃下去会发生什么。";
    red_berry.visibility = DialogObjectVisibility::Visible;
    red_berry.allowed_actions = {DialogActionKind::Eat};
    red_berry.safe_tags = {"fruit", "red"};
    scenario.objects.push_back(red_berry);

    // decayed_red_berry
    DialogScenarioObject decayed;
    decayed.object_key = "decayed_red_berry";
    decayed.display_name = "腐烂红果";
    decayed.player_description = "一枚气味奇怪的红果，看起来和普通红果相似，但状态不太一样。";
    decayed.visibility = DialogObjectVisibility::Visible;
    decayed.allowed_actions = {DialogActionKind::Eat};
    decayed.safe_tags = {"fruit", "red", "decayed"};
    scenario.objects.push_back(decayed);

    // bitter_leaf
    DialogScenarioObject bitter;
    bitter.object_key = "bitter_leaf";
    bitter.display_name = "苦叶";
    bitter.player_description = "一片发苦的叶子，看起来不像常见的食物。";
    bitter.visibility = DialogObjectVisibility::Visible;
    bitter.allowed_actions = {DialogActionKind::Eat};
    bitter.safe_tags = {"leaf", "green"};
    scenario.objects.push_back(bitter);

    // stone_flake
    DialogScenarioObject stone;
    stone.object_key = "stone_flake";
    stone.display_name = "石片";
    stone.player_description = "一块边缘锋利的石片，似乎可以用来做些什么。";
    stone.visibility = DialogObjectVisibility::Visible;
    stone.allowed_actions = {DialogActionKind::Use};
    stone.safe_tags = {"tool", "stone"};
    scenario.objects.push_back(stone);

    // axe
    DialogScenarioObject axe;
    axe.object_key = "axe";
    axe.display_name = "斧头";
    axe.player_description = "一把可以尝试砍伐的斧头，用久了可能需要维护。";
    axe.visibility = DialogObjectVisibility::Visible;
    axe.allowed_actions = {DialogActionKind::Use};
    axe.safe_tags = {"tool", "axe", "sharp"};
    scenario.objects.push_back(axe);

    // wood
    DialogScenarioObject wood;
    wood.object_key = "wood";
    wood.display_name = "木头";
    wood.player_description = "一段粗木头，似乎可以被工具处理。";
    wood.visibility = DialogObjectVisibility::Visible;
    wood.allowed_actions = {DialogActionKind::Use};
    wood.safe_tags = {"material", "wood"};
    scenario.objects.push_back(wood);

    // whetstone
    DialogScenarioObject whetstone;
    whetstone.object_key = "whetstone";
    whetstone.display_name = "磨石";
    whetstone.player_description = "一块粗糙的磨石，可能可以维护某些工具。";
    whetstone.visibility = DialogObjectVisibility::Visible;
    whetstone.allowed_actions = {DialogActionKind::Use};
    whetstone.safe_tags = {"tool", "maintenance"};
    scenario.objects.push_back(whetstone);

    // dry grass
    DialogScenarioObject dry_grass;
    dry_grass.object_key = "dry_grass";
    dry_grass.display_name = "干草";
    dry_grass.player_description = "一团干燥的草，似乎很容易被点燃。";
    dry_grass.visibility = DialogObjectVisibility::Visible;
    dry_grass.allowed_actions = {DialogActionKind::Use};
    dry_grass.safe_tags = {"fuel", "dry"};
    scenario.objects.push_back(dry_grass);

    // fire seed
    DialogScenarioObject fire_seed;
    fire_seed.object_key = "fire_seed";
    fire_seed.display_name = "火种";
    fire_seed.player_description = "一点微弱的火星，需要合适的材料才能变成火源。";
    fire_seed.visibility = DialogObjectVisibility::Visible;
    fire_seed.allowed_actions = {DialogActionKind::Use};
    fire_seed.safe_tags = {"ignition", "fire"};
    scenario.objects.push_back(fire_seed);

    // torch
    DialogScenarioObject torch;
    torch.object_key = "torch";
    torch.display_name = "火把";
    torch.player_description = "需要先理解材料和火源的用途，才能可靠地使用它。";
    torch.visibility = DialogObjectVisibility::Mentioned;
    torch.allowed_actions = {DialogActionKind::Use};
    torch.safe_tags = {"tool", "light_source", "generated_item"};
    scenario.objects.push_back(torch);

    // camp fire
    DialogScenarioObject camp_fire;
    camp_fire.object_key = "camp_fire";
    camp_fire.display_name = "火堆";
    camp_fire.player_description = "一处被点燃的火源，能带来光和热，也会影响靠近的野兽。";
    camp_fire.visibility = DialogObjectVisibility::Mentioned;
    camp_fire.allowed_actions = {};
    camp_fire.safe_tags = {"fire", "light_source", "generated_item"};
    scenario.objects.push_back(camp_fire);

    // beast shadow
    DialogScenarioObject beast;
    beast.object_key = "beast_shadow";
    beast.display_name = "靠近的野兽";
    beast.player_description = "树林里靠近的影子。它不是可以采集的物品，而是夜晚危险的征兆。";
    beast.visibility = DialogObjectVisibility::Mentioned;
    beast.allowed_actions = {DialogActionKind::Use};
    beast.safe_tags = {"threat", "creature"};
    scenario.objects.push_back(beast);

    // Feedbacks
    DialogFeedbackTemplate fb1;
    fb1.feedback_key = "red_berry_eat";
    fb1.object_key = "red_berry";
    fb1.action = DialogActionKind::Eat;
    fb1.effect_key = "restore_hunger";
    fb1.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::NeedImproved,
        pathfinder::cognition::CognitionOutcomeSignal::ObjectConsumed
    };
    fb1.condition_keys = {"fresh"};
    fb1.utility_delta = 0.7;
    fb1.risk_delta = 0.0;
    scenario.feedbacks.push_back(fb1);

    DialogFeedbackTemplate fb2;
    fb2.feedback_key = "decayed_red_berry_eat";
    fb2.object_key = "decayed_red_berry";
    fb2.action = DialogActionKind::Eat;
    fb2.effect_key = "poison";
    fb2.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::HealthWorsened,
        pathfinder::cognition::CognitionOutcomeSignal::DamageTaken
    };
    fb2.condition_keys = {"decayed"};
    fb2.utility_delta = -0.7;
    fb2.risk_delta = 0.8;
    scenario.feedbacks.push_back(fb2);

    DialogFeedbackTemplate fb3;
    fb3.feedback_key = "bitter_leaf_eat";
    fb3.object_key = "bitter_leaf";
    fb3.action = DialogActionKind::Eat;
    fb3.effect_key = "no_visible_effect";
    fb3.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::NoVisibleEffect
    };
    fb3.utility_delta = 0.0;
    fb3.risk_delta = 0.1;
    scenario.feedbacks.push_back(fb3);

    DialogFeedbackTemplate fb4;
    fb4.feedback_key = "stone_flake_use";
    fb4.object_key = "stone_flake";
    fb4.action = DialogActionKind::Use;
    fb4.effect_key = "use_hint";
    fb4.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::ToolActivated
    };
    fb4.utility_delta = 0.2;
    fb4.risk_delta = 0.1;
    scenario.feedbacks.push_back(fb4);

    DialogFeedbackTemplate fb5;
    fb5.feedback_key = "axe_use_cut_wood";
    fb5.object_key = "axe";
    fb5.target_object_key = "wood";
    fb5.action = DialogActionKind::Use;
    fb5.effect_key = "cut_wood";
    fb5.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::ToolActivated,
        pathfinder::cognition::CognitionOutcomeSignal::NeedImproved
    };
    fb5.state_conditions.push_back(DialogStateCondition{"axe", "sharpness", "gt", 0.0, ""});
    fb5.state_mutations.push_back(DialogStateMutation{"axe", "sharpness", "add_number", -1.0, ""});
    fb5.utility_delta = 0.4;
    fb5.risk_delta = 0.1;
    scenario.feedbacks.push_back(fb5);

    DialogFeedbackTemplate fb6;
    fb6.feedback_key = "axe_use_dull";
    fb6.object_key = "axe";
    fb6.target_object_key = "wood";
    fb6.action = DialogActionKind::Use;
    fb6.effect_key = "tool_dull";
    fb6.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::NoVisibleEffect
    };
    fb6.state_conditions.push_back(DialogStateCondition{"axe", "sharpness", "lte", 0.0, ""});
    fb6.utility_delta = -0.1;
    fb6.risk_delta = 0.1;
    scenario.feedbacks.push_back(fb6);

    DialogFeedbackTemplate fb7;
    fb7.feedback_key = "whetstone_restore_axe";
    fb7.object_key = "whetstone";
    fb7.target_object_key = "axe";
    fb7.action = DialogActionKind::Use;
    fb7.effect_key = "restore_sharpness";
    fb7.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::ToolActivated,
        pathfinder::cognition::CognitionOutcomeSignal::NeedImproved
    };
    fb7.state_mutations.push_back(DialogStateMutation{"axe", "sharpness", "set_number", 3.0, ""});
    fb7.state_mutations.push_back(DialogStateMutation{"axe", "tag_states", "add_tag", 0.0, "sharp"});
    fb7.state_mutations.push_back(DialogStateMutation{"axe", "tag_states", "remove_tag", 0.0, "dull"});
    fb7.utility_delta = 0.3;
    fb7.risk_delta = 0.0;
    scenario.feedbacks.push_back(fb7);

    DialogFeedbackTemplate fb8;
    fb8.feedback_key = "fire_seed_ignite_dry_grass";
    fb8.object_key = "fire_seed";
    fb8.target_object_key = "dry_grass";
    fb8.action = DialogActionKind::Use;
    fb8.effect_key = "ignite_fire";
    fb8.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::ToolActivated,
        pathfinder::cognition::CognitionOutcomeSignal::NeedImproved
    };
    fb8.utility_delta = 0.5;
    fb8.risk_delta = 0.1;
    fb8.reason_keys.push_back("story_reaction_fire_source");
    scenario.feedbacks.push_back(fb8);

    DialogFeedbackTemplate fb9;
    fb9.feedback_key = "wood_make_torch";
    fb9.object_key = "wood";
    fb9.target_object_key = "fire_seed";
    fb9.action = DialogActionKind::Use;
    fb9.effect_key = "make_torch";
    fb9.state_conditions.push_back(DialogStateCondition{"wood", "processed", "gt", 0.0, ""});
    fb9.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::ToolActivated,
        pathfinder::cognition::CognitionOutcomeSignal::NeedImproved
    };
    fb9.utility_delta = 0.6;
    fb9.risk_delta = 0.1;
    fb9.reason_keys.push_back("story_reaction_torch");
    scenario.feedbacks.push_back(fb9);

    DialogFeedbackTemplate fb10;
    fb10.feedback_key = "torch_repel_beast";
    fb10.object_key = "torch";
    fb10.target_object_key = "beast_shadow";
    fb10.action = DialogActionKind::Use;
    fb10.effect_key = "repel_beast";
    fb10.state_conditions.push_back(DialogStateCondition{"torch", "quantity", "gt", 0.0, ""});
    fb10.outcome_signals = {
        pathfinder::cognition::CognitionOutcomeSignal::ToolActivated,
        pathfinder::cognition::CognitionOutcomeSignal::NeedImproved
    };
    fb10.utility_delta = 0.8;
    fb10.risk_delta = 0.0;
    fb10.reason_keys.push_back("story_danger_countermeasure");
    scenario.feedbacks.push_back(fb10);

    scenario.quick_action_input_texts = {
        "观察", "吃红果", "等待一会", "用斧头砍木头", "用磨石打磨斧头", "用火种点燃干草", "制作火把", "用火把驱赶野兽", "教同伴", "查看知识", "查看同伴", "吃腐烂红果", "帮助", "重开"
    };

    return scenario;
}

} // namespace

Result<DialogScenario> DialogScenarioCatalog::defaultScenario() const {
    return Result<DialogScenario>::ok(buildDefaultScenario());
}

Result<const DialogScenarioObject*> DialogScenarioCatalog::findObject(
    const DialogScenario& scenario,
    const std::string& object_key) const {
    for (const auto& obj : scenario.objects) {
        if (obj.object_key == object_key) {
            return Result<const DialogScenarioObject*>::ok(&obj);
        }
    }
    return Result<const DialogScenarioObject*>::fail(
        makeError(ErrorCode::id_not_found, "object not found in scenario"));
}

Result<DialogFeedbackTemplate> DialogScenarioCatalog::findFeedback(
    const DialogScenario& scenario,
    const std::string& object_key,
    DialogActionKind action) const {
    for (const auto& fb : scenario.feedbacks) {
        if (fb.object_key == object_key && fb.action == action) {
            return Result<DialogFeedbackTemplate>::ok(fb);
        }
    }
    return Result<DialogFeedbackTemplate>::fail(
        makeError(ErrorCode::id_not_found, "feedback not found for object/action"));
}

} // namespace pathfinder::h5_dialog
