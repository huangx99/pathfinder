#include "pathfinder/foundation/serialization.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::foundation;

void test_state_version_to_string() {
    StateVersion v(42);
    assert(toStableString(v) == "42");
    std::cout << "  test_state_version_to_string passed" << std::endl;
}

void test_tick_to_string() {
    Tick t(100);
    assert(toStableString(t) == "100");
    std::cout << "  test_tick_to_string passed" << std::endl;
}

void test_duration_ticks_to_string() {
    DurationTicks d(50);
    assert(toStableString(d) == "50");
    std::cout << "  test_duration_ticks_to_string passed" << std::endl;
}

void test_hash_value_to_string() {
    HashValue h(12345);
    assert(toStableString(h) == "12345");
    std::cout << "  test_hash_value_to_string passed" << std::endl;
}

void test_random_seed_to_string() {
    RandomSeed s(99);
    assert(toStableString(s) == "99");
    std::cout << "  test_random_seed_to_string passed" << std::endl;
}

void test_same_value_same_string() {
    StateVersion v1(42);
    StateVersion v2(42);
    assert(toStableString(v1) == toStableString(v2));
    std::cout << "  test_same_value_same_string passed" << std::endl;
}

void run_serialization_contract_tests() {
    std::cout << "Running foundation_serialization_contract tests:" << std::endl;
    test_state_version_to_string();
    test_tick_to_string();
    test_duration_ticks_to_string();
    test_hash_value_to_string();
    test_random_seed_to_string();
    test_same_value_same_string();
    std::cout << "All foundation_serialization_contract tests passed!" << std::endl;
}
