#include "pathfinder/camp/camp_runtime.h"
#include <memory>

namespace pathfinder::camp {
namespace {

class WaitTickRule final : public CampRule {
public:
    std::string ruleKey() const override { return "camp.wait_tick"; }

    void buildOptions(const CampRuntime& runtime, std::vector<CampCommandOptionView>& options) const override {
        CampCommandOptionView option;
        option.option_id = "camp:wait:" + std::to_string(runtime.state().version);
        option.command_key = "camp.wait";
        option.label_text = "等待一会儿";
        option.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
        options.push_back(std::move(option));
    }

    bool canHandle(const CampCommandOptionView& option) const override {
        return option.command_key == "camp.wait";
    }

    bool execute(CampRuntime& runtime, const CampCommandOptionView&, std::string&) const override {
        runtime.advanceOneTick();
        return true;
    }
};

} // namespace

std::unique_ptr<CampRule> makeWaitTickRule() { return std::make_unique<WaitTickRule>(); }

} // namespace pathfinder::camp
