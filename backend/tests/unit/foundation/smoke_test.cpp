#include <cassert>
#include <iostream>
#include <string>

// Forward declarations of test functions from other test files
void run_error_tests();
void run_result_void_tests();
void run_result_value_tests();
void run_id_basic_tests();
void run_id_validation_tests();
void run_version_tick_tests();
void run_hash_tests();
void run_random_seed_tests();
void run_serialization_contract_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, error, result_void, result_value, id_basic, id_validation, version_tick, hash, random_seed, serialization_contract" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "foundation smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "error") {
        run_error_tests();
        return 0;
    } else if (test_name == "result_void") {
        run_result_void_tests();
        return 0;
    } else if (test_name == "result_value") {
        run_result_value_tests();
        return 0;
    } else if (test_name == "id_basic") {
        run_id_basic_tests();
        return 0;
    } else if (test_name == "id_validation") {
        run_id_validation_tests();
        return 0;
    } else if (test_name == "version_tick") {
        run_version_tick_tests();
        return 0;
    } else if (test_name == "hash") {
        run_hash_tests();
        return 0;
    } else if (test_name == "random_seed") {
        run_random_seed_tests();
        return 0;
    } else if (test_name == "serialization_contract") {
        run_serialization_contract_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
