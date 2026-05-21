#include "pathfinder/world_teaching/world_teaching_eligibility_service.h"
#include "pathfinder/world_teaching/world_teaching_owner_resolver.h"
#include "pathfinder/foundation/error.h"
#include <cmath>

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace pathfinder::world_teaching {

WorldTeachingEligibilityResult WorldTeachingEligibilityService::check(
    const WorldTeachingRequest& request,
    const pathfinder::knowledge::KnowledgeRepository& repository,
    const IWorldActorQueryPort& actor_query_port) const {

    WorldTeachingEligibilityResult result;
    result.allowed = false;
    result.failure_kind = WorldTeachingFailureKind::None;

    WorldActorKnowledgeOwnerResolver resolver;
    result.teacher_owner = resolver.resolve(request.teacher_actor_key);
    result.recipient_owner = resolver.resolve(request.recipient_actor_key);

    // 1. Validate teacher != recipient (default: not allowed)
    if (request.teacher_actor_key == request.recipient_actor_key) {
        result.failure_kind = WorldTeachingFailureKind::SameOwnerNotAllowed;
        result.reason_keys.push_back("same_owner_not_allowed");
        return result;
    }

    // 2. Validate actors exist via query port
    auto teacher_opt = actor_query_port.findActor(request.teacher_actor_key);
    auto recipient_opt = actor_query_port.findActor(request.recipient_actor_key);

    if (!teacher_opt.has_value()) {
        result.failure_kind = WorldTeachingFailureKind::TeacherMissing;
        result.reason_keys.push_back("teacher_missing");
        return result;
    }

    if (!recipient_opt.has_value()) {
        result.failure_kind = WorldTeachingFailureKind::RecipientMissing;
        result.reason_keys.push_back("recipient_missing");
        return result;
    }

    // 3. Validate same layer
    const auto& teacher_actor = teacher_opt.value();
    const auto& recipient_actor = recipient_opt.value();
    if (teacher_actor.coord.layer_key != recipient_actor.coord.layer_key) {
        result.failure_kind = WorldTeachingFailureKind::OutOfRange;
        result.reason_keys.push_back("different_layer");
        return result;
    }

    // 4. Validate distance (Manhattan)
    double distance = std::abs(teacher_actor.coord.x - recipient_actor.coord.x)
                    + std::abs(teacher_actor.coord.y - recipient_actor.coord.y);
    result.distance = distance;
    if (distance > request.max_distance) {
        result.failure_kind = WorldTeachingFailureKind::OutOfRange;
        result.reason_keys.push_back("out_of_range");
        return result;
    }

    // 5. Validate source claim exists
    auto find_res = repository.find(request.source_knowledge_id);
    if (find_res.is_error() || !find_res.value().has_value()) {
        result.failure_kind = WorldTeachingFailureKind::SourceClaimMissing;
        result.reason_keys.push_back("source_claim_missing");
        return result;
    }

    const auto& source_claim = find_res.value().value();
    result.source_claim = source_claim;

    // 6. Validate source claim owner == teacher
    if (!(source_claim.owner == result.teacher_owner)) {
        result.failure_kind = WorldTeachingFailureKind::SourceClaimOwnerMismatch;
        result.reason_keys.push_back("source_claim_owner_mismatch");
        return result;
    }

    // 7. Validate claim is teachable
    if (!source_claim.teaching_profile.teachable) {
        result.failure_kind = WorldTeachingFailureKind::ClaimNotTeachable;
        result.reason_keys.push_back("claim_not_teachable");
        return result;
    }

    result.allowed = true;
    result.reason_keys.push_back("eligibility_passed");
    return result;
}

} // namespace pathfinder::world_teaching
