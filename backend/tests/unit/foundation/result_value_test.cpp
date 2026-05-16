#include "pathfinder/foundation/result.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::foundation;

void test_result_value_ok() {
    auto result = Result<int>::ok(42);
    assert(result.is_ok());
    assert(!result.is_error());
    assert(result.value() == 42);
    assert(result.errors().empty());
    std::cout << "  test_result_value_ok passed" << std::endl;
}

void test_result_value_fail() {
    auto error = makeError(ErrorCode::id_not_found, "error.id.not_found");
    auto result = Result<int>::fail(std::move(error));
    assert(!result.is_ok());
    assert(result.is_error());
    assert(result.errors().size() == 1);
    assert(result.errors()[0].code == ErrorCode::id_not_found);
    std::cout << "  test_result_value_fail passed" << std::endl;
}

void test_result_value_string() {
    auto result = Result<std::string>::ok("hello");
    assert(result.is_ok());
    assert(result.value() == "hello");
    std::cout << "  test_result_value_string passed" << std::endl;
}

void test_result_value_warning() {
    auto result = Result<int>::ok(42);
    result.add_warning(makeError(ErrorCode::command_duplicate, "error.command.duplicate"));
    assert(result.is_ok());
    assert(result.value() == 42);
    assert(result.warnings().size() == 1);
    std::cout << "  test_result_value_warning passed" << std::endl;
}

void run_result_value_tests() {
    std::cout << "Running foundation_result_value tests:" << std::endl;
    test_result_value_ok();
    test_result_value_fail();
    test_result_value_string();
    test_result_value_warning();
    std::cout << "All foundation_result_value tests passed!" << std::endl;
}
