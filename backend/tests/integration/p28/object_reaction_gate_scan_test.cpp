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
            std::filesystem::exists(current / "backend" / "include")) return current;
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

static void test_reaction_gate_scan() {
    const auto root = findRepoRoot();
    const auto resolver = readFile(root / "backend" / "src" / "reaction" / "reaction_resolver.cpp");
    assert(resolver.find("ConditionExpressionEvaluator") != std::string::npos);
    assertNotContains(resolver, "condition_key ==");
    assertNotContains(resolver, "markConsumed");
    assertNotContains(resolver, "WorldObject*");
    assertNotContains(resolver, "hidden_truth");

    const auto rule = readFile(root / "backend" / "src" / "reaction" / "reaction_rule.cpp");
    assert(rule.find("isDynamicStateLikeKey") != std::string::npos);
    std::cout << "reaction_gate_scan passed" << std::endl;
}

int main() {
    test_reaction_gate_scan();
    return 0;
}
