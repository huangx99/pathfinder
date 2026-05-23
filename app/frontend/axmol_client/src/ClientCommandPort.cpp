#include "ClientCommandPort.h"

namespace pathfinder::axmol_client {

bool NullClientCommandPort::bootstrap() {
    return true;
}

bool NullClientCommandPort::refresh() {
    return true;
}

CommandSubmitResultView NullClientCommandPort::submitOption(const std::string& option_id) {
    CommandSubmitResultView result;
    result.accepted = false;
    result.result_kind = "blocked";
    result.failure_reason_keys.push_back(option_id.empty() ? "missing_option_id" : "transport_not_connected");
    return result;
}

bool NullClientCommandPort::reset() {
    return true;
}

std::vector<CommandOptionView> NullClientCommandPort::availableCommands() const {
    return {};
}

} // namespace pathfinder::axmol_client
