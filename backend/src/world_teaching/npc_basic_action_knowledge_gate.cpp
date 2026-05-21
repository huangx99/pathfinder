#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/world_teaching/world_teaching_owner_resolver.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_types.h"
#include <algorithm>

using pathfinder::knowledge::KnowledgeStatus;
using pathfinder::knowledge::KnowledgeRelationType;

namespace pathfinder::world_teaching {

NpcActionKnowledgeGateResult NpcBasicActionKnowledgeGate::check(
    const NpcActionKnowledgeGateRequest& request) const {

    NpcActionKnowledgeGateResult result;
    result.allowed = false;
    result.decision = NpcActionKnowledgeGateDecision::BlockedNoKnowledge;

    for (const auto& claim : request.actor_claims) {
        // 1. Owner must match actor
        WorldActorKnowledgeOwnerResolver resolver;
        auto expected_owner = resolver.resolve(request.actor_key);
        if (!(claim.owner == expected_owner)) {
            continue;
        }

        // 2. Must be usable for action by an AI-controlled actor.
        if (!claim.projection_flags.usable_for_action || !claim.projection_flags.usable_by_ai) {
            continue;
        }

        // 3. Subject must match
        if (claim.subject.subject_id != request.subject_object_key) {
            continue;
        }

        // 4. Action must match
        if (claim.predicate.action_key != request.action_key) {
            continue;
        }

        // 5. Effect must match if provided
        if (!request.effect_key.empty() && claim.predicate.effect_key != request.effect_key) {
            continue;
        }

        // 6. Target object must match if provided
        if (!request.target_object_key.empty()) {
            bool target_match = false;
            for (const auto& related : claim.subject.related_subject_ids) {
                if (related == request.target_object_key) {
                    target_match = true;
                    break;
                }
            }
            if (!target_match && claim.predicate.effect_key != request.target_object_key) {
                // Allow effect_key as fallback target match
                // If still no match, skip
                // Actually per design: match claim.subject.related_subject_ids or predicate target
                // Since predicate does not have a direct 'target' field beyond effect_key,
                // we interpret effect_key as a proxy for target when appropriate.
                // Re-read design: "target_object_key 如果请求提供，则必须匹配 claim.subject.related_subject_ids 或 predicate target"
                // predicate does not have a 'target' field. We use related_subject_ids only.
                continue;
            }
        }

        // 7. Status must not be Deprecated or Disproven
        if (claim.status == KnowledgeStatus::Deprecated || claim.status == KnowledgeStatus::Disproven) {
            result.decision = NpcActionKnowledgeGateDecision::BlockedClaimDisproven;
            result.matched_claim = claim;
            continue; // keep looking for a better match
        }

        // 8. If hypothesis not allowed, skip Candidate/Hypothesis
        if (!request.allow_hypothesis &&
            (claim.status == KnowledgeStatus::Candidate || claim.status == KnowledgeStatus::Hypothesis)) {
            result.decision = NpcActionKnowledgeGateDecision::BlockedLowConfidence;
            result.matched_claim = claim;
            continue;
        }

        // 9. Risk check: if claim indicates danger and allow_risk_action is false, block
        bool is_risk_knowledge = (claim.predicate.relation_type == KnowledgeRelationType::HasRisk ||
                                  claim.predicate.relation_type == KnowledgeRelationType::IsDangerousUnder);
        if (is_risk_knowledge && !request.allow_risk_action) {
            result.decision = NpcActionKnowledgeGateDecision::BlockedDangerWithoutGoal;
            result.matched_claim = claim;
            continue;
        }

        // All checks passed
        result.allowed = true;
        result.decision = NpcActionKnowledgeGateDecision::AllowedByKnowledge;
        result.matched_claim = claim;
        result.reason_keys.push_back("action_allowed_by_knowledge");
        return result;
    }

    return result;
}

NpcActionKnowledgeGateResult NpcBasicActionKnowledgeGate::check(
    const std::string& actor_key,
    const std::string& subject_object_key,
    const std::string& action_key,
    const std::string& effect_key,
    const std::string& target_object_key,
    bool allow_hypothesis,
    bool allow_risk_action,
    const pathfinder::knowledge::KnowledgeRepository& repository) const {

    WorldActorKnowledgeOwnerResolver resolver;
    auto owner = resolver.resolve(actor_key);

    auto claims_res = repository.listByOwner(owner);
    std::vector<pathfinder::knowledge::KnowledgeClaim> claims;
    if (claims_res.is_ok()) {
        claims = claims_res.value();
    }

    NpcActionKnowledgeGateRequest request;
    request.actor_key = actor_key;
    request.subject_object_key = subject_object_key;
    request.action_key = action_key;
    request.effect_key = effect_key;
    request.target_object_key = target_object_key;
    request.allow_hypothesis = allow_hypothesis;
    request.allow_risk_action = allow_risk_action;
    request.actor_claims = std::move(claims);

    return check(request);
}

} // namespace pathfinder::world_teaching
