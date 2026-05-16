#include <cassert>
#include <iostream>
#include "pathfinder/config/package.h"

using namespace pathfinder::config;
using namespace pathfinder::foundation;

static ConfigPackage makeValidPackage() {
    ConfigPackage pkg;
    pkg.setPackageId(ConfigPackageId("test_pack"));
    pkg.setConfigVersion(ConfigVersion("0.1.0"));
    pkg.setSchemaVersion(SchemaVersion("schema_1"));
    pkg.addFile(ConfigFileEntry{"objects/plants.json", "objects", true});
    pkg.addFile(ConfigFileEntry{"traits/food.json", "traits", true});
    return pkg;
}

void run_config_package_tests() {
    // Test 1: Valid package passes basic validation
    {
        ConfigPackage pkg = makeValidPackage();
        auto report = pkg.validateBasic();
        assert(!report.hasErrors());
        assert(report.issueCount() == 0);
    }

    // Test 2: Missing package_id fails
    {
        ConfigPackage pkg = makeValidPackage();
        pkg.setPackageId(ConfigPackageId(""));
        auto report = pkg.validateBasic();
        assert(report.hasErrors());
    }

    // Test 3: Missing config_version fails
    {
        ConfigPackage pkg = makeValidPackage();
        pkg.setConfigVersion(ConfigVersion(""));
        auto report = pkg.validateBasic();
        assert(report.hasErrors());
    }

    // Test 4: Missing schema_version fails
    {
        ConfigPackage pkg = makeValidPackage();
        pkg.setSchemaVersion(SchemaVersion(""));
        auto report = pkg.validateBasic();
        assert(report.hasErrors());
    }

    // Test 5: Empty file path fails
    {
        ConfigPackage pkg = makeValidPackage();
        pkg.addFile(ConfigFileEntry{"", "objects", true});
        auto report = pkg.validateBasic();
        assert(report.hasErrors());
    }

    // Test 6: No required files fails
    {
        ConfigPackage pkg;
        pkg.setPackageId(ConfigPackageId("test_pack"));
        pkg.setConfigVersion(ConfigVersion("0.1.0"));
        pkg.setSchemaVersion(SchemaVersion("schema_1"));
        pkg.addFile(ConfigFileEntry{"optional.json", "test", false});
        auto report = pkg.validateBasic();
        assert(report.hasErrors());
    }

    // Test 7: Accessors work correctly
    {
        ConfigPackage pkg = makeValidPackage();
        assert(pkg.packageId().value() == "test_pack");
        assert(pkg.configVersion().value() == "0.1.0");
        assert(pkg.schemaVersion().value() == "schema_1");
        assert(pkg.files().size() == 2);
        assert(pkg.files()[0].path == "objects/plants.json");
        assert(pkg.files()[0].category == "objects");
        assert(pkg.files()[0].required == true);
    }

    std::cout << "config_package_tests: all passed" << std::endl;
}
