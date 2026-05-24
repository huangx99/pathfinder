#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/goal_execution/goal_execution_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_beast_ecology {

// ---------------------------------------------------------------------------
// 7. Core Enums
// ---------------------------------------------------------------------------

enum class BeastTemperamentKind {
    Unknown,
    Passive,
    Skittish,
    Curious,
    Territorial,
    Predatory,
    Pack,
    Constructed,
    TestOnly
};

std::string toString(BeastTemperamentKind kind);
pathfinder::foundation::Result<BeastTemperamentKind> beastTemperamentKindFromString(const std::string& str);

enum class BeastNeedKind {
    Unknown,
    Hunger,
    Fear,
    Territory,
    Curiosity,
    Pain,
    Rest,
    ProtectYoung,
    TestOnly
};

std::string toString(BeastNeedKind kind);
pathfinder::foundation::Result<BeastNeedKind> beastNeedKindFromString(const std::string& str);

enum class BeastPerceptionKind {
    Unknown,
    Actor,
    Object,
    Resource,
    Effect,
    Sound,
    Area,
    MemoryCue,
    TestOnly
};

std::string toString(BeastPerceptionKind kind);
pathfinder::foundation::Result<BeastPerceptionKind> beastPerceptionKindFromString(const std::string& str);

enum class BeastActionIntentKind {
    Unknown,
    Wait,
    Wander,
    Approach,
    Flee,
    Attack,
    Threaten,
    Observe,
    AvoidArea,
    CallPack,
    TestOnly
};

std::string toString(BeastActionIntentKind kind);
pathfinder::foundation::Result<BeastActionIntentKind> beastActionIntentKindFromString(const std::string& str);

enum class BeastDecisionReasonKind {
    Unknown,
    InstinctNeed,
    PerceivedPrey,
    PerceivedDanger,
    LearnedRisk,
    TerritoryIntrusion,
    CommandBlocked,
    NoValidAction,
    TestOnly
};

std::string toString(BeastDecisionReasonKind kind);
pathfinder::foundation::Result<BeastDecisionReasonKind> beastDecisionReasonKindFromString(const std::string& str);

// ---------------------------------------------------------------------------
// 8. Core DTOs / Data Structures
// ---------------------------------------------------------------------------

struct BeastInstinctCapability {
    std::string capability_key;
    BeastActionIntentKind action_kind = BeastActionIntentKind::Unknown;
    pathfinder::world_command::WorldCommandKind command_kind = pathfinder::world_command::WorldCommandKind::Unknown;
    bool requires_target = false;
    double risk_limit = 100.0;
    uint64_t cooldown_ticks = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct BeastAgentProfile {
    std::string profile_key;
    std::string display_key;
    BeastTemperamentKind temperament = BeastTemperamentKind::Unknown;
    int vision_radius = 0;
    int hearing_radius = 0;
    double base_aggression = 0.0;
    double base_fear = 0.0;
    int attack_damage = 1;
    std::vector<std::string> prey_tags;
    std::vector<std::string> danger_tags;
    std::vector<std::string> deterrent_tags;
    std::vector<BeastInstinctCapability> instinct_capabilities;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct BeastPerceptionItem {
    std::string perception_id;
    BeastPerceptionKind kind = BeastPerceptionKind::Unknown;
    std::string target_ref;
    std::string target_key;
    std::optional<pathfinder::world_runtime::WorldCellCoord> coord;
    int distance = 0;
    std::vector<std::string> tag_keys;
    double intensity = 0.0;
    bool visible = true;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct BeastTickRequest {
    std::string request_id;
    std::string actor_key;
    uint64_t tick = 0;
    bool allow_issue_command = true;
    bool allow_learning_claims = true;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct BeastActionIntent {
    std::string intent_id;
    std::string actor_key;
    BeastActionIntentKind kind = BeastActionIntentKind::Unknown;
    std::string target_ref;
    std::string target_key;
    std::optional<pathfinder::world_runtime::WorldCellCoord> target_coord;
    pathfinder::world_command::WorldCommandKind command_kind = pathfinder::world_command::WorldCommandKind::Unknown;
    BeastDecisionReasonKind reason_kind = BeastDecisionReasonKind::Unknown;
    double risk_score = 0.0;
    int attack_damage = 1;
    std::string safe_summary_zh_cn;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct BeastTickResult {
    bool ok = false;
    std::string actor_key;
    std::optional<BeastActionIntent> selected_intent;
    std::optional<pathfinder::world_command::WorldCommandDto> issued_command;
    std::optional<pathfinder::world_command::WorldCommandExecutionResult> command_result;
    std::vector<pathfinder::goal_execution::ExternalInterruptSignal> interrupts;
    std::vector<pathfinder::world_command::WorldEventDto> events;
    std::vector<pathfinder::world_command::WorldStateDeltaDto> state_deltas;
    std::optional<pathfinder::world_command::WorldProjectionPatchDto> projection_patch;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 9. Port Interfaces
// ---------------------------------------------------------------------------

class IBeastProfileQueryPort {
public:
    virtual ~IBeastProfileQueryPort() = default;
    virtual pathfinder::foundation::Result<BeastAgentProfile> profileForActor(const std::string& actor_key) const = 0;
};

class IBeastWorldQueryPort {
public:
    virtual ~IBeastWorldQueryPort() = default;
    virtual pathfinder::foundation::Result<pathfinder::world_runtime::WorldActorRuntime> findBeastActor(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldActorRuntime>> nearbyActorsForBeast(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldEntityInstance>> nearbyEntitiesForBeast(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldResourceNodeRuntime>> nearbyResourcesForBeast(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<std::vector<BeastPerceptionItem>> nearbyEffectsForBeast(const std::string& actor_key) const = 0;
};

class IBeastKnowledgeQueryPort {
public:
    virtual ~IBeastKnowledgeQueryPort() = default;
    virtual pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>> claimsForBeast(const std::string& actor_key) const = 0;
};

class IBeastCommandPipelinePort {
public:
    virtual ~IBeastCommandPipelinePort() = default;
    virtual pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> executeBeastCommand(const pathfinder::world_command::WorldCommandDto& command) = 0;
};

} // namespace pathfinder::world_beast_ecology
