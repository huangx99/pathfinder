#pragma once

#include "pathfinder/agent/types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include <vector>
#include <string>

namespace pathfinder::agent {

// AgentObservedObject: what the agent can see about an object
// Only contains cognitive/perceptual fields, NOT visual reveal concepts
struct AgentObservedObject {
    pathfinder::foundation::ObjectId object_id;
    double known_edible_confidence = 0.0;   // [0.0, 1.0] subjective belief
    double known_usable_confidence = 0.0;   // [0.0, 1.0] subjective belief
    double risk_confidence = 0.0;           // [0.0, 1.0] subjective risk
    int evidence_count = 0;                 // how many observations support this
    bool can_teach_hint = false;            // whether this knowledge is teachable

    pathfinder::foundation::Result<void> validateBasic() const;
};

// Threat type enum
enum class AgentThreatType {
    Unknown,
    Fire,
    Predator,
    Environmental,
    Social
};

std::string toString(AgentThreatType type);

// AgentObservedThreat: what the agent perceives as a danger
struct AgentObservedThreat {
    pathfinder::foundation::EntityId source_id;
    AgentThreatType threat_type = AgentThreatType::Unknown;
    double severity = 0.0;     // [0.0, 1.0]
    double confidence = 0.0;   // [0.0, 1.0]

    pathfinder::foundation::Result<void> validateBasic() const;
};

// Knowledge claim type
enum class AgentKnowledgeClaimType {
    Unknown,
    EdibleEffect,
    HarmfulEffect,
    UseEffect,
    Teachable
};

std::string toString(AgentKnowledgeClaimType type);

// AgentObservedKnowledge: what the agent believes about knowledge
struct AgentObservedKnowledge {
    pathfinder::foundation::KnowledgeId knowledge_id;
    AgentKnowledgeClaimType claim_type = AgentKnowledgeClaimType::Unknown;
    double confidence = 0.0;       // [0.0, 1.0]
    int evidence_count = 0;
    bool teachable_hint = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// AgentObservation: the complete observation for an agent at a point in time
struct AgentObservation {
    AgentId agent_id;
    pathfinder::foundation::EntityId observer_actor_id;
    pathfinder::foundation::StateVersion state_version;
    pathfinder::foundation::Tick observed_tick;
    std::vector<AgentObservedObject> objects;
    std::vector<AgentObservedThreat> threats;
    std::vector<AgentObservedKnowledge> knowledge_claims;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
