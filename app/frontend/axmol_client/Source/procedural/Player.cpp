#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"
#include <array>
#include <memory>
#include <vector>

namespace pf::art {
namespace {
struct WalkPose {
    float left_leg_x{0.0F};
    float left_leg_y{0.0F};
    float right_leg_x{0.0F};
    float right_leg_y{0.0F};
    float left_arm_x{0.0F};
    float left_arm_y{0.0F};
    float right_arm_x{0.0F};
    float right_arm_y{0.0F};
    float body_x{0.0F};
    float body_y{0.0F};
    float head_x{0.0F};
};

const std::array<WalkPose, 6> kWalkCycle{{
    {-3.0F, 1.0F, 3.0F, -1.0F, 3.0F, -1.0F, -3.0F, 1.0F, -1.0F, 0.0F, -1.0F},
    {-2.0F, 0.0F, 2.0F, 0.0F, 2.0F, 0.0F, -2.0F, 0.0F, 0.0F, -1.0F, 0.0F},
    {0.0F, -1.0F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, -1.0F, 1.0F, 0.0F, 1.0F},
    {3.0F, -1.0F, -3.0F, 1.0F, -3.0F, 1.0F, 3.0F, -1.0F, 1.0F, 0.0F, 1.0F},
    {2.0F, 0.0F, -2.0F, 0.0F, -2.0F, 0.0F, 2.0F, 0.0F, 0.0F, -1.0F, 0.0F},
    {0.0F, 1.0F, 0.0F, -1.0F, 0.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, -1.0F},
}};

ax::Node* createPlayerFrame(float size, const WalkPose& pose) {
    auto* x = draw::canvas();
    const auto outline = draw::color(31, 41, 55);
    const auto outline_soft = draw::color(71, 85, 105);
    const auto boot = draw::color(124, 63, 18);
    const auto boot_hi = draw::color(180, 83, 9);
    const auto cloth = draw::color(37, 99, 235);
    const auto cloth_hi = draw::color(96, 165, 250);
    const auto skin = draw::color(231, 154, 106);
    const auto skin_hi = draw::color(255, 196, 147);
    const auto hair = draw::color(91, 43, 18);

    draw::rect(x, size, 10, 42, 28, 4, draw::color(2, 6, 23, 0.22F));

    draw::rect(x, size, 15 + pose.left_leg_x, 30 + pose.left_leg_y, 9, 14, outline);
    draw::rect(x, size, 24 + pose.right_leg_x, 30 + pose.right_leg_y, 9, 14, outline);
    draw::rect(x, size, 17 + pose.left_leg_x, 31 + pose.left_leg_y, 5, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 26 + pose.right_leg_x, 31 + pose.right_leg_y, 5, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 14 + pose.left_leg_x, 39 + pose.left_leg_y, 10, 5, boot);
    draw::rect(x, size, 24 + pose.right_leg_x, 39 + pose.right_leg_y, 10, 5, boot);
    draw::rect(x, size, 17 + pose.left_leg_x, 40 + pose.left_leg_y, 5, 2, boot_hi);
    draw::rect(x, size, 26 + pose.right_leg_x, 40 + pose.right_leg_y, 5, 2, boot_hi);

    draw::rect(x, size, 12 + pose.body_x, 16 + pose.body_y, 24, 19, outline);
    draw::rect(x, size, 14 + pose.body_x, 18 + pose.body_y, 20, 14, cloth);
    draw::rect(x, size, 16 + pose.body_x, 19 + pose.body_y, 6, 9, cloth_hi);
    draw::rect(x, size, 14 + pose.body_x, 31 + pose.body_y, 20, 3, outline_soft);
    draw::rect(x, size, 19 + pose.body_x, 18 + pose.body_y, 10, 3, draw::color(245, 158, 11));
    draw::rect(x, size, 27 + pose.body_x, 20 + pose.body_y, 3, 5, draw::color(245, 158, 11));

    draw::rect(x, size, 7 + pose.left_arm_x, 19 + pose.body_y + pose.left_arm_y, 7, 13, outline);
    draw::rect(x, size, 34 + pose.right_arm_x, 19 + pose.body_y + pose.right_arm_y, 7, 13, outline);
    draw::rect(x, size, 9 + pose.left_arm_x, 20 + pose.body_y + pose.left_arm_y, 4, 7, cloth);
    draw::rect(x, size, 35 + pose.right_arm_x, 20 + pose.body_y + pose.right_arm_y, 4, 7, cloth);
    draw::rect(x, size, 8 + pose.left_arm_x, 27 + pose.body_y + pose.left_arm_y, 5, 5, skin);
    draw::rect(x, size, 35 + pose.right_arm_x, 27 + pose.body_y + pose.right_arm_y, 5, 5, skin_hi);

    const float head_x = pose.head_x;
    draw::rect(x, size, 13 + head_x, 3 + pose.body_y, 22, 18, outline);
    draw::rect(x, size, 16 + head_x, 8 + pose.body_y, 16, 12, skin);
    draw::rect(x, size, 17 + head_x, 7 + pose.body_y, 14, 11, skin_hi);
    draw::rect(x, size, 13 + head_x, 3 + pose.body_y, 22, 7, hair);
    draw::rect(x, size, 13 + head_x, 8 + pose.body_y, 4, 5, hair);
    draw::rect(x, size, 31 + head_x, 8 + pose.body_y, 4, 5, hair);
    draw::rect(x, size, 18 + head_x, 4 + pose.body_y, 9, 2, draw::color(154, 84, 32));
    draw::rect(x, size, 19 + head_x, 12 + pose.body_y, 3, 3, outline);
    draw::rect(x, size, 27 + head_x, 12 + pose.body_y, 3, 3, outline);
    draw::rect(x, size, 20 + head_x, 12 + pose.body_y, 1, 1, draw::color(239, 246, 255));
    draw::rect(x, size, 28 + head_x, 12 + pose.body_y, 1, 1, draw::color(239, 246, 255));
    draw::rect(x, size, 23 + head_x, 16 + pose.body_y, 4, 1, skin);
    return x;
}
} // namespace

ax::Node* createPlayer(float size) {
    auto* root = ax::Node::create();
    auto* animated = ax::Node::create();
    root->addChild(animated, 0);

    std::vector<ax::Node*> frames;
    frames.reserve(kWalkCycle.size());
    for (size_t index = 0; index < kWalkCycle.size(); ++index) {
        auto* frame = createPlayerFrame(size, kWalkCycle[index]);
        frame->setVisible(index == 0);
        animated->addChild(frame, 0);
        frames.push_back(frame);
    }

    auto frame_index = std::make_shared<int>(0);
    animated->runAction(ax::RepeatForever::create(ax::Sequence::create(
        ax::DelayTime::create(0.085F),
        ax::CallFunc::create([frames, frame_index]() {
            frames[*frame_index]->setVisible(false);
            *frame_index = (*frame_index + 1) % static_cast<int>(frames.size());
            frames[*frame_index]->setVisible(true);
        }),
        nullptr)));
    return root;
}
} // namespace pf::art
