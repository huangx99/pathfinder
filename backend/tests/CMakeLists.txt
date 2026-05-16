# Foundation tests
add_executable(pathfinder_tests_foundation
    unit/foundation/smoke_test.cpp
    unit/foundation/error_test.cpp
    unit/foundation/result_void_test.cpp
    unit/foundation/result_value_test.cpp
    unit/foundation/id_basic_test.cpp
    unit/foundation/id_validation_test.cpp
    unit/foundation/version_tick_test.cpp
    unit/foundation/hash_test.cpp
    unit/foundation/random_seed_test.cpp
    unit/foundation/serialization_contract_test.cpp
)

target_link_libraries(pathfinder_tests_foundation PRIVATE
    pathfinder_foundation
)

add_test(
    NAME foundation_smoke
    COMMAND pathfinder_tests_foundation smoke
)

add_test(
    NAME foundation_error
    COMMAND pathfinder_tests_foundation error
)

add_test(
    NAME foundation_result_void
    COMMAND pathfinder_tests_foundation result_void
)

add_test(
    NAME foundation_result_value
    COMMAND pathfinder_tests_foundation result_value
)

add_test(
    NAME foundation_id_basic
    COMMAND pathfinder_tests_foundation id_basic
)

add_test(
    NAME foundation_id_validation
    COMMAND pathfinder_tests_foundation id_validation
)

add_test(
    NAME foundation_version_tick
    COMMAND pathfinder_tests_foundation version_tick
)

add_test(
    NAME foundation_hash
    COMMAND pathfinder_tests_foundation hash
)

add_test(
    NAME foundation_random_seed
    COMMAND pathfinder_tests_foundation random_seed
)

add_test(
    NAME foundation_serialization_contract
    COMMAND pathfinder_tests_foundation serialization_contract
)

# Config tests (P1)
add_executable(pathfinder_tests_config
    unit/config/config_smoke_test.cpp
    unit/config/config_validation_test.cpp
    unit/config/config_package_test.cpp
    unit/config/config_loader_test.cpp
)

target_link_libraries(pathfinder_tests_config PRIVATE
    pathfinder_config
)

add_test(
    NAME config_smoke
    COMMAND pathfinder_tests_config smoke
)

add_test(
    NAME config_validation
    COMMAND pathfinder_tests_config validation
)

add_test(
    NAME config_package
    COMMAND pathfinder_tests_config package
)

add_test(
    NAME config_loader
    COMMAND pathfinder_tests_config loader
)

# Command tests (P1)
add_executable(pathfinder_tests_command
    unit/command/command_smoke_test.cpp
    unit/command/command_types_test.cpp
    unit/command/action_target_test.cpp
    unit/command/action_command_test.cpp
    unit/command/command_envelope_test.cpp
    unit/command/command_validation_test.cpp
)

target_link_libraries(pathfinder_tests_command PRIVATE
    pathfinder_command
)

add_test(
    NAME command_smoke
    COMMAND pathfinder_tests_command smoke
)

add_test(
    NAME command_types
    COMMAND pathfinder_tests_command types
)

add_test(
    NAME command_target
    COMMAND pathfinder_tests_command target
)

add_test(
    NAME command_action_command
    COMMAND pathfinder_tests_command action_command
)

add_test(
    NAME command_envelope
    COMMAND pathfinder_tests_command envelope
)

add_test(
    NAME command_validation
    COMMAND pathfinder_tests_command validation
)
