#include "pathfinder/logging/logger.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::filesystem::path testPath() {
    return std::filesystem::temp_directory_path() / "pathfinder_logger_unit_test.log";
}

void run_global_switch_tests() {
    auto path = testPath();
    std::filesystem::remove(path);

    auto& logger = pathfinder::logging::Logger::instance();
    logger.initialize(false, path.string(), true);
    logger.log(pathfinder::logging::tag::App, "should_not_exist");
    logger.shutdown();
    assert(!std::filesystem::exists(path));

    std::cout << "logger_global_switch_tests: all passed" << std::endl;
}

void run_tag_and_custom_tests() {
    auto path = testPath();
    std::filesystem::remove(path);

    auto& logger = pathfinder::logging::Logger::instance();
    logger.initialize(true, path.string(), true);
    logger.setTagEnabled(pathfinder::logging::tag::Ui, false);
    logger.log(pathfinder::logging::tag::Ui, "hidden_ui_line");
    logger.registerCustomTag("custom_gameplay", true);
    logger.log("custom_gameplay", "visible_custom_line");
    logger.shutdown();

    const auto text = readFile(path);
    assert(text.find("hidden_ui_line") == std::string::npos);
    assert(text.find("visible_custom_line") != std::string::npos);
    std::filesystem::remove(path);

    std::cout << "logger_tag_and_custom_tests: all passed" << std::endl;
}

void run_truncate_on_start_tests() {
    auto path = testPath();
    std::filesystem::remove(path);

    auto& logger = pathfinder::logging::Logger::instance();
    logger.initialize(true, path.string(), true);
    logger.log(pathfinder::logging::tag::App, "old_line");
    logger.shutdown();

    logger.initialize(true, path.string(), true);
    logger.log(pathfinder::logging::tag::App, "new_line");
    logger.shutdown();

    const auto text = readFile(path);
    assert(text.find("old_line") == std::string::npos);
    assert(text.find("new_line") != std::string::npos);
    std::filesystem::remove(path);

    std::cout << "logger_truncate_on_start_tests: all passed" << std::endl;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_global_switch_tests();
        run_tag_and_custom_tests();
        run_truncate_on_start_tests();
        return 0;
    }

    const std::string test_name = argv[1];
    if (test_name == "global_switch") run_global_switch_tests();
    else if (test_name == "tag_and_custom") run_tag_and_custom_tests();
    else if (test_name == "truncate_on_start") run_truncate_on_start_tests();
    else {
        std::cerr << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
