#include "pathfinder/condition/condition_normalizer.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::condition;

static void test_legacy_mappings() {
    ConditionNormalizer normalizer;
    auto decayed = normalizer.normalizeKey("state_decayed");
    assert(decayed.is_ok());
    assert(decayed.value().canonical_condition_key == "condition:object_state:eq:decayed");

    auto tool = normalizer.normalizeKey("has_tool");
    assert(tool.is_ok());
    assert(tool.value().canonical_condition_key == "condition:actor_requirement:has:tool");
    std::cout << "legacy_mappings passed" << std::endl;
}

static void test_unknown_legacy_fails() {
    ConditionNormalizer normalizer;
    auto unknown = normalizer.normalizeKey("unknown_condition_key");
    assert(unknown.is_error());
    std::cout << "unknown_legacy_fails passed" << std::endl;
}

static void test_capability_condition() {
    ConditionNormalizer normalizer;
    auto condition = normalizer.normalizeCapabilityCondition("known_edible_count", 2.0);
    assert(condition.is_ok());
    assert(condition.value().canonical_condition_key == "condition:civilization:known_edible_count:gte:2");
    std::cout << "capability_condition passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "legacy_mappings") test_legacy_mappings();
    else if (mode == "unknown_legacy_fails") test_unknown_legacy_fails();
    else if (mode == "capability_condition") test_capability_condition();
    else {
        test_legacy_mappings();
        test_unknown_legacy_fails();
        test_capability_condition();
    }
    return 0;
}
