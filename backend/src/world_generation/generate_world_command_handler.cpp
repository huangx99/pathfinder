#include "pathfinder/world_generation/world_generation_command_handler.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/world_generation_projection_bridge.h"
#include <memory>

namespace pathfinder::world_command {

class GenerateWorldCommandHandler final : public IWorldCommandHandler {
public:
    explicit GenerateWorldCommandHandler(
        std::shared_ptr<world_generation::WorldGenerationService> service,
        world_runtime::IWorldRuntime* world_runtime,
        world_inventory::IWorldEntityLocationPort* location_port)
        : service_(std::move(service)),
          world_runtime_(world_runtime),
          location_port_(location_port) {
    }

    WorldCommandKind kind() const override {
        return WorldCommandKind::GenerateWorld;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& /*context*/,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        result.result_kind = WorldCommandResultKind::Succeeded;

        // Build generation request from command parameters
        world_generation::WorldGenerationRequest request;
        request.world_id = command.parameters.count("world_id") ? command.parameters.at("world_id") : "world_" + command.command_id;

        try {
            request.world_seed = command.parameters.count("world_seed") ? std::stoull(command.parameters.at("world_seed")) : 42;
        } catch (const std::exception&) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("InvalidRequest: world_seed");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        request.generator_version = command.parameters.count("generator_version") ? command.parameters.at("generator_version") : "1.0.0";
        request.content_version = command.parameters.count("content_version") ? command.parameters.at("content_version") : "1.0.0";
        request.worldgen_profile_key = command.parameters.count("worldgen_profile_key") ? command.parameters.at("worldgen_profile_key") : "first_world";

        try {
            request.region_size = command.parameters.count("region_size") ? std::stoi(command.parameters.at("region_size")) : 16;
        } catch (const std::exception&) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("InvalidRequest: region_size");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        // Parse region_x/region_y
        if (command.parameters.count("region_x")) {
            try {
                request.region_coord.rx = std::stoi(command.parameters.at("region_x"));
            } catch (const std::exception&) {
                result.result_kind = WorldCommandResultKind::Failed;
                result.failure_reason_keys.push_back("InvalidRequest: region_x");
                return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
            }
        }
        if (command.parameters.count("region_y")) {
            try {
                request.region_coord.ry = std::stoi(command.parameters.at("region_y"));
            } catch (const std::exception&) {
                result.result_kind = WorldCommandResultKind::Failed;
                result.failure_reason_keys.push_back("InvalidRequest: region_y");
                return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
            }
        }

        // Parse layer_keys (comma-separated or single)
        if (command.parameters.count("layer_keys")) {
            std::string layer_str = command.parameters.at("layer_keys");
            request.enabled_layer_keys.clear();
            size_t start = 0;
            while (start < layer_str.size()) {
                size_t comma = layer_str.find(',', start);
                if (comma == std::string::npos) comma = layer_str.size();
                std::string key = layer_str.substr(start, comma - start);
                // Trim whitespace
                size_t key_start = 0;
                while (key_start < key.size() && std::isspace(static_cast<unsigned char>(key[key_start]))) ++key_start;
                size_t key_end = key.size();
                while (key_end > key_start && std::isspace(static_cast<unsigned char>(key[key_end - 1]))) --key_end;
                if (key_start < key_end) {
                    request.enabled_layer_keys.push_back(key.substr(key_start, key_end - key_start));
                }
                start = comma + 1;
            }
        } else {
            request.enabled_layer_keys = std::vector<std::string>{"surface"};
        }

        // Generate
        auto gen_result = service_->generate(request);
        if (!gen_result.ok) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back(
                world_generation::toString(gen_result.failure_kind));
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        // Apply to runtime if available
        if (world_runtime_ && location_port_) {
            world_generation::WorldGenerationApplier applier(*world_runtime_, *location_port_);
            auto apply_result = applier.apply(gen_result);
            if (!apply_result.ok) {
                result.result_kind = WorldCommandResultKind::Failed;
                result.failure_reason_keys.push_back(
                    world_generation::toString(apply_result.failure_kind));
                return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
            }

            // Build projection patch
            auto patch = world_generation::WorldGenerationProjectionBridge::buildPatch(
                apply_result, 0);
            result.projection_patch_override = std::move(patch);

            for (const auto& cell_id : apply_result.changed_cell_ids) {
                result.changed_cell_ids.push_back(cell_id);
            }
            for (const auto& entity_id : apply_result.changed_entity_ids) {
                result.changed_entity_ids.push_back(entity_id);
            }
            for (const auto& delta : apply_result.state_deltas) {
                result.state_deltas.push_back(delta);
            }
        }

        // Forward events
        for (const auto& event : gen_result.events) {
            result.events.push_back(event);
        }

        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    std::shared_ptr<world_generation::WorldGenerationService> service_;
    world_runtime::IWorldRuntime* world_runtime_;
    world_inventory::IWorldEntityLocationPort* location_port_;
};

std::shared_ptr<IWorldCommandHandler> createGenerateWorldCommandHandler(
    std::shared_ptr<world_generation::WorldGenerationService> service,
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port) {
    return std::make_shared<GenerateWorldCommandHandler>(service, &world_runtime, &location_port);
}

} // namespace pathfinder::world_command
