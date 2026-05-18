#include "pathfinder/h5_dialog/dialog_wire_codec.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

std::string trim(const std::string& s) {
    auto start = s.begin();
    auto end = s.end();
    while (start != end && std::isspace(*start)) ++start;
    while (end != start && std::isspace(*(end - 1))) --end;
    return std::string(start, end);
}

std::string urlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int val = 0;
            for (int j = 1; j <= 2; ++j) {
                char c = s[i + j];
                val *= 16;
                if (c >= '0' && c <= '9') val += c - '0';
                else if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
                else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
            }
            out.push_back(static_cast<char>(val));
            i += 2;
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 10);
    for (unsigned char c : s) {
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
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    return out;
}

std::string encodeStringArray(const std::vector<std::string>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << jsonEscape(arr[i]) << "\"";
    }
    oss << "]";
    return oss.str();
}

std::string encodeQuickActions(const std::vector<DialogQuickActionDto>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{";
        oss << "\"label_text\":\"" << jsonEscape(arr[i].label_text) << "\",";
        oss << "\"input_text\":\"" << jsonEscape(arr[i].input_text) << "\",";
        oss << "\"intent_kind\":\"" << jsonEscape(toString(arr[i].intent_kind)) << "\",";
        oss << "\"enabled\":" << (arr[i].enabled ? "true" : "false");
        oss << "}";
    }
    oss << "]";
    return oss.str();
}

} // namespace

Result<DialogRequestDto> DialogWireCodec::parseFormUrlEncodedRequest(const std::string& body) const {
    DialogRequestDto req;
    std::istringstream iss(body);
    std::string token;
    while (std::getline(iss, token, '&')) {
        auto pos = token.find('=');
        if (pos == std::string::npos) continue;
        auto key = trim(urlDecode(token.substr(0, pos)));
        auto value = trim(urlDecode(token.substr(pos + 1)));
        if (key == "session_id") req.session_id = value;
        else if (key == "input_text") req.input_text = value;
        else if (key == "client_turn") {
            try {
                req.client_turn = std::stoull(value);
            } catch (...) {
                req.client_turn = 0;
            }
        }
    }
    return Result<DialogRequestDto>::ok(req);
}

Result<std::string> DialogWireCodec::encodeJsonResponse(const DialogResponseDto& response) const {
    auto valid_r = response.validateBasic();
    if (valid_r.is_error()) {
        return Result<std::string>::fail(valid_r.errors());
    }
    std::ostringstream oss;
    oss << "{";
    oss << "\"session_id\":\"" << jsonEscape(response.session_id) << "\",";
    oss << "\"decision\":\"" << jsonEscape(toString(response.decision)) << "\",";
    oss << "\"reply_text\":\"" << jsonEscape(response.reply_text) << "\",";
    oss << "\"state_summary_lines\":" << encodeStringArray(response.state_summary_lines) << ",";
    oss << "\"actor_knowledge_lines\":" << encodeStringArray(response.actor_knowledge_lines) << ",";
    oss << "\"recipient_knowledge_lines\":" << encodeStringArray(response.recipient_knowledge_lines) << ",";
    oss << "\"quick_actions\":" << encodeQuickActions(response.quick_actions) << ",";
    oss << "\"debug_keys\":" << encodeStringArray(response.debug_keys) << ",";
    oss << "\"warning_keys\":" << encodeStringArray(response.warning_keys);
    oss << "}";
    return Result<std::string>::ok(oss.str());
}

} // namespace pathfinder::h5_dialog
