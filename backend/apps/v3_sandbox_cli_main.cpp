#include "pathfinder/v3_sandbox/v3_sandbox_runtime.h"

#include <iostream>
#include <sstream>
#include <string>

using pathfinder::v3_sandbox::V3SandboxCommand;
using pathfinder::v3_sandbox::V3SandboxCommandKind;
using pathfinder::v3_sandbox::V3SandboxRuntime;

namespace {

void printEvents(const std::vector<std::string>& events) {
    for (const auto& event : events) std::cout << event << '\n';
}

void printSnapshot(const V3SandboxRuntime& runtime) {
    const auto snapshot = runtime.snapshot();
    std::cout << "Tick " << snapshot.tick << "  unlocked objects:";
    for (const auto& key : snapshot.unlocked_objects) std::cout << ' ' << key;
    std::cout << "\n";
    for (int y = 0; y < snapshot.height; ++y) {
        for (int x = 0; x < snapshot.width; ++x) {
            char glyph = '.';
            const auto& cell = snapshot.cells[static_cast<size_t>(y * snapshot.width + x)];
            if (!cell.walkable) glyph = '~';
            if (!cell.object_instance_ids.empty()) glyph = '*';
            if (!cell.agent_ids.empty()) glyph = '@';
            std::cout << glyph;
        }
        std::cout << '\n';
    }
    for (const auto& agent : snapshot.agents) {
        std::cout << agent.agent_id << ' ' << agent.name << " @(" << agent.x << ',' << agent.y << ") hunger="
                  << static_cast<int>(agent.hunger) << " health=" << static_cast<int>(agent.health)
                  << " intent=" << agent.current_intent << '\n';
        for (const auto& claim : agent.knowledge) {
            std::cout << "  " << claim.subject_object_key << " + " << claim.action_key << " -> "
                      << claim.effect_key << " [" << claim.status << "]\n";
        }
    }
}

void printHelp() {
    std::cout << "commands:\n"
              << "  show\n"
              << "  paint x y terrain_key\n"
              << "  place x y object_key\n"
              << "  agent x y name\n"
              << "  tick n\n"
              << "  inspect agent_id\n"
              << "  cell x y\n"
              << "  help\n"
              << "  quit\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string content_root;
    if (argc > 1) content_root = argv[1];
    auto runtime = V3SandboxRuntime::createDefault(content_root);
    std::cout << "Pathfinder V3 sandbox CLI. Type help.\n";
    printSnapshot(runtime);

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string command;
        input >> command;
        if (command == "quit" || command == "exit") break;
        if (command == "help") {
            printHelp();
            continue;
        }
        if (command == "show") {
            printSnapshot(runtime);
            continue;
        }

        V3SandboxCommand request;
        if (command == "paint") {
            request.kind = V3SandboxCommandKind::PaintTerrain;
            input >> request.x >> request.y >> request.terrain_key;
        } else if (command == "place") {
            request.kind = V3SandboxCommandKind::PlaceObject;
            input >> request.x >> request.y >> request.object_key;
        } else if (command == "agent") {
            request.kind = V3SandboxCommandKind::PlaceAgent;
            input >> request.x >> request.y >> request.agent_name;
        } else if (command == "tick") {
            request.kind = V3SandboxCommandKind::Tick;
            input >> request.tick_count;
        } else if (command == "inspect") {
            request.kind = V3SandboxCommandKind::InspectAgent;
            input >> request.agent_id;
        } else if (command == "cell") {
            request.kind = V3SandboxCommandKind::InspectCell;
            input >> request.x >> request.y;
        } else {
            std::cout << "unknown command\n";
            continue;
        }

        auto result = runtime.submit(request);
        if (!result.accepted) {
            std::cout << "blocked: " << result.error << '\n';
        } else {
            printEvents(result.events);
        }
    }
    return 0;
}
