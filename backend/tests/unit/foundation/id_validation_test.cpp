#include "pathfinder/foundation/id.h"
#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace pathfinder::foundation;

void test_id_empty_invalid() {
    assert(!isValidIdString(""));
    std::cout << "  test_id_empty_invalid passed" << std::endl;
}

void test_id_with_space_invalid() {
    assert(!isValidIdString("obj 001"));
    assert(!isValidIdString(" obj_001"));
    assert(!isValidIdString("obj_001 "));
    assert(!isValidIdString("obj\t001"));
    assert(!isValidIdString("obj\n001"));
    std::cout << "  test_id_with_space_invalid passed" << std::endl;
}

void test_id_valid_format() {
    assert(isValidIdString("object_001"));
    assert(isValidIdString("red_berry"));
    assert(isValidIdString("cmd-001"));
    assert(isValidIdString("a"));
    assert(isValidIdString("abc123"));
    std::cout << "  test_id_valid_format passed" << std::endl;
}

void test_id_unordered_map_validation() {
    std::unordered_map<ObjectId, int, std::hash<ObjectId>> map;
    ObjectId id1("obj_000001");
    ObjectId id2("obj_000002");
    map[id1] = 1;
    map[id2] = 2;
    assert(map[id1] == 1);
    assert(map[id2] == 2);
    assert(map.size() == 2);
    std::cout << "  test_id_unordered_map_validation passed" << std::endl;
}

void run_id_validation_tests() {
    std::cout << "Running foundation_id_validation tests:" << std::endl;
    test_id_empty_invalid();
    test_id_with_space_invalid();
    test_id_valid_format();
    test_id_unordered_map_validation();
    std::cout << "All foundation_id_validation tests passed!" << std::endl;
}
