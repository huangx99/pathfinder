#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/v3_sandbox/v3_agent_decision.h"
#include "pathfinder/v3_sandbox/v3_sandbox_types.h"

#include <memory>

namespace pathfinder::v3_sandbox {

class V3SandboxRuntime {
public:
    static std::shared_ptr<const pathfinder::content::ContentRegistry> loadCoreContent(const std::string& content_root);
    static V3SandboxConfig loadConfigFile(const std::string& file_path);
    static V3SandboxRuntime createDefault(const std::string& content_root);

    V3SandboxRuntime(std::shared_ptr<const pathfinder::content::ContentRegistry> content,
                     V3SandboxConfig config);

    V3SandboxCommandResult submit(const V3SandboxCommand& command);
    V3SandboxSnapshot snapshot() const;

    const V3SandboxConfig& config() const { return config_; }
    size_t knowledgeRepositorySize() const { return knowledge_repository_.size(); }

private:
    struct CellState {
        std::string terrain_key;
        std::vector<std::string> object_instance_ids;
        std::vector<std::string> agent_ids;
    };

    struct ObjectState {
        std::string instance_id;
        std::string object_key;
        int quantity{1};
        V3Coord position;
    };

    struct KnowledgeClaimState {
        std::string subject_object_key;
        std::string action_key;
        std::string effect_key;
        int evidence_count{0};
        double utility_delta{0.0};
        double risk_delta{0.0};
    };

    struct AgentState {
        std::string agent_id;
        std::string name;
        V3Coord position;
        double hunger{35.0};
        double health{100.0};
        std::string current_intent{"刚被投放，正在观察附近。"};
        std::map<std::string, int> inventory;
        int inventory_capacity_slots{8};
        std::vector<KnowledgeClaimState> knowledge;
        std::vector<std::string> recent_memory;
        std::set<std::string> attempted_actions;
        int exploration_cursor{0};
    };

    struct FeedbackMatch {
        std::string object_key;
        std::string action_key;
        std::string effect_key;
        std::string reply_key;
        double utility_delta{0.0};
        double risk_delta{0.0};
        std::vector<std::string> outcome_signal_keys;
    };

    V3SandboxCommandResult paintTerrain(const V3SandboxCommand& command);
    V3SandboxCommandResult placeObject(const V3SandboxCommand& command);
    V3SandboxCommandResult placeAgent(const V3SandboxCommand& command);
    V3SandboxCommandResult advanceTicks(int count);
    V3SandboxCommandResult inspectAgent(const V3SandboxCommand& command) const;
    V3SandboxCommandResult inspectCell(const V3SandboxCommand& command) const;

    void advanceOneTick(std::vector<std::string>& events);
    void advanceAgent(AgentState& agent, std::vector<std::string>& events);
    V3DecisionContext buildDecisionContext(const AgentState& agent, const std::vector<std::string>& perceived_object_ids) const;
    bool executeDecision(AgentState& agent, const V3AgentDecision& decision, std::vector<std::string>& events);
    std::vector<std::string> perceivedObjectIds(const AgentState& agent) const;
    std::optional<FeedbackMatch> findFeedback(const std::string& object_key, const std::string& action_key) const;
    bool applyObjectAction(AgentState& agent, const std::string& object_id, const std::string& action_key, std::vector<std::string>& events);
    bool applyInventoryAction(AgentState& agent, const std::string& object_key, const std::string& action_key, std::vector<std::string>& events);
    bool pickupObject(AgentState& agent, const std::string& object_id, std::vector<std::string>& events);
    void learnNoVisibleEffect(AgentState& agent, const std::string& object_key, const std::string& action_key, std::vector<std::string>& events);
    void remember(AgentState& agent, const std::string& text);
    void learn(AgentState& agent, const FeedbackMatch& feedback, std::vector<std::string>& events);
    void mirrorKnowledgeClaim(const AgentState& agent, const KnowledgeClaimState& claim);
    void evaluateUnlocks(std::vector<std::string>& events);
    void moveAgentOneStep(AgentState& agent, V3Coord destination, std::vector<std::string>& events);
    void wander(AgentState& agent, std::vector<std::string>& events);

    bool inBounds(int x, int y) const;
    bool isWalkable(int x, int y) const;
    bool canPickupObject(const ObjectState& object) const;
    CellState& cellAt(int x, int y);
    const CellState& cellAt(int x, int y) const;
    std::string objectDisplayName(const std::string& object_key) const;
    std::string terrainDisplayName(const std::string& terrain_key) const;
    bool hasKnowledge(const AgentState& agent, const std::string& object_key, const std::string& action_key) const;

    std::shared_ptr<const pathfinder::content::ContentRegistry> content_;
    V3SandboxConfig config_;
    std::vector<CellState> cells_;
    std::map<std::string, ObjectState> objects_;
    std::map<std::string, AgentState> agents_;
    std::set<std::string> unlocked_terrains_;
    std::set<std::string> unlocked_objects_;
    std::set<std::string> unlocked_agents_;
    std::set<std::string> fired_unlock_rules_;
    pathfinder::knowledge::KnowledgeRepository knowledge_repository_;
    std::vector<std::string> event_log_;
    int tick_{0};
    int next_object_id_{1};
    int next_agent_id_{1};
};

} // namespace pathfinder::v3_sandbox
