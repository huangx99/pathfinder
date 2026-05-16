#include "pathfinder/foundation/id.h"
#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace pathfinder::foundation;

void test_id_construct() {
    ObjectId id("obj_000001");
    assert(id.value() == "obj_000001");
    assert(!id.empty());
    std::cout << "  test_id_construct passed" << std::endl;
}

void test_id_empty() {
    ObjectId id;
    assert(id.empty());
    assert(id.value().empty());
    std::cout << "  test_id_empty passed" << std::endl;
}

void test_id_equality() {
    ObjectId id1("obj_000001");
    ObjectId id2("obj_000001");
    ObjectId id3("obj_000002");
    assert(id1 == id2);
    assert(id1 != id3);
    std::cout << "  test_id_equality passed" << std::endl;
}

void test_id_different_tags() {
    // Different tag types cannot be directly compared (compile-time safety)
    ObjectId obj_id("obj_000001");
    EntityId ent_id("ent_000001");
    // This would not compile: assert(obj_id == ent_id);
    assert(obj_id.value() != ent_id.value()); // Different values
    ObjectId obj_id2("obj_000001");
    assert(obj_id == obj_id2); // Same tag, same value
    std::cout << "  test_id_different_tags passed" << std::endl;
}

void test_id_hash() {
    ObjectId id1("obj_000001");
    ObjectId id2("obj_000001");
    std::hash<ObjectId> hasher;
    assert(hasher(id1) == hasher(id2));
    std::cout << "  test_id_hash passed" << std::endl;
}

void test_id_unordered_map() {
    std::unordered_map<ObjectId, int> map;
    ObjectId id1("obj_000001");
    ObjectId id2("obj_000002");
    map[id1] = 1;
    map[id2] = 2;
    assert(map[id1] == 1);
    assert(map[id2] == 2);
    assert(map.size() == 2);
    std::cout << "  test_id_unordered_map passed" << std::endl;
}

void run_id_basic_tests() {
    std::cout << "Running foundation_id_basic tests:" << std::endl;
    test_id_construct();
    test_id_empty();
    test_id_equality();
    test_id_different_tags();
    test_id_hash();
    test_id_unordered_map();
    std::cout << "All foundation_id_basic tests passed!" << std::endl;
}
