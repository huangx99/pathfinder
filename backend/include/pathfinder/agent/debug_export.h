#pragma once

#include "pathfinder/agent/debug_report.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::agent {

// --- Enums ---

enum class AgentExportFormat {
    Unknown,
    PlainText,
    MarkdownLike,
    ProtocolText
};

std::string toString(AgentExportFormat format);
pathfinder::foundation::Result<AgentExportFormat> agentExportFormatFromString(const std::string& str);

// --- Data Contracts ---

struct AgentExportChunk {
    std::string chunk_id;
    AgentExportFormat format = AgentExportFormat::Unknown;
    std::string title_key;
    std::string payload;
    size_t item_count = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportManifest {
    std::string manifest_id;
    AgentExportFormat format = AgentExportFormat::Unknown;
    size_t chunk_count = 0;
    size_t total_item_count = 0;
    std::vector<std::string> chunk_ids;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentExportDraft {
    AgentExportManifest manifest;
    std::vector<AgentExportChunk> chunks;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Builder ---

struct AgentExportDraftBuildRequest {
    AgentDebugReport report;
    std::optional<AgentDiagnosticsSummary> diagnostics;
    AgentExportFormat format = AgentExportFormat::PlainText;
    size_t max_items_per_chunk = 50;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentExportDraftBuilder {
public:
    pathfinder::foundation::Result<AgentExportDraft> build(
        const AgentExportDraftBuildRequest& request) const;
};

} // namespace pathfinder::agent
