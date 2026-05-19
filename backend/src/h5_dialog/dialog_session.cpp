#include "pathfinder/h5_dialog/dialog_session.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::EntityId;
using pathfinder::knowledge::KnowledgeOwner;
using pathfinder::knowledge::KnowledgeOwnerKind;
using pathfinder::memory::MemoryOwner;
using pathfinder::memory::MemoryOwnerKind;
using pathfinder::cognition::CognitionSubject;
using pathfinder::cognition::CognitionSubjectKind;

namespace {

DialogSessionState createNewSession(const std::string& session_id) {
    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    DialogScenario scenario = scenario_r.value();

    DialogSessionState state;
    state.session_id = session_id;
    state.scenario_key = scenario.scenario_key;
    state.turn_index = 0;
    state.current_tick = Tick(1);

    for (const auto& obj : scenario.objects) {
        if (obj.visibility == DialogObjectVisibility::Visible || obj.visibility == DialogObjectVisibility::Mentioned) {
            state.visible_object_keys.push_back(obj.object_key);
        }
        DialogObjectRuntimeState runtime;
        runtime.object_key = obj.object_key;
        runtime.tag_states = obj.safe_tags;
        if (obj.object_key == "axe") {
            runtime.numeric_states["sharpness"] = 3.0;
        }
        state.object_runtime_states[obj.object_key] = runtime;
    }

    // Actor: pioneer
    EntityId actor_id("pioneer_" + session_id);
    state.actor.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, actor_id, {}, {}, "pioneer"};
    state.actor.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, actor_id, {}, "pioneer"};
    state.actor.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, actor_id, std::nullopt};
    state.actor.display_key = "pioneer";

    // Recipient: companion
    EntityId recip_id("companion_" + session_id);
    state.recipient.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, recip_id, {}, {}, "companion"};
    state.recipient.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, recip_id, {}, "companion"};
    state.recipient.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, recip_id, std::nullopt};
    state.recipient.display_key = "companion";

    return state;
}

} // namespace

Result<DialogSessionState> InMemoryDialogSessionStore::loadOrCreate(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return Result<DialogSessionState>::ok(it->second);
    }
    auto state = createNewSession(session_id);
    sessions_[session_id] = state;
    return Result<DialogSessionState>::ok(state);
}

Result<void> InMemoryDialogSessionStore::save(const DialogSessionState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[state.session_id] = state;
    return Result<void>::ok();
}

Result<void> InMemoryDialogSessionStore::reset(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
    auto state = createNewSession(session_id);
    sessions_[session_id] = state;
    return Result<void>::ok();
}

} // namespace pathfinder::h5_dialog
