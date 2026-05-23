#pragma once

#include <optional>
#include <string>
#include <vector>

namespace pathfinder::axmol_client {

struct CommandOptionView {
    std::string option_id;
    std::string command_kind;
    std::string label_text;
    bool enabled{false};
};

struct CommandSubmitResultView {
    bool accepted{false};
    bool requires_full_refresh{false};
    std::string result_kind;
    std::vector<std::string> failure_reason_keys;
};

class ClientCommandPort {
public:
    virtual ~ClientCommandPort() = default;

    virtual bool bootstrap() = 0;
    virtual bool refresh() = 0;
    virtual CommandSubmitResultView submitOption(const std::string& option_id) = 0;
    virtual bool reset() = 0;

    virtual std::vector<CommandOptionView> availableCommands() const = 0;
};

class NullClientCommandPort final : public ClientCommandPort {
public:
    bool bootstrap() override;
    bool refresh() override;
    CommandSubmitResultView submitOption(const std::string& option_id) override;
    bool reset() override;
    std::vector<CommandOptionView> availableCommands() const override;
};

} // namespace pathfinder::axmol_client
