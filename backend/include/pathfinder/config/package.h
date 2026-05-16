#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/config/validation.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::config {

// ConfigPackageId wraps a StrongId for config package identification
using ConfigPackageId = pathfinder::foundation::StrongId<pathfinder::foundation::ConfigPackIdTag>;

// ConfigVersion wraps a string for config version tracking
class ConfigVersion {
public:
    ConfigVersion() = default;
    explicit ConfigVersion(std::string value) : value_(std::move(value)) {}
    const std::string& value() const { return value_; }
    bool empty() const { return value_.empty(); }
    bool operator==(const ConfigVersion& other) const { return value_ == other.value_; }
    bool operator!=(const ConfigVersion& other) const { return value_ != other.value_; }
private:
    std::string value_;
};

// SchemaVersion wraps a string for config schema version
class SchemaVersion {
public:
    SchemaVersion() = default;
    explicit SchemaVersion(std::string value) : value_(std::move(value)) {}
    const std::string& value() const { return value_; }
    bool empty() const { return value_.empty(); }
    bool operator==(const SchemaVersion& other) const { return value_ == other.value_; }
    bool operator!=(const SchemaVersion& other) const { return value_ != other.value_; }
private:
    std::string value_;
};

// A single config file entry in the package
struct ConfigFileEntry {
    std::string path;
    std::string category;   // e.g. "objects", "traits", "actions"
    bool required = true;
};

// ConfigPackage holds metadata about a configuration package
class ConfigPackage {
public:
    ConfigPackage() = default;

    // Accessors
    const ConfigPackageId& packageId() const { return package_id_; }
    void setPackageId(ConfigPackageId id) { package_id_ = std::move(id); }

    const ConfigVersion& configVersion() const { return config_version_; }
    void setConfigVersion(ConfigVersion v) { config_version_ = std::move(v); }

    const SchemaVersion& schemaVersion() const { return schema_version_; }
    void setSchemaVersion(SchemaVersion v) { schema_version_ = std::move(v); }

    const std::vector<ConfigFileEntry>& files() const { return files_; }
    void addFile(ConfigFileEntry entry) { files_.push_back(std::move(entry)); }
    void setFiles(std::vector<ConfigFileEntry> entries) { files_ = std::move(entries); }

    const std::vector<std::string>& localeFiles() const { return locale_files_; }
    void setLocaleFiles(std::vector<std::string> paths) { locale_files_ = std::move(paths); }

    const std::vector<std::string>& testFiles() const { return test_files_; }
    void setTestFiles(std::vector<std::string> paths) { test_files_ = std::move(paths); }

    const pathfinder::foundation::HashValue& checksum() const { return checksum_; }
    void setChecksum(pathfinder::foundation::HashValue h) { checksum_ = h; }

    // P1 basic validation: checks non-empty required fields
    // Does NOT parse JSON, does NOT check cross-file references
    ConfigValidationReport validateBasic() const;

private:
    ConfigPackageId package_id_;
    ConfigVersion config_version_;
    SchemaVersion schema_version_;
    std::vector<ConfigFileEntry> files_;
    std::vector<std::string> locale_files_;
    std::vector<std::string> test_files_;
    pathfinder::foundation::HashValue checksum_;
};

} // namespace pathfinder::config
