#include "pathfinder/world_beast_ecology/beast_decision_policy.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <array>
#include <cmath>

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


static std::optional<pathfinder::world_runtime::WorldCellCoord> oneStepToward(
    const std::optional<pathfinder::world_runtime::WorldCellCoord>& from,
    const std::optional<pathfinder::world_runtime::WorldCellCoord>& to) {
    if (!from.has_value() || !to.has_value() || from->layer_key != to->layer_key) return std::nullopt;
    int dx = (to->x > from->x) ? 1 : (to->x < from->x ? -1 : 0);
    int dy = (to->y > from->y) ? 1 : (to->y < from->y ? -1 : 0);
    if (std::abs(to->x - from->x) >= std::abs(to->y - from->y) && dx != 0) {
        return pathfinder::world_runtime::WorldCellCoord{from->x + dx, from->y, from->layer_key};
    }
    if (dy != 0) {
        return pathfinder::world_runtime::WorldCellCoord{from->x, from->y + dy, from->layer_key};
    }
    if (dx != 0) {
        return pathfinder::world_runtime::WorldCellCoord{from->x + dx, from->y, from->layer_key};
    }
    return std::nullopt;
}

static std::optional<pathfinder::world_runtime::WorldCellCoord> oneStepAway(
    const std::optional<pathfinder::world_runtime::WorldCellCoord>& from,
    const std::optional<pathfinder::world_runtime::WorldCellCoord>& threat) {
    if (!from.has_value() || !threat.has_value() || from->layer_key != threat->layer_key) return std::nullopt;
    int dx = (from->x > threat->x) ? 1 : (from->x < threat->x ? -1 : 0);
    int dy = (from->y > threat->y) ? 1 : (from->y < threat->y ? -1 : 0);
    if (std::abs(from->x - threat->x) >= std::abs(from->y - threat->y) && dx != 0) {
        return pathfinder::world_runtime::WorldCellCoord{from->x + dx, from->y, from->layer_key};
    }
    if (dy != 0) {
        return pathfinder::world_runtime::WorldCellCoord{from->x, from->y + dy, from->layer_key};
    }
    if (dx != 0) {
        return pathfinder::world_runtime::WorldCellCoord{from->x + dx, from->y, from->layer_key};
    }
    return std::nullopt;
}

static int manhattan(
    const pathfinder::world_runtime::WorldCellCoord& a,
    const pathfinder::world_runtime::WorldCellCoord& b) {
    if (a.layer_key != b.layer_key) return 1000000;
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

static std::optional<pathfinder::world_runtime::WorldCellCoord> wanderStep(
    const BeastDecisionPolicy::PolicyContext& context,
    const std::optional<pathfinder::world_runtime::WorldCellCoord>& avoid_coord = std::nullopt) {
    if (!context.actor_coord.has_value()) return std::nullopt;

    const auto from = context.actor_coord.value();
    // Runtime movement is cardinal-only; choosing diagonal targets would compile
    // valid-looking Move commands that the authoritative runtime must block.
    constexpr std::array<std::pair<int, int>, 4> steps{{
        {1, 0}, {0, 1}, {-1, 0}, {0, -1},
    }};

    const auto seed = static_cast<size_t>(context.tick) +
        static_cast<size_t>(std::abs(from.x) * 17 + std::abs(from.y) * 31 + context.actor_key.size() * 13);

    std::optional<pathfinder::world_runtime::WorldCellCoord> fallback;
    for (size_t offset = 0; offset < steps.size(); ++offset) {
        const auto& step = steps[(seed + offset) % steps.size()];
        pathfinder::world_runtime::WorldCellCoord candidate{from.x + step.first, from.y + step.second, from.layer_key};
        if (!fallback.has_value()) fallback = candidate;
        if (!avoid_coord.has_value() || manhattan(candidate, avoid_coord.value()) >= manhattan(from, avoid_coord.value())) {
            return candidate;
        }
    }
    return fallback;
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

    // 1. Evaluate learned risks + perceived dangers.
    // Close danger causes an immediate Flee. Visible but non-adjacent deterrents
    // suppress hunting and make the beast roam around instead of unrealistically
    // beelining back into the deterrent every tick.
    std::optional<BeastPerceptionItem> visible_deterrent;
    for (const auto& item : context.perceptions) {
        if (!item.visible) continue;
        const bool matches_danger_tag = itemMatchesDangerTag(item, context.profile.danger_tags);
        const bool matches_deterrent_tag = hasAnyTag(item.tag_keys, context.profile.deterrent_tags);
        bool learned_risk = false;
        for (const auto& claim : context.learned_claims) {
            if (claimImpliesRiskForTarget(claim, item.target_key)) {
                learned_risk = true;
                break;
            }
        }

        if ((matches_danger_tag || matches_deterrent_tag || learned_risk) && !visible_deterrent.has_value()) {
            visible_deterrent = item;
        }

        const bool close_danger = (matches_danger_tag || matches_deterrent_tag || learned_risk) && item.distance <= 2;
        if (close_danger) {
            intent.kind = BeastActionIntentKind::Flee;
            intent.target_ref = item.target_ref;
            intent.target_key = item.target_key;
            intent.target_coord = oneStepAway(context.actor_coord, item.coord);
            if (!intent.target_coord.has_value() && item.coord.has_value()) {
                intent.target_coord = item.coord;
            }
            intent.command_kind = pathfinder::world_command::WorldCommandKind::Move;
            intent.reason_kind = learned_risk ? BeastDecisionReasonKind::LearnedRisk : BeastDecisionReasonKind::PerceivedDanger;
            intent.risk_score = std::max(context.profile.base_fear, matches_deterrent_tag ? 60.0 : 80.0);
            intent.safe_summary_zh_cn = "perceived_danger_flee";
            return Result<BeastActionIntent>::ok(std::move(intent));
        }
    }

    if (visible_deterrent.has_value()) {
        const int flee_radius = std::max(2, std::min(5, context.profile.vision_radius));
        intent.target_ref = visible_deterrent->target_ref;
        intent.target_key = visible_deterrent->target_key;
        intent.reason_kind = BeastDecisionReasonKind::PerceivedDanger;
        if (visible_deterrent->distance <= flee_radius) {
            intent.kind = BeastActionIntentKind::Flee;
            intent.target_coord = oneStepAway(context.actor_coord, visible_deterrent->coord);
            if (!intent.target_coord.has_value() && visible_deterrent->coord.has_value()) {
                intent.target_coord = visible_deterrent->coord;
            }
            intent.risk_score = std::max(20.0, context.profile.base_fear);
            intent.safe_summary_zh_cn = "deterrent_visible_flee";
        } else {
            intent.kind = BeastActionIntentKind::Wander;
            intent.target_coord = wanderStep(context, visible_deterrent->coord);
            intent.risk_score = std::max(5.0, context.profile.base_fear * 0.5);
            intent.safe_summary_zh_cn = "deterrent_visible_wander";
        }
        intent.command_kind = intent.target_coord.has_value()
            ? pathfinder::world_command::WorldCommandKind::Move
            : pathfinder::world_command::WorldCommandKind::Wait;
        return Result<BeastActionIntent>::ok(std::move(intent));
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
                    intent.attack_damage = context.profile.attack_damage;
                    intent.reason_kind = BeastDecisionReasonKind::PerceivedPrey;
                    intent.risk_score = context.profile.base_aggression;
                    intent.safe_summary_zh_cn = "attack_prey";
                } else {
                    intent.kind = BeastActionIntentKind::Approach;
                    intent.target_ref = item.target_ref;
                    intent.target_key = item.target_key;
                    intent.target_coord = oneStepToward(context.actor_coord, item.coord);
                    if (!intent.target_coord.has_value() && item.coord.has_value()) {
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

    // 5. No target -> deterministic roaming when a coordinate is available.
    intent.kind = context.actor_coord.has_value() ? BeastActionIntentKind::Wander : BeastActionIntentKind::Wait;
    intent.target_coord = wanderStep(context);
    intent.command_kind = intent.target_coord.has_value()
        ? pathfinder::world_command::WorldCommandKind::Move
        : pathfinder::world_command::WorldCommandKind::Wait;
    intent.reason_kind = BeastDecisionReasonKind::NoValidAction;
    intent.risk_score = 0.0;
    intent.safe_summary_zh_cn = intent.target_coord.has_value() ? "wander_no_target" : "wait_no_target";
    return Result<BeastActionIntent>::ok(std::move(intent));
}

} // namespace pathfinder::world_beast_ecology
