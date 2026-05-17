#include "pathfinder/agent/debug_cli.h"
#include <iostream>

int main(int argc, const char* argv[]) {
    pathfinder::agent::AgentDebugCliParser parser;
    auto parse_result = parser.parse(argc, argv);

    if (parse_result.is_error()) {
        std::cerr << "Error: internal parse failure" << std::endl;
        return pathfinder::agent::toInt(pathfinder::agent::AgentDebugCliExitCode::InternalError);
    }

    const auto& pr = parse_result.value();

    if (!pr.should_execute) {
        if (!pr.result.summary_text.empty()) {
            if (pr.result.exit_code == pathfinder::agent::AgentDebugCliExitCode::Success) {
                std::cout << pr.result.summary_text << std::endl;
            } else {
                std::cerr << pr.result.summary_text << std::endl;
            }
        }
        return pathfinder::agent::toInt(pr.result.exit_code);
    }

    // Execute
    pathfinder::agent::AgentDebugCliRunner runner;
    auto run_result = runner.run(pr.options);

    if (run_result.is_error()) {
        std::cerr << "Error: internal run failure" << std::endl;
        return pathfinder::agent::toInt(pathfinder::agent::AgentDebugCliExitCode::InternalError);
    }

    const auto& rr = run_result.value();

    if (!rr.summary_text.empty()) {
        if (rr.exit_code == pathfinder::agent::AgentDebugCliExitCode::Success) {
            std::cout << rr.summary_text << std::endl;
        } else {
            std::cerr << rr.summary_text << std::endl;
        }
    }

    return pathfinder::agent::toInt(rr.exit_code);
}
