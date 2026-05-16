#include "pathfinder/config/package.h"

namespace pathfinder::config {

ConfigValidationReport ConfigPackage::validateBasic() const {
    ConfigValidationReport report;

    // Check package_id non-empty
    if (package_id_.empty()) {
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            pathfinder::foundation::ErrorCode::id_missing,
            "ConfigPackage: package_id is empty"
        });
    }

    // Check config_version non-empty
    if (config_version_.empty()) {
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            pathfinder::foundation::ErrorCode::validation_config_invalid,
            "ConfigPackage: config_version is empty"
        });
    }

    // Check schema_version non-empty
    if (schema_version_.empty()) {
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            pathfinder::foundation::ErrorCode::validation_config_invalid,
            "ConfigPackage: schema_version is empty"
        });
    }

    // Check file paths non-empty
    for (size_t i = 0; i < files_.size(); ++i) {
        if (files_[i].path.empty()) {
            report.addIssue(ConfigValidationIssue{
                ConfigValidationSeverity::Error,
                pathfinder::foundation::ErrorCode::validation_config_invalid,
                "ConfigPackage: file path is empty at index " + std::to_string(i)
            });
        }
    }

    // Check at least one required file exists
    bool has_required = false;
    for (const auto& file : files_) {
        if (file.required) {
            has_required = true;
            break;
        }
    }
    if (!has_required) {
        report.addIssue(ConfigValidationIssue{
            ConfigValidationSeverity::Error,
            pathfinder::foundation::ErrorCode::validation_config_invalid,
            "ConfigPackage: no required config files defined"
        });
    }

    return report;
}

} // namespace pathfinder::config
