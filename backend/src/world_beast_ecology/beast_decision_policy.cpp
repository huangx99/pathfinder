#include "pathfinder/world_beast_ecology/beast_decision_policy.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_beast_ecology {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using pathfinder::knowledge::KnowledgeClaim;
using pathfinder::knowledge::KnowledgeRelationType;

static bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
    for (const auto& t : tags) {
        if (t == tag) return true;
    }
    return false;
}

static bool hasAnyTag(const std::vector<std::string>& tags, const std::vector<std::string>& search) {
    for (const auto& s : search) {
        if (hasTag(tags, s)) return true;
    }
    return false;
}

static bool itemMatchesDangerTag(const BeastPerceptionItem& item, const std::vector<std::string>& danger_tags) {
    return hasAnyTag(item.tag_keys, danger_tags);
}

static bool itemMatchesPreyTag(const BeastPerceptionItem& item, const std::vector<std::string>& prey_tags) {
    return hasAnyTag(item.tag_keys, prey_tags);
}

static bool claimImpliesRiskForTarget(const KnowledgeClaim& claim, const std::string& target_key) {
    if (claim.status != pathfinder::knowledge::KnowledgeStatus::Active) return false;
    if (claim.predicate.relation_type != KnowledgeRelationType::HasRisk &&
        claim.predicate.relation_type != KnowledgeRelationType::IsDangerousUnder) {
        return false;
    }
    if (claim.subject.subject_id == target_key) return true;
    for (const auto& related : claim.subject.related_subject_ids) {
        if (related == target_key) return true;
    }
    return false;
}

Result<BeastActionIntent> BeastDecisionPolicy::selectIntent(const PolicyContext& context) {
    BeastActionIntent intent;
    intent.intent_id = "intent_" + context.actor_key + "_" + std::to_string(context.tick);
    intent.actor_key = context.actor_key;
    intent.risk_score = 0.0;

    // 1. Evaluate learned risks + perceived dangers -> Flee / AvoidArea
    for (const auto& item : context.perceptions) {
        if (!item.visible) continue;
        bool is_danger = itemMatchesDangerTag(item, context.profile.danger_tags);
        bool is_deterrent = hasAnyTag(item.tag_keys, context.profile.deterrent_tags);
        bool learned_risk = false;
        for (const auto& claim : context.learned_claims) {
            if (claimImpliesRiskForTarget(claim, item.target_key)) {
                learned_risk = true;
                break;
            }
        }
        if (is_danger || is_deterrent || learned_risk) {
            intent.kind = BeastActionIntentKind::Flee;
            intent.target_ref = item.target_ref;
            intent.target_key = item.target_key;
            if (item.coord.has_value()) {
                intent.target_coord = item.coord;
            }
            intent.command_kind = pathfinder::world_command::WorldCommandKind::Flee;
            intent.reason_kind = learned_risk ? BeastDecisionReasonKind::LearnedRisk : BeastDecisionReasonKind::PerceivedDanger;
            intent.risk_score = std::max(context.profile.base_fear, is_deterrent ? 60.0 : 80.0);
            intent.safe_summary_zh_cn = "perceived_danger_flee";
            return Result<BeastActionIntent>::ok(std::move(intent));
        }
    }

    // 2. Territory intrusion -> Threaten / Attack (simplified to Threaten for P52)
    if (context.profile.temperament == BeastTemperamentKind::Territorial) {
        for (const auto& item : context.perceptions) {
            if (item.kind == BeastPerceptionKind::Actor && item.visible && item.distance <= context.profile.vision_radius / 2) {
                intent.kind = BeastActionIntentKind::Threaten;
                intent.target_ref = item.target_ref;
                intent.target_key = item.target_key;
                intent.command_kind = pathfinder::world_command::WorldCommandKind::Attack; // Threaten maps to Attack command for now
                intent.reason_kind = BeastDecisionReasonKind::TerritoryIntrusion;
                intent.risk_score = context.profile.base_aggression;
                intent.safe_summary_zh_cn = "territory_intrusion_threaten";
                return Result<BeastActionIntent>::ok(std::move(intent));
            }
        }
    }

    // 3. Hunger / PerceivedPrey -> Approach / Attack
    if (!context.profile.prey_tags.empty()) {
        for (const auto& item : context.perceptions) {
            if (item.visible && itemMatchesPreyTag(item, context.profile.prey_tags)) {
                if (item.distance <= 1) {
                    intent.kind = BeastActionIntentKind::Attack;
                    intent.target_ref = item.target_ref;
                    intent.target_key = item.target_key;
                    intent.command_kind = pathfinder::world_command::WorldCommandKind::Attack;
                    intent.reason_kind = BeastDecisionReasonKind::PerceivedPrey;
                    intent.risk_score = context.profile.base_aggression;
                    intent.safe_summary_zh_cn = "attack_prey";
                } else {
                    intent.kind = BeastActionIntentKind::Approach;
                    intent.target_ref = item.target_ref;
                    intent.target_key = item.target_key;
                    if (item.coord.has_value()) {
                        intent.target_coord = item.coord;
                    }
                    intent.command_kind = pathfinder::world_command::WorldCommandKind::Move;
                    intent.reason_kind = BeastDecisionReasonKind::PerceivedPrey;
                    intent.risk_score = context.profile.base_aggression * 0.5;
                    intent.safe_summary_zh_cn = "approach_prey";
                }
                return Result<BeastActionIntent>::ok(std::move(intent));
            }
        }
    }

    // 4. Curiosity -> Observe / Approach
    if (context.profile.temperament == BeastTemperamentKind::Curious) {
        for (const auto& item : context.perceptions) {
            if (item.visible && item.kind == BeastPerceptionKind::Object && item.intensity > 50.0) {
                intent.kind = BeastActionIntentKind::Observe;
                intent.target_ref = item.target_ref;
                intent.target_key = item.target_key;
                intent.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
                intent.reason_kind = BeastDecisionReasonKind::InstinctNeed;
                intent.risk_score = 10.0;
                intent.safe_summary_zh_cn = "observe_curious_object";
                return Result<BeastActionIntent>::ok(std::move(intent));
            }
        }
    }

    // 5. No target -> Wait (P52 has no wander algorithm; safe fallback)
    intent.kind = BeastActionIntentKind::Wait;
    intent.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
    intent.reason_kind = BeastDecisionReasonKind::NoValidAction;
    intent.risk_score = 0.0;
    intent.safe_summary_zh_cn = "wait_no_target";
    return Result<BeastActionIntent>::ok(std::move(intent));
}

} // namespace pathfinder::world_beast_ecology
