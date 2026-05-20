#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/agent_reasoning/effect_semantics.h"
#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <memory>

namespace pathfinder::content {

// ============================================================
// ContentRuntimeAdapter - bridges ContentRegistry to existing systems
// ============================================================

class ContentRuntimeAdapter {
public:
    explicit ContentRuntimeAdapter(std::shared_ptr<const ContentRegistry> registry);

    // Build a DialogScenario from a scenario definition in the content registry
    pathfinder::foundation::Result<pathfinder::h5_dialog::DialogScenario> buildDialogScenario(
        const std::string& scenario_key) const;

    // Build DialogScenarioObjects from object definitions
    pathfinder::foundation::Result<pathfinder::h5_dialog::DialogScenarioObject> buildDialogObject(
        const ObjectDefinitionContent& obj_def) const;

    // Build DialogFeedbackTemplates from feedback definitions
    pathfinder::foundation::Result<pathfinder::h5_dialog::DialogFeedbackTemplate> buildDialogFeedback(
        const FeedbackDefinitionContent& fb_def) const;

    // Build effect execution specs from JSON effect definitions.
    pathfinder::foundation::Result<std::vector<pathfinder::agent_reasoning::EffectExecutionSpec>> buildEffectSpecs() const;

    // Build effect semantics from JSON effect definitions.
    pathfinder::foundation::Result<std::vector<pathfinder::agent_reasoning::EffectSemantics>> buildEffectSemantics() const;

    // Check if the registry has a given scenario
    bool hasScenario(const std::string& scenario_key) const;

    // Get the content registry
    const ContentRegistry& registry() const { return *registry_; }

private:
    std::shared_ptr<const ContentRegistry> registry_;

    pathfinder::h5_dialog::DialogActionKind mapActionKind(const std::string& action_key) const;
    pathfinder::h5_dialog::DialogObjectVisibility mapVisibility(ContentVisibility vis) const;
    pathfinder::foundation::Result<pathfinder::agent_reasoning::EffectExecutionSpec> buildEffectSpec(
        const EffectDefinitionContent& effect_def) const;
    pathfinder::foundation::Result<pathfinder::agent_reasoning::EffectSemantics> buildEffectSemantic(
        const EffectDefinitionContent& effect_def) const;
};

} // namespace pathfinder::content
