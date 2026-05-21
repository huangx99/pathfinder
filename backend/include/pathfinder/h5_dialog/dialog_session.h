// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include <string>
#include <unordered_map>
#include <mutex>

namespace pathfinder::h5_dialog {

class DialogSessionStore {
public:
    virtual ~DialogSessionStore() = default;

    virtual pathfinder::foundation::Result<DialogSessionState> loadOrCreate(
        const std::string& session_id) = 0;

    virtual pathfinder::foundation::Result<void> save(
        const DialogSessionState& state) = 0;

    virtual pathfinder::foundation::Result<void> reset(
        const std::string& session_id) = 0;
};

class InMemoryDialogSessionStore : public DialogSessionStore {
public:
    pathfinder::foundation::Result<DialogSessionState> loadOrCreate(
        const std::string& session_id) override;

    pathfinder::foundation::Result<void> save(
        const DialogSessionState& state) override;

    pathfinder::foundation::Result<void> reset(
        const std::string& session_id) override;

private:
    std::unordered_map<std::string, DialogSessionState> sessions_;
    std::mutex mutex_;
};

} // namespace pathfinder::h5_dialog
