#include "pathfinder/h5_playable/playable_wire_codec.h"
#include <cctype>
#include <cstdio>
#include <sstream>
#include <unordered_map>

namespace pathfinder::h5_playable {

using pathfinder::foundation::Result;
using namespace pathfinder::h5_projection;

namespace {

std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) ++start;
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return value.substr(start, end - start);
}

std::string urlDecode(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            int decoded = 0;
            bool ok = true;
            for (int j = 1; j <= 2; ++j) {
                decoded *= 16;
                const char c = value[i + j];
                if (c >= '0' && c <= '9') decoded += c - '0';
                else if (c >= 'A' && c <= 'F') decoded += c - 'A' + 10;
                else if (c >= 'a' && c <= 'f') decoded += c - 'a' + 10;
                else ok = false;
            }
            if (ok) {
                out.push_back(static_cast<char>(decoded));
                i += 2;
            } else {
                out.push_back(value[i]);
            }
        } else if (value[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

std::unordered_map<std::string, std::string> parseForm(const std::string& body) {
    std::unordered_map<std::string, std::string> fields;
    std::istringstream stream(body);
    std::string token;
    while (std::getline(stream, token, '&')) {
        auto pos = token.find('=');
        if (pos == std::string::npos) continue;
        fields[trim(urlDecode(token.substr(0, pos)))] = trim(urlDecode(token.substr(pos + 1)));
    }
    return fields;
}

bool formBool(const std::unordered_map<std::string, std::string>& fields, const std::string& key) {
    auto it = fields.find(key);
    if (it == fields.end()) return false;
    return it->second == "true" || it->second == "1" || it->second == "yes";
}

uint64_t formU64(const std::unordered_map<std::string, std::string>& fields, const std::string& key) {
    auto it = fields.find(key);
    if (it == fields.end()) return 0;
    try { return std::stoull(it->second); } catch (...) { return 0; }
}

std::string formString(const std::unordered_map<std::string, std::string>& fields, const std::string& key) {
    auto it = fields.find(key);
    return it == fields.end() ? std::string{} : it->second;
}

std::string q(const std::string& value) {
    return "\"" + playableJsonEscape(value) + "\"";
}

std::string stringArray(const std::vector<std::string>& values) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out << ",";
        out << q(values[i]);
    }
    out << "]";
    return out.str();
}

std::string safeText(const SafeTextProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"text_key\":" << q(value.text_key) << ",";
    out << "\"kind\":" << q(toString(value.kind)) << ",";
    out << "\"zh_cn\":" << q(value.zh_cn) << ",";
    out << "\"reason_keys\":" << stringArray(value.reason_keys);
    out << "}";
    return out.str();
}

std::string badge(const StatusBadgeProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"badge_key\":" << q(value.badge_key) << ",";
    out << "\"label\":" << safeText(value.label) << ",";
    out << "\"tone_key\":" << q(value.tone_key) << ",";
    out << "\"priority\":" << value.priority;
    out << "}";
    return out.str();
}

std::string condition(const ConditionSummaryProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"condition_key\":" << q(value.condition_key) << ",";
    out << "\"summary_text\":" << safeText(value.summary_text) << ",";
    out << "\"satisfied_hint\":";
    if (value.satisfied_hint.has_value()) out << (*value.satisfied_hint ? "true" : "false"); else out << "null";
    out << ",\"blocking\":" << (value.blocking ? "true" : "false") << ",";
    out << "\"reason_keys\":" << stringArray(value.reason_keys);
    out << "}";
    return out.str();
}

std::string action(const ActionProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"action_key\":" << q(value.action_key) << ",";
    out << "\"affordance\":" << q(toString(value.affordance)) << ",";
    out << "\"label\":" << safeText(value.label) << ",";
    out << "\"input_text\":" << q(value.input_text) << ",";
    out << "\"enabled\":" << (value.enabled ? "true" : "false") << ",";
    out << "\"disabled_reason\":";
    if (value.disabled_reason.has_value()) out << condition(*value.disabled_reason); else out << "null";
    out << ",\"command_preview_key\":" << q(value.command_preview_key) << ",";
    out << "\"target_object_refs\":" << stringArray(value.target_object_refs);
    out << "}";
    return out.str();
}

template <typename T, typename Fn>
std::string arrayOf(const std::vector<T>& values, Fn encode) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out << ",";
        out << encode(values[i]);
    }
    out << "]";
    return out.str();
}

std::string objectCard(const ObjectCardProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"object_ref_key\":" << q(value.object_ref_key) << ",";
    out << "\"display_name\":" << safeText(value.display_name) << ",";
    out << "\"description\":" << safeText(value.description) << ",";
    out << "\"visibility_key\":" << q(value.visibility_key) << ",";
    out << "\"safe_tags\":" << stringArray(value.safe_tags) << ",";
    out << "\"knowledge_badges\":" << arrayOf(value.knowledge_badges, badge) << ",";
    out << "\"actions\":" << arrayOf(value.actions, action) << ",";
    out << "\"warning_keys\":" << stringArray(value.warning_keys);
    out << "}";
    return out.str();
}

std::string knowledge(const KnowledgeLineProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"knowledge_id\":" << q(value.knowledge_id) << ",";
    out << "\"owner_label\":" << safeText(value.owner_label) << ",";
    out << "\"subject_label\":" << safeText(value.subject_label) << ",";
    out << "\"predicate_label\":" << safeText(value.predicate_label) << ",";
    out << "\"effect_summary\":" << safeText(value.effect_summary) << ",";
    out << "\"status_key\":" << q(value.status_key) << ",";
    out << "\"confidence_label\":";
    if (value.confidence_label.has_value()) out << safeText(*value.confidence_label); else out << "null";
    out << ",\"teachable\":" << (value.teachable ? "true" : "false") << ",";
    out << "\"reason_keys\":" << stringArray(value.reason_keys);
    out << "}";
    return out.str();
}

std::string cognition(const CognitionLineProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"cognition_key\":" << q(value.cognition_key) << ",";
    out << "\"target_label\":" << safeText(value.target_label) << ",";
    out << "\"aspect_label\":" << safeText(value.aspect_label) << ",";
    out << "\"claim_summary\":" << safeText(value.claim_summary) << ",";
    out << "\"confidence_band\":" << q(value.confidence_band) << ",";
    out << "\"source_hint\":";
    if (value.source_hint.has_value()) out << safeText(*value.source_hint); else out << "null";
    out << "}";
    return out.str();
}

std::string danger(const DangerHintProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"danger_key\":" << q(value.danger_key) << ",";
    out << "\"severity_label\":" << safeText(value.severity_label) << ",";
    out << "\"hint_text\":" << safeText(value.hint_text) << ",";
    out << "\"countermeasure_labels\":" << arrayOf(value.countermeasure_labels, safeText) << ",";
    out << "\"related_object_refs\":" << stringArray(value.related_object_refs) << ",";
    out << "\"warning_keys\":" << stringArray(value.warning_keys);
    out << "}";
    return out.str();
}

std::string tribe(const TribeStatusProjection& value) {
    std::ostringstream out;
    auto optionalText = [](const std::optional<SafeTextProjection>& text_value) {
        return text_value.has_value() ? safeText(*text_value) : std::string("null");
    };
    out << "{";
    out << "\"tribe_ref_key\":" << q(value.tribe_ref_key) << ",";
    out << "\"morale_label\":" << optionalText(value.morale_label) << ",";
    out << "\"trust_label\":" << optionalText(value.trust_label) << ",";
    out << "\"split_risk_label\":" << optionalText(value.split_risk_label) << ",";
    out << "\"coordination_label\":" << optionalText(value.coordination_label) << ",";
    out << "\"conflict_label\":" << optionalText(value.conflict_label) << ",";
    out << "\"reason_keys\":" << stringArray(value.reason_keys);
    out << "}";
    return out.str();
}

std::string issue(const ProjectionIssue& value) {
    std::ostringstream out;
    out << "{";
    out << "\"issue_kind\":" << q(toString(value.issue_kind)) << ",";
    out << "\"severity_key\":" << q(value.severity_key) << ",";
    out << "\"section\":";
    if (value.section.has_value()) out << q(toString(*value.section)); else out << "null";
    out << ",\"field_path\":" << q(value.field_path) << ",";
    out << "\"safe_summary\":" << safeText(value.safe_summary) << ",";
    out << "\"blocked_output\":" << (value.blocked_output ? "true" : "false");
    out << "}";
    return out.str();
}

std::string header(const H5ProjectionHeader& value) {
    std::ostringstream out;
    out << "{";
    out << "\"projection_id\":" << q(value.projection_id) << ",";
    out << "\"protocol_version\":" << q(value.protocol_version) << ",";
    out << "\"session_id\":" << q(value.session_id) << ",";
    out << "\"audience\":" << q(toString(value.audience)) << ",";
    out << "\"mode\":" << q(toString(value.mode)) << ",";
    out << "\"build_status\":" << q(toString(value.build_status)) << ",";
    out << "\"truncated\":" << (value.truncated ? "true" : "false");
    out << "}";
    return out.str();
}

std::string projection(const H5GameProjection& value) {
    std::ostringstream out;
    out << "{";
    out << "\"header\":" << header(value.header) << ",";
    out << "\"scene_title\":" << safeText(value.scene_title) << ",";
    out << "\"scene_summary\":" << arrayOf(value.scene_summary, safeText) << ",";
    out << "\"object_cards\":" << arrayOf(value.object_cards, objectCard) << ",";
    out << "\"action_bar\":" << arrayOf(value.action_bar, action) << ",";
    out << "\"actor_knowledge\":" << arrayOf(value.actor_knowledge, knowledge) << ",";
    out << "\"recipient_knowledge\":" << arrayOf(value.recipient_knowledge, knowledge) << ",";
    out << "\"cognition_lines\":" << arrayOf(value.cognition_lines, cognition) << ",";
    out << "\"condition_hints\":" << arrayOf(value.condition_hints, condition) << ",";
    out << "\"danger_hints\":" << arrayOf(value.danger_hints, danger) << ",";
    out << "\"tribe_status\":";
    if (value.tribe_status.has_value()) out << tribe(*value.tribe_status); else out << "null";
    out << ",\"replay_status\":null,";
    out << "\"issues\":" << arrayOf(value.issues, issue) << ",";
    out << "\"debug_keys\":" << stringArray(value.debug_keys) << ",";
    out << "\"warning_keys\":" << stringArray(value.warning_keys);
    out << "}";
    return out.str();
}

} // namespace

std::string playableJsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (unsigned char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    return out;
}

Result<H5PlayableBootstrapRequest> H5PlayableWireCodec::parseBootstrapForm(const std::string& body) const {
    const auto fields = parseForm(body);
    H5PlayableBootstrapRequest request;
    request.session_id = formString(fields, "session_id");
    request.reset = formBool(fields, "reset");
    auto language = formString(fields, "language");
    if (!language.empty()) request.language = language;
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<H5PlayableBootstrapRequest>::fail(valid.errors());
    return Result<H5PlayableBootstrapRequest>::ok(std::move(request));
}

Result<H5PlayableRequest> H5PlayableWireCodec::parseTurnForm(const std::string& body) const {
    const auto fields = parseForm(body);
    H5PlayableRequest request;
    request.session_id = formString(fields, "session_id");
    request.client_turn = formU64(fields, "client_turn");
    request.input_text = formString(fields, "input_text");
    request.request_projection = true;
    const auto action_key = formString(fields, "action_key");
    if (!action_key.empty()) request.action_key = action_key;
    const auto target = formString(fields, "target_object_ref");
    if (!target.empty()) request.target_object_ref = target;
    auto kind = formString(fields, "input_kind");
    if (kind.empty()) kind = request.action_key.has_value() ? "projection_action" : "free_text";
    auto kind_result = playableInputKindFromString(kind);
    if (kind_result.is_error()) return Result<H5PlayableRequest>::fail(kind_result.errors());
    request.input_kind = kind_result.value();
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<H5PlayableRequest>::fail(valid.errors());
    return Result<H5PlayableRequest>::ok(std::move(request));
}

Result<std::string> H5PlayableWireCodec::encodeResponse(const H5PlayableResponse& response) const {
    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    std::ostringstream out;
    out << "{";
    out << "\"ok\":" << (response.ok ? "true" : "false") << ",";
    out << "\"session_id\":" << q(response.session_id) << ",";
    out << "\"server_turn\":" << response.server_turn << ",";
    out << "\"tone\":" << q(toString(response.tone)) << ",";
    out << "\"reply_text\":" << safeText(response.reply_text) << ",";
    out << "\"projection\":" << projection(response.projection) << ",";
    out << "\"issues\":" << arrayOf(response.issues, issue) << ",";
    out << "\"debug_keys\":" << stringArray(response.debug_keys);
    out << "}";
    return Result<std::string>::ok(out.str());
}

} // namespace pathfinder::h5_playable
