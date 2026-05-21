#include <iostream>
#include <string>

// Test function declarations
void run_world_learning_enum_roundtrip_tests();
void run_world_learning_extracts_experience_from_command_result_tests();
void run_world_learning_resolves_template_from_reason_keys_tests();
void run_world_learning_resolves_template_by_action_effect_fallback_tests();
void run_world_learning_skips_missing_template_without_world_mutation_tests();
void run_world_learning_rejects_unsafe_experience_tests();
void run_world_learning_builds_safe_feedback_tests();
void run_world_learning_direct_experience_forms_claim_tests();
void run_world_learning_repeated_experience_reinforces_claim_tests();
void run_world_learning_high_risk_effect_is_normalized_tests();
void run_world_learning_no_frontend_knowledge_input_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests:" << std::endl;
        std::cout << "  enum_roundtrip" << std::endl;
        std::cout << "  extracts_experience_from_command_result" << std::endl;
        std::cout << "  resolves_template_from_reason_keys" << std::endl;
        std::cout << "  resolves_template_by_action_effect_fallback" << std::endl;
        std::cout << "  skips_missing_template_without_world_mutation" << std::endl;
        std::cout << "  rejects_unsafe_experience" << std::endl;
        std::cout << "  builds_safe_feedback" << std::endl;
        std::cout << "  direct_experience_forms_claim" << std::endl;
        std::cout << "  repeated_experience_reinforces_claim" << std::endl;
        std::cout << "  high_risk_effect_is_normalized" << std::endl;
        std::cout << "  no_frontend_knowledge_input" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "enum_roundtrip") {
        run_world_learning_enum_roundtrip_tests();
    } else if (test_name == "extracts_experience_from_command_result") {
        run_world_learning_extracts_experience_from_command_result_tests();
    } else if (test_name == "resolves_template_from_reason_keys") {
        run_world_learning_resolves_template_from_reason_keys_tests();
    } else if (test_name == "resolves_template_by_action_effect_fallback") {
        run_world_learning_resolves_template_by_action_effect_fallback_tests();
    } else if (test_name == "skips_missing_template_without_world_mutation") {
        run_world_learning_skips_missing_template_without_world_mutation_tests();
    } else if (test_name == "rejects_unsafe_experience") {
        run_world_learning_rejects_unsafe_experience_tests();
    } else if (test_name == "builds_safe_feedback") {
        run_world_learning_builds_safe_feedback_tests();
    } else if (test_name == "direct_experience_forms_claim") {
        run_world_learning_direct_experience_forms_claim_tests();
    } else if (test_name == "repeated_experience_reinforces_claim") {
        run_world_learning_repeated_experience_reinforces_claim_tests();
    } else if (test_name == "high_risk_effect_is_normalized") {
        run_world_learning_high_risk_effect_is_normalized_tests();
    } else if (test_name == "no_frontend_knowledge_input") {
        run_world_learning_no_frontend_knowledge_input_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
