#include "pathfinder/foundation/result.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::foundation;

void test_result_void_ok() {
    auto result = Result<void>::ok();
    assert(result.is_ok());
    assert(!result.is_error());
    assert(result.errors().empty());
    std::cout << "  test_result_void_ok passed" << std::endl;
}

void test_result_void_fail() {
    auto error = makeError(ErrorCode::id_not_found, "error.id.not_found");
    auto result = Result<void>::fail(std::move(error));
    assert(!result.is_ok());
    assert(result.is_error());
    assert(result.errors().size() == 1);
    assert(result.errors()[0].code == ErrorCode::id_not_found);
    std::cout << "  test_result_void_fail passed" << std::endl;
}

void test_result_void_multiple_errors() {
    std::vector<ErrorDetail> errors;
    errors.push_back(makeError(ErrorCode::id_not_found, "error.id.not_found"));
    errors.push_back(makeError(ErrorCode::command_invalid_format, "error.command.invalid_format"));
    auto result = Result<void>::fail(std::move(errors));
    assert(result.is_error());
    assert(result.errors().size() == 2);
    std::cout << "  test_result_void_multiple_errors passed" << std::endl;
}

void test_result_void_warning() {
    auto result = Result<void>::ok();
    result.add_warning(makeError(ErrorCode::command_duplicate, "error.command.duplicate"));
    assert(result.is_ok());
    assert(result.warnings().size() == 1);
    std::cout << "  test_result_void_warning passed" << std::endl;
}

void run_result_void_tests() {
    std::cout << "Running foundation_result_void tests:" << std::endl;
    test_result_void_ok();
    test_result_void_fail();
    test_result_void_multiple_errors();
    test_result_void_warning();
    std::cout << "All foundation_result_void tests passed!" << std::endl;
}
