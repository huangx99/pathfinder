#include "pathfinder/agent/observation.h"
#include "pathfinder/foundation/id.h"

namespace pathfinder::agent {

// AgentThreatType
std::string toString(AgentThreatType type) {
    switch (type) {
        case AgentThreatType::Unknown: return "unknown";
        case AgentThreatType::Fire: return "fire";
        case AgentThreatType::Predator: return "predator";
        case AgentThreatType::Environmental: return "environmental";
        case AgentThreatType::Social: return "social";
        default: return "unknown";
    }
}

// AgentKnowledgeClaimType
std::string toString(AgentKnowledgeClaimType type) {
    switch (type) {
        case AgentKnowledgeClaimType::Unknown: return "unknown";
        case AgentKnowledgeClaimType::EdibleEffect: return "edible_effect";
        case AgentKnowledgeClaimType::HarmfulEffect: return "harmful_effect";
        case AgentKnowledgeClaimType::UseEffect: return "use_effect";
        case AgentKnowledgeClaimType::Teachable: return "teachable";
        default: return "unknown";
    }
}

// Helper to check confidence range
static foundation::Result<void> checkConfidence(double val, const char* field) {
    if (val < 0.0 || val > 1.0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "observation_confidence_out_of_range",
                std::string(field) + " must be in [0.0, 1.0], got " + std::to_string(val)));
    }
    return foundation::Result<void>::ok();
}

// AgentObservedObject
foundation::Result<void> AgentObservedObject::validateBasic() const {
    if (object_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "observed_object_id_missing", "object_id must not be empty"));
    }
    if (!foundation::isValidIdString(object_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "observed_object_id_invalid", "object_id has invalid format: " + object_id.value()));
    }
    if (evidence_count < 0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "observed_object_evidence_count_negative",
                "evidence_count must not be negative, got " + std::to_string(evidence_count)));
    }
    auto r = checkConfidence(known_edible_confidence, "known_edible_confidence");
    if (r.is_error()) return r;
    r = checkConfidence(known_usable_confidence, "known_usable_confidence");
    if (r.is_error()) return r;
    r = checkConfidence(risk_confidence, "risk_confidence");
    if (r.is_error()) return r;
    return foundation::Result<void>::ok();
}

// AgentObservedThreat
foundation::Result<void> AgentObservedThreat::validateBasic() const {
    if (source_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "observed_threat_source_id_missing", "source_id must not be empty"));
    }
    if (!foundation::isValidIdString(source_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "observed_threat_source_id_invalid", "source_id has invalid format: " + source_id.value()));
    }
    if (threat_type == AgentThreatType::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "observed_threat_type_unknown", "threat_type must not be Unknown"));
    }
    auto r = checkConfidence(severity, "severity");
    if (r.is_error()) return r;
    r = checkConfidence(confidence, "confidence");
    if (r.is_error()) return r;
    return foundation::Result<void>::ok();
}

// AgentObservedKnowledge
foundation::Result<void> AgentObservedKnowledge::validateBasic() const {
    if (knowledge_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "observed_knowledge_id_missing", "knowledge_id must not be empty"));
    }
    if (!foundation::isValidIdString(knowledge_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "observed_knowledge_id_invalid", "knowledge_id has invalid format: " + knowledge_id.value()));
    }
    if (claim_type == AgentKnowledgeClaimType::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "observed_claim_type_unknown", "claim_type must not be Unknown"));
    }
    if (evidence_count < 0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "observed_knowledge_evidence_count_negative",
                "evidence_count must not be negative, got " + std::to_string(evidence_count)));
    }
    auto r = checkConfidence(confidence, "confidence");
    if (r.is_error()) return r;
    return foundation::Result<void>::ok();
}

// AgentObservation
foundation::Result<void> AgentObservation::validateBasic() const {
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "observation_agent_id_missing", "agent_id must not be empty"));
    }
    if (!foundation::isValidIdString(agent_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "observation_agent_id_invalid", "agent_id has invalid format: " + agent_id.value()));
    }
    if (observer_actor_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "observation_observer_actor_id_missing", "observer_actor_id must not be empty"));
    }
    if (!foundation::isValidIdString(observer_actor_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "observation_observer_actor_id_invalid", "observer_actor_id has invalid format: " + observer_actor_id.value()));
    }

    // Validate sub-structures
    for (const auto& obj : objects) {
        auto r = obj.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& threat : threats) {
        auto r = threat.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& knowledge : knowledge_claims) {
        auto r = knowledge.validateBasic();
        if (r.is_error()) return r;
    }

    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
