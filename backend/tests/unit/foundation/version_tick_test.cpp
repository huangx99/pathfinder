#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::foundation;

void test_state_version_construct() {
    StateVersion v1;
    assert(v1.value() == 0);
    StateVersion v2(42);
    assert(v2.value() == 42);
    std::cout << "  test_state_version_construct passed" << std::endl;
}

void test_state_version_compare() {
    StateVersion v1(10);
    StateVersion v2(20);
    assert(v1 < v2);
    assert(v2 > v1);
    assert(v1 <= v2);
    assert(v2 >= v1);
    assert(v1 != v2);
    StateVersion v3(10);
    assert(v1 == v3);
    std::cout << "  test_state_version_compare passed" << std::endl;
}

void test_state_version_next() {
    StateVersion v1(10);
    StateVersion v2 = v1.next();
    assert(v2.value() == 11);
    assert(v2 > v1);
    std::cout << "  test_state_version_next passed" << std::endl;
}

void test_tick_construct() {
    Tick t1;
    assert(t1.value() == 0);
    Tick t2(100);
    assert(t2.value() == 100);
    std::cout << "  test_tick_construct passed" << std::endl;
}

void test_tick_compare() {
    Tick t1(10);
    Tick t2(20);
    assert(t1 < t2);
    assert(t2 > t1);
    assert(t1 != t2);
    Tick t3(10);
    assert(t1 == t3);
    std::cout << "  test_tick_compare passed" << std::endl;
}

void test_duration_ticks() {
    DurationTicks d1(5);
    assert(d1.value() == 5);
    Tick t1(10);
    Tick t2 = t1 + d1;
    assert(t2.value() == 15);
    std::cout << "  test_duration_ticks passed" << std::endl;
}

void run_version_tick_tests() {
    std::cout << "Running foundation_version_tick tests:" << std::endl;
    test_state_version_construct();
    test_state_version_compare();
    test_state_version_next();
    test_tick_construct();
    test_tick_compare();
    test_duration_ticks();
    std::cout << "All foundation_version_tick tests passed!" << std::endl;
}
