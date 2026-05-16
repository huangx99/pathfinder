#include "pathfinder/foundation/random.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::foundation;

void test_random_seed_construct() {
    RandomSeed s1;
    assert(s1.value() == 0);
    RandomSeed s2(42);
    assert(s2.value() == 42);
    std::cout << "  test_random_seed_construct passed" << std::endl;
}

void test_random_seed_equal() {
    RandomSeed s1(42);
    RandomSeed s2(42);
    RandomSeed s3(100);
    assert(s1 == s2);
    assert(s1 != s3);
    std::cout << "  test_random_seed_equal passed" << std::endl;
}

void test_random_draw_record() {
    RandomDrawRecord record;
    record.seed = RandomSeed(42);
    record.draw_index = 5;
    record.value = 12345;
    assert(record.seed.value() == 42);
    assert(record.draw_index == 5);
    assert(record.value == 12345);
    std::cout << "  test_random_draw_record passed" << std::endl;
}

void run_random_seed_tests() {
    std::cout << "Running foundation_random_seed tests:" << std::endl;
    test_random_seed_construct();
    test_random_seed_equal();
    test_random_draw_record();
    std::cout << "All foundation_random_seed tests passed!" << std::endl;
}
