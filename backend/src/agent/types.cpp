#include "pathfinder/agent/types.h"

namespace pathfinder::agent {

// AgentScale
std::string toString(AgentScale scale) {
    switch (scale) {
        case AgentScale::Unknown: return "unknown";
        case AgentScale::MicroCreature: return "micro_creature";
        case AgentScale::SmallCreature: return "small_creature";
        case AgentScale::Individual: return "individual";
        case AgentScale::Squad: return "squad";
        case AgentScale::Tribe: return "tribe";
        case AgentScale::Civilization: return "civilization";
        case AgentScale::Planetary: return "planetary";
        case AgentScale::Interstellar: return "interstellar";
        default: return "unknown";
    }
}

foundation::Result<AgentScale> agentScaleFromString(const std::string& str) {
    if (str == "micro_creature") return foundation::Result<AgentScale>::ok(AgentScale::MicroCreature);
    if (str == "small_creature") return foundation::Result<AgentScale>::ok(AgentScale::SmallCreature);
    if (str == "individual") return foundation::Result<AgentScale>::ok(AgentScale::Individual);
    if (str == "squad") return foundation::Result<AgentScale>::ok(AgentScale::Squad);
    if (str == "tribe") return foundation::Result<AgentScale>::ok(AgentScale::Tribe);
    if (str == "civilization") return foundation::Result<AgentScale>::ok(AgentScale::Civilization);
    if (str == "planetary") return foundation::Result<AgentScale>::ok(AgentScale::Planetary);
    if (str == "interstellar") return foundation::Result<AgentScale>::ok(AgentScale::Interstellar);
    return foundation::Result<AgentScale>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_scale_invalid", "Unknown AgentScale: " + str));
}

// AgentCognitionBand
std::string toString(AgentCognitionBand band) {
    switch (band) {
        case AgentCognitionBand::Unknown: return "unknown";
        case AgentCognitionBand::Reflex: return "reflex";
        case AgentCognitionBand::Instinct: return "instinct";
        case AgentCognitionBand::Associative: return "associative";
        case AgentCognitionBand::ToolUse: return "tool_use";
        case AgentCognitionBand::SocialLearning: return "social_learning";
        case AgentCognitionBand::AbstractReasoning: return "abstract_reasoning";
        case AgentCognitionBand::Civilizational: return "civilizational";
        default: return "unknown";
    }
}

foundation::Result<AgentCognitionBand> agentCognitionBandFromString(const std::string& str) {
    if (str == "reflex") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::Reflex);
    if (str == "instinct") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::Instinct);
    if (str == "associative") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::Associative);
    if (str == "tool_use") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::ToolUse);
    if (str == "social_learning") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::SocialLearning);
    if (str == "abstract_reasoning") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::AbstractReasoning);
    if (str == "civilizational") return foundation::Result<AgentCognitionBand>::ok(AgentCognitionBand::Civilizational);
    return foundation::Result<AgentCognitionBand>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_cognition_band_invalid", "Unknown AgentCognitionBand: " + str));
}

// AgentEmbodiment
std::string toString(AgentEmbodiment embodiment) {
    switch (embodiment) {
        case AgentEmbodiment::Unknown: return "unknown";
        case AgentEmbodiment::SingleBody: return "single_body";
        case AgentEmbodiment::MultiBody: return "multi_body";
        case AgentEmbodiment::DistributedGroup: return "distributed_group";
        case AgentEmbodiment::RemoteInfluence: return "remote_influence";
        case AgentEmbodiment::SystemProcess: return "system_process";
        default: return "unknown";
    }
}

foundation::Result<AgentEmbodiment> agentEmbodimentFromString(const std::string& str) {
    if (str == "single_body") return foundation::Result<AgentEmbodiment>::ok(AgentEmbodiment::SingleBody);
    if (str == "multi_body") return foundation::Result<AgentEmbodiment>::ok(AgentEmbodiment::MultiBody);
    if (str == "distributed_group") return foundation::Result<AgentEmbodiment>::ok(AgentEmbodiment::DistributedGroup);
    if (str == "remote_influence") return foundation::Result<AgentEmbodiment>::ok(AgentEmbodiment::RemoteInfluence);
    if (str == "system_process") return foundation::Result<AgentEmbodiment>::ok(AgentEmbodiment::SystemProcess);
    return foundation::Result<AgentEmbodiment>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_embodiment_invalid", "Unknown AgentEmbodiment: " + str));
}

// AgentControllerType
std::string toString(AgentControllerType type) {
    switch (type) {
        case AgentControllerType::Unknown: return "unknown";
        case AgentControllerType::Player: return "player";
        case AgentControllerType::Ai: return "ai";
        case AgentControllerType::Script: return "script";
        case AgentControllerType::Training: return "training";
        case AgentControllerType::System: return "system";
        case AgentControllerType::Test: return "test";
        default: return "unknown";
    }
}

foundation::Result<AgentControllerType> agentControllerTypeFromString(const std::string& str) {
    if (str == "player") return foundation::Result<AgentControllerType>::ok(AgentControllerType::Player);
    if (str == "ai") return foundation::Result<AgentControllerType>::ok(AgentControllerType::Ai);
    if (str == "script") return foundation::Result<AgentControllerType>::ok(AgentControllerType::Script);
    if (str == "training") return foundation::Result<AgentControllerType>::ok(AgentControllerType::Training);
    if (str == "system") return foundation::Result<AgentControllerType>::ok(AgentControllerType::System);
    if (str == "test") return foundation::Result<AgentControllerType>::ok(AgentControllerType::Test);
    return foundation::Result<AgentControllerType>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_controller_type_invalid", "Unknown AgentControllerType: " + str));
}

// AgentControlAuthority
std::string toString(AgentControlAuthority authority) {
    switch (authority) {
        case AgentControlAuthority::Unknown: return "unknown";
        case AgentControlAuthority::Primary: return "primary";
        case AgentControlAuthority::Assistant: return "assistant";
        case AgentControlAuthority::Observer: return "observer";
        default: return "unknown";
    }
}

foundation::Result<AgentControlAuthority> agentControlAuthorityFromString(const std::string& str) {
    if (str == "primary") return foundation::Result<AgentControlAuthority>::ok(AgentControlAuthority::Primary);
    if (str == "assistant") return foundation::Result<AgentControlAuthority>::ok(AgentControlAuthority::Assistant);
    if (str == "observer") return foundation::Result<AgentControlAuthority>::ok(AgentControlAuthority::Observer);
    return foundation::Result<AgentControlAuthority>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_control_authority_invalid", "Unknown AgentControlAuthority: " + str));
}

// AgentIntentType
std::string toString(AgentIntentType type) {
    switch (type) {
        case AgentIntentType::Unknown: return "unknown";
        case AgentIntentType::Eat: return "eat";
        case AgentIntentType::Use: return "use";
        case AgentIntentType::AvoidRisk: return "avoid_risk";
        case AgentIntentType::Flee: return "flee";
        case AgentIntentType::CallGroup: return "call_group";
        case AgentIntentType::Explore: return "explore";
        case AgentIntentType::Teach: return "teach";
        case AgentIntentType::Combine: return "combine";
        case AgentIntentType::Fight: return "fight";
        case AgentIntentType::Wait: return "wait";
        default: return "unknown";
    }
}

foundation::Result<AgentIntentType> agentIntentTypeFromString(const std::string& str) {
    if (str == "eat") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Eat);
    if (str == "use") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Use);
    if (str == "avoid_risk") return foundation::Result<AgentIntentType>::ok(AgentIntentType::AvoidRisk);
    if (str == "flee") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Flee);
    if (str == "call_group") return foundation::Result<AgentIntentType>::ok(AgentIntentType::CallGroup);
    if (str == "explore") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Explore);
    if (str == "teach") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Teach);
    if (str == "combine") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Combine);
    if (str == "fight") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Fight);
    if (str == "wait") return foundation::Result<AgentIntentType>::ok(AgentIntentType::Wait);
    return foundation::Result<AgentIntentType>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_intent_type_invalid", "Unknown AgentIntentType: " + str));
}

} // namespace pathfinder::agent
