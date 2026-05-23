#include "pathfinder/content/content_validation.h"
#include "pathfinder/foundation/error.h"
#include <set>
#include <algorithm>

namespace pathfinder::content {

using pathfinder::config::ConfigValidationIssue;
using pathfinder::config::ConfigValidationSeverity;
using pathfinder::config::ConfigValidationReport;
using pathfinder::foundation::ErrorCode;

// ============================================================
// Helpers
// ============================================================

bool ContentStructuralValidator::isValidKey(const std::string& key) {
    return pathfinder::foundation::isValidIdString(key);
}

bool ContentStructuralValidator::isValidOperationOp(const std::string& op) {
    static const std::set<std::string> valid_ops = {
        "consume_object_quantity",
        "produce_object_quantity",
        "change_actor_need",
        "add_object_state_number",
        "set_object_state_number",
        "hint_object_use",
        "remove_threat",
        "instinct_fear_response"
    };
    return valid_ops.count(op) > 0;
}

template <typename Range, typename KeyFn>
void checkDuplicateKeys(
    const Range& values,
    KeyFn key_fn,
    const std::string& label,
    const std::string& json_path,
    ConfigValidationReport& report) {
    std::set<std::string> seen;
    for (const auto& value : values) {
        const auto key = key_fn(value);
        if (key.empty()) continue;
        if (!seen.insert(key).second) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::command_duplicate,
                "duplicate " + label + " key: " + key, "", json_path});
        }
    }
}

// ============================================================
// ContentStructuralValidator
// ============================================================

ConfigValidationReport ContentStructuralValidator::validate(const ContentDraftRegistry& draft) const {
    ConfigValidationReport report;

    checkDuplicateKeys(draft.objects(), [](const auto& item) { return item.key.value(); }, "object", "$.objects[*].object_key", report);
    checkDuplicateKeys(draft.actions(), [](const auto& item) { return item.key.value(); }, "action", "$.actions[*].action_key", report);
    checkDuplicateKeys(draft.effects(), [](const auto& item) { return item.key.value(); }, "effect", "$.effects[*].effect_key", report);
    checkDuplicateKeys(draft.feedbacks(), [](const auto& item) { return item.key.value(); }, "feedback", "$.feedbacks[*].feedback_key", report);
    checkDuplicateKeys(draft.reactions(), [](const auto& item) { return item.key.value(); }, "reaction", "$.reactions[*].reaction_key", report);
    checkDuplicateKeys(draft.agents(), [](const auto& item) { return item.key.value(); }, "agent", "$.agents[*].agent_key", report);
    checkDuplicateKeys(draft.threats(), [](const auto& item) { return item.key.value(); }, "threat", "$.threats[*].threat_key", report);
    checkDuplicateKeys(draft.knowledge_templates(), [](const auto& item) { return item.key.value(); }, "knowledge template", "$.knowledge_templates[*].knowledge_key", report);
    checkDuplicateKeys(draft.scenarios(), [](const auto& item) { return item.key.value(); }, "scenario", "$.scenario_key", report);
    checkDuplicateKeys(draft.worldgen_profiles(), [](const auto& item) { return item.profile_key; }, "worldgen profile", "$.worldgen_profiles[*].profile_key", report);

    for (const auto& obj : draft.objects()) {
        validateObject(obj, report);
    }
    for (const auto& effect : draft.effects()) {
        validateEffect(effect, report);
    }
    for (const auto& fb : draft.feedbacks()) {
        validateFeedback(fb, report);
    }
    for (const auto& reaction : draft.reactions()) {
        validateReaction(reaction, report);
    }
    for (const auto& agent : draft.agents()) {
        validateAgent(agent, report);
    }
    for (const auto& threat : draft.threats()) {
        validateThreat(threat, report);
    }
    for (const auto& kt : draft.knowledge_templates()) {
        validateKnowledgeTemplate(kt, report);
    }
    for (const auto& scenario : draft.scenarios()) {
        validateScenario(scenario, report);
    }
    for (const auto& profile : draft.worldgen_profiles()) {
        validateWorldgenProfile(profile, report);
    }

    return report;
}

void ContentStructuralValidator::validateObject(const ObjectDefinitionContent& obj, ConfigValidationReport& report) const {
    if (obj.key.empty() || !isValidKey(obj.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "object has invalid key", "", "$.objects[*].object_key"});
    }
    if (obj.display_key.empty()) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
            "object has empty display_key: " + obj.key.value(), "", "$.objects[*].display_key"});
    }
    if (obj.kind.empty()) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
            "object has empty kind: " + obj.key.value(), "", "$.objects[*].kind"});
    }
}

void ContentStructuralValidator::validateEffect(const EffectDefinitionContent& effect, ConfigValidationReport& report) const {
    if (effect.key.empty() || !isValidKey(effect.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "effect has invalid key", "", "$.effects[*].effect_key"});
    }
    for (const auto& op : effect.operations) {
        if (!isValidOperationOp(op.op)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_enum_unknown,
                "effect operation has unknown op: " + op.op + " in effect: " + effect.key.value(),
                "", "$.effects[*].operations[*].op"});
        }
    }
}

void ContentStructuralValidator::validateFeedback(const FeedbackDefinitionContent& fb, ConfigValidationReport& report) const {
    if (fb.key.empty() || !isValidKey(fb.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "feedback has invalid key", "", "$.feedbacks[*].feedback_key"});
    }
    if (fb.object_key.empty()) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
            "feedback has empty object_key: " + fb.key.value(), "", "$.feedbacks[*].object_key"});
    }
    if (fb.action_key.empty()) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
            "feedback has empty action_key: " + fb.key.value(), "", "$.feedbacks[*].action_key"});
    }
}

void ContentStructuralValidator::validateReaction(const ReactionDefinitionContent& reaction, ConfigValidationReport& report) const {
    if (reaction.key.empty() || !isValidKey(reaction.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "reaction has invalid key", "", "$.reactions[*].reaction_key"});
    }
    if (reaction.inputs.empty()) {
        report.addIssue({ConfigValidationSeverity::Warning, ErrorCode::validation_failed,
            "reaction has no inputs: " + reaction.key.value(), "", "$.reactions[*].inputs"});
    }
}

void ContentStructuralValidator::validateAgent(const AgentTemplateContent& agent, ConfigValidationReport& report) const {
    if (agent.key.empty() || !isValidKey(agent.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "agent has invalid key", "", "$.agents[*].agent_key"});
    }
}

void ContentStructuralValidator::validateThreat(const ThreatDefinitionContent& threat, ConfigValidationReport& report) const {
    if (threat.key.empty() || !isValidKey(threat.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "threat has invalid key", "", "$.threats[*].threat_key"});
    }
}

void ContentStructuralValidator::validateKnowledgeTemplate(const KnowledgeTemplateContent& kt, ConfigValidationReport& report) const {
    if (kt.key.empty() || !isValidKey(kt.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "knowledge template has invalid key", "", "$.knowledge_templates[*].knowledge_key"});
    }
}

void ContentStructuralValidator::validateScenario(const ScenarioDefinitionContent& scenario, ConfigValidationReport& report) const {
    if (scenario.key.empty() || !isValidKey(scenario.key.value())) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "scenario has invalid key", "", "$.scenario_key"});
    }
}

void ContentStructuralValidator::validateWorldgenProfile(const WorldgenProfileDefinitionContent& profile, ConfigValidationReport& report) const {
    if (profile.profile_key.empty() || !isValidKey(profile.profile_key)) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
            "worldgen profile has invalid key", "", "$.worldgen_profiles[*].profile_key"});
    }
    if (profile.region_size <= 0) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
            "worldgen profile region_size must be positive: " + profile.profile_key, "", "$.worldgen_profiles[*].region_size"});
    }
    if (profile.default_layer.empty()) {
        report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
            "worldgen profile default_layer is empty: " + profile.profile_key, "", "$.worldgen_profiles[*].default_layer"});
    }
    for (const auto& rule : profile.resource_rules) {
        if (rule.resource_key.empty() || !isValidKey(rule.resource_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
                "worldgen resource rule has invalid resource_key in profile: " + profile.profile_key, "", "$.worldgen_profiles[*].resource_rules[*].resource_key"});
        }
        if (rule.density < 0.0) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
                "worldgen resource rule density must be >= 0: " + rule.resource_key, "", "$.worldgen_profiles[*].resource_rules[*].density"});
        }
        if (rule.charges <= 0) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
                "worldgen resource rule charges must be positive: " + rule.resource_key, "", "$.worldgen_profiles[*].resource_rules[*].charges"});
        }
        if (rule.output_object_keys.empty()) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
                "worldgen resource rule has no output_object_keys: " + rule.resource_key, "", "$.worldgen_profiles[*].resource_rules[*].output_object_keys"});
        }
    }
    for (const auto& rule : profile.ground_item_rules) {
        if (rule.object_key.empty() || !isValidKey(rule.object_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_invalid_format,
                "worldgen ground item rule has invalid object_key in profile: " + profile.profile_key, "", "$.worldgen_profiles[*].ground_item_rules[*].object_key"});
        }
        if (rule.density < 0.0) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
                "worldgen ground item density must be >= 0: " + rule.object_key, "", "$.worldgen_profiles[*].ground_item_rules[*].density"});
        }
        if (rule.quantity < 0) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_failed,
                "worldgen ground item quantity must be >= 0: " + rule.object_key, "", "$.worldgen_profiles[*].ground_item_rules[*].quantity"});
        }
    }
}

// ============================================================
// ContentReferenceValidator
// ============================================================

ConfigValidationReport ContentReferenceValidator::validate(const ContentDraftRegistry& draft) const {
    ConfigValidationReport report;
    validateObjectReferences(draft, report);
    validateFeedbackReferences(draft, report);
    validateReactionReferences(draft, report);
    validateThreatReferences(draft, report);
    validateScenarioReferences(draft, report);
    validateKnowledgeReferences(draft, report);
    validateWorldgenReferences(draft, report);
    return report;
}

void ContentReferenceValidator::validateObjectReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    // Build set of valid object keys
    std::set<std::string> object_keys;
    for (const auto& obj : draft.objects()) {
        object_keys.insert(obj.key.value());
    }

    // Check that each object's allowed_actions reference valid actions if we have actions registered
    // (skip if no actions loaded - actions are optional in first version)
}

void ContentReferenceValidator::validateFeedbackReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    std::set<std::string> object_keys;
    for (const auto& obj : draft.objects()) object_keys.insert(obj.key.value());
    std::set<std::string> effect_keys;
    for (const auto& eff : draft.effects()) effect_keys.insert(eff.key.value());

    for (const auto& fb : draft.feedbacks()) {
        if (!fb.object_key.empty() && !object_keys.count(fb.object_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "feedback references unknown object: " + fb.object_key + " in " + fb.key.value(),
                "", "$.feedbacks[*].object_key"});
        }
        if (!fb.target_object_key.empty() && !object_keys.count(fb.target_object_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "feedback references unknown target_object: " + fb.target_object_key + " in " + fb.key.value(),
                "", "$.feedbacks[*].target_object_key"});
        }
        if (!fb.effect_key.empty() && !effect_keys.count(fb.effect_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "feedback references unknown effect: " + fb.effect_key + " in " + fb.key.value(),
                "", "$.feedbacks[*].effect_key"});
        }
    }
}

void ContentReferenceValidator::validateReactionReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    std::set<std::string> object_keys;
    for (const auto& obj : draft.objects()) object_keys.insert(obj.key.value());
    std::set<std::string> effect_keys;
    for (const auto& eff : draft.effects()) effect_keys.insert(eff.key.value());

    for (const auto& reaction : draft.reactions()) {
        for (const auto& input : reaction.inputs) {
            if (!object_keys.count(input.object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "reaction input references unknown object: " + input.object_key + " in " + reaction.key.value(),
                    "", "$.reactions[*].inputs[*].object_key"});
            }
        }
        for (const auto& output : reaction.outputs) {
            if (!object_keys.count(output.object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "reaction output references unknown object: " + output.object_key + " in " + reaction.key.value(),
                    "", "$.reactions[*].outputs[*].object_key"});
            }
        }
        for (const auto& consume : reaction.consume) {
            if (!object_keys.count(consume.object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "reaction consume references unknown object: " + consume.object_key + " in " + reaction.key.value(),
                    "", "$.reactions[*].consume[*].object_key"});
            }
        }
        if (!reaction.effect_key.empty() && !effect_keys.count(reaction.effect_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "reaction references unknown effect: " + reaction.effect_key + " in " + reaction.key.value(),
                "", "$.reactions[*].effect_key"});
        }
    }
}

void ContentReferenceValidator::validateThreatReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    std::set<std::string> agent_keys;
    for (const auto& agent : draft.agents()) agent_keys.insert(agent.key.value());
    std::set<std::string> effect_keys;
    for (const auto& eff : draft.effects()) effect_keys.insert(eff.key.value());

    for (const auto& threat : draft.threats()) {
        if (!threat.agent_key.empty() && !agent_keys.count(threat.agent_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "threat references unknown agent: " + threat.agent_key + " in " + threat.key.value(),
                "", "$.threats[*].agent_key"});
        }
        for (const auto& cm : threat.countermeasures) {
            if (!cm.effect_key.empty() && !effect_keys.count(cm.effect_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "threat countermeasure references unknown effect: " + cm.effect_key + " in " + threat.key.value(),
                    "", "$.threats[*].countermeasures[*].effect_key"});
            }
        }
    }
}

void ContentReferenceValidator::validateScenarioReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    std::set<std::string> object_keys;
    for (const auto& obj : draft.objects()) object_keys.insert(obj.key.value());
    std::set<std::string> agent_keys;
    for (const auto& agent : draft.agents()) agent_keys.insert(agent.key.value());
    std::set<std::string> threat_keys;
    for (const auto& threat : draft.threats()) threat_keys.insert(threat.key.value());

    std::set<std::string> effect_keys;
    for (const auto& effect : draft.effects()) effect_keys.insert(effect.key.value());
    std::set<std::string> knowledge_keys;
    for (const auto& kt : draft.knowledge_templates()) knowledge_keys.insert(kt.key.value());

    for (const auto& scenario : draft.scenarios()) {
        for (const auto& obj_init : scenario.initial_objects) {
            if (!object_keys.count(obj_init.object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario initial object references unknown: " + obj_init.object_key + " in " + scenario.key.value(),
                    "", "$.initial_objects[*].object_key"});
            }
        }
        for (const auto& agent_init : scenario.initial_agents) {
            if (!agent_keys.count(agent_init.agent_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario initial agent references unknown: " + agent_init.agent_key + " in " + scenario.key.value(),
                    "", "$.initial_agents[*].agent_key"});
            }
        }
        for (const auto& threat_init : scenario.initial_threats) {
            if (!threat_keys.count(threat_init.threat_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario initial threat references unknown: " + threat_init.threat_key + " in " + scenario.key.value(),
                    "", "$.initial_threats[*].threat_key"});
            }
        }
        for (const auto& act_tmpl : scenario.suggested_action_templates) {
            if (!act_tmpl.object_key.empty() && !object_keys.count(act_tmpl.object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario action template object references unknown: " + act_tmpl.object_key + " in " + scenario.key.value(),
                    "", "$.suggested_action_templates[*].object_key"});
            }
            if (!act_tmpl.required_effect_key.empty() && !effect_keys.count(act_tmpl.required_effect_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario action template effect references unknown: " + act_tmpl.required_effect_key + " in " + scenario.key.value(),
                    "", "$.suggested_action_templates[*].required_effect_key"});
            }
        }
        for (const auto& thr_tmpl : scenario.threat_knowledge_templates) {
            if (!thr_tmpl.threat_object_key.empty() && !object_keys.count(thr_tmpl.threat_object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario threat template object references unknown: " + thr_tmpl.threat_object_key + " in " + scenario.key.value(),
                    "", "$.threat_knowledge_templates[*].threat_object_key"});
            }
            if (!thr_tmpl.required_effect_key.empty() && !effect_keys.count(thr_tmpl.required_effect_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "scenario threat template effect references unknown: " + thr_tmpl.required_effect_key + " in " + scenario.key.value(),
                    "", "$.threat_knowledge_templates[*].required_effect_key"});
            }
        }
        for (const auto& kt_key : scenario.default_player_knowledge) {
            if (!knowledge_keys.count(kt_key)) {
                report.addIssue({ConfigValidationSeverity::Warning, ErrorCode::id_not_found,
                    "scenario default player knowledge references unknown: " + kt_key + " in " + scenario.key.value(),
                    "", "$.default_player_knowledge[*]"});
            }
        }
        for (const auto& kt_key : scenario.default_agent_knowledge) {
            if (!knowledge_keys.count(kt_key)) {
                report.addIssue({ConfigValidationSeverity::Warning, ErrorCode::id_not_found,
                    "scenario default agent knowledge references unknown: " + kt_key + " in " + scenario.key.value(),
                    "", "$.default_agent_knowledge[*]"});
            }
        }
    }
}

void ContentReferenceValidator::validateKnowledgeReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    std::set<std::string> object_keys;
    for (const auto& obj : draft.objects()) object_keys.insert(obj.key.value());
    std::set<std::string> effect_keys;
    for (const auto& eff : draft.effects()) effect_keys.insert(eff.key.value());

    for (const auto& kt : draft.knowledge_templates()) {
        if (!kt.subject_object_key.empty() && !object_keys.count(kt.subject_object_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "knowledge template subject references unknown object: " + kt.subject_object_key + " in " + kt.key.value(),
                "", "$.knowledge_templates[*].subject_object_key"});
        }
        if (!kt.effect_key.empty() && !effect_keys.count(kt.effect_key)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                "knowledge template effect references unknown: " + kt.effect_key + " in " + kt.key.value(),
                "", "$.knowledge_templates[*].effect_key"});
        }
    }
}

void ContentReferenceValidator::validateWorldgenReferences(const ContentDraftRegistry& draft, ConfigValidationReport& report) const {
    std::set<std::string> object_keys;
    for (const auto& obj : draft.objects()) object_keys.insert(obj.key.value());

    for (const auto& profile : draft.worldgen_profiles()) {
        for (const auto& rule : profile.resource_rules) {
            if (!rule.required_tool_key.empty() && !object_keys.count(rule.required_tool_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "worldgen resource rule references unknown required tool: " + rule.required_tool_key + " in " + profile.profile_key,
                    "", "$.worldgen_profiles[*].resource_rules[*].required_tool_key"});
            }
            for (const auto& object_key : rule.output_object_keys) {
                if (!object_keys.count(object_key)) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                        "worldgen resource rule output references unknown object: " + object_key + " in " + profile.profile_key,
                        "", "$.worldgen_profiles[*].resource_rules[*].output_object_keys[*]"});
                }
            }
        }

        for (const auto& rule : profile.ground_item_rules) {
            if (!object_keys.count(rule.object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "worldgen ground item references unknown object: " + rule.object_key + " in " + profile.profile_key,
                    "", "$.worldgen_profiles[*].ground_item_rules[*].object_key"});
            }
        }

        for (const auto& rule : profile.drop_rules) {
            if (!rule.source_object_key.empty() && !object_keys.count(rule.source_object_key)) {
                report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                    "worldgen drop rule source references unknown object: " + rule.source_object_key + " in " + profile.profile_key,
                    "", "$.worldgen_profiles[*].drop_rules[*].source_object_key"});
            }
            for (const auto& object_key : rule.output_object_keys) {
                if (!object_keys.count(object_key)) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::id_not_found,
                        "worldgen drop rule output references unknown object: " + object_key + " in " + profile.profile_key,
                        "", "$.worldgen_profiles[*].drop_rules[*].output_object_keys[*]"});
                }
            }
        }
    }
}

} // namespace pathfinder::content
