#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

namespace pathfinder::logging {

namespace tag {
inline constexpr const char* App = "app";
inline constexpr const char* Ui = "ui";
inline constexpr const char* Input = "input";
inline constexpr const char* Runtime = "runtime";
inline constexpr const char* Command = "command";
inline constexpr const char* Map = "map";
inline constexpr const char* Inventory = "inventory";
inline constexpr const char* Learning = "learning";
inline constexpr const char* Agent = "agent";
inline constexpr const char* Tool = "tool";
inline constexpr const char* Error = "error";
} // namespace tag

class Logger {
public:
    static Logger& instance();

    void initialize(bool enabled, const std::string& log_file_path = {}, bool truncate_old_file = true);
    void shutdown();

    void setEnabled(bool enabled);
    bool enabled() const;

    void setTagEnabled(const std::string& tag, bool enabled);
    bool tagEnabled(const std::string& tag) const;
    void registerCustomTag(const std::string& tag, bool enabled = true);

    void log(const std::string& tag, const std::string& message);
    void logError(const std::string& tag, const std::string& message);

    const std::string& logFilePath() const;

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeLine(const std::string& level, const std::string& tag, const std::string& message);
    void ensureOpenLocked();
    static std::string defaultLogPath();
    static std::string timestampText();

    mutable std::mutex mutex_;
    bool enabled_{false};
    std::string log_file_path_;
    std::unordered_map<std::string, bool> tags_;
    std::ofstream stream_;
};

void log(const std::string& tag, const std::string& message);
void logError(const std::string& tag, const std::string& message);

} // namespace pathfinder::logging
