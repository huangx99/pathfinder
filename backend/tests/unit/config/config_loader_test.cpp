#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "pathfinder/config/loader.h"

using namespace pathfinder::config;
using namespace pathfinder::foundation;

static std::string getTempDir() {
    return std::filesystem::temp_directory_path().string() + "/pathfinder_test_config";
}

static void setupTestDir() {
    std::filesystem::create_directories(getTempDir());
    std::filesystem::create_directories(getTempDir() + "/objects");

    // Create a test file
    std::ofstream f(getTempDir() + "/objects/test.json");
    f << R"({"id": "test_obj", "category": "plant"})";
    f.close();

    // Create an empty file
    std::ofstream empty(getTempDir() + "/empty.json");
    empty.close();
}

static void cleanupTestDir() {
    std::filesystem::remove_all(getTempDir());
}

void run_config_loader_tests() {
    setupTestDir();

    // Test 1: Read existing file succeeds
    {
        ConfigLoadRequest request;
        request.root_path = getTempDir();
        request.relative_files = {"objects/test.json"};

        auto result = ConfigLoader::loadFiles(request);
        assert(result.files.size() == 1);
        assert(result.files[0].path == "objects/test.json");
        assert(!result.files[0].content.empty());
        assert(!result.files[0].content_hash.empty());
        assert(!result.validation_report.hasErrors());
    }

    // Test 2: Read non-existent file returns error
    {
        ConfigLoadRequest request;
        request.root_path = getTempDir();
        request.relative_files = {"nonexistent.json"};

        auto result = ConfigLoader::loadFiles(request);
        assert(result.files.size() == 0);
        assert(result.validation_report.hasErrors());
    }

    // Test 3: Empty file returns warning
    {
        ConfigLoadRequest request;
        request.root_path = getTempDir();
        request.relative_files = {"empty.json"};

        auto result = ConfigLoader::loadFiles(request);
        assert(result.files.size() == 1);
        assert(result.files[0].content.empty());
        // Empty file should produce a warning
        assert(result.validation_report.issueCount() > 0);
    }

    // Test 4: Same content produces stable hash
    {
        ConfigLoadRequest request;
        request.root_path = getTempDir();
        request.relative_files = {"objects/test.json"};

        auto result1 = ConfigLoader::loadFiles(request);
        auto result2 = ConfigLoader::loadFiles(request);
        assert(result1.files[0].content_hash == result2.files[0].content_hash);
    }

    // Test 5: Path traversal with .. is rejected
    {
        ConfigLoadRequest request;
        request.root_path = getTempDir();
        request.relative_files = {"../etc/passwd"};

        auto result = ConfigLoader::loadFiles(request);
        assert(result.files.size() == 0);
        assert(result.validation_report.hasErrors());
    }

    // Test 6: Absolute path is rejected
    {
        ConfigLoadRequest request;
        request.root_path = getTempDir();
        request.relative_files = {"/etc/passwd"};

        auto result = ConfigLoader::loadFiles(request);
        assert(result.files.size() == 0);
        assert(result.validation_report.hasErrors());
    }

    cleanupTestDir();
    std::cout << "config_loader_tests: all passed" << std::endl;
}
