#include "pathfinder/foundation/hash.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::foundation;

void test_hash_construct() {
    HashValue h1;
    assert(h1.empty());
    assert(h1.value() == 0);
    HashValue h2(42);
    assert(!h2.empty());
    assert(h2.value() == 42);
    std::cout << "  test_hash_construct passed" << std::endl;
}

void test_hash_from_string() {
    HashValue h1 = HashValue::fromString("hello");
    HashValue h2 = HashValue::fromString("hello");
    assert(h1 == h2);
    assert(!h1.empty());
    std::cout << "  test_hash_from_string passed" << std::endl;
}

void test_hash_different_input() {
    HashValue h1 = HashValue::fromString("hello");
    HashValue h2 = HashValue::fromString("world");
    assert(h1 != h2);
    std::cout << "  test_hash_different_input passed" << std::endl;
}

void test_hash_combine() {
    HashValue h1 = HashValue::fromString("hello");
    HashValue h2 = HashValue::fromString("world");
    HashValue combined1 = HashValue::combineHash(h1, h2);
    HashValue combined2 = HashValue::combineHash(h1, h2);
    assert(combined1 == combined2);
    std::cout << "  test_hash_combine passed" << std::endl;
}

void test_hash_empty() {
    HashValue h1;
    assert(h1.empty());
    HashValue h2 = HashValue::fromString("test");
    assert(!h2.empty());
    std::cout << "  test_hash_empty passed" << std::endl;
}

void run_hash_tests() {
    std::cout << "Running foundation_hash tests:" << std::endl;
    test_hash_construct();
    test_hash_from_string();
    test_hash_different_input();
    test_hash_combine();
    test_hash_empty();
    std::cout << "All foundation_hash tests passed!" << std::endl;
}
