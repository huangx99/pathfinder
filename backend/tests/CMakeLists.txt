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

# State tests (P2 + P3)
add_executable(pathfinder_tests_state
    unit/state/state_smoke_test.cpp
    unit/state/state_types_test.cpp
    unit/state/game_state_test.cpp
    unit/state/state_change_test.cpp
    unit/state/state_change_gate_test.cpp
    unit/state/actor_state_test.cpp
    unit/state/state_value_test.cpp
    unit/state/minimal_state_applier_test.cpp
)

target_link_libraries(pathfinder_tests_state PRIVATE
    pathfinder_state
)

add_test(
    NAME state_smoke
    COMMAND pathfinder_tests_state smoke
)

add_test(
    NAME state_types
    COMMAND pathfinder_tests_state types
)

add_test(
    NAME state_game_state
    COMMAND pathfinder_tests_state game_state
)

add_test(
    NAME state_change
    COMMAND pathfinder_tests_state state_change
)

add_test(
    NAME state_change_gate
    COMMAND pathfinder_tests_state state_change_gate
)

add_test(
    NAME state_actor_state
    COMMAND pathfinder_tests_state actor_state
)

add_test(
    NAME state_value
    COMMAND pathfinder_tests_state state_value
)

add_test(
    NAME state_minimal_applier
    COMMAND pathfinder_tests_state minimal_applier
)

# Event tests (P2)
add_executable(pathfinder_tests_event
    unit/event/event_smoke_test.cpp
    unit/event/event_types_test.cpp
    unit/event/event_record_test.cpp
    unit/event/event_stream_test.cpp
)

target_link_libraries(pathfinder_tests_event PRIVATE
    pathfinder_event
)

add_test(
    NAME event_smoke
    COMMAND pathfinder_tests_event smoke
)

add_test(
    NAME event_types
    COMMAND pathfinder_tests_event types
)

add_test(
    NAME event_record
    COMMAND pathfinder_tests_event record
)

add_test(
    NAME event_stream
    COMMAND pathfinder_tests_event stream
)

# Pipeline tests (P2)
add_executable(pathfinder_tests_pipeline
    unit/pipeline/pipeline_smoke_test.cpp
    unit/pipeline/pipeline_types_test.cpp
    unit/pipeline/pipeline_context_test.cpp
    unit/pipeline/pipeline_result_test.cpp
    unit/pipeline/rule_pipeline_test.cpp
)

target_link_libraries(pathfinder_tests_pipeline PRIVATE
    pathfinder_pipeline
)

add_test(
    NAME pipeline_smoke
    COMMAND pathfinder_tests_pipeline smoke
)

add_test(
    NAME pipeline_types
    COMMAND pathfinder_tests_pipeline types
)

add_test(
    NAME pipeline_context
    COMMAND pathfinder_tests_pipeline context
)

add_test(
    NAME pipeline_result
    COMMAND pathfinder_tests_pipeline result
)

add_test(
    NAME pipeline_rule_pipeline
    COMMAND pathfinder_tests_pipeline rule_pipeline
)

# Object tests (P3)
add_executable(pathfinder_tests_object
    unit/object/object_smoke_test.cpp
    unit/object/object_types_test.cpp
    unit/object/world_object_test.cpp
)

target_link_libraries(pathfinder_tests_object PRIVATE
    pathfinder_object
)

add_test(
    NAME object_smoke
    COMMAND pathfinder_tests_object smoke
)

add_test(
    NAME object_types
    COMMAND pathfinder_tests_object types
)

add_test(
    NAME object_world_object
    COMMAND pathfinder_tests_object world_object
)

# Cognition tests (P3)
add_executable(pathfinder_tests_cognition
    unit/cognition/cognition_smoke_test.cpp
    unit/cognition/cognition_types_test.cpp
    unit/cognition/cognition_state_test.cpp
)

target_link_libraries(pathfinder_tests_cognition PRIVATE
    pathfinder_cognition
)

add_test(
    NAME cognition_smoke
    COMMAND pathfinder_tests_cognition smoke
)

add_test(
    NAME cognition_types
    COMMAND pathfinder_tests_cognition types
)

add_test(
    NAME cognition_state
    COMMAND pathfinder_tests_cognition cognition_state
)

# Rules tests (P3)
add_executable(pathfinder_tests_rules
    unit/rules/rules_smoke_test.cpp
    unit/rules/unknown_fruit_fixture_test.cpp
    unit/rules/eat_object_resolver_test.cpp
)

target_link_libraries(pathfinder_tests_rules PRIVATE
    pathfinder_rules
)

add_test(
    NAME rules_smoke
    COMMAND pathfinder_tests_rules smoke
)

add_test(
    NAME rules_unknown_fruit_fixture
    COMMAND pathfinder_tests_rules unknown_fruit_fixture
)

add_test(
    NAME rules_eat_object_resolver
    COMMAND pathfinder_tests_rules eat_object_resolver
)

# P3 integration tests
add_executable(pathfinder_tests_p3
    integration/p3/p3_smoke_test.cpp
    integration/p3/unknown_fruit_eat_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p3 PRIVATE
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME p3_smoke
    COMMAND pathfinder_tests_p3 smoke
)

add_test(
    NAME p3_integration
    COMMAND pathfinder_tests_p3 integration
)

# Replay tests (P4)
add_executable(pathfinder_tests_replay
    unit/replay/command_replay_test.cpp
)

target_link_libraries(pathfinder_tests_replay PRIVATE
    pathfinder_replay
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME replay_smoke
    COMMAND pathfinder_tests_replay smoke
)

add_test(
    NAME replay_record_valid
    COMMAND pathfinder_tests_replay valid_record_passes
)

add_test(
    NAME replay_record_missing_id
    COMMAND pathfinder_tests_replay missing_record_id_fails
)

add_test(
    NAME replay_record_missing_cmd_id
    COMMAND pathfinder_tests_replay missing_command_id_fails
)

add_test(
    NAME replay_record_unknown_status
    COMMAND pathfinder_tests_replay unknown_status_fails
)

add_test(
    NAME replay_record_invalid_record_id
    COMMAND pathfinder_tests_replay invalid_record_id_format_fails
)

add_test(
    NAME replay_record_invalid_command_id
    COMMAND pathfinder_tests_replay invalid_command_id_format_fails
)

add_test(
    NAME replay_record_command_id_mismatch
    COMMAND pathfinder_tests_replay command_id_mismatch_fails
)

add_test(
    NAME replay_record_empty_input_hash
    COMMAND pathfinder_tests_replay empty_input_hash_fails
)

add_test(
    NAME replay_record_empty_output_hash
    COMMAND pathfinder_tests_replay empty_output_hash_fails
)

add_test(
    NAME replay_append_invalid_record
    COMMAND pathfinder_tests_replay append_invalid_record_fails
)

add_test(
    NAME replay_append_invalid_entry
    COMMAND pathfinder_tests_replay append_invalid_entry_fails
)

add_test(
    NAME replay_record_duplicate
    COMMAND pathfinder_tests_replay duplicate_record_id_rejected
)

add_test(
    NAME replay_record_find
    COMMAND pathfinder_tests_replay find_by_command_id
)

add_test(
    NAME replay_log_empty_valid
    COMMAND pathfinder_tests_replay empty_log_valid
)

add_test(
    NAME replay_status_enum
    COMMAND pathfinder_tests_replay status_enum_roundtrip
)

add_test(
    NAME replay_random_valid
    COMMAND pathfinder_tests_replay valid_entry_passes
)

add_test(
    NAME replay_random_missing_id
    COMMAND pathfinder_tests_replay missing_entry_id_fails
)

add_test(
    NAME replay_random_min_max
    COMMAND pathfinder_tests_replay min_greater_than_max_fails
)

add_test(
    NAME replay_random_range
    COMMAND pathfinder_tests_replay result_out_of_range_fails
)

add_test(
    NAME replay_random_find
    COMMAND pathfinder_tests_replay append_and_find
)

add_test(
    NAME replay_random_duplicate
    COMMAND pathfinder_tests_replay duplicate_entry_rejected
)

add_test(
    NAME replay_random_stable
    COMMAND pathfinder_tests_replay stable_seed_comparison
)

add_test(
    NAME replay_random_negative_draw_index
    COMMAND pathfinder_tests_replay negative_draw_index_fails
)

add_test(
    NAME replay_runner_p3_match
    COMMAND pathfinder_tests_replay p3_eat_replay_matches
)

add_test(
    NAME replay_runner_same_input
    COMMAND pathfinder_tests_replay same_input_gets_match
)

add_test(
    NAME replay_runner_tampered
    COMMAND pathfinder_tests_replay tampered_output_mismatch
)

add_test(
    NAME replay_runner_empty_log
    COMMAND pathfinder_tests_replay empty_log_returns_error
)

add_test(
    NAME replay_runner_random_missing
    COMMAND pathfinder_tests_replay random_log_missing_seed_fails
)

add_test(
    NAME replay_runner_seed_mismatch
    COMMAND pathfinder_tests_replay random_seed_mismatch_fails
)

# Protocol tests (P4)
add_executable(pathfinder_tests_protocol
    unit/protocol/protocol_smoke_test.cpp
)

target_link_libraries(pathfinder_tests_protocol PRIVATE
    pathfinder_protocol
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME protocol_smoke
    COMMAND pathfinder_tests_protocol smoke
)

add_test(
    NAME protocol_message_type_tostring
    COMMAND pathfinder_tests_protocol message_type_tostring
)

add_test(
    NAME protocol_message_type_fromstring
    COMMAND pathfinder_tests_protocol message_type_fromstring
)

add_test(
    NAME protocol_domain_tostring
    COMMAND pathfinder_tests_protocol domain_tostring
)

add_test(
    NAME protocol_domain_fromstring
    COMMAND pathfinder_tests_protocol domain_fromstring
)

add_test(
    NAME protocol_payload_type_tostring
    COMMAND pathfinder_tests_protocol payload_type_tostring
)

add_test(
    NAME protocol_payload_type_fromstring
    COMMAND pathfinder_tests_protocol payload_type_fromstring
)

add_test(
    NAME protocol_envelope_valid
    COMMAND pathfinder_tests_protocol envelope_valid_passes
)

add_test(
    NAME protocol_envelope_no_version
    COMMAND pathfinder_tests_protocol envelope_missing_version_fails
)

add_test(
    NAME protocol_envelope_no_msg_id
    COMMAND pathfinder_tests_protocol envelope_missing_message_id_fails
)

add_test(
    NAME protocol_envelope_bad_msg_id
    COMMAND pathfinder_tests_protocol envelope_invalid_message_id_fails
)

add_test(
    NAME protocol_envelope_unknown_type
    COMMAND pathfinder_tests_protocol envelope_unknown_message_type_fails
)

add_test(
    NAME protocol_envelope_unknown_domain
    COMMAND pathfinder_tests_protocol envelope_unknown_domain_fails
)

add_test(
    NAME protocol_envelope_unknown_payload
    COMMAND pathfinder_tests_protocol envelope_unknown_payload_type_fails
)

add_test(
    NAME protocol_envelope_with_errors
    COMMAND pathfinder_tests_protocol envelope_with_errors_passes
)

add_test(
    NAME protocol_adapter_success
    COMMAND pathfinder_tests_protocol success_pipeline_to_envelope
)

add_test(
    NAME protocol_adapter_failed
    COMMAND pathfinder_tests_protocol failed_pipeline_to_envelope
)

add_test(
    NAME protocol_adapter_no_draft
    COMMAND pathfinder_tests_protocol envelope_no_state_change_draft
)

# P4 integration tests
add_executable(pathfinder_tests_p4
    integration/p4/replay_protocol_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p4 PRIVATE
    pathfinder_replay
    pathfinder_protocol
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME p4_smoke
    COMMAND pathfinder_tests_p4 smoke
)

add_test(
    NAME p4_replay_protocol_flow
    COMMAND pathfinder_tests_p4 replay_protocol_flow
)

# Agent unit tests (P5)
add_executable(pathfinder_tests_agent
    unit/agent/agent_smoke_test.cpp
    unit/agent/agent_types_test.cpp
    unit/agent/agent_definition_test.cpp
    unit/agent/agent_binding_test.cpp
    unit/agent/agent_observation_test.cpp
    unit/agent/agent_action_space_test.cpp
    unit/agent/agent_intent_test.cpp
    unit/agent/agent_command_adapter_test.cpp
)

target_link_libraries(pathfinder_tests_agent PRIVATE
    pathfinder_agent
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME agent_smoke
    COMMAND pathfinder_tests_agent smoke
)

add_test(
    NAME agent_types
    COMMAND pathfinder_tests_agent types
)

add_test(
    NAME agent_definition
    COMMAND pathfinder_tests_agent definition
)

add_test(
    NAME agent_binding
    COMMAND pathfinder_tests_agent binding
)

add_test(
    NAME agent_observation
    COMMAND pathfinder_tests_agent observation
)

add_test(
    NAME agent_action_space
    COMMAND pathfinder_tests_agent action_space
)

add_test(
    NAME agent_intent
    COMMAND pathfinder_tests_agent intent
)

add_test(
    NAME agent_command_adapter
    COMMAND pathfinder_tests_agent command_adapter
)

# P5 integration tests
add_executable(pathfinder_tests_p5
    integration/p5/agent_unknown_fruit_flow_test.cpp
    integration/p5/spider_flee_fire_test.cpp
    integration/p5/wolf_call_pack_test.cpp
)

target_link_libraries(pathfinder_tests_p5 PRIVATE
    pathfinder_agent
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME p5_smoke
    COMMAND pathfinder_tests_p5 smoke
)

add_test(
    NAME p5_agent_unknown_fruit_flow
    COMMAND pathfinder_tests_p5 agent_unknown_fruit_flow
)

add_test(
    NAME p5_spider_flee_fire
    COMMAND pathfinder_tests_p5 spider_flee_fire
)

add_test(
    NAME p5_wolf_call_pack
    COMMAND pathfinder_tests_p5 wolf_call_pack
)

# Agent Builder unit tests (P6)
add_executable(pathfinder_tests_agent_builders
    unit/agent/agent_builder_smoke_test.cpp
    unit/agent/agent_builder_types_test.cpp
    unit/agent/agent_observation_builder_test.cpp
    unit/agent/agent_action_space_builder_test.cpp
)

target_link_libraries(pathfinder_tests_agent_builders PRIVATE
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME agent_builder_smoke
    COMMAND pathfinder_tests_agent_builders smoke
)

add_test(
    NAME agent_builder_types
    COMMAND pathfinder_tests_agent_builders builder_types
)

add_test(
    NAME agent_observation_builder
    COMMAND pathfinder_tests_agent_builders observation_builder
)

add_test(
    NAME agent_action_space_builder
    COMMAND pathfinder_tests_agent_builders action_space_builder
)

# P6 integration tests
add_executable(pathfinder_tests_p6
    integration/p6/agent_builder_unknown_fruit_flow_test.cpp
    integration/p6/spider_fire_action_space_test.cpp
    integration/p6/wolf_pack_action_space_test.cpp
)

target_link_libraries(pathfinder_tests_p6 PRIVATE
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME p6_smoke
    COMMAND pathfinder_tests_p6 smoke
)

add_test(
    NAME p6_agent_builder_unknown_fruit_flow
    COMMAND pathfinder_tests_p6 agent_builder_unknown_fruit_flow
)

add_test(
    NAME p6_spider_fire_action_space
    COMMAND pathfinder_tests_p6 spider_fire_action_space
)

add_test(
    NAME p6_wolf_pack_action_space
    COMMAND pathfinder_tests_p6 wolf_pack_action_space
)

# Agent Runtime unit tests (P7)
add_executable(pathfinder_tests_agent_runtime
    unit/agent/agent_runtime_smoke_test.cpp
    unit/agent/agent_policy_test.cpp
    unit/agent/agent_runtime_types_test.cpp
    unit/agent/agent_runtime_test.cpp
)

target_link_libraries(pathfinder_tests_agent_runtime PRIVATE
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME agent_runtime_smoke
    COMMAND pathfinder_tests_agent_runtime smoke
)

add_test(
    NAME agent_policy
    COMMAND pathfinder_tests_agent_runtime policy
)

add_test(
    NAME agent_runtime_types
    COMMAND pathfinder_tests_agent_runtime runtime_types
)

add_test(
    NAME agent_runtime
    COMMAND pathfinder_tests_agent_runtime runtime
)

# P7 integration tests
add_executable(pathfinder_tests_p7
    integration/p7/agent_runtime_unknown_fruit_flow_test.cpp
    integration/p7/agent_runtime_threat_no_submit_test.cpp
)

target_link_libraries(pathfinder_tests_p7 PRIVATE
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_pipeline
    pathfinder_rules
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

add_test(
    NAME p7_smoke
    COMMAND pathfinder_tests_p7 smoke
)

add_test(
    NAME p7_agent_runtime_unknown_fruit_flow
    COMMAND pathfinder_tests_p7 p7_agent_runtime_unknown_fruit_flow
)

add_test(
    NAME p7_agent_runtime_threat_no_submit
    COMMAND pathfinder_tests_p7 p7_agent_runtime_threat_no_submit
)
