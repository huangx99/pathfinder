#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::filesystem::path findRepoRoot() {
    auto current = std::filesystem::current_path();
    for (int i = 0; i < 8; ++i) {
        if (std::filesystem::exists(current / "backend" / "src") &&
            std::filesystem::exists(current / "backend" / "include")) {
            return current;
        }
        current = current.parent_path();
    }
    return std::filesystem::current_path();
}

static std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    assert(input.good());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

static void assertNotContains(const std::string& content, const std::string& pattern) {
    assert(content.find(pattern) == std::string::npos);
}

static void test_condition_authority_gate() {
    const auto root = findRepoRoot();
    const auto civilization = readFile(root / "backend" / "src" / "civilization" / "civilization_state.cpp");
    assert(civilization.find("ConditionExpressionEvaluator") != std::string::npos);
    assert(civilization.find("normalizeCapabilityCondition") != std::string::npos);

    const auto h5_presenter = readFile(root / "backend" / "src" / "h5_dialog" / "dialog_presenter.cpp");
    assertNotContains(h5_presenter, "condition.condition_key ==");
    assert(h5_presenter.find("canonicalKnowledgeConditionKey") != std::string::npos);

    const auto revision = readFile(root / "backend" / "src" / "knowledge" / "knowledge_revision.cpp");
    assertNotContains(revision, "a[i].condition_key != b[i].condition_key");
    assert(revision.find("canonicalKnowledgeConditionKey") != std::string::npos);

    const auto learning = readFile(root / "backend" / "src" / "learning" / "learning_loop.cpp");
    assertNotContains(learning, "left.push_back(condition.condition_key)");
    assert(learning.find("ConditionNormalizer") != std::string::npos);

    std::cout << "condition_authority_gate passed" << std::endl;
}

int main() {
    test_condition_authority_gate();
    return 0;
}
