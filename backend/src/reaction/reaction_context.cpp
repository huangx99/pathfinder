#include "pathfinder/reaction/reaction_context.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static pathfinder::condition::ObjectConditionView toView(const ReactionObjectRef& object) {
    pathfinder::condition::ObjectConditionView view;
    view.state_keys = object.state_keys;
    view.tag_keys = object.tag_keys;
    return view;
}

Result<pathfinder::condition::ConditionEvaluationContext> ReactionConditionContextBuilder::build(const ReactionInputSet& input) const {
    auto valid = input.validateBasic();
    if (valid.is_error()) return Result<pathfinder::condition::ConditionEvaluationContext>::fail(valid.errors());

    pathfinder::condition::ConditionEvaluationContext context;
    context.context_type = "reaction";
    context.tick = input.tick;
    context.safe_context_keys = input.safe_context_keys;
    context.actor = pathfinder::condition::ActorConditionView{};

    bool has_target = false;
    for (const auto& object : input.objects) {
        if (object.role == ReactionObjectRole::Source) {
            context.source = toView(object);
        }
        if (object.role == ReactionObjectRole::Target || object.role == ReactionObjectRole::Material ||
            object.role == ReactionObjectRole::Tool || object.role == ReactionObjectRole::Container) {
            context.target = toView(object);
            has_target = true;
        }
    }
    if (!has_target && !input.objects.empty()) {
        context.target = toView(input.objects.front());
        has_target = true;
    }
    if (!has_target) {
        return Result<pathfinder::condition::ConditionEvaluationContext>::fail(makeError(ErrorCode::validation_failed, "ReactionConditionContextBuilder requires at least one target-capable object"));
    }
    auto context_valid = context.validateBasic();
    if (context_valid.is_error()) return Result<pathfinder::condition::ConditionEvaluationContext>::fail(context_valid.errors());
    return Result<pathfinder::condition::ConditionEvaluationContext>::ok(std::move(context));
}

} // namespace pathfinder::reaction
