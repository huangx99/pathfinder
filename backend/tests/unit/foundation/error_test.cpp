#include "pathfinder/foundation/error.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::foundation;

void test_error_code_to_string() {
    assert(toString(ErrorCode::common_unknown) == "common_unknown");
    assert(toString(ErrorCode::id_invalid_format) == "id_invalid_format");
    assert(toString(ErrorCode::precondition_target_too_far) == "precondition_target_too_far");
    std::cout << "  test_error_code_to_string passed" << std::endl;
}

void test_error_severity_to_string() {
    assert(toString(ErrorSeverity::Warning) == "Warning");
    assert(toString(ErrorSeverity::Fatal) == "Fatal");
    std::cout << "  test_error_severity_to_string passed" << std::endl;
}

void test_error_category_to_string() {
    assert(toString(ErrorCategory::Common) == "Common");
    assert(toString(ErrorCategory::Command) == "Command");
    std::cout << "  test_error_category_to_string passed" << std::endl;
}

void test_error_detail_message_key() {
    auto detail = makeError(ErrorCode::id_invalid_format, "error.id.invalid_format");
    assert(detail.message_key == "error.id.invalid_format");
    assert(detail.code == ErrorCode::id_invalid_format);
    assert(detail.severity == ErrorSeverity::UserError);
    assert(detail.category == ErrorCategory::Id);
    std::cout << "  test_error_detail_message_key passed" << std::endl;
}

void test_error_detail_to_string() {
    auto detail = makeError(ErrorCode::common_unknown, "error.common.unknown", "test debug");
    auto str = toString(detail);
    assert(str.find("common_unknown") != std::string::npos);
    assert(str.find("test debug") != std::string::npos);
    std::cout << "  test_error_detail_to_string passed" << std::endl;
}

void test_get_category() {
    assert(getCategory(ErrorCode::command_invalid_format) == ErrorCategory::Command);
    assert(getCategory(ErrorCode::id_not_found) == ErrorCategory::Id);
    assert(getCategory(ErrorCode::state_version_conflict) == ErrorCategory::State);
    std::cout << "  test_get_category passed" << std::endl;
}

void test_get_default_severity() {
    assert(getDefaultSeverity(ErrorCode::common_unknown) == ErrorSeverity::Critical);
    assert(getDefaultSeverity(ErrorCode::command_invalid_format) == ErrorSeverity::UserError);
    assert(getDefaultSeverity(ErrorCode::state_corrupted) == ErrorSeverity::Fatal);
    std::cout << "  test_get_default_severity passed" << std::endl;
}

void run_error_tests() {
    std::cout << "Running foundation_error tests:" << std::endl;
    test_error_code_to_string();
    test_error_severity_to_string();
    test_error_category_to_string();
    test_error_detail_message_key();
    test_error_detail_to_string();
    test_get_category();
    test_get_default_severity();
    std::cout << "All foundation_error tests passed!" << std::endl;
}
