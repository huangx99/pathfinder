#include "pathfinder/logging/logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace pathfinder::logging {
namespace {

void registerDefaultTags(std::unordered_map<std::string, bool>& tags) {
    tags[tag::App] = true;
    tags[tag::Ui] = true;
    tags[tag::Input] = true;
    tags[tag::Runtime] = true;
    tags[tag::Command] = true;
    tags[tag::Map] = true;
    tags[tag::Inventory] = true;
    tags[tag::Learning] = true;
    tags[tag::Agent] = true;
    tags[tag::Tool] = true;
    tags[tag::Error] = true;
}

std::string sanitizeMessage(std::string message) {
    for (auto& ch : message) {
        if (ch == '\n' || ch == '\r') ch = ' ';
    }
    return message;
}

} // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    registerDefaultTags(tags_);
}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(bool enabled, const std::string& log_file_path, bool truncate_old_file) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) stream_.close();
    enabled_ = enabled;
    log_file_path_ = log_file_path.empty() ? defaultLogPath() : log_file_path;
    registerDefaultTags(tags_);

    if (!enabled_) return;

    std::filesystem::path path(log_file_path_);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    stream_.open(log_file_path_, truncate_old_file ? std::ios::out : (std::ios::out | std::ios::app));
    if (stream_.is_open()) {
        stream_ << "# Pathfinder client log started at " << timestampText() << "\n";
        stream_ << "# file=" << log_file_path_ << "\n";
        stream_.flush();
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) {
        stream_ << "# Pathfinder client log stopped at " << timestampText() << "\n";
        stream_.flush();
        stream_.close();
    }
}

void Logger::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    if (!enabled_ && stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
}

bool Logger::enabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

void Logger::setTagEnabled(const std::string& tag, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    tags_[tag] = enabled;
}

bool Logger::tagEnabled(const std::string& tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tags_.find(tag);
    return it == tags_.end() ? true : it->second;
}

void Logger::registerCustomTag(const std::string& tag, bool enabled) {
    if (tag.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    tags_.try_emplace(tag, enabled);
}

void Logger::log(const std::string& tag, const std::string& message) {
    writeLine("info", tag, message);
}

void Logger::logError(const std::string& tag, const std::string& message) {
    writeLine("error", tag.empty() ? tag::Error : tag, message);
}

const std::string& Logger::logFilePath() const {
    return log_file_path_;
}

void Logger::writeLine(const std::string& level, const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!enabled_) return;
    auto it = tags_.find(tag);
    if (it != tags_.end() && !it->second) return;
    if (it == tags_.end()) tags_[tag] = true;

    ensureOpenLocked();
    if (!stream_.is_open()) return;

    stream_ << timestampText()
            << " [" << level << "]"
            << " [" << (tag.empty() ? "custom" : tag) << "] "
            << sanitizeMessage(message) << "\n";
    stream_.flush();
}

void Logger::ensureOpenLocked() {
    if (stream_.is_open()) return;
    if (log_file_path_.empty()) log_file_path_ = defaultLogPath();
    std::filesystem::path path(log_file_path_);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    stream_.open(log_file_path_, std::ios::out | std::ios::app);
}

std::string Logger::defaultLogPath() {
    std::error_code ec;
    auto root = std::filesystem::current_path(ec);
    if (ec) root = std::filesystem::temp_directory_path(ec);
    if (ec) return "pathfinder_client.log";
    return (root / "logs" / "pathfinder_client.log").string();
}

std::string Logger::timestampText() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms;
    return oss.str();
}

void log(const std::string& tag, const std::string& message) {
    Logger::instance().log(tag, message);
}

void logError(const std::string& tag, const std::string& message) {
    Logger::instance().logError(tag, message);
}

} // namespace pathfinder::logging
