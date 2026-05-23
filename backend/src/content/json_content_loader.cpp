#include "pathfinder/content/json_content_loader.h"
#include "pathfinder/content/content_validation.h"
#include "pathfinder/foundation/error.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <filesystem>

namespace pathfinder::content {

using pathfinder::config::ConfigValidationIssue;
using pathfinder::config::ConfigValidationSeverity;
using pathfinder::config::ConfigValidationReport;
using pathfinder::foundation::ErrorCode;

// ============================================================
// Path safety
// ============================================================

bool JsonContentLoader::isPathSafe(const std::string& relative_path) {
    if (relative_path.empty()) return false;
    if (relative_path.find("..") != std::string::npos) return false;
    if (relative_path[0] == '/' || relative_path[0] == '\\') return false;
    // Check for backslash traversal on Windows-like paths
    if (relative_path.find("\\") != std::string::npos) return false;
    return true;
}

// ============================================================
// File reading
// ============================================================

pathfinder::foundation::Result<std::string> JsonContentLoader::readFileSafe(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return pathfinder::foundation::Result<std::string>::fail(
            pathfinder::foundation::makeError(ErrorCode::storage_read_failed,
                "Failed to open file: " + file_path));
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return pathfinder::foundation::Result<std::string>::ok(buffer.str());
}

// ============================================================
// JsonContentParser - JSON parsing helpers
// ============================================================

std::string JsonContentParser::safeGetString(const nlohmann::json& j, const std::string& key, const std::string& default_val) {
    if (!j.contains(key)) return default_val;
    const auto& v = j[key];
    if (v.is_string()) return v.get<std::string>();
    return default_val;
}

int JsonContentParser::safeGetInt(const nlohmann::json& j, const std::string& key, int default_val) {
    if (!j.contains(key)) return default_val;
    const auto& v = j[key];
    if (v.is_number_integer()) return v.get<int>();
    return default_val;
}

double JsonContentParser::safeGetDouble(const nlohmann::json& j, const std::string& key, double default_val) {
    if (!j.contains(key)) return default_val;
    const auto& v = j[key];
    if (v.is_number()) return v.get<double>();
    return default_val;
}

bool JsonContentParser::safeGetBool(const nlohmann::json& j, const std::string& key, bool default_val) {
    if (!j.contains(key)) return default_val;
    const auto& v = j[key];
    if (v.is_boolean()) return v.get<bool>();
    return default_val;
}

std::vector<std::string> JsonContentParser::safeGetStringArray(const nlohmann::json& j, const std::string& key) {
    std::vector<std::string> result;
    if (!j.contains(key)) return result;
    const auto& v = j[key];
    if (v.is_array()) {
        for (const auto& item : v) {
            if (item.is_string()) result.push_back(item.get<std::string>());
        }
    }
    return result;
}

std::map<std::string, double> JsonContentParser::safeGetStringDoubleMap(const nlohmann::json& j, const std::string& key) {
    std::map<std::string, double> result;
    if (!j.contains(key)) return result;
    const auto& v = j[key];
    if (v.is_object()) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (it.value().is_number()) {
                result[it.key()] = it.value().get<double>();
            }
        }
    }
    return result;
}

// ============================================================
// JsonContentParser - Manifest parsing
// ============================================================

pathfinder::foundation::Result<ContentManifestDto> JsonContentParser::parseManifest(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        ContentManifestDto manifest;
        manifest.package_key = safeGetString(j, "package_key");
        manifest.display_name = safeGetString(j, "display_name");
        manifest.content_version = safeGetString(j, "content_version");
        manifest.schema_version = safeGetString(j, "schema_version");
        manifest.locale_default = safeGetString(j, "locale_default", "zh_cn");
        manifest.load_order = safeGetInt(j, "load_order", 0);

        if (j.contains("files") && j["files"].is_array()) {
            for (const auto& f : j["files"]) {
                ContentFileRefDto ref;
                ref.path = safeGetString(f, "path");
                ref.required = safeGetBool(f, "required", true);
                auto cat_str = safeGetString(f, "category");
                auto cat_result = contentFileCategoryFromString(cat_str);
                if (cat_result.is_ok()) {
                    ref.category = cat_result.value();
                }
                manifest.files.push_back(std::move(ref));
            }
        }
        return pathfinder::foundation::Result<ContentManifestDto>::ok(std::move(manifest));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<ContentManifestDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in manifest: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Objects parsing
// ============================================================

pathfinder::foundation::Result<ObjectDefinitionFileDto> JsonContentParser::parseObjects(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        ObjectDefinitionFileDto file_dto;

        if (j.contains("objects") && j["objects"].is_array()) {
            for (const auto& obj : j["objects"]) {
                ObjectDefinitionDto dto;
                dto.object_key = safeGetString(obj, "object_key");
                dto.display_key = safeGetString(obj, "display_key");
                dto.description_key = safeGetString(obj, "description_key");
                dto.kind = safeGetString(obj, "kind");

                auto vis_str = safeGetString(obj, "visibility", "runtime_visible");
                auto vis_result = contentVisibilityFromString(vis_str);
                if (vis_result.is_ok()) dto.visibility = vis_result.value();

                dto.safe_tags = safeGetStringArray(obj, "safe_tags");
                dto.allowed_actions = safeGetStringArray(obj, "allowed_actions");

                if (obj.contains("initial_state")) {
                    const auto& init = obj["initial_state"];
                    dto.initial_state.quantity = safeGetInt(init, "quantity", 0);
                    dto.initial_state.numeric = safeGetStringDoubleMap(init, "numeric");
                    dto.initial_state.tags = safeGetStringArray(init, "tags");
                }

                dto.knowledge_hints = safeGetStringArray(obj, "knowledge_hints");

                if (obj.contains("projection")) {
                    const auto& proj = obj["projection"];
                    dto.projection.safe_trait_keys = safeGetStringArray(proj, "safe_trait_keys");
                    dto.projection.unknown_use_text_key = safeGetString(proj, "unknown_use_text_key");
                }

                file_dto.objects.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<ObjectDefinitionFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<ObjectDefinitionFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in objects: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Effects parsing
// ============================================================

pathfinder::foundation::Result<EffectDefinitionFileDto> JsonContentParser::parseEffects(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        EffectDefinitionFileDto file_dto;

        if (j.contains("effects") && j["effects"].is_array()) {
            for (const auto& eff : j["effects"]) {
                EffectDefinitionDto dto;
                dto.effect_key = safeGetString(eff, "effect_key");
                dto.display_key = safeGetString(eff, "display_key");
                dto.semantic_kind = safeGetString(eff, "semantic_kind");
                dto.goal_kinds = safeGetStringArray(eff, "goal_kinds");
                dto.target_kind = safeGetString(eff, "target_kind");
                dto.preconditions = safeGetStringArray(eff, "preconditions");

                if (eff.contains("operations") && eff["operations"].is_array()) {
                    for (const auto& op : eff["operations"]) {
                        EffectOperationDto op_dto;
                        op_dto.op = safeGetString(op, "op");
                        op_dto.target = safeGetString(op, "target");
                        op_dto.quantity = safeGetInt(op, "quantity", 0);
                        op_dto.state_key = safeGetString(op, "state_key");
                        op_dto.need_key = safeGetString(op, "need_key");
                        op_dto.delta = safeGetDouble(op, "delta", 0.0);
                        op_dto.value = safeGetDouble(op, "value", 0.0);
                        op_dto.source = safeGetString(op, "source");
                        op_dto.summary_key = safeGetString(op, "summary_key");
                        dto.operations.push_back(std::move(op_dto));
                    }
                }

                if (eff.contains("learning")) {
                    const auto& learn = eff["learning"];
                    dto.learning.knowledge_effect_key = safeGetString(learn, "knowledge_effect_key");
                    dto.learning.confidence_delta = safeGetDouble(learn, "confidence_delta", 0.0);
                    dto.learning.teachable = safeGetBool(learn, "teachable", false);
                }

                if (eff.contains("agent")) {
                    const auto& ag = eff["agent"];
                    dto.agent.usable_by_ai = safeGetBool(ag, "usable_by_ai", false);
                    dto.agent.risk_score = safeGetInt(ag, "risk_score", 0);
                    dto.agent.time_cost = safeGetInt(ag, "time_cost", 1);
                }

                file_dto.effects.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<EffectDefinitionFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<EffectDefinitionFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in effects: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Feedbacks parsing
// ============================================================

pathfinder::foundation::Result<FeedbackDefinitionFileDto> JsonContentParser::parseFeedbacks(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        FeedbackDefinitionFileDto file_dto;

        if (j.contains("feedbacks") && j["feedbacks"].is_array()) {
            for (const auto& fb : j["feedbacks"]) {
                FeedbackDefinitionDto dto;
                dto.feedback_key = safeGetString(fb, "feedback_key");
                dto.object_key = safeGetString(fb, "object_key");
                dto.action_key = safeGetString(fb, "action_key");
                dto.target_object_key = safeGetString(fb, "target_object_key");
                dto.effect_key = safeGetString(fb, "effect_key");
                dto.priority = safeGetInt(fb, "priority", 100);
                dto.conditions = safeGetStringArray(fb, "conditions");
                dto.reply_key = safeGetString(fb, "reply_key");
                dto.utility_delta = safeGetDouble(fb, "utility_delta", 0.0);
                dto.risk_delta = safeGetDouble(fb, "risk_delta", 0.0);
                dto.causal_tags = safeGetStringArray(fb, "causal_tags");
                dto.outcome_signal_keys = safeGetStringArray(fb, "outcome_signal_keys");

                if (fb.contains("knowledge")) {
                    const auto& know = fb["knowledge"];
                    dto.knowledge.subject_object_key = safeGetString(know, "subject_object_key");
                    dto.knowledge.action_key = safeGetString(know, "action_key");
                    dto.knowledge.effect_key = safeGetString(know, "effect_key");
                }

                file_dto.feedbacks.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<FeedbackDefinitionFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<FeedbackDefinitionFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in feedbacks: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Reactions parsing
// ============================================================

pathfinder::foundation::Result<ReactionDefinitionFileDto> JsonContentParser::parseReactions(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        ReactionDefinitionFileDto file_dto;

        if (j.contains("reactions") && j["reactions"].is_array()) {
            for (const auto& r : j["reactions"]) {
                ReactionDefinitionDto dto;
                dto.reaction_key = safeGetString(r, "reaction_key");

                if (r.contains("inputs") && r["inputs"].is_array()) {
                    for (const auto& inp : r["inputs"]) {
                        ReactionInputDto in_dto;
                        in_dto.object_key = safeGetString(inp, "object_key");
                        in_dto.state = safeGetString(inp, "state");
                        in_dto.min = safeGetInt(inp, "min", 1);
                        in_dto.quantity = safeGetInt(inp, "quantity", 0);
                        dto.inputs.push_back(std::move(in_dto));
                    }
                }

                dto.action_key = safeGetString(r, "action_key");
                dto.effect_key = safeGetString(r, "effect_key");

                if (r.contains("outputs") && r["outputs"].is_array()) {
                    for (const auto& out : r["outputs"]) {
                        ReactionOutputDto out_dto;
                        out_dto.object_key = safeGetString(out, "object_key");
                        out_dto.quantity_delta = safeGetInt(out, "quantity_delta", 0);
                        dto.outputs.push_back(std::move(out_dto));
                    }
                }

                if (r.contains("consume") && r["consume"].is_array()) {
                    for (const auto& con : r["consume"]) {
                        ReactionConsumeDto con_dto;
                        con_dto.object_key = safeGetString(con, "object_key");
                        con_dto.state = safeGetString(con, "state");
                        con_dto.delta = safeGetInt(con, "delta", 0);
                        dto.consume.push_back(std::move(con_dto));
                    }
                }

                dto.summary_key = safeGetString(r, "summary_key");
                dto.knowledge_templates = safeGetStringArray(r, "knowledge_templates");

                file_dto.reactions.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<ReactionDefinitionFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<ReactionDefinitionFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in reactions: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Agents parsing
// ============================================================

pathfinder::foundation::Result<AgentTemplateFileDto> JsonContentParser::parseAgents(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        AgentTemplateFileDto file_dto;

        if (j.contains("agents") && j["agents"].is_array()) {
            for (const auto& ag : j["agents"]) {
                AgentTemplateDto dto;
                dto.agent_key = safeGetString(ag, "agent_key");
                dto.display_key = safeGetString(ag, "display_key");
                dto.scale = safeGetString(ag, "scale");
                dto.cognition_band = safeGetString(ag, "cognition_band");
                dto.embodiment = safeGetString(ag, "embodiment");

                if (ag.contains("default_needs")) {
                    const auto& needs = ag["default_needs"];
                    dto.default_needs.fear = safeGetDouble(needs, "fear", 0.0);
                    dto.default_needs.hunger = safeGetDouble(needs, "hunger", 0.0);
                    dto.default_needs.trust = safeGetDouble(needs, "trust", 0.0);
                }

                dto.instinct_effect_keys = safeGetStringArray(ag, "instinct_effect_keys");
                dto.default_policy_key = safeGetString(ag, "default_policy_key");
                dto.can_learn = safeGetBool(ag, "can_learn", true);
                dto.can_teach = safeGetBool(ag, "can_teach", false);

                file_dto.agents.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<AgentTemplateFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<AgentTemplateFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in agents: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Threats parsing
// ============================================================

pathfinder::foundation::Result<ThreatDefinitionFileDto> JsonContentParser::parseThreats(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        ThreatDefinitionFileDto file_dto;

        if (j.contains("threats") && j["threats"].is_array()) {
            for (const auto& thr : j["threats"]) {
                ThreatDefinitionDto dto;
                dto.threat_key = safeGetString(thr, "threat_key");
                dto.display_key = safeGetString(thr, "display_key");
                dto.agent_key = safeGetString(thr, "agent_key");
                dto.initial_level = safeGetInt(thr, "initial_level", 0);

                if (thr.contains("progression")) {
                    const auto& prog = thr["progression"];
                    dto.progression.wait_delta = safeGetInt(prog, "wait_delta", 25);
                    if (prog.contains("phases") && prog["phases"].is_array()) {
                        for (const auto& ph : prog["phases"]) {
                            ThreatPhaseDto ph_dto;
                            ph_dto.min = safeGetInt(ph, "min", 0);
                            ph_dto.phase = safeGetString(ph, "phase");
                            ph_dto.summary_key = safeGetString(ph, "summary_key");
                            dto.progression.phases.push_back(std::move(ph_dto));
                        }
                    }
                }

                if (thr.contains("countermeasures") && thr["countermeasures"].is_array()) {
                    for (const auto& cm : thr["countermeasures"]) {
                        ThreatCountermeasureDto cm_dto;
                        cm_dto.effect_key = safeGetString(cm, "effect_key");
                        cm_dto.level_delta = safeGetInt(cm, "level_delta", 0);
                        dto.countermeasures.push_back(std::move(cm_dto));
                    }
                }

                if (thr.contains("reentry")) {
                    const auto& re = thr["reentry"];
                    dto.reentry.enabled = safeGetBool(re, "enabled", false);
                    dto.reentry.waits = safeGetInt(re, "waits", 3);
                    dto.reentry.level = safeGetInt(re, "level", 50);
                    dto.reentry.tag = safeGetString(re, "tag");
                }

                file_dto.threats.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<ThreatDefinitionFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<ThreatDefinitionFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in threats: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Knowledge templates parsing
// ============================================================

pathfinder::foundation::Result<KnowledgeTemplateFileDto> JsonContentParser::parseKnowledgeTemplates(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        KnowledgeTemplateFileDto file_dto;

        if (j.contains("knowledge_templates") && j["knowledge_templates"].is_array()) {
            for (const auto& kt : j["knowledge_templates"]) {
                KnowledgeTemplateDto dto;
                dto.knowledge_key = safeGetString(kt, "knowledge_key");
                dto.subject_object_key = safeGetString(kt, "subject_object_key");
                dto.action_key = safeGetString(kt, "action_key");
                dto.effect_key = safeGetString(kt, "effect_key");
                dto.target_object_key = safeGetString(kt, "target_object_key");
                dto.default_status = safeGetString(kt, "default_status", "hypothesis");
                dto.teachable = safeGetBool(kt, "teachable", true);
                dto.usable_by_ai = safeGetBool(kt, "usable_by_ai", true);
                dto.summary_key = safeGetString(kt, "summary_key");

                file_dto.knowledge_templates.push_back(std::move(dto));
            }
        }
        return pathfinder::foundation::Result<KnowledgeTemplateFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<KnowledgeTemplateFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in knowledge templates: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Scenario parsing
// ============================================================

pathfinder::foundation::Result<ScenarioDefinitionDto> JsonContentParser::parseScenario(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        ScenarioDefinitionDto dto;
        dto.scenario_key = safeGetString(j, "scenario_key");
        dto.display_key = safeGetString(j, "display_key");

        if (j.contains("initial_objects") && j["initial_objects"].is_array()) {
            for (const auto& obj : j["initial_objects"]) {
                ScenarioInitialObjectDto obj_dto;
                obj_dto.object_key = safeGetString(obj, "object_key");
                obj_dto.quantity = safeGetInt(obj, "quantity", 1);
                obj_dto.visible = safeGetBool(obj, "visible", true);
                obj_dto.numeric = safeGetStringDoubleMap(obj, "numeric");
                dto.initial_objects.push_back(std::move(obj_dto));
            }
        }

        if (j.contains("initial_agents") && j["initial_agents"].is_array()) {
            for (const auto& ag : j["initial_agents"]) {
                ScenarioInitialAgentDto ag_dto;
                ag_dto.agent_key = safeGetString(ag, "agent_key");
                ag_dto.visible = safeGetBool(ag, "visible", true);
                ag_dto.trust = safeGetDouble(ag, "trust", 0.5);
                dto.initial_agents.push_back(std::move(ag_dto));
            }
        }

        if (j.contains("initial_threats") && j["initial_threats"].is_array()) {
            for (const auto& thr : j["initial_threats"]) {
                ScenarioInitialThreatDto thr_dto;
                thr_dto.threat_key = safeGetString(thr, "threat_key");
                thr_dto.level = safeGetInt(thr, "level", 0);
                dto.initial_threats.push_back(std::move(thr_dto));
            }
        }

        dto.welcome_text_key = safeGetString(j, "welcome_text_key");
        dto.quick_action_input_texts = safeGetStringArray(j, "quick_action_input_texts");

        if (j.contains("suggested_action_templates") && j["suggested_action_templates"].is_array()) {
            for (const auto& act : j["suggested_action_templates"]) {
                ScenarioActionTemplateDto act_dto;
                act_dto.action_key = safeGetString(act, "action_key");
                act_dto.label_text = safeGetString(act, "label_text");
                act_dto.input_text = safeGetString(act, "input_text");
                act_dto.object_key = safeGetString(act, "object_key");
                act_dto.required_effect_key = safeGetString(act, "required_effect_key");
                act_dto.target_object_key = safeGetString(act, "target_object_key");
                act_dto.reason_keys = safeGetStringArray(act, "reason_keys");
                dto.suggested_action_templates.push_back(std::move(act_dto));
            }
        }

        if (j.contains("threat_knowledge_templates") && j["threat_knowledge_templates"].is_array()) {
            for (const auto& thr : j["threat_knowledge_templates"]) {
                ScenarioThreatTemplateDto thr_dto;
                thr_dto.template_key = safeGetString(thr, "template_key");
                thr_dto.threat_object_key = safeGetString(thr, "threat_object_key");
                thr_dto.required_effect_key = safeGetString(thr, "required_effect_key");
                thr_dto.fallback_effect_keys = safeGetStringArray(thr, "fallback_effect_keys");
                dto.threat_knowledge_templates.push_back(std::move(thr_dto));
            }
        }

        dto.default_player_knowledge = safeGetStringArray(j, "default_player_knowledge");
        dto.default_agent_knowledge = safeGetStringArray(j, "default_agent_knowledge");

        return pathfinder::foundation::Result<ScenarioDefinitionDto>::ok(std::move(dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<ScenarioDefinitionDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in scenario: ") + e.what()));
    }
}


// ============================================================
// JsonContentParser - Worldgen parsing
// ============================================================

pathfinder::foundation::Result<WorldgenProfileDefinitionFileDto> JsonContentParser::parseWorldgenProfiles(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        WorldgenProfileDefinitionFileDto file_dto;

        if (j.contains("worldgen_profiles") && j["worldgen_profiles"].is_array()) {
            for (const auto& profile_json : j["worldgen_profiles"]) {
                WorldgenProfileDefinitionDto dto;
                dto.profile_key = safeGetString(profile_json, "profile_key");
                dto.region_size = safeGetInt(profile_json, "region_size", 16);
                dto.default_layer = safeGetString(profile_json, "default_layer", "surface");
                dto.terrain_generation_mode = safeGetString(profile_json, "terrain_generation_mode", "NoiseField");

                if (profile_json.contains("terrain_weights") && profile_json["terrain_weights"].is_array()) {
                    for (const auto& item : profile_json["terrain_weights"]) {
                        WorldgenTerrainWeightDto weight;
                        weight.terrain_key = safeGetString(item, "terrain_key");
                        weight.weight = safeGetInt(item, "weight", 0);
                        dto.terrain_weights.push_back(std::move(weight));
                    }
                }

                if (profile_json.contains("noise_channels") && profile_json["noise_channels"].is_array()) {
                    for (const auto& item : profile_json["noise_channels"]) {
                        WorldgenNoiseChannelDto channel;
                        channel.channel = safeGetString(item, "channel");
                        channel.algorithm = safeGetString(item, "algorithm", "FractalPerlin2D");
                        channel.scale = safeGetDouble(item, "scale", 24.0);
                        channel.octaves = safeGetInt(item, "octaves", 3);
                        channel.persistence = safeGetDouble(item, "persistence", 0.5);
                        channel.lacunarity = safeGetDouble(item, "lacunarity", 2.0);
                        channel.bias = safeGetDouble(item, "bias", 0.0);
                        channel.weight = safeGetDouble(item, "weight", 1.0);
                        channel.salt = static_cast<uint64_t>(safeGetInt(item, "salt", 0));
                        dto.noise_channels.push_back(std::move(channel));
                    }
                }

                if (profile_json.contains("terrain_threshold_rules") && profile_json["terrain_threshold_rules"].is_array()) {
                    for (const auto& item : profile_json["terrain_threshold_rules"]) {
                        WorldgenTerrainThresholdRuleDto rule;
                        rule.terrain_key = safeGetString(item, "terrain_key");
                        rule.min_elevation = safeGetDouble(item, "min_elevation", -1.0);
                        rule.max_elevation = safeGetDouble(item, "max_elevation", 1.0);
                        rule.min_moisture = safeGetDouble(item, "min_moisture", -1.0);
                        rule.max_moisture = safeGetDouble(item, "max_moisture", 1.0);
                        rule.min_roughness = safeGetDouble(item, "min_roughness", -1.0);
                        rule.max_roughness = safeGetDouble(item, "max_roughness", 1.0);
                        rule.priority = safeGetInt(item, "priority", 0);
                        rule.tag_keys = safeGetStringArray(item, "tag_keys");
                        dto.terrain_threshold_rules.push_back(std::move(rule));
                    }
                }

                if (profile_json.contains("connectivity_policy") && profile_json["connectivity_policy"].is_object()) {
                    const auto& item = profile_json["connectivity_policy"];
                    dto.connectivity_policy.enabled = safeGetBool(item, "enabled", true);
                    dto.connectivity_policy.spawn_safe_radius = safeGetInt(item, "spawn_safe_radius", 3);
                    dto.connectivity_policy.min_walkable_ratio = safeGetDouble(item, "min_walkable_ratio", 0.72);
                    dto.connectivity_policy.max_blocked_ratio_in_spawn_radius = safeGetDouble(item, "max_blocked_ratio_in_spawn_radius", 0.10);
                    dto.connectivity_policy.min_reachable_cells_from_spawn = safeGetInt(item, "min_reachable_cells_from_spawn", 80);
                    dto.connectivity_policy.carve_cardinal_corridors = safeGetBool(item, "carve_cardinal_corridors", true);
                    dto.connectivity_policy.corridor_half_width = safeGetInt(item, "corridor_half_width", 1);
                    dto.connectivity_policy.repair_preferred_terrain_keys = safeGetStringArray(item, "repair_preferred_terrain_keys");
                }

                if (profile_json.contains("resource_rules") && profile_json["resource_rules"].is_array()) {
                    for (const auto& item : profile_json["resource_rules"]) {
                        WorldgenResourceRuleDto rule;
                        rule.resource_key = safeGetString(item, "resource_key");
                        rule.allowed_terrain_tags = safeGetStringArray(item, "allowed_terrain_tags");
                        rule.density = safeGetDouble(item, "density", 0.0);
                        rule.min_distance_from_spawn = safeGetInt(item, "min_distance_from_spawn", 0);
                        rule.max_distance_from_spawn = safeGetInt(item, "max_distance_from_spawn", -1);
                        rule.tag_keys = safeGetStringArray(item, "tag_keys");
                        rule.node_kind = safeGetString(item, "node_kind");
                        rule.required_action_key = safeGetString(item, "required_action_key");
                        rule.required_tool_key = safeGetString(item, "required_tool_key");
                        rule.output_object_keys = safeGetStringArray(item, "output_object_keys");
                        rule.charges = safeGetInt(item, "charges", 1);
                        dto.resource_rules.push_back(std::move(rule));
                    }
                }

                if (profile_json.contains("ground_item_rules") && profile_json["ground_item_rules"].is_array()) {
                    for (const auto& item : profile_json["ground_item_rules"]) {
                        WorldgenGroundItemRuleDto rule;
                        rule.object_key = safeGetString(item, "object_key");
                        rule.allowed_terrain_tags = safeGetStringArray(item, "allowed_terrain_tags");
                        rule.density = safeGetDouble(item, "density", 0.0);
                        rule.min_distance_from_spawn = safeGetInt(item, "min_distance_from_spawn", 0);
                        rule.max_distance_from_spawn = safeGetInt(item, "max_distance_from_spawn", -1);
                        rule.quantity = safeGetInt(item, "quantity", 0);
                        rule.stackable = safeGetBool(item, "stackable", true);
                        rule.stack_key = safeGetString(item, "stack_key");
                        rule.tag_keys = safeGetStringArray(item, "tag_keys");
                        rule.state_keys = safeGetStringArray(item, "state_keys");
                        rule.numeric_states = safeGetStringDoubleMap(item, "numeric_states");
                        dto.ground_item_rules.push_back(std::move(rule));
                    }
                }

                if (profile_json.contains("drop_rules") && profile_json["drop_rules"].is_array()) {
                    for (const auto& item : profile_json["drop_rules"]) {
                        WorldgenDropRuleDto rule;
                        rule.drop_rule_key = safeGetString(item, "drop_rule_key");
                        rule.source_object_key = safeGetString(item, "source_object_key");
                        rule.output_object_keys = safeGetStringArray(item, "output_object_keys");
                        dto.drop_rules.push_back(std::move(rule));
                    }
                }

                if (profile_json.contains("spawn_safety") && profile_json["spawn_safety"].is_object()) {
                    const auto& item = profile_json["spawn_safety"];
                    dto.spawn_safety.safe_radius = safeGetInt(item, "safe_radius", 2);
                    dto.spawn_safety.basic_food_min_count = safeGetInt(item, "basic_food_min_count", 1);
                    dto.spawn_safety.basic_material_min_count = safeGetInt(item, "basic_material_min_count", 2);
                    dto.spawn_safety.tool_hint_min_count = safeGetInt(item, "tool_hint_min_count", 1);
                    dto.spawn_safety.immediate_threat_max_count = safeGetInt(item, "immediate_threat_max_count", 0);
                    dto.spawn_safety.guaranteed_resource_keys = safeGetStringArray(item, "guaranteed_resource_keys");
                    dto.spawn_safety.forbidden_danger_keys = safeGetStringArray(item, "forbidden_danger_keys");
                }

                file_dto.worldgen_profiles.push_back(std::move(dto));
            }
        }

        return pathfinder::foundation::Result<WorldgenProfileDefinitionFileDto>::ok(std::move(file_dto));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<WorldgenProfileDefinitionFileDto>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in worldgen profiles: ") + e.what()));
    }
}

// ============================================================
// JsonContentParser - Locale parsing
// ============================================================

pathfinder::foundation::Result<LocaleMap> JsonContentParser::parseLocale(const std::string& json_text) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_text);
        LocaleMap locale_map;

        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it.value().is_string()) {
                    locale_map[it.key()] = it.value().get<std::string>();
                }
            }
        }
        return pathfinder::foundation::Result<LocaleMap>::ok(std::move(locale_map));
    } catch (const nlohmann::json::parse_error& e) {
        return pathfinder::foundation::Result<LocaleMap>::fail(
            pathfinder::foundation::makeError(ErrorCode::validation_config_invalid,
                std::string("JSON parse error in locale: ") + e.what()));
    }
}

// ============================================================
// DTO to Content conversion helpers
// ============================================================

void JsonContentLoader::convertToContent(const ObjectDefinitionFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& obj_dto : dto.objects) {
        ObjectDefinitionContent content;
        content.key = ObjectDefinitionKey(obj_dto.object_key);
        content.display_key = obj_dto.display_key;
        content.description_key = obj_dto.description_key;
        content.kind = obj_dto.kind;
        content.visibility = obj_dto.visibility;
        content.safe_tags = obj_dto.safe_tags;
        content.allowed_actions = obj_dto.allowed_actions;
        content.default_quantity = obj_dto.initial_state.quantity;
        content.default_numeric = obj_dto.initial_state.numeric;
        content.knowledge_hints = obj_dto.knowledge_hints;
        content.safe_trait_keys = obj_dto.projection.safe_trait_keys;
        content.unknown_use_text_key = obj_dto.projection.unknown_use_text_key;
        draft.addObject(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const EffectDefinitionFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& eff_dto : dto.effects) {
        EffectDefinitionContent content;
        content.key = EffectDefinitionKey(eff_dto.effect_key);
        content.display_key = eff_dto.display_key;
        content.semantic_kind = eff_dto.semantic_kind;
        content.goal_kinds = eff_dto.goal_kinds;
        content.target_kind = eff_dto.target_kind;
        content.preconditions = eff_dto.preconditions;
        content.operations = eff_dto.operations;
        content.knowledge_effect_key = eff_dto.learning.knowledge_effect_key;
        content.confidence_delta = eff_dto.learning.confidence_delta;
        content.teachable = eff_dto.learning.teachable;
        content.usable_by_ai = eff_dto.agent.usable_by_ai;
        content.risk_score = eff_dto.agent.risk_score;
        content.time_cost = eff_dto.agent.time_cost;
        draft.addEffect(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const FeedbackDefinitionFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& fb_dto : dto.feedbacks) {
        FeedbackDefinitionContent content;
        content.key = FeedbackDefinitionKey(fb_dto.feedback_key);
        content.object_key = fb_dto.object_key;
        content.action_key = fb_dto.action_key;
        content.target_object_key = fb_dto.target_object_key;
        content.effect_key = fb_dto.effect_key;
        content.priority = fb_dto.priority;
        content.conditions = fb_dto.conditions;
        content.reply_key = fb_dto.reply_key;
        content.utility_delta = fb_dto.utility_delta;
        content.risk_delta = fb_dto.risk_delta;
        content.causal_tags = fb_dto.causal_tags;
        content.outcome_signal_keys = fb_dto.outcome_signal_keys;
        content.subject_object_key = fb_dto.knowledge.subject_object_key;
        content.knowledge_action_key = fb_dto.knowledge.action_key;
        content.knowledge_effect_key = fb_dto.knowledge.effect_key;
        draft.addFeedback(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const ReactionDefinitionFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& r_dto : dto.reactions) {
        ReactionDefinitionContent content;
        content.key = ReactionDefinitionKey(r_dto.reaction_key);
        content.inputs = r_dto.inputs;
        content.action_key = r_dto.action_key;
        content.effect_key = r_dto.effect_key;
        content.outputs = r_dto.outputs;
        content.consume = r_dto.consume;
        content.summary_key = r_dto.summary_key;
        content.knowledge_templates = r_dto.knowledge_templates;
        draft.addReaction(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const AgentTemplateFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& ag_dto : dto.agents) {
        AgentTemplateContent content;
        content.key = AgentTemplateKey(ag_dto.agent_key);
        content.display_key = ag_dto.display_key;
        content.scale = ag_dto.scale;
        content.cognition_band = ag_dto.cognition_band;
        content.embodiment = ag_dto.embodiment;
        content.default_fear = ag_dto.default_needs.fear;
        content.default_hunger = ag_dto.default_needs.hunger;
        content.default_trust = ag_dto.default_needs.trust;
        content.instinct_effect_keys = ag_dto.instinct_effect_keys;
        content.default_policy_key = ag_dto.default_policy_key;
        content.can_learn = ag_dto.can_learn;
        content.can_teach = ag_dto.can_teach;
        draft.addAgent(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const ThreatDefinitionFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& thr_dto : dto.threats) {
        ThreatDefinitionContent content;
        content.key = ThreatDefinitionKey(thr_dto.threat_key);
        content.display_key = thr_dto.display_key;
        content.agent_key = thr_dto.agent_key;
        content.initial_level = thr_dto.initial_level;
        content.wait_delta = thr_dto.progression.wait_delta;
        content.phases = thr_dto.progression.phases;
        content.countermeasures = thr_dto.countermeasures;
        content.reentry_enabled = thr_dto.reentry.enabled;
        content.reentry_waits = thr_dto.reentry.waits;
        content.reentry_level = thr_dto.reentry.level;
        content.reentry_tag = thr_dto.reentry.tag;
        draft.addThreat(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const KnowledgeTemplateFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& kt_dto : dto.knowledge_templates) {
        KnowledgeTemplateContent content;
        content.key = KnowledgeTemplateKey(kt_dto.knowledge_key);
        content.subject_object_key = kt_dto.subject_object_key;
        content.action_key = kt_dto.action_key;
        content.effect_key = kt_dto.effect_key;
        content.target_object_key = kt_dto.target_object_key;
        content.default_status = kt_dto.default_status;
        content.teachable = kt_dto.teachable;
        content.usable_by_ai = kt_dto.usable_by_ai;
        content.summary_key = kt_dto.summary_key;
        draft.addKnowledgeTemplate(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const ScenarioDefinitionDto& dto, ContentDraftRegistry& draft) const {
    ScenarioDefinitionContent content;
    content.key = ScenarioDefinitionKey(dto.scenario_key);
    content.display_key = dto.display_key;
    content.welcome_text_key = dto.welcome_text_key;
    content.initial_objects = dto.initial_objects;
    content.initial_agents = dto.initial_agents;
    content.initial_threats = dto.initial_threats;
    content.quick_action_input_texts = dto.quick_action_input_texts;
    content.suggested_action_templates = dto.suggested_action_templates;
    content.threat_knowledge_templates = dto.threat_knowledge_templates;
    content.default_player_knowledge = dto.default_player_knowledge;
    content.default_agent_knowledge = dto.default_agent_knowledge;
    draft.addScenario(std::move(content));
}


void JsonContentLoader::convertToContent(const WorldgenProfileDefinitionFileDto& dto, ContentDraftRegistry& draft) const {
    for (const auto& profile_dto : dto.worldgen_profiles) {
        WorldgenProfileDefinitionContent content;
        content.profile_key = profile_dto.profile_key;
        content.region_size = profile_dto.region_size;
        content.default_layer = profile_dto.default_layer;
        content.terrain_generation_mode = profile_dto.terrain_generation_mode;
        content.terrain_weights = profile_dto.terrain_weights;
        content.noise_channels = profile_dto.noise_channels;
        content.terrain_threshold_rules = profile_dto.terrain_threshold_rules;
        content.connectivity_policy = profile_dto.connectivity_policy;
        content.resource_rules = profile_dto.resource_rules;
        content.ground_item_rules = profile_dto.ground_item_rules;
        content.drop_rules = profile_dto.drop_rules;
        content.spawn_safety = profile_dto.spawn_safety;
        draft.addWorldgenProfile(std::move(content));
    }
}

void JsonContentLoader::convertToContent(const LocaleMap& dto, const std::string& locale_key, ContentDraftRegistry& draft) const {
    draft.addLocale(locale_key, dto);
}

// ============================================================
// JsonContentLoader - Loading pipeline
// ============================================================

pathfinder::foundation::Result<ContentPackageManifest> JsonContentLoader::loadManifest(
    const std::string& package_dir) const {
    std::string manifest_path = package_dir + "/manifest.json";
    auto file_result = readFileSafe(manifest_path);
    if (file_result.is_error()) {
        return pathfinder::foundation::Result<ContentPackageManifest>::fail(file_result.errors());
    }

    auto manifest_dto_result = JsonContentParser::parseManifest(file_result.value());
    if (manifest_dto_result.is_error()) {
        return pathfinder::foundation::Result<ContentPackageManifest>::fail(manifest_dto_result.errors());
    }

    const auto& dto = manifest_dto_result.value();
    ContentPackageManifest manifest;
    manifest.package_key = ContentPackageKey(dto.package_key);
    manifest.display_name = dto.display_name;
    manifest.content_version = dto.content_version;
    manifest.schema_version = dto.schema_version;
    manifest.locale_default = dto.locale_default;
    manifest.load_order = dto.load_order;

    for (const auto& file_ref : dto.files) {
        ContentFileEntry entry;
        entry.path = file_ref.path;
        entry.category = file_ref.category;
        entry.required = file_ref.required;
        manifest.files.push_back(std::move(entry));
    }

    return pathfinder::foundation::Result<ContentPackageManifest>::ok(std::move(manifest));
}

pathfinder::foundation::Result<void> JsonContentLoader::loadPackageFiles(
    const ContentPackageManifest& manifest,
    const std::string& package_dir,
    ContentDraftRegistry& draft,
    ConfigValidationReport& report) const {

    for (const auto& file_entry : manifest.files) {
        if (!isPathSafe(file_entry.path)) {
            report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                "unsafe file path: " + file_entry.path, file_entry.path});
            if (file_entry.required) continue;
            else continue;
        }

        std::string full_path = package_dir + "/" + file_entry.path;
        auto file_result = readFileSafe(full_path);
        if (file_result.is_error()) {
            auto severity = file_entry.required ? ConfigValidationSeverity::Error : ConfigValidationSeverity::Warning;
            report.addIssue({severity, ErrorCode::storage_read_failed,
                "failed to read file: " + full_path, file_entry.path});
            continue;
        }

        const std::string& content = file_result.value();
        if (content.empty()) {
            report.addIssue({ConfigValidationSeverity::Warning, ErrorCode::validation_config_invalid,
                "empty file: " + file_entry.path, file_entry.path});
            continue;
        }

        // Parse and convert by category
        switch (file_entry.category) {
            case ContentFileCategory::Objects: {
                auto result = JsonContentParser::parseObjects(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse objects: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Effects: {
                auto result = JsonContentParser::parseEffects(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse effects: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Feedbacks: {
                auto result = JsonContentParser::parseFeedbacks(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse feedbacks: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Reactions: {
                auto result = JsonContentParser::parseReactions(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse reactions: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Agents: {
                auto result = JsonContentParser::parseAgents(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse agents: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Threats: {
                auto result = JsonContentParser::parseThreats(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse threats: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Knowledge: {
                auto result = JsonContentParser::parseKnowledgeTemplates(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse knowledge templates: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Scenarios: {
                auto result = JsonContentParser::parseScenario(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse scenario: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Worldgen: {
                auto result = JsonContentParser::parseWorldgenProfiles(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse worldgen profiles: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), draft);
                }
                break;
            }
            case ContentFileCategory::Locales: {
                auto result = JsonContentParser::parseLocale(content);
                if (result.is_error()) {
                    report.addIssue({ConfigValidationSeverity::Error, ErrorCode::validation_config_invalid,
                        "failed to parse locale: " + file_entry.path, file_entry.path});
                } else {
                    convertToContent(result.value(), manifest.locale_default, draft);
                }
                break;
            }
            default:
                report.addIssue({ConfigValidationSeverity::Warning, ErrorCode::common_not_implemented,
                    "unsupported category for file: " + file_entry.path, file_entry.path});
                break;
        }
    }

    return pathfinder::foundation::Result<void>::ok();
}

// ============================================================
// JsonContentLoader::load - main entry point
// ============================================================

pathfinder::foundation::Result<ContentLoadResult> JsonContentLoader::load(const ContentLoadOptions& options) const {
    ContentLoadResult result;
    ContentDraftRegistry draft;

    result.trace_keys.push_back("Load mode: " + toString(options.load_mode));

    std::string root = options.root_path;
    if (root.empty()) {
        root = "content";
    }

    // Determine packages to load
    std::vector<std::string> packages_to_load;
    if (options.enabled_package_keys.empty()) {
        packages_to_load.push_back("core");
    } else {
        packages_to_load = options.enabled_package_keys;
    }

    // Load each package
    for (const auto& pkg_key : packages_to_load) {
        std::string package_dir = root + "/" + pkg_key;
        result.trace_keys.push_back("Loading package: " + pkg_key);

        auto manifest_result = loadManifest(package_dir);
        if (manifest_result.is_error()) {
            result.validation_report.addIssue({ConfigValidationSeverity::Fatal, ErrorCode::storage_read_failed,
                "failed to load manifest for package: " + pkg_key, package_dir + "/manifest.json"});
            return pathfinder::foundation::Result<ContentLoadResult>::ok(std::move(result));
        }

        const auto& manifest = manifest_result.value();
        result.trace_keys.push_back("Manifest loaded: " + manifest.package_key.value());

        auto load_result = loadPackageFiles(manifest, package_dir, draft, result.validation_report);
        if (load_result.is_error()) {
            result.validation_report.addIssue({ConfigValidationSeverity::Fatal, ErrorCode::validation_config_invalid,
                "failed to load package files: " + pkg_key, package_dir});
            return pathfinder::foundation::Result<ContentLoadResult>::ok(std::move(result));
        }
    }

    // Structural validation
    result.trace_keys.push_back("Running structural validation");
    ContentStructuralValidator struct_validator;
    auto struct_report = struct_validator.validate(draft);
    for (const auto& issue : struct_report.issues()) {
        result.validation_report.addIssue(issue);
    }

    // Reference validation
    result.trace_keys.push_back("Running reference validation");
    ContentReferenceValidator ref_validator;
    auto ref_report = ref_validator.validate(draft);
    for (const auto& issue : ref_report.issues()) {
        result.validation_report.addIssue(issue);
    }

    // Check for fatal/error before building registry
    const bool reject_on_error = options.load_mode == ContentLoadMode::StrictContentRequired;
    if (result.validation_report.hasFatal() || (reject_on_error && result.validation_report.hasErrors())) {
        result.trace_keys.push_back(reject_on_error
            ? "Fatal or error issues found in strict mode, skipping registry build"
            : "Fatal errors found, skipping registry build");
        return pathfinder::foundation::Result<ContentLoadResult>::ok(std::move(result));
    }

    // Build registry
    result.trace_keys.push_back("Building ContentRegistry");
    result.registry = ContentRegistry::build(draft);
    if (result.registry) {
        // Copy package info to registry
        // (registry is const after build, so we use const_cast for setup)
    }

    result.trace_keys.push_back("Content loading complete");
    return pathfinder::foundation::Result<ContentLoadResult>::ok(std::move(result));
}

} // namespace pathfinder::content
