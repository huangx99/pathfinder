#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::agent {

// Tag types for Agent module IDs
struct AgentIdTag {};
struct AgentDefinitionIdTag {};

// Type aliases
using AgentId = pathfinder::foundation::StrongId<AgentIdTag>;
using AgentDefinitionId = pathfinder::foundation::StrongId<AgentDefinitionIdTag>;

// Agent scale: the size/scope of the agent
enum class AgentScale {
    Unknown,
    MicroCreature,
    SmallCreature,
    Individual,
    Squad,
    Tribe,
    Civilization,
    Planetary,
    Interstellar
};

std::string toString(AgentScale scale);
pathfinder::foundation::Result<AgentScale> agentScaleFromString(const std::string& str);

// Agent cognition band: the cognitive capability level
enum class AgentCognitionBand {
    Unknown,
    Reflex,
    Instinct,
    Associative,
    ToolUse,
    SocialLearning,
    AbstractReasoning,
    Civilizational
};

std::string toString(AgentCognitionBand band);
pathfinder::foundation::Result<AgentCognitionBand> agentCognitionBandFromString(const std::string& str);

// Agent embodiment: how the agent exists in the world
enum class AgentEmbodiment {
    Unknown,
    SingleBody,
    MultiBody,
    DistributedGroup,
    RemoteInfluence,
    SystemProcess
};

std::string toString(AgentEmbodiment embodiment);
pathfinder::foundation::Result<AgentEmbodiment> agentEmbodimentFromString(const std::string& str);

// Agent controller type: who controls the agent
enum class AgentControllerType {
    Unknown,
    Player,
    Ai,
    Script,
    Training,
    System,
    Test
};

std::string toString(AgentControllerType type);
pathfinder::foundation::Result<AgentControllerType> agentControllerTypeFromString(const std::string& str);

// Agent control authority: the level of control an agent has over its binding
enum class AgentControlAuthority {
    Unknown,
    Primary,
    Assistant,
    Observer
};

std::string toString(AgentControlAuthority authority);
pathfinder::foundation::Result<AgentControlAuthority> agentControlAuthorityFromString(const std::string& str);

// Agent intent type: the type of action the agent wants to perform
enum class AgentIntentType {
    Unknown,
    Eat,
    Use,
    AvoidRisk,
    Flee,
    CallGroup,
    Explore,
    Teach,
    Combine,
    Fight,
    Wait
};

std::string toString(AgentIntentType type);
pathfinder::foundation::Result<AgentIntentType> agentIntentTypeFromString(const std::string& str);

} // namespace pathfinder::agent
