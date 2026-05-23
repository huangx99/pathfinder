#pragma once

#include "pathfinder/content/content_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/config/loader.h"
#include "pathfinder/config/validation.h"
#include "pathfinder/foundation/result.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <memory>

namespace pathfinder::content {

// ============================================================
// JsonContentParser - parses JSON text into DTOs
// ============================================================

class JsonContentParser {
public:
    // Parse a manifest JSON string
    static pathfinder::foundation::Result<ContentManifestDto> parseManifest(const std::string& json_text);

    // Parse objects JSON string
    static pathfinder::foundation::Result<ObjectDefinitionFileDto> parseObjects(const std::string& json_text);

    // Parse effects JSON string
    static pathfinder::foundation::Result<EffectDefinitionFileDto> parseEffects(const std::string& json_text);

    // Parse feedbacks JSON string
    static pathfinder::foundation::Result<FeedbackDefinitionFileDto> parseFeedbacks(const std::string& json_text);

    // Parse reactions JSON string
    static pathfinder::foundation::Result<ReactionDefinitionFileDto> parseReactions(const std::string& json_text);

    // Parse agents JSON string
    static pathfinder::foundation::Result<AgentTemplateFileDto> parseAgents(const std::string& json_text);

    // Parse threats JSON string
    static pathfinder::foundation::Result<ThreatDefinitionFileDto> parseThreats(const std::string& json_text);

    // Parse knowledge templates JSON string
    static pathfinder::foundation::Result<KnowledgeTemplateFileDto> parseKnowledgeTemplates(const std::string& json_text);

    // Parse scenario JSON string
    static pathfinder::foundation::Result<ScenarioDefinitionDto> parseScenario(const std::string& json_text);

    // Parse world generation profiles JSON string
    static pathfinder::foundation::Result<WorldgenProfileDefinitionFileDto> parseWorldgenProfiles(const std::string& json_text);

    // Parse locale JSON string
    static pathfinder::foundation::Result<LocaleMap> parseLocale(const std::string& json_text);

private:
    // Helper to read optional string
    static std::string safeGetString(const nlohmann::json& j, const std::string& key, const std::string& default_val = "");
    // Helper to read optional int
    static int safeGetInt(const nlohmann::json& j, const std::string& key, int default_val = 0);
    // Helper to read optional double
    static double safeGetDouble(const nlohmann::json& j, const std::string& key, double default_val = 0.0);
    // Helper to read optional bool
    static bool safeGetBool(const nlohmann::json& j, const std::string& key, bool default_val = false);
    // Helper to read optional string array
    static std::vector<std::string> safeGetStringArray(const nlohmann::json& j, const std::string& key);
    // Helper to read optional string-to-double map
    static std::map<std::string, double> safeGetStringDoubleMap(const nlohmann::json& j, const std::string& key);
};

// ============================================================
// JsonContentLoader - full loading pipeline
// ============================================================

class JsonContentLoader {
public:
    // Load content from the given options, return registry + validation report
    pathfinder::foundation::Result<ContentLoadResult> load(const ContentLoadOptions& options) const;

    // Check if a path is safe (no traversal)
    static bool isPathSafe(const std::string& relative_path);

private:
    // Load a single package manifest
    pathfinder::foundation::Result<ContentPackageManifest> loadManifest(
        const std::string& package_dir) const;

    // Load files from a manifest into the draft registry
    pathfinder::foundation::Result<void> loadPackageFiles(
        const ContentPackageManifest& manifest,
        const std::string& package_dir,
        ContentDraftRegistry& draft,
        pathfinder::config::ConfigValidationReport& report) const;

    // Convert DTOs to Content definitions
    void convertToContent(const ObjectDefinitionFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const EffectDefinitionFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const FeedbackDefinitionFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const ReactionDefinitionFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const AgentTemplateFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const ThreatDefinitionFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const KnowledgeTemplateFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const ScenarioDefinitionDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const WorldgenProfileDefinitionFileDto& dto, ContentDraftRegistry& draft) const;
    void convertToContent(const LocaleMap& dto, const std::string& locale_key, ContentDraftRegistry& draft) const;

    // Read a file from disk safely
    static pathfinder::foundation::Result<std::string> readFileSafe(const std::string& file_path);
};

} // namespace pathfinder::content
