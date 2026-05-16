#include "pathfinder/agent/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;

void test_agent_scale_roundtrip() {
    auto result1 = agentScaleFromString("micro_creature");
    assert(result1.is_ok());
    assert(result1.value() == AgentScale::MicroCreature);
    assert(toString(result1.value()) == "micro_creature");

    auto result2 = agentScaleFromString("small_creature");
    assert(result2.is_ok());
    assert(result2.value() == AgentScale::SmallCreature);
    assert(toString(result2.value()) == "small_creature");

    auto result3 = agentScaleFromString("individual");
    assert(result3.is_ok());
    assert(result3.value() == AgentScale::Individual);

    auto result4 = agentScaleFromString("tribe");
    assert(result4.is_ok());
    assert(result4.value() == AgentScale::Tribe);

    auto result5 = agentScaleFromString("civilization");
    assert(result5.is_ok());
    assert(result5.value() == AgentScale::Civilization);

    auto result6 = agentScaleFromString("planetary");
    assert(result6.is_ok());
    assert(result6.value() == AgentScale::Planetary);

    auto result7 = agentScaleFromString("interstellar");
    assert(result7.is_ok());
    assert(result7.value() == AgentScale::Interstellar);

    auto result8 = agentScaleFromString("squad");
    assert(result8.is_ok());
    assert(result8.value() == AgentScale::Squad);

    std::cout << "  PASS: agent_scale_roundtrip\n";
}

void test_agent_scale_unknown_tostring() {
    assert(toString(AgentScale::Unknown) == "unknown");
    std::cout << "  PASS: agent_scale_unknown_tostring\n";
}

void test_agent_scale_invalid_string() {
    auto result = agentScaleFromString("invalid_scale");
    assert(result.is_error());
    std::cout << "  PASS: agent_scale_invalid_string\n";
}

void test_agent_scale_case_sensitive() {
    auto result = agentScaleFromString("MicroCreature");
    assert(result.is_error());
    std::cout << "  PASS: agent_scale_case_sensitive\n";
}

void test_agent_cognition_band_roundtrip() {
    auto r1 = agentCognitionBandFromString("reflex");
    assert(r1.is_ok() && r1.value() == AgentCognitionBand::Reflex);
    assert(toString(r1.value()) == "reflex");

    auto r2 = agentCognitionBandFromString("instinct");
    assert(r2.is_ok() && r2.value() == AgentCognitionBand::Instinct);

    auto r3 = agentCognitionBandFromString("associative");
    assert(r3.is_ok() && r3.value() == AgentCognitionBand::Associative);

    auto r4 = agentCognitionBandFromString("tool_use");
    assert(r4.is_ok() && r4.value() == AgentCognitionBand::ToolUse);

    auto r5 = agentCognitionBandFromString("social_learning");
    assert(r5.is_ok() && r5.value() == AgentCognitionBand::SocialLearning);

    auto r6 = agentCognitionBandFromString("abstract_reasoning");
    assert(r6.is_ok() && r6.value() == AgentCognitionBand::AbstractReasoning);

    auto r7 = agentCognitionBandFromString("civilizational");
    assert(r7.is_ok() && r7.value() == AgentCognitionBand::Civilizational);

    std::cout << "  PASS: agent_cognition_band_roundtrip\n";
}

void test_agent_cognition_band_invalid() {
    auto result = agentCognitionBandFromString("genius");
    assert(result.is_error());
    std::cout << "  PASS: agent_cognition_band_invalid\n";
}

void test_agent_embodiment_roundtrip() {
    auto r1 = agentEmbodimentFromString("single_body");
    assert(r1.is_ok() && r1.value() == AgentEmbodiment::SingleBody);
    assert(toString(r1.value()) == "single_body");

    auto r2 = agentEmbodimentFromString("multi_body");
    assert(r2.is_ok() && r2.value() == AgentEmbodiment::MultiBody);

    auto r3 = agentEmbodimentFromString("distributed_group");
    assert(r3.is_ok() && r3.value() == AgentEmbodiment::DistributedGroup);

    auto r4 = agentEmbodimentFromString("remote_influence");
    assert(r4.is_ok() && r4.value() == AgentEmbodiment::RemoteInfluence);

    auto r5 = agentEmbodimentFromString("system_process");
    assert(r5.is_ok() && r5.value() == AgentEmbodiment::SystemProcess);

    std::cout << "  PASS: agent_embodiment_roundtrip\n";
}

void test_agent_embodiment_invalid() {
    auto result = agentEmbodimentFromString("swarm");
    assert(result.is_error());
    std::cout << "  PASS: agent_embodiment_invalid\n";
}

void test_agent_controller_type_roundtrip() {
    auto r1 = agentControllerTypeFromString("player");
    assert(r1.is_ok() && r1.value() == AgentControllerType::Player);
    assert(toString(r1.value()) == "player");

    auto r2 = agentControllerTypeFromString("ai");
    assert(r2.is_ok() && r2.value() == AgentControllerType::Ai);

    auto r3 = agentControllerTypeFromString("script");
    assert(r3.is_ok() && r3.value() == AgentControllerType::Script);

    auto r4 = agentControllerTypeFromString("training");
    assert(r4.is_ok() && r4.value() == AgentControllerType::Training);

    auto r5 = agentControllerTypeFromString("system");
    assert(r5.is_ok() && r5.value() == AgentControllerType::System);

    auto r6 = agentControllerTypeFromString("test");
    assert(r6.is_ok() && r6.value() == AgentControllerType::Test);

    std::cout << "  PASS: agent_controller_type_roundtrip\n";
}

void test_agent_controller_type_invalid() {
    auto result = agentControllerTypeFromString("remote");
    assert(result.is_error());
    std::cout << "  PASS: agent_controller_type_invalid\n";
}

void test_agent_control_authority_roundtrip() {
    auto r1 = agentControlAuthorityFromString("primary");
    assert(r1.is_ok() && r1.value() == AgentControlAuthority::Primary);
    assert(toString(r1.value()) == "primary");

    auto r2 = agentControlAuthorityFromString("assistant");
    assert(r2.is_ok() && r2.value() == AgentControlAuthority::Assistant);

    auto r3 = agentControlAuthorityFromString("observer");
    assert(r3.is_ok() && r3.value() == AgentControlAuthority::Observer);

    std::cout << "  PASS: agent_control_authority_roundtrip\n";
}

void test_agent_control_authority_invalid() {
    auto result = agentControlAuthorityFromString("admin");
    assert(result.is_error());
    std::cout << "  PASS: agent_control_authority_invalid\n";
}

void test_agent_intent_type_roundtrip() {
    auto r1 = agentIntentTypeFromString("eat");
    assert(r1.is_ok() && r1.value() == AgentIntentType::Eat);
    assert(toString(r1.value()) == "eat");

    auto r2 = agentIntentTypeFromString("use");
    assert(r2.is_ok() && r2.value() == AgentIntentType::Use);

    auto r3 = agentIntentTypeFromString("avoid_risk");
    assert(r3.is_ok() && r3.value() == AgentIntentType::AvoidRisk);

    auto r4 = agentIntentTypeFromString("flee");
    assert(r4.is_ok() && r4.value() == AgentIntentType::Flee);

    auto r5 = agentIntentTypeFromString("call_group");
    assert(r5.is_ok() && r5.value() == AgentIntentType::CallGroup);

    auto r6 = agentIntentTypeFromString("explore");
    assert(r6.is_ok() && r6.value() == AgentIntentType::Explore);

    auto r7 = agentIntentTypeFromString("teach");
    assert(r7.is_ok() && r7.value() == AgentIntentType::Teach);

    auto r8 = agentIntentTypeFromString("combine");
    assert(r8.is_ok() && r8.value() == AgentIntentType::Combine);

    auto r9 = agentIntentTypeFromString("fight");
    assert(r9.is_ok() && r9.value() == AgentIntentType::Fight);

    auto r10 = agentIntentTypeFromString("wait");
    assert(r10.is_ok() && r10.value() == AgentIntentType::Wait);

    std::cout << "  PASS: agent_intent_type_roundtrip\n";
}

void test_agent_intent_type_invalid() {
    auto result = agentIntentTypeFromString("dance");
    assert(result.is_error());
    std::cout << "  PASS: agent_intent_type_invalid\n";
}

void run_agent_types_tests() {
    test_agent_scale_roundtrip();
    test_agent_scale_unknown_tostring();
    test_agent_scale_invalid_string();
    test_agent_scale_case_sensitive();
    test_agent_cognition_band_roundtrip();
    test_agent_cognition_band_invalid();
    test_agent_embodiment_roundtrip();
    test_agent_embodiment_invalid();
    test_agent_controller_type_roundtrip();
    test_agent_controller_type_invalid();
    test_agent_control_authority_roundtrip();
    test_agent_control_authority_invalid();
    test_agent_intent_type_roundtrip();
    test_agent_intent_type_invalid();
    std::cout << "All agent types tests passed!\n";
}
