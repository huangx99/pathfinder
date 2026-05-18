#pragma once

#include "pathfinder/civilization/civilization_types.h"
#include <vector>

namespace pathfinder::civilization {

// ---------------------------------------------------------------------------
// CivilizationCandidateBuilder
// ---------------------------------------------------------------------------

class CivilizationCandidateBuilder {
public:
    Result<std::vector<CivilizationCapabilityCandidate>> build(
        const CivilizationResolveInput& input) const;
};

// ---------------------------------------------------------------------------
// CapabilityRequirementEvaluator
// ---------------------------------------------------------------------------

class CapabilityRequirementEvaluator {
public:
    Result<CapabilityRequirementSnapshot> evaluate(
        const CivilizationCapability& capability,
        const CivilizationResolveInput& input) const;
};

// ---------------------------------------------------------------------------
// CapabilityStateResolver
// ---------------------------------------------------------------------------

class CapabilityStateResolver {
public:
    Result<CivilizationCapabilityState> resolveState(
        const CivilizationCapabilityState& current,
        const CivilizationCapabilityCandidate& candidate,
        const CivilizationCapability& definition) const;
};

// ---------------------------------------------------------------------------
// CapabilityUsabilityResolver
// ---------------------------------------------------------------------------

class CapabilityUsabilityResolver {
public:
    Result<CivilizationCapabilityState> resolveUsability(
        const CivilizationCapabilityState& state,
        const CivilizationResolveInput& input) const;
};

// ---------------------------------------------------------------------------
// CapabilityEffectResolver
// ---------------------------------------------------------------------------

class CapabilityEffectResolver {
public:
    Result<std::vector<CapabilityEffectDraft>> resolveEffects(
        const CivilizationState& state,
        const std::vector<CivilizationCapability>& definitions) const;
};

// ---------------------------------------------------------------------------
// CivilizationStageResolver
// ---------------------------------------------------------------------------

struct CivilizationStageResolveResult {
    CivilizationStage stage{CivilizationStage::Unknown};
    double confidence{0.0};
    std::string stage_label_key;
    std::vector<std::string> stage_steps;
};

class CivilizationStageResolver {
public:
    Result<CivilizationStageResolveResult> resolveStage(
        const CivilizationState& state) const;
};

// ---------------------------------------------------------------------------
// CivilizationResolver (12-step orchestrator)
// ---------------------------------------------------------------------------

class CivilizationResolver {
public:
    Result<CivilizationResolveResult> resolve(
        const CivilizationResolveInput& input) const;

private:
    CivilizationCandidateBuilder candidate_builder_;
    CapabilityRequirementEvaluator requirement_evaluator_;
    CapabilityStateResolver state_resolver_;
    CapabilityUsabilityResolver usability_resolver_;
    CapabilityEffectResolver effect_resolver_;
    CivilizationStageResolver stage_resolver_;
};

} // namespace pathfinder::civilization
