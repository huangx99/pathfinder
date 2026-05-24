#include "pathfinder/world_modules/knowledge/actor_knowledge_inspect_module.h"
#include "pathfinder/world_modules/core/world_module_utils.h"
#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include <sstream>
#include <utility>

namespace pathfinder::world_modules {
namespace {

class ActorKnowledgeInspectCommandHandler final : public pathfinder::world_command::IWorldCommandHandler {
public:
    ActorKnowledgeInspectCommandHandler(
        pathfinder::world_runtime::IWorldRuntime& runtime,
        pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
        const pathfinder::content::ContentRegistry& content_registry)
        : bridge_(runtime)
        , knowledge_repository_(knowledge_repository)
        , content_registry_(content_registry) {}

    pathfinder::world_command::WorldCommandKind kind() const override {
        return pathfinder::world_command::WorldCommandKind::Inspect;
    }

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const override {
        if (command.command_key != "inspect_actor_knowledge") {
            return bridge_.handleInspect(context, command);
        }

        pathfinder::world_command::WorldCommandExecutionResult result;
        const auto actor_key = command.target.target_actor_key;
        if (actor_key.empty()) {
            result.result_kind = pathfinder::world_command::WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("inspect_actor_missing");
            return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
        }

        auto claims_res = knowledge_repository_.listByOwner(actorKnowledgeOwner(actor_key));
        if (claims_res.is_error()) {
            result.result_kind = pathfinder::world_command::WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("inspect_actor_knowledge_failed");
            return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
        }

        std::ostringstream body;
        const auto& claims = claims_res.value();
        if (claims.empty()) {
            body << "这个NPC还没有可用知识。";
        } else {
            body << "这个NPC知道：";
            for (size_t index = 0; index < claims.size(); ++index) {
                const auto& claim = claims[index];
                if (index > 0) body << "；";
                body << readableClaim(claim);
            }
        }

        pathfinder::world_command::WorldEventDto event;
        event.event_id = command.command_id + "_npc_knowledge";
        event.event_kind = "NpcKnowledgeInspected";
        event.tick = context.currentTick();
        event.title_text = "NPC知识";
        event.body_text = body.str();
        event.actor_key = command.actor_key;
        event.target_entity_id = command.target.target_entity_id;
        result.result_kind = pathfinder::world_command::WorldCommandResultKind::Succeeded;
        result.events.push_back(std::move(event));
        return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    mutable pathfinder::world_runtime::WorldCommandRuntimeBridge bridge_;
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository_;
    const pathfinder::content::ContentRegistry& content_registry_;

    std::string readableClaim(const pathfinder::knowledge::KnowledgeClaim& claim) const {
        for (const auto* tmpl : content_registry_.allKnowledgeTemplates()) {
            if (!tmpl) continue;
            if (tmpl->subject_object_key == claim.subject.subject_id &&
                tmpl->action_key == claim.predicate.action_key &&
                tmpl->effect_key == claim.predicate.effect_key &&
                !tmpl->summary_key.empty()) {
                const auto translated = content_registry_.translate("zh_cn", tmpl->summary_key);
                if (translated != tmpl->summary_key) return translated;
            }
        }
        return pathfinder::world_modules::objectName(content_registry_, claim.subject.subject_id) + " " + claim.predicate.action_key + " -> " + claim.predicate.effect_key;
    }
};



} // namespace

std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createActorKnowledgeInspectCommandHandler(
    pathfinder::world_runtime::IWorldRuntime& runtime,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    const pathfinder::content::ContentRegistry& content_registry) {
    return std::make_shared<ActorKnowledgeInspectCommandHandler>(runtime, knowledge_repository, content_registry);
}

} // namespace pathfinder::world_modules
