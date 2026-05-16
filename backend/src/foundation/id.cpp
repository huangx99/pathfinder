#include "pathfinder/foundation/id.h"

namespace pathfinder::foundation {

bool isValidIdString(const std::string& value) {
    if (value.empty()) return false;
    for (char c : value) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return false;
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

} // namespace pathfinder::foundation
