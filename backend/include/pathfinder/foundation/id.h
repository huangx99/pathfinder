#pragma once

#include <string>
#include <functional>

namespace pathfinder::foundation {

// Validate ID string format
// Rules:
// - Empty string is invalid
// - Contains space, tab, newline is invalid
// - Only allows lowercase letters, digits, underscore, hyphen
bool isValidIdString(const std::string& value);

// StrongId<Tag> provides type-safe ID wrapping
template <typename Tag>
class StrongId {
public:
    // Default construct an empty ID
    StrongId() = default;

    // Construct from string (does not validate)
    explicit StrongId(std::string value) : value_(std::move(value)) {}

    // Get the underlying string value
    const std::string& value() const { return value_; }

    // Check if the ID is empty
    bool empty() const { return value_.empty(); }

    // Comparison operators
    bool operator==(const StrongId& other) const { return value_ == other.value_; }
    bool operator!=(const StrongId& other) const { return value_ != other.value_; }
    bool operator<(const StrongId& other) const { return value_ < other.value_; }

private:
    std::string value_;
};

// Tag types for different ID semantics
struct GameIdTag {};
struct PlayerIdTag {};
struct TribeIdTag {};
struct EntityIdTag {};
struct ObjectIdTag {};
struct RegionIdTag {};
struct CommandIdTag {};
struct EventIdTag {};
struct MemoryIdTag {};
struct KnowledgeIdTag {};
struct ObjectDefinitionIdTag {};
struct TraitIdTag {};
struct ActionIdTag {};
struct CapabilityIdTag {};
struct RegionDefinitionIdTag {};
struct EffectIdTag {};
struct ExpressionIdTag {};
struct StateChangeIdTag {};
struct ConfigPackIdTag {};
struct ConfigVersionIdTag {};
struct EpisodeIdTag {};
struct DecisionIdTag {};
struct TargetIdTag {};

// Type aliases for P0 common IDs
using GameId = StrongId<GameIdTag>;
using PlayerId = StrongId<PlayerIdTag>;
using TribeId = StrongId<TribeIdTag>;
using EntityId = StrongId<EntityIdTag>;
using ObjectId = StrongId<ObjectIdTag>;
using RegionId = StrongId<RegionIdTag>;
using CommandId = StrongId<CommandIdTag>;
using EventId = StrongId<EventIdTag>;
using MemoryId = StrongId<MemoryIdTag>;
using KnowledgeId = StrongId<KnowledgeIdTag>;
using ObjectDefinitionId = StrongId<ObjectDefinitionIdTag>;
using TraitId = StrongId<TraitIdTag>;
using ActionId = StrongId<ActionIdTag>;
using CapabilityId = StrongId<CapabilityIdTag>;
using RegionDefinitionId = StrongId<RegionDefinitionIdTag>;
using EffectId = StrongId<EffectIdTag>;
using ExpressionId = StrongId<ExpressionIdTag>;
using StateChangeId = StrongId<StateChangeIdTag>;
using ConfigPackId = StrongId<ConfigPackIdTag>;
using ConfigVersionId = StrongId<ConfigVersionIdTag>;
using EpisodeId = StrongId<EpisodeIdTag>;
using DecisionId = StrongId<DecisionIdTag>;
using TargetId = StrongId<TargetIdTag>;

} // namespace pathfinder::foundation

// Hash specialization for StrongId
namespace std {
template <typename Tag>
struct hash<pathfinder::foundation::StrongId<Tag>> {
    size_t operator()(const pathfinder::foundation::StrongId<Tag>& id) const {
        return std::hash<std::string>{}(id.value());
    }
};
} // namespace std
