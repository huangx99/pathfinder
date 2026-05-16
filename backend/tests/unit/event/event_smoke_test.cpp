#include <iostream>
#include <string>

// Forward declarations of test functions
void run_event_types_tests();
void run_event_record_tests();
void run_event_stream_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, types, record, stream" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "event smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_event_types_tests();
        return 0;
    } else if (test_name == "record") {
        run_event_record_tests();
        return 0;
    } else if (test_name == "stream") {
        run_event_stream_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
