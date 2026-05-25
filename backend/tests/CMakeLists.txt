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

# Cognition tests (P3 + P15)
add_executable(pathfinder_tests_cognition
    unit/cognition/cognition_smoke_test.cpp
    unit/cognition/cognition_types_test.cpp
    unit/cognition/cognition_state_test.cpp
    unit/cognition/cognition_v2_types_test.cpp
    unit/cognition/cognition_confidence_model_test.cpp
    unit/cognition/cognition_query_service_test.cpp
    unit/cognition/cognition_v2_update_test.cpp
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

add_test(
    NAME cognition_v2_types
    COMMAND pathfinder_tests_cognition v2_types
)

add_test(
    NAME cognition_confidence_model
    COMMAND pathfinder_tests_cognition confidence_model
)

add_test(
    NAME cognition_query_service
    COMMAND pathfinder_tests_cognition query_service
)

add_test(
    NAME cognition_v2_update
    COMMAND pathfinder_tests_cognition v2_update
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

# P15 integration tests
add_executable(pathfinder_tests_p15
    integration/p15/p15_main.cpp
    integration/p15/cognition_eat_feedback_flow_test.cpp
    integration/p15/cognition_use_feedback_flow_test.cpp
    integration/p15/cognition_hidden_truth_boundary_test.cpp
)

target_link_libraries(pathfinder_tests_p15 PRIVATE
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
    NAME p15_smoke
    COMMAND pathfinder_tests_p15 smoke
)

add_test(
    NAME p15_eat_feedback_flow
    COMMAND pathfinder_tests_p15 eat_feedback
)

add_test(
    NAME p15_use_feedback_flow
    COMMAND pathfinder_tests_p15 use_feedback
)

add_test(
    NAME p15_hidden_truth_boundary
    COMMAND pathfinder_tests_p15 hidden_truth
)

# Memory unit tests (P16)
add_executable(pathfinder_tests_memory_types
    unit/memory/memory_types_test.cpp
)

target_link_libraries(pathfinder_tests_memory_types PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_types_roundtrip
    COMMAND pathfinder_tests_memory_types types
)

add_executable(pathfinder_tests_memory_record
    unit/memory/memory_record_test.cpp
)

target_link_libraries(pathfinder_tests_memory_record PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_record_validate_basic
    COMMAND pathfinder_tests_memory_record record
)

add_executable(pathfinder_tests_memory_store
    unit/memory/memory_store_test.cpp
)

target_link_libraries(pathfinder_tests_memory_store PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_store_put_find_query
    COMMAND pathfinder_tests_memory_store store
)

add_executable(pathfinder_tests_memory_writer
    unit/memory/memory_writer_test.cpp
)

target_link_libraries(pathfinder_tests_memory_writer PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_writer_creates_short_term
    COMMAND pathfinder_tests_memory_writer writer
)

add_executable(pathfinder_tests_memory_decay
    unit/memory/memory_decay_test.cpp
)

target_link_libraries(pathfinder_tests_memory_decay PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_decay_short_term
    COMMAND pathfinder_tests_memory_decay decay
)

# P16 integration tests
add_executable(pathfinder_tests_p16_memory_from_cognition
    integration/p16/memory_from_cognition_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p16_memory_from_cognition PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p16_memory_from_cognition
    COMMAND pathfinder_tests_p16_memory_from_cognition from_cognition
)

add_test(
    NAME p16_memory_repeated_evidence_promotes
    COMMAND pathfinder_tests_p16_memory_from_cognition repeated_evidence_promotes
)

add_test(
    NAME p16_memory_critical_risk_protected
    COMMAND pathfinder_tests_p16_memory_from_cognition critical_risk_protected
)

add_executable(pathfinder_tests_p16_memory_decay
    integration/p16/memory_decay_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p16_memory_decay PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p16_memory_decay_flow
    COMMAND pathfinder_tests_p16_memory_decay decay_flow
)

add_executable(pathfinder_tests_p16_memory_security
    integration/p16/memory_boundary_security_test.cpp
)

target_link_libraries(pathfinder_tests_p16_memory_security PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p16_memory_security_hidden_truth
    COMMAND pathfinder_tests_p16_memory_security hidden_truth
)

add_test(
    NAME p16_memory_security_raw_state
    COMMAND pathfinder_tests_p16_memory_security raw_state
)

add_test(
    NAME p16_memory_security_test_only_gate
    COMMAND pathfinder_tests_p16_memory_security test_only_gate
)

# Memory unit tests (P17)
add_executable(pathfinder_tests_memory_summary
    unit/memory/memory_summary_test.cpp
)

target_link_libraries(pathfinder_tests_memory_summary PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_summary_validate_basic
    COMMAND pathfinder_tests_memory_summary summary
)

add_executable(pathfinder_tests_memory_compression
    unit/memory/memory_compression_test.cpp
)

target_link_libraries(pathfinder_tests_memory_compression PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_compression_validate_basic
    COMMAND pathfinder_tests_memory_compression compression
)

add_executable(pathfinder_tests_memory_recall
    unit/memory/memory_recall_test.cpp
)

target_link_libraries(pathfinder_tests_memory_recall PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME memory_recall_validate_basic
    COMMAND pathfinder_tests_memory_recall recall
)

# P17 integration tests
add_executable(pathfinder_tests_p17_memory_compression
    integration/p17/memory_compression_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p17_memory_compression PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p17_memory_compression_flow
    COMMAND pathfinder_tests_p17_memory_compression compression_flow
)

add_executable(pathfinder_tests_p17_memory_recall
    integration/p17/memory_recall_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p17_memory_recall PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p17_memory_recall_flow
    COMMAND pathfinder_tests_p17_memory_recall recall_flow
)

add_executable(pathfinder_tests_p17_memory_security
    integration/p17/memory_boundary_security_test.cpp
)

target_link_libraries(pathfinder_tests_p17_memory_security PRIVATE
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p17_memory_security_hidden_truth
    COMMAND pathfinder_tests_p17_memory_security hidden_truth
)

add_test(
    NAME p17_memory_security_raw_state
    COMMAND pathfinder_tests_p17_memory_security raw_state
)

add_test(
    NAME p17_memory_security_test_only_gate
    COMMAND pathfinder_tests_p17_memory_security test_only_gate
)

# Knowledge unit tests (P18)
add_executable(pathfinder_tests_knowledge_types
    unit/knowledge/knowledge_types_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_types PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_types_roundtrip
    COMMAND pathfinder_tests_knowledge_types types
)

add_executable(pathfinder_tests_knowledge_claim
    unit/knowledge/knowledge_claim_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_claim PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_claim_validate_basic
    COMMAND pathfinder_tests_knowledge_claim claim
)

add_executable(pathfinder_tests_knowledge_repository
    unit/knowledge/knowledge_repository_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_repository PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_repository_query_order
    COMMAND pathfinder_tests_knowledge_repository repository
)

add_executable(pathfinder_tests_knowledge_formation
    unit/knowledge/knowledge_formation_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_formation PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_formation_planner_service
    COMMAND pathfinder_tests_knowledge_formation formation
)

add_executable(pathfinder_tests_knowledge_projection
    unit/knowledge/knowledge_projection_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_projection PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_projection_builder
    COMMAND pathfinder_tests_knowledge_projection projection
)

# P18 integration tests
add_executable(pathfinder_tests_p18_knowledge_from_memory
    integration/p18/knowledge_from_memory_summary_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p18_knowledge_from_memory PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p18_knowledge_from_memory_summary_flow
    COMMAND pathfinder_tests_p18_knowledge_from_memory flow
)

add_executable(pathfinder_tests_p18_knowledge_projection
    integration/p18/knowledge_projection_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p18_knowledge_projection PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p18_knowledge_projection_flow
    COMMAND pathfinder_tests_p18_knowledge_projection projection_flow
)

add_executable(pathfinder_tests_p18_knowledge_security
    integration/p18/knowledge_boundary_security_test.cpp
)

target_link_libraries(pathfinder_tests_p18_knowledge_security PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p18_knowledge_security_hidden_truth
    COMMAND pathfinder_tests_p18_knowledge_security hidden_truth
)

add_test(
    NAME p18_knowledge_security_test_only_gate
    COMMAND pathfinder_tests_p18_knowledge_security test_only_gate
)

# P19 unit tests
add_executable(pathfinder_tests_knowledge_revision
    unit/knowledge/knowledge_revision_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_revision PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_revision_options_validate
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_revision_options_validate_basic
)

add_test(
    NAME knowledge_revision_conflict_signal_validate
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_conflict_signal_validate_basic
)

add_test(
    NAME knowledge_revision_input_owner_mismatch
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_revision_input_owner_mismatch_rejected
)

add_test(
    NAME knowledge_revision_input_invalid_claim
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_revision_input_invalid_existing_claim_rejected
)

add_test(
    NAME knowledge_revision_assessment_invalid_enum
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_revision_assessment_invalid_enum_rejected
)

add_test(
    NAME knowledge_revision_patch_disproven_requires_negative
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_claim_patch_disproven_requires_negative_evidence
)

add_test(
    NAME knowledge_revision_plan_validate
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_revision_plan_ok_dto_validate_basic
)

add_test(
    NAME knowledge_revision_result_validate
    COMMAND pathfinder_tests_knowledge_revision test_knowledge_revision_result_ok_dto_validate_basic
)

add_test(
    NAME knowledge_revision_reinforce
    COMMAND pathfinder_tests_knowledge_revision test_supporting_evidence_reinforces
)

add_test(
    NAME knowledge_revision_weaken
    COMMAND pathfinder_tests_knowledge_revision test_weak_contradiction_lowers_confidence
)

add_test(
    NAME knowledge_revision_disprove
    COMMAND pathfinder_tests_knowledge_revision test_strong_contradiction_disproves
)

add_test(
    NAME knowledge_revision_specialize
    COMMAND pathfinder_tests_knowledge_revision test_condition_mismatch_creates_specialized_claim
)

add_test(
    NAME knowledge_revision_skipped
    COMMAND pathfinder_tests_knowledge_revision test_no_change_skipped_valid
)

add_test(
    NAME knowledge_revision_repo_store_deprecated
    COMMAND pathfinder_tests_knowledge_revision test_repository_stores_deprecated_disproven
)

add_test(
    NAME knowledge_revision_repo_query_exclude
    COMMAND pathfinder_tests_knowledge_revision test_repository_default_query_excludes_deprecated_disproven
)

add_test(
    NAME knowledge_revision_repo_query_include
    COMMAND pathfinder_tests_knowledge_revision test_repository_include_deprecated_disproven
)

add_test(
    NAME knowledge_revision_projection_skip
    COMMAND pathfinder_tests_knowledge_revision test_projection_builder_skips_deprecated_disproven
)

add_test(
    NAME knowledge_revision_hidden_truth
    COMMAND pathfinder_tests_knowledge_revision test_hidden_truth_rejected
)

add_test(
    NAME knowledge_revision_test_only_gate
    COMMAND pathfinder_tests_knowledge_revision test_test_only_enum_rejected
)

add_test(
    NAME knowledge_revision_invalid_fail
    COMMAND pathfinder_tests_knowledge_revision test_invalid_claim_input_returns_fail
)

# P19 integration tests
add_executable(pathfinder_tests_p19_knowledge_revision_flow
    integration/p19/knowledge_revision_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p19_knowledge_revision_flow PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p19_knowledge_revision_flow
    COMMAND pathfinder_tests_p19_knowledge_revision_flow
)

add_executable(pathfinder_tests_p19_knowledge_revision_security
    integration/p19/knowledge_revision_security_test.cpp
)

target_link_libraries(pathfinder_tests_p19_knowledge_revision_security PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p19_knowledge_revision_security
    COMMAND pathfinder_tests_p19_knowledge_revision_security
)

# P20 unit tests
add_executable(pathfinder_tests_knowledge_propagation
    unit/knowledge/knowledge_propagation_test.cpp
)

target_link_libraries(pathfinder_tests_knowledge_propagation PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME knowledge_propagation_enum_roundtrip
    COMMAND pathfinder_tests_knowledge_propagation enum_roundtrip
)

add_test(
    NAME knowledge_propagation_options_validate
    COMMAND pathfinder_tests_knowledge_propagation options_validate
)

add_test(
    NAME knowledge_propagation_propagator_validate
    COMMAND pathfinder_tests_knowledge_propagation propagator_validate
)

add_test(
    NAME knowledge_propagation_target_validate
    COMMAND pathfinder_tests_knowledge_propagation target_validate
)

add_test(
    NAME knowledge_propagation_context_validate
    COMMAND pathfinder_tests_knowledge_propagation context_validate
)

add_test(
    NAME knowledge_propagation_attempt_validate
    COMMAND pathfinder_tests_knowledge_propagation attempt_validate
)

add_test(
    NAME knowledge_propagation_transfer_assessment_validate
    COMMAND pathfinder_tests_knowledge_propagation transfer_assessment_validate
)

add_test(
    NAME knowledge_propagation_recipient_draft_validate
    COMMAND pathfinder_tests_knowledge_propagation recipient_claim_draft_validate
)

add_test(
    NAME knowledge_propagation_plan_validate
    COMMAND pathfinder_tests_knowledge_propagation plan_validate
)

add_test(
    NAME knowledge_propagation_result_validate
    COMMAND pathfinder_tests_knowledge_propagation result_validate
)

add_test(
    NAME knowledge_propagation_hidden_truth
    COMMAND pathfinder_tests_knowledge_propagation hidden_truth_rejected
)

add_test(
    NAME knowledge_propagation_test_only_gate
    COMMAND pathfinder_tests_knowledge_propagation test_only_rejected
)

add_test(
    NAME knowledge_propagation_planner_success
    COMMAND pathfinder_tests_knowledge_propagation planner_success
)

add_test(
    NAME knowledge_propagation_planner_low_trust
    COMMAND pathfinder_tests_knowledge_propagation planner_low_trust_fails
)

add_test(
    NAME knowledge_propagation_planner_deprecated
    COMMAND pathfinder_tests_knowledge_propagation planner_deprecated_blocked
)

add_test(
    NAME knowledge_propagation_planner_disproven
    COMMAND pathfinder_tests_knowledge_propagation planner_disproven_blocked
)

add_test(
    NAME knowledge_propagation_planner_same_owner
    COMMAND pathfinder_tests_knowledge_propagation planner_same_owner_blocked
)

add_test(
    NAME knowledge_propagation_planner_candidate
    COMMAND pathfinder_tests_knowledge_propagation planner_candidate_hypothesis_penalty
)

add_test(
    NAME knowledge_propagation_planner_reinforcement
    COMMAND pathfinder_tests_knowledge_propagation planner_reinforcement
)

add_test(
    NAME knowledge_propagation_service_propagate
    COMMAND pathfinder_tests_knowledge_propagation service_propagate
)

add_test(
    NAME knowledge_propagation_service_skipped
    COMMAND pathfinder_tests_knowledge_propagation service_skipped
)

add_test(
    NAME knowledge_propagation_applier
    COMMAND pathfinder_tests_knowledge_propagation applier_adds_claim
)

add_test(
    NAME knowledge_propagation_reinforced_same_id
    COMMAND pathfinder_tests_knowledge_propagation reinforced_existing_claim_updates_same_knowledge_id
)

add_test(
    NAME knowledge_propagation_different_effect_revision
    COMMAND pathfinder_tests_knowledge_propagation propagation_different_effect_routes_to_revision
)

add_test(
    NAME knowledge_propagation_different_effect_no_create
    COMMAND pathfinder_tests_knowledge_propagation propagation_different_effect_does_not_create_plain_claim
)

add_test(
    NAME knowledge_propagation_allow_create_false
    COMMAND pathfinder_tests_knowledge_propagation allow_create_false_does_not_create_claim
)

add_test(
    NAME knowledge_propagation_allow_update_false
    COMMAND pathfinder_tests_knowledge_propagation allow_update_false_does_not_update_existing_claim
)

add_test(
    NAME knowledge_propagation_failed_ok
    COMMAND pathfinder_tests_knowledge_propagation failed_result_is_business_ok_and_validate_ok
)

add_test(
    NAME knowledge_propagation_reinforced_no_duplicate
    COMMAND pathfinder_tests_knowledge_propagation reinforced_existing_claim_applier_does_not_duplicate
)

# P20 integration tests
add_executable(pathfinder_tests_p20_knowledge_propagation_flow
    integration/p20/knowledge_propagation_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p20_knowledge_propagation_flow PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p20_knowledge_propagation_flow
    COMMAND pathfinder_tests_p20_knowledge_propagation_flow
)

add_executable(pathfinder_tests_p20_knowledge_propagation_security
    integration/p20/knowledge_propagation_security_test.cpp
)

target_link_libraries(pathfinder_tests_p20_knowledge_propagation_security PRIVATE
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_foundation
)

add_test(
    NAME p20_knowledge_propagation_security
    COMMAND pathfinder_tests_p20_knowledge_propagation_security
)

# Agent Record type tests (P8)
add_executable(pathfinder_tests_agent_record_types
    unit/agent/agent_record_types_test.cpp
)

target_link_libraries(pathfinder_tests_agent_record_types PRIVATE
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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

# Agent Record builder tests (P8)
add_executable(pathfinder_tests_agent_record_builder
    unit/agent/agent_record_builder_test.cpp
)

target_link_libraries(pathfinder_tests_agent_record_builder PRIVATE
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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

# Agent Replay bridge tests (P8)
add_executable(pathfinder_tests_agent_replay_bridge
    unit/agent/agent_replay_bridge_test.cpp
)

target_link_libraries(pathfinder_tests_agent_replay_bridge PRIVATE
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME agent_record_smoke
    COMMAND pathfinder_tests_agent_record_types smoke
)

add_test(
    NAME agent_record_decision_status_roundtrip
    COMMAND pathfinder_tests_agent_record_types decision_record_status_roundtrip
)

add_test(
    NAME agent_record_tick_status_roundtrip
    COMMAND pathfinder_tests_agent_record_types tick_record_status_roundtrip
)

add_test(
    NAME agent_record_id_helpers
    COMMAND pathfinder_tests_agent_record_types id_helpers
)

add_test(
    NAME agent_record_valid_decision_passes
    COMMAND pathfinder_tests_agent_record_types valid_decision_record_passes
)

add_test(
    NAME agent_record_missing_record_id
    COMMAND pathfinder_tests_agent_record_types missing_record_id_fails
)

add_test(
    NAME agent_record_unknown_status
    COMMAND pathfinder_tests_agent_record_types unknown_status_fails
)

add_test(
    NAME agent_record_confidence_range
    COMMAND pathfinder_tests_agent_record_types confidence_out_of_range_fails
)

add_test(
    NAME agent_record_pipeline_needs_cmd_id
    COMMAND pathfinder_tests_agent_record_types pipeline_succeeded_requires_command_id
)

add_test(
    NAME agent_record_no_decision_empty_id
    COMMAND pathfinder_tests_agent_record_types no_decision_allows_empty_decision_id
)

add_test(
    NAME agent_record_no_decision_no_explanation
    COMMAND pathfinder_tests_agent_record_types no_decision_no_explanation_fails
)

add_test(
    NAME agent_record_no_decision_phase_keys
    COMMAND pathfinder_tests_agent_record_types no_decision_with_phase_keys_passes
)

add_test(
    NAME agent_record_valid_tick_passes
    COMMAND pathfinder_tests_agent_record_types valid_tick_record_passes
)

add_test(
    NAME agent_record_missing_input_hash
    COMMAND pathfinder_tests_agent_record_types missing_input_hash_fails
)

add_test(
    NAME agent_record_missing_output_hash
    COMMAND pathfinder_tests_agent_record_types missing_output_hash_fails
)

add_test(
    NAME agent_record_pipeline_needs_status
    COMMAND pathfinder_tests_agent_record_types pipeline_succeeded_requires_pipeline_status
)

add_test(
    NAME agent_record_submit_skipped_no_cmd
    COMMAND pathfinder_tests_agent_record_types submit_skipped_no_command_required
)

add_test(
    NAME agent_record_unknown_record_status
    COMMAND pathfinder_tests_agent_record_types unknown_record_status_fails
)

add_test(
    NAME agent_record_unknown_runtime_status
    COMMAND pathfinder_tests_agent_record_types unknown_runtime_status_fails
)

add_test(
    NAME agent_record_append_valid
    COMMAND pathfinder_tests_agent_record_types append_valid_record
)

add_test(
    NAME agent_record_append_duplicate
    COMMAND pathfinder_tests_agent_record_types append_duplicate_rejected
)

add_test(
    NAME agent_record_append_invalid
    COMMAND pathfinder_tests_agent_record_types append_invalid_rejected
)

add_test(
    NAME agent_record_find_by_record_id
    COMMAND pathfinder_tests_agent_record_types find_by_record_id
)

add_test(
    NAME agent_record_find_by_agent_id
    COMMAND pathfinder_tests_agent_record_types find_by_agent_id
)

add_test(
    NAME agent_record_find_by_decision_id
    COMMAND pathfinder_tests_agent_record_types find_by_decision_id
)

add_test(
    NAME agent_record_empty_log_valid
    COMMAND pathfinder_tests_agent_record_types empty_log_valid
)

add_test(
    NAME agent_record_find_nonexistent
    COMMAND pathfinder_tests_agent_record_types find_nonexistent_returns_empty
)

add_test(
    NAME agent_record_build_req_hash
    COMMAND pathfinder_tests_agent_record_types build_request_missing_hash_fails
)

# Record builder tests
add_test(
    NAME agent_record_builder_pipeline_succeeded
    COMMAND pathfinder_tests_agent_record_builder pipeline_succeeded_build
)

add_test(
    NAME agent_record_builder_submit_skipped
    COMMAND pathfinder_tests_agent_record_builder submit_skipped_build
)

add_test(
    NAME agent_record_builder_no_decision
    COMMAND pathfinder_tests_agent_record_builder no_decision_build
)

add_test(
    NAME agent_record_builder_missing_hash
    COMMAND pathfinder_tests_agent_record_builder missing_hash_returns_error
)

add_test(
    NAME agent_record_builder_deterministic_id
    COMMAND pathfinder_tests_agent_record_builder record_id_deterministic
)

add_test(
    NAME agent_record_builder_selected_candidate
    COMMAND pathfinder_tests_agent_record_builder selected_candidate_from_trace
)

# Replay bridge tests
add_test(
    NAME agent_replay_bridge_pipeline_succeeded
    COMMAND pathfinder_tests_agent_replay_bridge pipeline_succeeded_to_replay_record
)

add_test(
    NAME agent_replay_bridge_cmd_id_match
    COMMAND pathfinder_tests_agent_replay_bridge command_id_matches
)

add_test(
    NAME agent_replay_bridge_state_changes
    COMMAND pathfinder_tests_agent_replay_bridge state_change_ids_preserved
)

add_test(
    NAME agent_replay_bridge_submit_skipped
    COMMAND pathfinder_tests_agent_replay_bridge submit_skipped_fails
)

add_test(
    NAME agent_replay_bridge_no_decision
    COMMAND pathfinder_tests_agent_replay_bridge no_decision_fails
)

add_test(
    NAME agent_replay_bridge_append_log
    COMMAND pathfinder_tests_agent_replay_bridge append_to_log
)

add_test(
    NAME agent_replay_bridge_append_dup
    COMMAND pathfinder_tests_agent_replay_bridge append_duplicate_returns_error
)

# ReplayLockChecker tests
add_test(
    NAME agent_lock_check_with_cmd
    COMMAND pathfinder_tests_agent_replay_bridge lock_check_with_matching_command
)

add_test(
    NAME agent_lock_check_missing_cmd
    COMMAND pathfinder_tests_agent_replay_bridge lock_check_missing_command
)

add_test(
    NAME agent_lock_check_no_decision
    COMMAND pathfinder_tests_agent_replay_bridge lock_check_no_decision
)

add_test(
    NAME agent_lock_check_submit_skipped
    COMMAND pathfinder_tests_agent_replay_bridge lock_check_submit_skipped
)

# P8 integration test: unknown fruit replay
add_executable(pathfinder_tests_p8_unknown_fruit
    integration/p8/agent_runtime_record_unknown_fruit_replay_test.cpp
)

target_link_libraries(pathfinder_tests_p8_unknown_fruit PRIVATE
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME p8_smoke
    COMMAND pathfinder_tests_p8_unknown_fruit smoke
)

add_test(
    NAME p8_unknown_fruit_replay
    COMMAND pathfinder_tests_p8_unknown_fruit p8_unknown_fruit_replay
)

# P8 integration test: no-submit / no-decision
add_executable(pathfinder_tests_p8_no_submit
    integration/p8/agent_runtime_record_no_submit_test.cpp
)

target_link_libraries(pathfinder_tests_p8_no_submit PRIVATE
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME p8_no_submit_record
    COMMAND pathfinder_tests_p8_no_submit p8_no_submit_record
)

# Agent Replay Lock tests (P9)
add_executable(pathfinder_tests_agent_replay_lock
    unit/agent/agent_replay_lock_test.cpp
)

target_link_libraries(pathfinder_tests_agent_replay_lock PRIVATE
    pathfinder_agent_replay_lock
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME agent_replay_lock_smoke
    COMMAND pathfinder_tests_agent_replay_lock smoke
)

add_test(
    NAME agent_replay_lock_status_roundtrip
    COMMAND pathfinder_tests_agent_replay_lock lock_status_roundtrip
)

add_test(
    NAME agent_replay_lock_reason_roundtrip
    COMMAND pathfinder_tests_agent_replay_lock lock_reason_roundtrip
)

add_test(
    NAME agent_replay_lock_locked_entry_passes
    COMMAND pathfinder_tests_agent_replay_lock locked_entry_passes
)

add_test(
    NAME agent_replay_lock_locked_entry_no_cmd
    COMMAND pathfinder_tests_agent_replay_lock locked_entry_missing_command_id_fails
)

add_test(
    NAME agent_replay_lock_locked_entry_no_replay_id
    COMMAND pathfinder_tests_agent_replay_lock locked_entry_missing_replay_record_id_fails
)

add_test(
    NAME agent_replay_lock_explained_only
    COMMAND pathfinder_tests_agent_replay_lock explained_only_no_command_passes
)

add_test(
    NAME agent_replay_lock_broken_needs_reason
    COMMAND pathfinder_tests_agent_replay_lock broken_entry_requires_reason
)

add_test(
    NAME agent_replay_lock_unknown_status
    COMMAND pathfinder_tests_agent_replay_lock unknown_status_fails
)

add_test(
    NAME agent_replay_lock_empty_record_id
    COMMAND pathfinder_tests_agent_replay_lock empty_agent_record_id_fails
)

add_test(
    NAME agent_replay_lock_empty_agent_id
    COMMAND pathfinder_tests_agent_replay_lock empty_agent_id_fails
)

add_test(
    NAME agent_replay_lock_valid_set
    COMMAND pathfinder_tests_agent_replay_lock valid_lock_set_passes
)

add_test(
    NAME agent_replay_lock_empty_set
    COMMAND pathfinder_tests_agent_replay_lock empty_lock_set_fails
)

add_test(
    NAME agent_replay_lock_all_replayable
    COMMAND pathfinder_tests_agent_replay_lock all_replayable_without_policy
)

add_test(
    NAME agent_replay_lock_has_broken
    COMMAND pathfinder_tests_agent_replay_lock has_broken_entry
)

add_test(
    NAME agent_replay_lock_set_id_helper
    COMMAND pathfinder_tests_agent_replay_lock lock_set_id_helper
)

add_test(
    NAME agent_replay_lock_builder_locked
    COMMAND pathfinder_tests_agent_replay_lock builder_matching_command_locked
)

add_test(
    NAME agent_replay_lock_builder_broken
    COMMAND pathfinder_tests_agent_replay_lock builder_missing_command_broken
)

add_test(
    NAME agent_replay_lock_builder_submit_skipped
    COMMAND pathfinder_tests_agent_replay_lock builder_submit_skipped_explained_only
)

add_test(
    NAME agent_replay_lock_builder_no_decision
    COMMAND pathfinder_tests_agent_replay_lock builder_no_decision_explained_only
)

add_test(
    NAME agent_replay_lock_builder_empty_hash
    COMMAND pathfinder_tests_agent_replay_lock builder_source_hash_empty_fails
)

add_test(
    NAME agent_replay_lock_builder_null_log
    COMMAND pathfinder_tests_agent_replay_lock builder_null_agent_log_fails
)

add_test(
    NAME agent_replay_lock_builder_empty_log
    COMMAND pathfinder_tests_agent_replay_lock builder_empty_agent_log_fails
)

add_test(
    NAME agent_replay_lock_set_empty_source_hash
    COMMAND pathfinder_tests_agent_replay_lock lock_set_empty_source_hash_fails
)

# Agent Training Sample tests (P9)
add_executable(pathfinder_tests_agent_training_sample
    unit/agent/agent_training_sample_test.cpp
)

target_link_libraries(pathfinder_tests_agent_training_sample PRIVATE
    pathfinder_agent_training_sample
    pathfinder_agent_replay_lock
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME agent_training_sample_smoke
    COMMAND pathfinder_tests_agent_training_sample smoke
)

add_test(
    NAME agent_training_sample_status_roundtrip
    COMMAND pathfinder_tests_agent_training_sample sample_status_roundtrip
)

add_test(
    NAME agent_training_sample_reward_roundtrip
    COMMAND pathfinder_tests_agent_training_sample reward_status_roundtrip
)

add_test(
    NAME agent_training_sample_done_roundtrip
    COMMAND pathfinder_tests_agent_training_sample done_status_roundtrip
)

add_test(
    NAME agent_training_sample_id_deterministic
    COMMAND pathfinder_tests_agent_training_sample sample_id_deterministic
)

add_test(
    NAME agent_training_sample_valid_passes
    COMMAND pathfinder_tests_agent_training_sample valid_sample_passes
)

add_test(
    NAME agent_training_sample_no_sample_id
    COMMAND pathfinder_tests_agent_training_sample missing_sample_id_fails
)

add_test(
    NAME agent_training_sample_no_source_id
    COMMAND pathfinder_tests_agent_training_sample missing_source_record_id_fails
)

add_test(
    NAME agent_training_sample_unknown_status
    COMMAND pathfinder_tests_agent_training_sample unknown_status_fails
)

add_test(
    NAME agent_training_sample_unknown_runtime
    COMMAND pathfinder_tests_agent_training_sample result_unknown_runtime_status_fails
)

add_test(
    NAME agent_training_sample_empty_input_hash
    COMMAND pathfinder_tests_agent_training_sample empty_input_hash_fails
)

add_test(
    NAME agent_training_sample_empty_output_hash
    COMMAND pathfinder_tests_agent_training_sample empty_output_hash_fails
)

add_test(
    NAME agent_training_sample_reward_unknown
    COMMAND pathfinder_tests_agent_training_sample reward_status_unknown_fails
)

add_test(
    NAME agent_training_sample_done_unknown
    COMMAND pathfinder_tests_agent_training_sample done_status_unknown_fails
)

add_test(
    NAME agent_training_sample_pipeline_needs_cmd
    COMMAND pathfinder_tests_agent_training_sample pipeline_succeeded_requires_command_id
)

add_test(
    NAME agent_training_sample_no_decision_no_cmd
    COMMAND pathfinder_tests_agent_training_sample no_decision_allows_no_command_id
)

add_test(
    NAME agent_training_sample_no_decision_no_reason
    COMMAND pathfinder_tests_agent_training_sample no_decision_trace_no_reason_fails
)

add_test(
    NAME agent_training_sample_builder_draft
    COMMAND pathfinder_tests_agent_training_sample builder_pipeline_succeeded_draft
)

add_test(
    NAME agent_training_sample_builder_replay_locked
    COMMAND pathfinder_tests_agent_training_sample builder_replay_locked_sample
)

add_test(
    NAME agent_training_sample_builder_skipped
    COMMAND pathfinder_tests_agent_training_sample builder_submit_skipped_sample
)

add_test(
    NAME agent_training_sample_builder_no_decision
    COMMAND pathfinder_tests_agent_training_sample builder_no_decision_sample
)

add_test(
    NAME agent_training_sample_builder_no_obs_hash
    COMMAND pathfinder_tests_agent_training_sample builder_missing_observation_hash_fails
)

add_test(
    NAME agent_training_sample_builder_deterministic
    COMMAND pathfinder_tests_agent_training_sample builder_sample_id_deterministic
)

add_test(
    NAME agent_training_sample_builder_counts
    COMMAND pathfinder_tests_agent_training_sample builder_state_counts_preserved
)

add_test(
    NAME agent_training_sample_trace_replay_locked
    COMMAND pathfinder_tests_agent_training_sample trace_replay_locked_consistency
)

add_test(
    NAME agent_training_sample_trace_phase_not_reason
    COMMAND pathfinder_tests_agent_training_sample trace_phase_keys_not_in_reason_keys
)

add_test(
    NAME agent_training_sample_trace_rejected_reasons
    COMMAND pathfinder_tests_agent_training_sample trace_rejected_reason_keys_in_reason_keys
)

# P8 replay_record_id passthrough test
add_test(
    NAME agent_replay_bridge_custom_replay_id
    COMMAND pathfinder_tests_agent_replay_bridge custom_replay_record_id_passthrough
)

# P9 integration test: replay locked training draft
add_executable(pathfinder_tests_p9_replay_locked
    integration/p9/agent_replay_locked_training_draft_test.cpp
)

target_link_libraries(pathfinder_tests_p9_replay_locked PRIVATE
    pathfinder_agent_training_sample
    pathfinder_agent_replay_lock
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME p9_smoke
    COMMAND pathfinder_tests_p9_replay_locked smoke
)

add_test(
    NAME p9_replay_locked_training_draft
    COMMAND pathfinder_tests_p9_replay_locked p9_replay_locked_training_draft
)

# P9 integration test: no-decision / skipped training draft
add_executable(pathfinder_tests_p9_no_decision
    integration/p9/agent_no_decision_training_draft_test.cpp
)

target_link_libraries(pathfinder_tests_p9_no_decision PRIVATE
    pathfinder_agent_training_sample
    pathfinder_agent_replay_lock
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME p9_no_decision_smoke
    COMMAND pathfinder_tests_p9_no_decision smoke
)

add_test(
    NAME p9_no_decision_training_draft
    COMMAND pathfinder_tests_p9_no_decision p9_no_decision_training_draft
)

# Agent History Query tests (P10)
add_executable(pathfinder_tests_agent_history_query
    unit/agent/agent_history_query_test.cpp
)

target_link_libraries(pathfinder_tests_agent_history_query PRIVATE
    pathfinder_agent_history_query
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
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
    NAME agent_history_query_smoke
    COMMAND pathfinder_tests_agent_history_query smoke
)

add_test(
    NAME agent_history_query_sort_order_roundtrip
    COMMAND pathfinder_tests_agent_history_query sort_order_roundtrip
)

add_test(
    NAME agent_history_query_projection_mode_roundtrip
    COMMAND pathfinder_tests_agent_history_query projection_mode_roundtrip
)

add_test(
    NAME agent_history_query_id_helper
    COMMAND pathfinder_tests_agent_history_query query_id_helper
)

add_test(
    NAME agent_history_query_valid_options
    COMMAND pathfinder_tests_agent_history_query valid_options_passes
)

add_test(
    NAME agent_history_query_empty_id
    COMMAND pathfinder_tests_agent_history_query empty_query_id_fails
)

add_test(
    NAME agent_history_query_unknown_sort
    COMMAND pathfinder_tests_agent_history_query unknown_sort_order_fails
)

add_test(
    NAME agent_history_query_unknown_mode
    COMMAND pathfinder_tests_agent_history_query unknown_projection_mode_fails
)

add_test(
    NAME agent_history_query_limit_zero
    COMMAND pathfinder_tests_agent_history_query limit_zero_fails
)

add_test(
    NAME agent_history_query_limit_over
    COMMAND pathfinder_tests_agent_history_query limit_over_1000_fails
)

add_test(
    NAME agent_history_query_bad_tick_range
    COMMAND pathfinder_tests_agent_history_query tick_from_greater_than_to_fails
)

add_test(
    NAME agent_history_query_public_debug
    COMMAND pathfinder_tests_agent_history_query public_debug_trace_fails
)

add_test(
    NAME agent_history_query_filter_agent
    COMMAND pathfinder_tests_agent_history_query filter_by_agent_id
)

add_test(
    NAME agent_history_query_filter_tick
    COMMAND pathfinder_tests_agent_history_query filter_by_tick_range
)

add_test(
    NAME agent_history_query_sort_asc
    COMMAND pathfinder_tests_agent_history_query sort_ascending
)

add_test(
    NAME agent_history_query_sort_desc
    COMMAND pathfinder_tests_agent_history_query sort_descending
)

add_test(
    NAME agent_history_query_limit
    COMMAND pathfinder_tests_agent_history_query limit_truncates
)

add_test(
    NAME agent_history_query_no_match
    COMMAND pathfinder_tests_agent_history_query no_match_empty_result
)

add_test(
    NAME agent_history_query_read_only
    COMMAND pathfinder_tests_agent_history_query query_does_not_modify_log
)

# Agent Projection tests (P10)
add_executable(pathfinder_tests_agent_projection
    unit/protocol/agent_projection_test.cpp
)

target_link_libraries(pathfinder_tests_agent_projection PRIVATE
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME agent_projection_smoke
    COMMAND pathfinder_tests_agent_projection smoke
)

add_test(
    NAME agent_projection_valid_item
    COMMAND pathfinder_tests_agent_projection valid_history_item_passes
)

add_test(
    NAME agent_projection_missing_record_id
    COMMAND pathfinder_tests_agent_projection missing_record_id_fails
)

add_test(
    NAME agent_projection_missing_agent_id
    COMMAND pathfinder_tests_agent_projection missing_agent_id_fails
)

add_test(
    NAME agent_projection_missing_runtime
    COMMAND pathfinder_tests_agent_projection missing_runtime_status_fails
)

add_test(
    NAME agent_projection_valid_history
    COMMAND pathfinder_tests_agent_projection valid_history_projection_passes
)

add_test(
    NAME agent_projection_unknown_mode
    COMMAND pathfinder_tests_agent_projection unknown_mode_fails
)

add_test(
    NAME agent_projection_valid_lock_item
    COMMAND pathfinder_tests_agent_projection valid_lock_item_passes
)

add_test(
    NAME agent_projection_lock_item_status
    COMMAND pathfinder_tests_agent_projection lock_item_missing_status_fails
)

add_test(
    NAME agent_projection_valid_lock
    COMMAND pathfinder_tests_agent_projection valid_lock_projection_passes
)

add_test(
    NAME agent_projection_lock_id
    COMMAND pathfinder_tests_agent_projection lock_projection_missing_id_fails
)

add_test(
    NAME agent_projection_valid_training
    COMMAND pathfinder_tests_agent_projection valid_training_sample_passes
)

add_test(
    NAME agent_projection_training_reward
    COMMAND pathfinder_tests_agent_projection training_missing_reward_status_fails
)

add_test(
    NAME agent_projection_no_reward_done
    COMMAND pathfinder_tests_agent_projection training_no_reward_value_or_done_bool
)

add_test(
    NAME agent_projection_proj_cmd_id
    COMMAND pathfinder_tests_agent_projection projector_pipeline_succeeded_has_command_id
)

add_test(
    NAME agent_projection_proj_no_cmd
    COMMAND pathfinder_tests_agent_projection projector_no_decision_no_command_id
)

add_test(
    NAME agent_projection_proj_lock
    COMMAND pathfinder_tests_agent_projection projector_replay_lock_status_locked
)

add_test(
    NAME agent_projection_proj_training
    COMMAND pathfinder_tests_agent_projection projector_training_sample_status
)

add_test(
    NAME agent_projection_proj_public
    COMMAND pathfinder_tests_agent_projection projector_public_no_trace_keys
)

add_test(
    NAME agent_projection_proj_debug
    COMMAND pathfinder_tests_agent_projection projector_debug_has_trace_keys
)

add_test(
    NAME agent_projection_envelope_history
    COMMAND pathfinder_tests_agent_projection history_envelope_valid
)

add_test(
    NAME agent_projection_envelope_lock
    COMMAND pathfinder_tests_agent_projection replay_lock_envelope_valid
)

add_test(
    NAME agent_projection_envelope_training
    COMMAND pathfinder_tests_agent_projection training_sample_envelope_valid
)

add_test(
    NAME agent_projection_envelope_options
    COMMAND pathfinder_tests_agent_projection envelope_passthrough_options
)

# P10 integration test: unknown fruit history projection flow
add_executable(pathfinder_tests_p10_history_projection
    integration/p10/agent_history_projection_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p10_history_projection PRIVATE
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME p10_smoke
    COMMAND pathfinder_tests_p10_history_projection smoke
)

add_test(
    NAME p10_agent_history_projection_flow
    COMMAND pathfinder_tests_p10_history_projection p10_agent_history_projection_flow
)

# P10 integration test: no-decision history projection flow
add_executable(pathfinder_tests_p10_no_decision_projection
    integration/p10/agent_no_decision_projection_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p10_no_decision_projection PRIVATE
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME p10_no_decision_smoke
    COMMAND pathfinder_tests_p10_no_decision_projection smoke
)

add_test(
    NAME p10_agent_no_decision_projection_flow
    COMMAND pathfinder_tests_p10_no_decision_projection p10_agent_no_decision_projection_flow
)

# Agent Debug Report tests (P11)
add_executable(pathfinder_tests_agent_debug_report
    unit/agent/agent_debug_report_test.cpp
)

target_link_libraries(pathfinder_tests_agent_debug_report PRIVATE
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME agent_debug_report_smoke
    COMMAND pathfinder_tests_agent_debug_report smoke
)

add_test(
    NAME agent_debug_report_mode_roundtrip
    COMMAND pathfinder_tests_agent_debug_report mode_roundtrip
)

add_test(
    NAME agent_debug_report_severity_roundtrip
    COMMAND pathfinder_tests_agent_debug_report severity_roundtrip
)

add_test(
    NAME agent_debug_report_diagnostics_status_roundtrip
    COMMAND pathfinder_tests_agent_debug_report diagnostics_status_roundtrip
)

add_test(
    NAME agent_debug_report_id_helper
    COMMAND pathfinder_tests_agent_debug_report id_helper
)

add_test(
    NAME agent_debug_report_valid_item_passes
    COMMAND pathfinder_tests_agent_debug_report valid_item_passes
)

add_test(
    NAME agent_debug_report_missing_record_id_fails
    COMMAND pathfinder_tests_agent_debug_report missing_record_id_fails
)

add_test(
    NAME agent_debug_report_missing_agent_id_fails
    COMMAND pathfinder_tests_agent_debug_report missing_agent_id_fails
)

add_test(
    NAME agent_debug_report_missing_actor_id_fails
    COMMAND pathfinder_tests_agent_debug_report missing_actor_id_fails
)

add_test(
    NAME agent_debug_report_missing_runtime_status_fails
    COMMAND pathfinder_tests_agent_debug_report missing_runtime_status_fails
)

add_test(
    NAME agent_debug_report_missing_decision_status_fails
    COMMAND pathfinder_tests_agent_debug_report missing_decision_status_fails
)

add_test(
    NAME agent_debug_report_severity_unknown_fails
    COMMAND pathfinder_tests_agent_debug_report severity_unknown_fails
)

add_test(
    NAME agent_debug_report_valid_report_passes
    COMMAND pathfinder_tests_agent_debug_report valid_report_passes
)

add_test(
    NAME agent_debug_report_empty_report_id_fails
    COMMAND pathfinder_tests_agent_debug_report empty_report_id_fails
)

add_test(
    NAME agent_debug_report_mode_unknown_fails
    COMMAND pathfinder_tests_agent_debug_report mode_unknown_fails
)

add_test(
    NAME agent_debug_report_items_exceed_total_fails
    COMMAND pathfinder_tests_agent_debug_report items_exceed_total_fails
)

add_test(
    NAME agent_debug_report_count_exceeds_total_fails
    COMMAND pathfinder_tests_agent_debug_report count_exceeds_total_fails
)

add_test(
    NAME agent_debug_report_invalid_item_in_report_fails
    COMMAND pathfinder_tests_agent_debug_report invalid_item_in_report_fails
)

add_test(
    NAME agent_debug_report_valid_issue_passes
    COMMAND pathfinder_tests_agent_debug_report valid_issue_passes
)

add_test(
    NAME agent_debug_report_issue_unknown_severity_fails
    COMMAND pathfinder_tests_agent_debug_report issue_unknown_severity_fails
)

add_test(
    NAME agent_debug_report_issue_empty_key_fails
    COMMAND pathfinder_tests_agent_debug_report issue_empty_key_fails
)

add_test(
    NAME agent_debug_report_valid_diagnostics_passes
    COMMAND pathfinder_tests_agent_debug_report valid_diagnostics_passes
)

add_test(
    NAME agent_debug_report_diagnostics_unknown_status_fails
    COMMAND pathfinder_tests_agent_debug_report diagnostics_unknown_status_fails
)

add_test(
    NAME agent_debug_report_builder_pipeline_succeeded
    COMMAND pathfinder_tests_agent_debug_report builder_pipeline_succeeded_has_command_id
)

add_test(
    NAME agent_debug_report_builder_no_decision
    COMMAND pathfinder_tests_agent_debug_report builder_no_decision_no_command_id
)

add_test(
    NAME agent_debug_report_builder_broken_severity
    COMMAND pathfinder_tests_agent_debug_report builder_broken_item_severity_error
)

add_test(
    NAME agent_debug_report_builder_public_clears_trace
    COMMAND pathfinder_tests_agent_debug_report builder_public_clears_trace_keys
)

add_test(
    NAME agent_debug_report_builder_debug_keeps_trace
    COMMAND pathfinder_tests_agent_debug_report builder_debug_keeps_trace_keys
)

add_test(
    NAME agent_debug_report_builder_explained_only
    COMMAND pathfinder_tests_agent_debug_report builder_explained_only_count
)

add_test(
    NAME agent_debug_report_builder_public_trace_not_allowed
    COMMAND pathfinder_tests_agent_debug_report builder_public_trace_keys_not_allowed
)

add_test(
    NAME agent_debug_report_diagnostics_clean
    COMMAND pathfinder_tests_agent_debug_report diagnostics_clean_report
)

add_test(
    NAME agent_debug_report_diagnostics_broken_error
    COMMAND pathfinder_tests_agent_debug_report diagnostics_broken_item_error
)

add_test(
    NAME agent_debug_report_diagnostics_no_decision_warning
    COMMAND pathfinder_tests_agent_debug_report diagnostics_no_decision_no_explanation_warning
)

add_test(
    NAME agent_debug_report_diagnostics_public_trace_leak
    COMMAND pathfinder_tests_agent_debug_report diagnostics_public_trace_leak_error
)

add_test(
    NAME agent_debug_report_diagnostics_public_no_decision_clean
    COMMAND pathfinder_tests_agent_debug_report diagnostics_no_decision_public_no_warning
)

# Agent Debug Export tests (P11)
add_executable(pathfinder_tests_agent_debug_export
    unit/agent/agent_debug_export_test.cpp
)

target_link_libraries(pathfinder_tests_agent_debug_export PRIVATE
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME agent_debug_export_smoke
    COMMAND pathfinder_tests_agent_debug_export smoke
)

add_test(
    NAME agent_debug_export_format_roundtrip
    COMMAND pathfinder_tests_agent_debug_export format_roundtrip
)

add_test(
    NAME agent_debug_export_valid_chunk_passes
    COMMAND pathfinder_tests_agent_debug_export valid_chunk_passes
)

add_test(
    NAME agent_debug_export_empty_chunk_id_fails
    COMMAND pathfinder_tests_agent_debug_export empty_chunk_id_fails
)

add_test(
    NAME agent_debug_export_format_unknown_fails
    COMMAND pathfinder_tests_agent_debug_export format_unknown_fails
)

add_test(
    NAME agent_debug_export_empty_payload_fails
    COMMAND pathfinder_tests_agent_debug_export empty_payload_fails
)

add_test(
    NAME agent_debug_export_valid_manifest_passes
    COMMAND pathfinder_tests_agent_debug_export valid_manifest_passes
)

add_test(
    NAME agent_debug_export_manifest_chunk_count_mismatch
    COMMAND pathfinder_tests_agent_debug_export manifest_chunk_count_mismatch_fails
)

add_test(
    NAME agent_debug_export_valid_draft_passes
    COMMAND pathfinder_tests_agent_debug_export valid_draft_passes
)

add_test(
    NAME agent_debug_export_draft_manifest_mismatch
    COMMAND pathfinder_tests_agent_debug_export draft_manifest_mismatch_fails
)

add_test(
    NAME agent_debug_export_builder_plaintext
    COMMAND pathfinder_tests_agent_debug_export builder_plaintext_export
)

add_test(
    NAME agent_debug_export_builder_markdown
    COMMAND pathfinder_tests_agent_debug_export builder_markdown_export
)

add_test(
    NAME agent_debug_export_builder_protocol_text
    COMMAND pathfinder_tests_agent_debug_export builder_protocol_text_export
)

add_test(
    NAME agent_debug_export_builder_zero_chunk_fails
    COMMAND pathfinder_tests_agent_debug_export builder_max_items_zero_fails
)

add_test(
    NAME agent_debug_export_builder_chunk_count
    COMMAND pathfinder_tests_agent_debug_export builder_chunk_count_correct
)

add_test(
    NAME agent_debug_export_builder_warning_keys
    COMMAND pathfinder_tests_agent_debug_export builder_manifest_warning_keys
)

add_test(
    NAME agent_debug_export_builder_no_hidden_truth
    COMMAND pathfinder_tests_agent_debug_export builder_payload_no_hidden_truth
)

# P11 integration test: unknown fruit debug report export flow
add_executable(pathfinder_tests_p11_debug_report_export
    integration/p11/agent_debug_report_export_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p11_debug_report_export PRIVATE
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME p11_smoke
    COMMAND pathfinder_tests_p11_debug_report_export smoke
)

add_test(
    NAME p11_agent_debug_report_export_flow
    COMMAND pathfinder_tests_p11_debug_report_export p11_agent_debug_report_export_flow
)

# P11 integration test: no-decision debug report
add_executable(pathfinder_tests_p11_no_decision_debug
    integration/p11/agent_no_decision_debug_report_test.cpp
)

target_link_libraries(pathfinder_tests_p11_no_decision_debug PRIVATE
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME p11_no_decision_smoke
    COMMAND pathfinder_tests_p11_no_decision_debug smoke
)

add_test(
    NAME p11_agent_no_decision_debug_report
    COMMAND pathfinder_tests_p11_no_decision_debug p11_agent_no_decision_debug_report
)

# Agent Local Export tests (P12)
add_executable(pathfinder_tests_agent_local_export
    unit/agent/agent_local_export_test.cpp
)

target_link_libraries(pathfinder_tests_agent_local_export PRIVATE
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME agent_local_export_smoke
    COMMAND pathfinder_tests_agent_local_export smoke
)

add_test(
    NAME agent_local_export_file_kind_roundtrip
    COMMAND pathfinder_tests_agent_local_export file_kind_roundtrip
)

add_test(
    NAME agent_local_export_file_extension_roundtrip
    COMMAND pathfinder_tests_agent_local_export file_extension_roundtrip
)

add_test(
    NAME agent_local_export_write_status_roundtrip
    COMMAND pathfinder_tests_agent_local_export write_status_roundtrip
)

add_test(
    NAME agent_local_export_verification_status_roundtrip
    COMMAND pathfinder_tests_agent_local_export verification_status_roundtrip
)

add_test(
    NAME agent_local_export_file_id_helper
    COMMAND pathfinder_tests_agent_local_export file_id_helper
)

add_test(
    NAME agent_local_export_write_id_helper
    COMMAND pathfinder_tests_agent_local_export write_id_helper
)

add_test(
    NAME agent_local_export_verify_id_helper
    COMMAND pathfinder_tests_agent_local_export verify_id_helper
)

add_test(
    NAME agent_local_export_path_policy_empty_root
    COMMAND pathfinder_tests_agent_local_export path_policy_empty_root_dir_fails
)

add_test(
    NAME agent_local_export_path_policy_traversal
    COMMAND pathfinder_tests_agent_local_export path_policy_root_dir_traversal_fails
)

add_test(
    NAME agent_local_export_path_policy_max_file_count
    COMMAND pathfinder_tests_agent_local_export path_policy_max_file_count_fails
)

add_test(
    NAME agent_local_export_path_policy_max_file_bytes
    COMMAND pathfinder_tests_agent_local_export path_policy_max_file_bytes_zero_fails
)

add_test(
    NAME agent_local_export_path_policy_total_bytes
    COMMAND pathfinder_tests_agent_local_export path_policy_total_bytes_less_than_max_fails
)

add_test(
    NAME agent_local_export_file_plan_empty_id
    COMMAND pathfinder_tests_agent_local_export file_plan_empty_file_id_fails
)

add_test(
    NAME agent_local_export_file_plan_unknown_kind
    COMMAND pathfinder_tests_agent_local_export file_plan_unknown_kind_fails
)

add_test(
    NAME agent_local_export_file_plan_unknown_ext
    COMMAND pathfinder_tests_agent_local_export file_plan_unknown_extension_fails
)

add_test(
    NAME agent_local_export_file_plan_empty_path
    COMMAND pathfinder_tests_agent_local_export file_plan_empty_relative_path_fails
)

add_test(
    NAME agent_local_export_file_plan_traversal
    COMMAND pathfinder_tests_agent_local_export file_plan_relative_path_traversal_fails
)

add_test(
    NAME agent_local_export_file_plan_backslash
    COMMAND pathfinder_tests_agent_local_export file_plan_relative_path_backslash_fails
)

add_test(
    NAME agent_local_export_file_plan_absolute
    COMMAND pathfinder_tests_agent_local_export file_plan_absolute_relative_path_fails
)

add_test(
    NAME agent_local_export_file_plan_manifest_empty
    COMMAND pathfinder_tests_agent_local_export file_plan_manifest_empty_content_fails
)

add_test(
    NAME agent_local_export_file_plan_size_mismatch
    COMMAND pathfinder_tests_agent_local_export file_plan_byte_size_mismatch_fails
)

add_test(
    NAME agent_local_export_write_plan_empty_id
    COMMAND pathfinder_tests_agent_local_export write_plan_empty_write_id_fails
)

add_test(
    NAME agent_local_export_write_plan_empty_export
    COMMAND pathfinder_tests_agent_local_export write_plan_empty_export_id_fails
)

add_test(
    NAME agent_local_export_write_plan_empty_files
    COMMAND pathfinder_tests_agent_local_export write_plan_empty_files_fails
)

add_test(
    NAME agent_local_export_write_plan_exceed_count
    COMMAND pathfinder_tests_agent_local_export write_plan_exceeds_max_file_count_fails
)

add_test(
    NAME agent_local_export_write_plan_dup_path
    COMMAND pathfinder_tests_agent_local_export write_plan_duplicate_relative_path_fails
)

add_test(
    NAME agent_local_export_write_plan_no_manifest
    COMMAND pathfinder_tests_agent_local_export write_plan_missing_manifest_fails
)

add_test(
    NAME agent_local_export_write_plan_total_mismatch
    COMMAND pathfinder_tests_agent_local_export write_plan_total_bytes_mismatch_fails
)

add_test(
    NAME agent_local_export_write_plan_exceed_total
    COMMAND pathfinder_tests_agent_local_export write_plan_exceeds_max_total_bytes_fails
)

add_test(
    NAME agent_local_export_file_write_result_written_errors
    COMMAND pathfinder_tests_agent_local_export file_write_result_written_with_errors_fails
)

add_test(
    NAME agent_local_export_file_write_result_failed_no_errors
    COMMAND pathfinder_tests_agent_local_export file_write_result_failed_without_errors_fails
)

add_test(
    NAME agent_local_export_write_result_written_errors
    COMMAND pathfinder_tests_agent_local_export write_result_written_with_errors_fails
)

add_test(
    NAME agent_local_export_write_result_failed_no_errors
    COMMAND pathfinder_tests_agent_local_export write_result_failed_without_errors_fails
)

add_test(
    NAME agent_local_export_write_result_count_mismatch
    COMMAND pathfinder_tests_agent_local_export write_result_written_count_mismatch_fails
)

add_test(
    NAME agent_local_export_verify_passed_count
    COMMAND pathfinder_tests_agent_local_export verify_report_passed_file_count_mismatch_fails
)

add_test(
    NAME agent_local_export_verify_passed_bytes
    COMMAND pathfinder_tests_agent_local_export verify_report_passed_total_bytes_mismatch_fails
)

add_test(
    NAME agent_local_export_verify_dup_paths
    COMMAND pathfinder_tests_agent_local_export verify_report_duplicate_checked_paths_fails
)

add_test(
    NAME agent_local_export_planner_plaintext
    COMMAND pathfinder_tests_agent_local_export planner_plaintext_txt
)

add_test(
    NAME agent_local_export_planner_markdown
    COMMAND pathfinder_tests_agent_local_export planner_markdown_md
)

add_test(
    NAME agent_local_export_planner_protocol_text
    COMMAND pathfinder_tests_agent_local_export planner_protocol_text_txt
)

add_test(
    NAME agent_local_export_planner_manifest_name
    COMMAND pathfinder_tests_agent_local_export planner_manifest_name_stable
)

add_test(
    NAME agent_local_export_planner_chunk_name
    COMMAND pathfinder_tests_agent_local_export planner_chunk_name_by_index
)

add_test(
    NAME agent_local_export_planner_bad_base_name_traversal
    COMMAND pathfinder_tests_agent_local_export planner_base_name_traversal_fails
)

add_test(
    NAME agent_local_export_planner_bad_base_name_space
    COMMAND pathfinder_tests_agent_local_export planner_base_name_space_fails
)

add_test(
    NAME agent_local_export_planner_bad_base_name_colon
    COMMAND pathfinder_tests_agent_local_export planner_base_name_colon_fails
)

add_test(
    NAME agent_local_export_write_create_dir
    COMMAND pathfinder_tests_agent_local_export write_creates_directory
)

add_test(
    NAME agent_local_export_write_no_create_dir
    COMMAND pathfinder_tests_agent_local_export write_no_create_dir_fails
)

add_test(
    NAME agent_local_export_write_no_overwrite
    COMMAND pathfinder_tests_agent_local_export write_no_overwrite_fails
)

add_test(
    NAME agent_local_export_write_overwrite
    COMMAND pathfinder_tests_agent_local_export write_overwrite_succeeds
)

add_test(
    NAME agent_local_export_write_result_count
    COMMAND pathfinder_tests_agent_local_export write_written_count_correct
)

add_test(
    NAME agent_local_export_write_result_errors
    COMMAND pathfinder_tests_agent_local_export write_failed_has_error_keys
)

add_test(
    NAME agent_local_export_verify_passed
    COMMAND pathfinder_tests_agent_local_export verify_passed_after_write
)

add_test(
    NAME agent_local_export_verify_missing_file
    COMMAND pathfinder_tests_agent_local_export verify_missing_file_fails
)

add_test(
    NAME agent_local_export_verify_size_mismatch
    COMMAND pathfinder_tests_agent_local_export verify_size_mismatch_fails
)

add_test(
    NAME agent_local_export_verify_forbidden_content
    COMMAND pathfinder_tests_agent_local_export verify_forbidden_content_fails
)

add_test(
    NAME agent_local_export_verify_no_scan
    COMMAND pathfinder_tests_agent_local_export verify_no_content_scan
)

# P12 integration test: MarkdownLike export flow
add_executable(pathfinder_tests_p12_export_flow
    integration/p12/agent_local_export_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p12_export_flow PRIVATE
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME p12_smoke
    COMMAND pathfinder_tests_p12_export_flow smoke
)

add_test(
    NAME p12_agent_local_export_flow
    COMMAND pathfinder_tests_p12_export_flow p12_agent_local_export_flow
)

# P12 integration test: path security
add_executable(pathfinder_tests_p12_security
    integration/p12/agent_local_export_security_test.cpp
)

target_link_libraries(pathfinder_tests_p12_security PRIVATE
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent_runtime
    pathfinder_agent_builders
    pathfinder_agent
    pathfinder_protocol
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
    NAME p12_security_smoke
    COMMAND pathfinder_tests_p12_security smoke
)

add_test(
    NAME p12_path_security
    COMMAND pathfinder_tests_p12_security p12_path_security
)

# Agent Debug CLI tests (P13)
add_executable(pathfinder_tests_agent_debug_cli
    unit/agent/agent_debug_cli_test.cpp
)

target_link_libraries(pathfinder_tests_agent_debug_cli PRIVATE
    pathfinder_agent_debug_cli_lib
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

add_test(
    NAME agent_debug_cli_smoke
    COMMAND pathfinder_tests_agent_debug_cli smoke
)

add_test(
    NAME agent_debug_cli_command_roundtrip
    COMMAND pathfinder_tests_agent_debug_cli command_roundtrip
)

add_test(
    NAME agent_debug_cli_fixture_roundtrip
    COMMAND pathfinder_tests_agent_debug_cli fixture_roundtrip
)

add_test(
    NAME agent_debug_cli_format_roundtrip
    COMMAND pathfinder_tests_agent_debug_cli format_roundtrip
)

add_test(
    NAME agent_debug_cli_exit_code_int
    COMMAND pathfinder_tests_agent_debug_cli exit_code_int
)

add_test(
    NAME agent_debug_cli_format_conversion
    COMMAND pathfinder_tests_agent_debug_cli format_conversion
)

add_test(
    NAME agent_debug_cli_options_unknown_command_fails
    COMMAND pathfinder_tests_agent_debug_cli options_unknown_command_fails
)

add_test(
    NAME agent_debug_cli_options_export_missing_fixture_fails
    COMMAND pathfinder_tests_agent_debug_cli options_export_missing_fixture_fails
)

add_test(
    NAME agent_debug_cli_options_export_missing_output_dir_fails
    COMMAND pathfinder_tests_agent_debug_cli options_export_missing_output_dir_fails
)

add_test(
    NAME agent_debug_cli_options_invalid_base_name_fails
    COMMAND pathfinder_tests_agent_debug_cli options_invalid_base_name_fails
)

add_test(
    NAME agent_debug_cli_options_max_items_zero_fails
    COMMAND pathfinder_tests_agent_debug_cli options_max_items_zero_fails
)

add_test(
    NAME agent_debug_cli_options_help_valid
    COMMAND pathfinder_tests_agent_debug_cli options_help_valid
)

add_test(
    NAME agent_debug_cli_result_success_with_error_keys_fails
    COMMAND pathfinder_tests_agent_debug_cli result_success_with_error_keys_fails
)

add_test(
    NAME agent_debug_cli_result_failure_without_error_keys_fails
    COMMAND pathfinder_tests_agent_debug_cli result_failure_without_error_keys_fails
)

add_test(
    NAME agent_debug_cli_result_absolute_path_in_output_files_fails
    COMMAND pathfinder_tests_agent_debug_cli result_absolute_path_in_output_files_fails
)

add_test(
    NAME agent_debug_cli_result_empty_summary_text_fails
    COMMAND pathfinder_tests_agent_debug_cli result_empty_summary_text_fails
)

add_test(
    NAME agent_debug_cli_result_valid_success_passes
    COMMAND pathfinder_tests_agent_debug_cli result_valid_success_passes
)

add_test(
    NAME agent_debug_cli_result_valid_failure_passes
    COMMAND pathfinder_tests_agent_debug_cli result_valid_failure_passes
)

add_test(
    NAME agent_debug_cli_parser_help
    COMMAND pathfinder_tests_agent_debug_cli parser_help
)

add_test(
    NAME agent_debug_cli_parser_export_unknown_fruit_markdown
    COMMAND pathfinder_tests_agent_debug_cli parser_export_unknown_fruit_markdown
)

add_test(
    NAME agent_debug_cli_parser_dry_run
    COMMAND pathfinder_tests_agent_debug_cli parser_dry_run
)

add_test(
    NAME agent_debug_cli_parser_overwrite
    COMMAND pathfinder_tests_agent_debug_cli parser_overwrite
)

add_test(
    NAME agent_debug_cli_parser_unknown_arg_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_unknown_arg_fails
)

add_test(
    NAME agent_debug_cli_parser_json_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_json_fails
)

add_test(
    NAME agent_debug_cli_parser_load_save_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_load_save_fails
)

add_test(
    NAME agent_debug_cli_parser_scan_content_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_scan_content_fails
)

add_test(
    NAME agent_debug_cli_parser_no_scan_content_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_no_scan_content_fails
)

add_test(
    NAME agent_debug_cli_parser_fixture_missing_value_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_fixture_missing_value_fails
)

add_test(
    NAME agent_debug_cli_parser_max_items_non_number_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_max_items_non_number_fails
)

add_test(
    NAME agent_debug_cli_parser_base_name_escape_fails
    COMMAND pathfinder_tests_agent_debug_cli parser_base_name_escape_fails
)

add_test(
    NAME agent_debug_cli_fixture_unknown_fruit_valid
    COMMAND pathfinder_tests_agent_debug_cli fixture_unknown_fruit_valid
)

add_test(
    NAME agent_debug_cli_fixture_no_decision_valid
    COMMAND pathfinder_tests_agent_debug_cli fixture_no_decision_valid
)

add_test(
    NAME agent_debug_cli_fixture_public_safe_valid
    COMMAND pathfinder_tests_agent_debug_cli fixture_public_safe_valid
)

add_test(
    NAME agent_debug_cli_fixture_unknown_fails
    COMMAND pathfinder_tests_agent_debug_cli fixture_unknown_fails
)

add_test(
    NAME agent_debug_cli_fixture_no_hidden_truth
    COMMAND pathfinder_tests_agent_debug_cli fixture_no_hidden_truth
)

add_test(
    NAME agent_debug_cli_runner_help
    COMMAND pathfinder_tests_agent_debug_cli runner_help
)

add_test(
    NAME agent_debug_cli_runner_dry_run
    COMMAND pathfinder_tests_agent_debug_cli runner_dry_run
)

add_test(
    NAME agent_debug_cli_runner_unknown_fruit_markdown
    COMMAND pathfinder_tests_agent_debug_cli runner_unknown_fruit_markdown
)

add_test(
    NAME agent_debug_cli_runner_no_decision_text
    COMMAND pathfinder_tests_agent_debug_cli runner_no_decision_text
)

add_test(
    NAME agent_debug_cli_runner_public_safe_protocol_text
    COMMAND pathfinder_tests_agent_debug_cli runner_public_safe_protocol_text
)

add_test(
    NAME agent_debug_cli_runner_no_overwrite_conflict
    COMMAND pathfinder_tests_agent_debug_cli runner_no_overwrite_conflict
)

add_test(
    NAME agent_debug_cli_runner_invalid_output_dir
    COMMAND pathfinder_tests_agent_debug_cli runner_invalid_output_dir
)

# P13 integration test: CLI export flow
add_executable(pathfinder_tests_p13_flow
    integration/p13/agent_debug_cli_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p13_flow PRIVATE
    pathfinder_agent_debug_cli_lib
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

add_test(
    NAME p13_smoke
    COMMAND pathfinder_tests_p13_flow smoke
)

add_test(
    NAME p13_agent_debug_cli_flow
    COMMAND pathfinder_tests_p13_flow p13_agent_debug_cli_flow
)

# P13 integration test: CLI security
add_executable(pathfinder_tests_p13_security
    integration/p13/agent_debug_cli_security_test.cpp
)

target_link_libraries(pathfinder_tests_p13_security PRIVATE
    pathfinder_agent_debug_cli_lib
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

add_test(
    NAME p13_security_smoke
    COMMAND pathfinder_tests_p13_security smoke
)

add_test(
    NAME p13_base_name_escape
    COMMAND pathfinder_tests_p13_security p13_base_name_escape
)

add_test(
    NAME p13_json_rejected
    COMMAND pathfinder_tests_p13_security p13_json_rejected
)

add_test(
    NAME p13_load_save_rejected
    COMMAND pathfinder_tests_p13_security p13_load_save_rejected
)

add_test(
    NAME p13_scan_content_rejected
    COMMAND pathfinder_tests_p13_security p13_scan_content_rejected
)

add_test(
    NAME p13_no_scan_content_rejected
    COMMAND pathfinder_tests_p13_security p13_no_scan_content_rejected
)

add_test(
    NAME p13_output_dir_missing_value
    COMMAND pathfinder_tests_p13_security p13_output_dir_missing_value
)

add_test(
    NAME p13_dry_run_no_files
    COMMAND pathfinder_tests_p13_security p13_dry_run_no_files
)

add_test(
    NAME p13_overwrite_false_conflict
    COMMAND pathfinder_tests_p13_security p13_overwrite_false_conflict
)

add_test(
    NAME p13_no_files_outside_root
    COMMAND pathfinder_tests_p13_security p13_no_files_outside_root
)

# Agent Debug Input unit tests (P14)
add_executable(pathfinder_tests_agent_debug_input
    unit/agent/agent_debug_input_test.cpp
)

target_link_libraries(pathfinder_tests_agent_debug_input PRIVATE
    pathfinder_agent_debug_input
    pathfinder_agent_debug_cli_lib
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

add_test(
    NAME agent_debug_input_smoke
    COMMAND pathfinder_tests_agent_debug_input smoke
)

add_test(
    NAME agent_debug_input_kind_roundtrip
    COMMAND pathfinder_tests_agent_debug_input kind_roundtrip
)

add_test(
    NAME agent_debug_input_trust_level_roundtrip
    COMMAND pathfinder_tests_agent_debug_input trust_level_roundtrip
)

add_test(
    NAME agent_debug_input_validation_status_roundtrip
    COMMAND pathfinder_tests_agent_debug_input validation_status_roundtrip
)

add_test(
    NAME agent_debug_input_capability_roundtrip
    COMMAND pathfinder_tests_agent_debug_input capability_roundtrip
)

add_test(
    NAME agent_debug_input_reject_reason_roundtrip
    COMMAND pathfinder_tests_agent_debug_input reject_reason_roundtrip
)

add_test(
    NAME agent_debug_input_factory_from_fixture
    COMMAND pathfinder_tests_agent_debug_input factory_from_fixture
)

add_test(
    NAME agent_debug_input_factory_from_report
    COMMAND pathfinder_tests_agent_debug_input factory_from_report
)

add_test(
    NAME agent_debug_input_factory_from_projection
    COMMAND pathfinder_tests_agent_debug_input factory_from_projection
)

add_test(
    NAME agent_debug_input_validator_valid_fixture
    COMMAND pathfinder_tests_agent_debug_input validator_valid_fixture
)

add_test(
    NAME agent_debug_input_validator_valid_report
    COMMAND pathfinder_tests_agent_debug_input validator_valid_report
)

add_test(
    NAME agent_debug_input_validator_valid_projection
    COMMAND pathfinder_tests_agent_debug_input validator_valid_projection
)

add_test(
    NAME agent_debug_input_validator_rejects_unknown_kind
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_unknown_kind
)

add_test(
    NAME agent_debug_input_validator_rejects_unknown_trust_level
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_unknown_trust_level
)

add_test(
    NAME agent_debug_input_validator_rejects_external_untrusted
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_external_untrusted
)

add_test(
    NAME agent_debug_input_validator_rejects_missing_payload
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_missing_payload
)

add_test(
    NAME agent_debug_input_validator_rejects_conflicting_payloads
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_conflicting_payloads
)

add_test(
    NAME agent_debug_input_validator_rejects_hidden_truth
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_hidden_truth
)

add_test(
    NAME agent_debug_input_validator_rejects_raw_state
    COMMAND pathfinder_tests_agent_debug_input validator_rejects_raw_state
)

add_test(
    NAME agent_debug_input_adapter_fixture_to_report
    COMMAND pathfinder_tests_agent_debug_input adapter_fixture_to_report
)

add_test(
    NAME agent_debug_input_adapter_report_passthrough
    COMMAND pathfinder_tests_agent_debug_input adapter_report_passthrough
)

add_test(
    NAME agent_debug_input_adapter_report_adds_diagnostics
    COMMAND pathfinder_tests_agent_debug_input adapter_report_adds_diagnostics
)

add_test(
    NAME agent_debug_input_adapter_projection_to_report
    COMMAND pathfinder_tests_agent_debug_input adapter_projection_to_report
)

add_test(
    NAME agent_debug_input_adapter_rejects_export_draft_only_by_default
    COMMAND pathfinder_tests_agent_debug_input adapter_rejects_export_draft_only_by_default
)

add_test(
    NAME agent_debug_input_adapter_accepts_export_draft_only_when_allowed
    COMMAND pathfinder_tests_agent_debug_input adapter_accepts_export_draft_only_when_allowed
)

add_test(
    NAME agent_debug_input_bundle_validation_report_rules
    COMMAND pathfinder_tests_agent_debug_input bundle_validation_report_rules
)

# P14 integration test: flow tests
add_executable(pathfinder_tests_p14_flow
    integration/p14/agent_debug_input_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p14_flow PRIVATE
    pathfinder_agent_debug_input
    pathfinder_agent_debug_cli_lib
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

add_test(
    NAME p14_smoke
    COMMAND pathfinder_tests_p14_flow smoke
)

add_test(
    NAME p14_p13_fixture_via_p14_adapter
    COMMAND pathfinder_tests_p14_flow p13_fixture_via_p14_adapter
)

add_test(
    NAME p14_p13_no_decision_dry_run
    COMMAND pathfinder_tests_p14_flow p13_no_decision_dry_run
)

add_test(
    NAME p14_p11_report_via_p14_to_p12
    COMMAND pathfinder_tests_p14_flow p11_report_via_p14_to_p12
)

add_test(
    NAME p14_p10_projection_via_p14_to_report
    COMMAND pathfinder_tests_p14_flow p10_projection_via_p14_to_report
)

add_test(
    NAME p14_export_draft_no_reverse_report
    COMMAND pathfinder_tests_p14_flow export_draft_no_reverse_report
)

# P14 integration test: security tests
add_executable(pathfinder_tests_p14_security
    integration/p14/agent_debug_input_security_test.cpp
)

target_link_libraries(pathfinder_tests_p14_security PRIVATE
    pathfinder_agent_debug_input
    pathfinder_agent_debug_cli_lib
    pathfinder_agent_local_export
    pathfinder_agent_debug_export
    pathfinder_agent_debug_report
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

add_test(
    NAME p14_security_smoke
    COMMAND pathfinder_tests_p14_security smoke
)

add_test(
    NAME p14_source_label_gamestate
    COMMAND pathfinder_tests_p14_security source_label_gamestate
)

add_test(
    NAME p14_summary_keys_edible_profile
    COMMAND pathfinder_tests_p14_security summary_keys_edible_profile
)

add_test(
    NAME p14_reason_keys_hunger_delta
    COMMAND pathfinder_tests_p14_security reason_keys_hunger_delta
)

add_test(
    NAME p14_phase_keys_health_delta
    COMMAND pathfinder_tests_p14_security phase_keys_health_delta
)

add_test(
    NAME p14_warning_keys_effect_kind
    COMMAND pathfinder_tests_p14_security warning_keys_effect_kind
)

add_test(
    NAME p14_export_payload_object_definition
    COMMAND pathfinder_tests_p14_security export_payload_object_definition
)

add_test(
    NAME p14_external_untrusted
    COMMAND pathfinder_tests_p14_security external_untrusted
)

add_test(
    NAME p14_test_only_not_allowed
    COMMAND pathfinder_tests_p14_security test_only_not_allowed
)

add_test(
    NAME p14_test_only_allowed
    COMMAND pathfinder_tests_p14_security test_only_allowed
)

# P21 unit tests
add_executable(pathfinder_tests_learning
    unit/learning/learning_loop_test.cpp
)

target_link_libraries(pathfinder_tests_learning PRIVATE
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME learning_smoke
    COMMAND pathfinder_tests_learning smoke
)

add_test(
    NAME learning_enum_roundtrip
    COMMAND pathfinder_tests_learning enum_roundtrip
)

add_test(
    NAME learning_options_validate
    COMMAND pathfinder_tests_learning options_validateBasic
)

add_test(
    NAME learning_actor_ref_validate
    COMMAND pathfinder_tests_learning actor_ref_validateBasic
)

add_test(
    NAME learning_feedback_validate
    COMMAND pathfinder_tests_learning feedback_validateBasic
)

add_test(
    NAME learning_input_validate
    COMMAND pathfinder_tests_learning input_validateBasic
)

add_test(
    NAME learning_trace_validate
    COMMAND pathfinder_tests_learning trace_validateBasic
)

add_test(
    NAME learning_result_validate
    COMMAND pathfinder_tests_learning result_validateBasic
)

add_test(
    NAME learning_report_validate
    COMMAND pathfinder_tests_learning report_validateBasic
)

add_test(
    NAME learning_unknown_testonly_rejected
    COMMAND pathfinder_tests_learning unknown_testonly_rejected
)

add_test(
    NAME learning_forbidden_key_rejected
    COMMAND pathfinder_tests_learning forbidden_key_rejected
)

add_test(
    NAME learning_ok_dto_must_validate
    COMMAND pathfinder_tests_learning ok_dto_must_validate
)

add_test(
    NAME learning_planner_skips_propagation_disabled
    COMMAND pathfinder_tests_learning planner_skips_propagation_when_disabled
)

add_test(
    NAME learning_planner_skips_propagation_no_recipient
    COMMAND pathfinder_tests_learning planner_skips_propagation_without_recipient
)

add_test(
    NAME learning_service_direct_discovery
    COMMAND pathfinder_tests_learning service_runs_direct_discovery
)

add_test(
    NAME learning_service_with_recipient
    COMMAND pathfinder_tests_learning service_runs_with_recipient
)

add_test(
    NAME learning_applier_actor_claims
    COMMAND pathfinder_tests_learning applier_actor_claims
)

add_test(
    NAME learning_report_builder
    COMMAND pathfinder_tests_learning report_builder
)

# P21 integration tests: discovery story
add_executable(pathfinder_tests_p21_discovery
    integration/p21/learning_loop_discovery_story_test.cpp
)

target_link_libraries(pathfinder_tests_p21_discovery PRIVATE
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p21_discovery_smoke
    COMMAND pathfinder_tests_p21_discovery smoke
)

add_test(
    NAME p21_discovery_and_teaching
    COMMAND pathfinder_tests_p21_discovery discovery_and_teaching
)

add_test(
    NAME p21_result_no_hidden_truth
    COMMAND pathfinder_tests_p21_discovery result_no_hidden_truth
)

# P21 integration tests: revision story
add_executable(pathfinder_tests_p21_revision
    integration/p21/learning_loop_revision_story_test.cpp
)

target_link_libraries(pathfinder_tests_p21_revision PRIVATE
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p21_revision_smoke
    COMMAND pathfinder_tests_p21_revision smoke
)

add_test(
    NAME p21_revision_and_propagation_route
    COMMAND pathfinder_tests_p21_revision revision_and_propagation_route
)

add_test(
    NAME p21_low_confidence_teaching_trace
    COMMAND pathfinder_tests_p21_revision low_confidence_teaching_trace
)

# P21 integration tests: security
add_executable(pathfinder_tests_p21_security
    integration/p21/learning_loop_security_test.cpp
)

target_link_libraries(pathfinder_tests_p21_security PRIVATE
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p21_security_smoke
    COMMAND pathfinder_tests_p21_security smoke
)

add_test(
    NAME p21_feedback_key_hidden_truth
    COMMAND pathfinder_tests_p21_security feedback_key_hidden_truth_rejected
)

add_test(
    NAME p21_effect_key_real_effect
    COMMAND pathfinder_tests_p21_security effect_key_real_effect_rejected
)

add_test(
    NAME p21_reason_key_raw_state
    COMMAND pathfinder_tests_p21_security reason_key_raw_state_rejected
)

add_test(
    NAME p21_input_gamestate_key
    COMMAND pathfinder_tests_p21_security input_gamestate_key_rejected
)

add_test(
    NAME p21_recipient_owner_mismatch
    COMMAND pathfinder_tests_p21_security recipient_owner_mismatch_rejected
)

add_test(
    NAME p21_propagation_no_teaching
    COMMAND pathfinder_tests_p21_security propagation_without_teaching_evidence_rejected
)


# P23 unit tests: tribe types
add_executable(pathfinder_tests_tribe_types
    unit/tribe/tribe_types_test.cpp
)

target_link_libraries(pathfinder_tests_tribe_types PRIVATE
    pathfinder_tribe
    pathfinder_foundation
)

add_test(
    NAME tribe_role_enum_roundtrip
    COMMAND pathfinder_tests_tribe_types role_enum_roundtrip
)

add_test(
    NAME tribe_cohesion_enum_roundtrip
    COMMAND pathfinder_tests_tribe_types cohesion_enum_roundtrip
)

add_test(
    NAME tribe_member_validate
    COMMAND pathfinder_tests_tribe_types member_validate
)

add_test(
    NAME tribe_split_risk_validate
    COMMAND pathfinder_tests_tribe_types split_risk_validate
)

# P23 unit tests: tribe state
add_executable(pathfinder_tests_tribe_state
    unit/tribe/tribe_state_test.cpp
)

target_link_libraries(pathfinder_tests_tribe_state PRIVATE
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(
    NAME tribe_state_validate
    COMMAND pathfinder_tests_tribe_state state_validate
)

add_test(
    NAME tribe_projection_safe_summary
    COMMAND pathfinder_tests_tribe_state projection_safe_summary
)

add_test(
    NAME tribe_resolver_member_join
    COMMAND pathfinder_tests_tribe_state resolver_member_join
)

add_test(
    NAME tribe_resolver_role_change
    COMMAND pathfinder_tests_tribe_state resolver_role_change
)

add_test(
    NAME tribe_resolver_morale_trust
    COMMAND pathfinder_tests_tribe_state resolver_morale_trust
)

add_test(
    NAME tribe_resolver_split_risk
    COMMAND pathfinder_tests_tribe_state resolver_split_risk
)

add_test(
    NAME tribe_resolver_member_leave
    COMMAND pathfinder_tests_tribe_state resolver_member_leave
)

# P23 integration tests: tribe state flows
add_executable(pathfinder_tests_p23_flow
    integration/p23/tribe_state_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p23_flow PRIVATE
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(
    NAME p23_tribe_creation_flow
    COMMAND pathfinder_tests_p23_flow tribe_creation_flow
)

add_test(
    NAME p23_tribe_role_and_projection_flow
    COMMAND pathfinder_tests_p23_flow tribe_role_and_projection_flow
)

add_test(
    NAME p23_split_risk_deterministic_flow
    COMMAND pathfinder_tests_p23_flow split_risk_deterministic_flow
)

# P23 integration tests: tribe security
add_executable(pathfinder_tests_p23_security
    integration/p23/tribe_state_security_test.cpp
)

target_link_libraries(pathfinder_tests_p23_security PRIVATE
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(
    NAME p23_security_forbidden_keys
    COMMAND pathfinder_tests_p23_security forbidden_keys
)

add_test(
    NAME p23_security_rejects_random_split
    COMMAND pathfinder_tests_p23_security rejects_random_split
)

add_test(
    NAME p23_security_tribe_id_empty_rejected
    COMMAND pathfinder_tests_p23_security tribe_id_empty_rejected
)

add_test(
    NAME p23_security_member_id_empty_rejected
    COMMAND pathfinder_tests_p23_security member_id_empty_rejected
)

add_test(
    NAME p23_security_ratio_out_of_range_rejected
    COMMAND pathfinder_tests_p23_security ratio_out_of_range_rejected
)

add_test(
    NAME p23_security_active_population_exceeds_total_rejected
    COMMAND pathfinder_tests_p23_security active_population_exceeds_total_rejected
)

add_test(
    NAME p23_security_duplicate_member_id_rejected
    COMMAND pathfinder_tests_p23_security duplicate_member_id_rejected
)

add_test(
    NAME p23_security_duplicate_knowledge_id_rejected
    COMMAND pathfinder_tests_p23_security duplicate_knowledge_id_rejected
)

add_test(
    NAME p23_security_test_only_enum_rejected
    COMMAND pathfinder_tests_p23_security test_only_enum_rejected
)

add_test(
    NAME p23_security_trust_changed_missing_trust_rejected
    COMMAND pathfinder_tests_p23_security trust_changed_missing_trust_rejected
)

add_test(
    NAME p23_security_morale_changed_missing_morale_rejected
    COMMAND pathfinder_tests_p23_security morale_changed_missing_morale_rejected
)

add_test(
    NAME p23_security_duplicate_member_events_rejected
    COMMAND pathfinder_tests_p23_security duplicate_member_events_rejected
)

add_test(
    NAME p23_security_duplicate_knowledge_events_rejected
    COMMAND pathfinder_tests_p23_security duplicate_knowledge_events_rejected
)

add_test(
    NAME p23_security_draft_validation_rejected
    COMMAND pathfinder_tests_p23_security draft_validation_rejected
)

# P24 unit tests: tribe coordination
add_executable(pathfinder_tests_tribe_coordination
    unit/tribe/tribe_coordination_test.cpp
)

target_link_libraries(pathfinder_tests_tribe_coordination PRIVATE
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(
    NAME tribe_coordination_intent_enum_roundtrip
    COMMAND pathfinder_tests_tribe_coordination intent_enum_roundtrip
)

add_test(
    NAME tribe_coordination_action_enum_roundtrip
    COMMAND pathfinder_tests_tribe_coordination action_enum_roundtrip
)

add_test(
    NAME tribe_coordination_quality_threshold
    COMMAND pathfinder_tests_tribe_coordination quality_threshold
)

add_test(
    NAME tribe_coordination_input_validate
    COMMAND pathfinder_tests_tribe_coordination input_validate
)

add_test(
    NAME tribe_coordination_result_validate
    COMMAND pathfinder_tests_tribe_coordination result_validate
)

add_test(
    NAME tribe_coordination_projection_safe_summary
    COMMAND pathfinder_tests_tribe_coordination projection_safe_summary
)

add_test(
    NAME tribe_coordination_member_summary_unique
    COMMAND pathfinder_tests_tribe_coordination member_summary_unique
)

# P24 integration tests: coordination flows
add_executable(pathfinder_tests_p24_flow
    integration/p24/tribe_coordination_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p24_flow PRIVATE
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(
    NAME p24_guardian_defends_learner_flow
    COMMAND pathfinder_tests_p24_flow guardian_defends_learner
)

add_test(
    NAME p24_low_morale_avoid_engagement_flow
    COMMAND pathfinder_tests_p24_flow low_morale_avoid_engagement
)

add_test(
    NAME p24_high_trust_focus_threat_flow
    COMMAND pathfinder_tests_p24_flow high_trust_focus_threat
)

add_test(
    NAME p24_split_risk_weakens_coordination_flow
    COMMAND pathfinder_tests_p24_flow split_risk_weakens_coordination
)

add_test(
    NAME p24_deterministic_coordination_flow
    COMMAND pathfinder_tests_p24_flow deterministic_coordination
)

add_test(
    NAME p24_state_draft_unique_member_effects_flow
    COMMAND pathfinder_tests_p24_flow state_draft_unique_member_effects
)

add_test(
    NAME p24_explorer_warns_group_flow
    COMMAND pathfinder_tests_p24_flow explorer_warns_group
)

# P24 integration tests: coordination security
add_executable(pathfinder_tests_p24_security
    integration/p24/tribe_coordination_security_test.cpp
)

target_link_libraries(pathfinder_tests_p24_security PRIVATE
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(
    NAME p24_security_forbidden_keys
    COMMAND pathfinder_tests_p24_security forbidden_keys
)

add_test(
    NAME p24_security_rejects_damage_or_death
    COMMAND pathfinder_tests_p24_security rejects_damage_or_death
)

add_test(
    NAME p24_security_rejects_duplicate_member_effects
    COMMAND pathfinder_tests_p24_security rejects_duplicate_member_effects
)

add_test(
    NAME p24_security_rejects_test_only_enums
    COMMAND pathfinder_tests_p24_security rejects_test_only_enums
)

add_test(
    NAME p24_security_empty_ids_rejected
    COMMAND pathfinder_tests_p24_security empty_ids_rejected
)

add_test(
    NAME p24_security_protected_key_forbidden
    COMMAND pathfinder_tests_p24_security protected_key_forbidden
)

add_test(
    NAME p24_security_generic_damage_death_kill_loot
    COMMAND pathfinder_tests_p24_security generic_damage_death_kill_loot
)

add_test(
    NAME p24_security_empty_tribe_state
    COMMAND pathfinder_tests_p24_security empty_tribe_state
)

add_test(
    NAME p24_security_invalid_knowledge_id
    COMMAND pathfinder_tests_p24_security invalid_knowledge_id
)

# --- P25 unit tests: tribe_conflict ---

add_executable(pathfinder_tests_tribe_conflict
    unit/tribe/tribe_conflict_test.cpp
)

target_link_libraries(pathfinder_tests_tribe_conflict PRIVATE
    pathfinder_tribe
)

add_test(
    NAME tribe_conflict_hostility_enum_roundtrip
    COMMAND pathfinder_tests_tribe_conflict hostility_enum_roundtrip
)
add_test(
    NAME tribe_conflict_intent_enum_roundtrip
    COMMAND pathfinder_tests_tribe_conflict conflict_intent_enum_roundtrip
)
add_test(
    NAME tribe_conflict_outcome_enum_roundtrip
    COMMAND pathfinder_tests_tribe_conflict conflict_outcome_enum_roundtrip
)
add_test(
    NAME tribe_conflict_tribe_relation_validate
    COMMAND pathfinder_tests_tribe_conflict tribe_relation_validate
)
add_test(
    NAME tribe_conflict_pressure_summary_validate
    COMMAND pathfinder_tests_tribe_conflict conflict_pressure_summary_validate
)
add_test(
    NAME tribe_conflict_participant_requires_coordination
    COMMAND pathfinder_tests_tribe_conflict conflict_participant_requires_coordination
)
add_test(
    NAME tribe_conflict_resolution_input_validate
    COMMAND pathfinder_tests_tribe_conflict conflict_resolution_input_validate
)
add_test(
    NAME tribe_conflict_resolution_result_validate
    COMMAND pathfinder_tests_tribe_conflict conflict_resolution_result_validate
)
add_test(
    NAME tribe_conflict_projection_safe_summary
    COMMAND pathfinder_tests_tribe_conflict conflict_projection_safe_summary
)
add_test(
    NAME tribe_conflict_trace_safe_summary
    COMMAND pathfinder_tests_tribe_conflict conflict_trace_safe_summary
)
add_test(
    NAME tribe_conflict_state_draft_unique_member
    COMMAND pathfinder_tests_tribe_conflict conflict_state_draft_unique_member
)
add_test(
    NAME tribe_conflict_intent_rejects_invalid_stance_combo
    COMMAND pathfinder_tests_tribe_conflict conflict_intent_invalid_stance
)

# --- P25 integration tests: conflict flow ---

add_executable(pathfinder_tests_p25_flow
    integration/p25/tribe_conflict_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p25_flow PRIVATE
    pathfinder_tribe
)

add_test(
    NAME p25_neutral_low_pressure_avoids_conflict_flow
    COMMAND pathfinder_tests_p25_flow neutral_low_pressure
)
add_test(
    NAME p25_resource_contest_standoff_flow
    COMMAND pathfinder_tests_p25_flow resource_contest
)
add_test(
    NAME p25_high_coordination_intimidates_flow
    COMMAND pathfinder_tests_p25_flow high_coordination_intimidates
)
add_test(
    NAME p25_low_morale_forced_retreat_flow
    COMMAND pathfinder_tests_p25_flow low_morale_retreat
)
add_test(
    NAME p25_hostility_escalates_deterministically_flow
    COMMAND pathfinder_tests_p25_flow hostility_escalates
)
add_test(
    NAME p25_loss_pressure_without_death_flow
    COMMAND pathfinder_tests_p25_flow loss_pressure_no_death
)
add_test(
    NAME p25_truce_offered_when_pressure_drops_flow
    COMMAND pathfinder_tests_p25_flow truce_pressure_drops
)
add_test(
    NAME p25_uses_p24_coordination_result_flow
    COMMAND pathfinder_tests_p25_flow uses_p24
)
add_test(
    NAME p25_relation_draft_does_not_mutate_state_flow
    COMMAND pathfinder_tests_p25_flow relation_draft
)
add_test(
    NAME p25_deterministic_resolution_flow
    COMMAND pathfinder_tests_p25_flow deterministic
)
add_test(
    NAME p25_merges_p24_state_draft_pressure_flow
    COMMAND pathfinder_tests_p25_flow merges_p24
)
add_test(
    NAME p25_conflict_outcome_trust_delta_not_overwritten_flow
    COMMAND pathfinder_tests_p25_flow trust_delta_not_overwritten
)
add_test(
    NAME p25_truce_offered_with_medium_strength_high_loss_flow
    COMMAND pathfinder_tests_p25_flow truce_medium_strength
)
add_test(
    NAME p25_projection_includes_target_pressure_flow
    COMMAND pathfinder_tests_p25_flow projection_target
)
add_test(
    NAME p25_intimidated_applies_pressure_to_losing_tribe_flow
    COMMAND pathfinder_tests_p25_flow intimidated_loser_pressure
)
add_test(
    NAME p25_forced_retreat_applies_retreat_pressure_to_forced_side_flow
    COMMAND pathfinder_tests_p25_flow forced_retreat_pressure
)

# --- P25 integration tests: conflict security ---

add_executable(pathfinder_tests_p25_security
    integration/p25/tribe_conflict_security_test.cpp
)

target_link_libraries(pathfinder_tests_p25_security PRIVATE
    pathfinder_tribe
)

add_test(
    NAME p25_security_rejects_missing_coordination
    COMMAND pathfinder_tests_p25_security missing_coordination
)
add_test(
    NAME p25_security_rejects_raw_game_state
    COMMAND pathfinder_tests_p25_security raw_game_state
)
add_test(
    NAME p25_security_rejects_hidden_truth
    COMMAND pathfinder_tests_p25_security hidden_truth
)
add_test(
    NAME p25_security_rejects_damage_death_keys
    COMMAND pathfinder_tests_p25_security damage_death_keys
)
add_test(
    NAME p25_security_projection_no_hidden_truth
    COMMAND pathfinder_tests_p25_security projection_no_hidden
)
add_test(
    NAME p25_security_trace_no_hp_death
    COMMAND pathfinder_tests_p25_security trace_no_hp_death
)
add_test(
    NAME p25_security_rejects_duplicate_member
    COMMAND pathfinder_tests_p25_security duplicate_member
)
add_test(
    NAME p25_security_rejects_same_tribe
    COMMAND pathfinder_tests_p25_security same_tribe
)
add_test(
    NAME p25_security_rejects_test_only_enum
    COMMAND pathfinder_tests_p25_security test_only_enum
)
add_test(
    NAME p25_security_rejects_target_relation_direction_mismatch
    COMMAND pathfinder_tests_p25_security relation_direction
)
add_test(
    NAME p25_security_safe_key_not_rejected_by_short_hp_token
    COMMAND pathfinder_tests_p25_security safe_hp_key
)

# Replay diagnostics tests (P31)
add_executable(pathfinder_tests_p31_golden_replay_diagnostics
    integration/p31/golden_replay_diagnostics_test.cpp
)

target_link_libraries(pathfinder_tests_p31_golden_replay_diagnostics PRIVATE
    pathfinder_replay_diagnostics
    pathfinder_save
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

add_test(NAME p31_golden_replay_enum COMMAND pathfinder_tests_p31_golden_replay_diagnostics enum)
add_test(NAME p31_golden_replay_pass COMMAND pathfinder_tests_p31_golden_replay_diagnostics pass)
add_test(NAME p31_package_integrity_diagnostic COMMAND pathfinder_tests_p31_golden_replay_diagnostics integrity)
add_test(NAME p31_replay_mismatch_diagnostic COMMAND pathfinder_tests_p31_golden_replay_diagnostics mismatch)
add_test(NAME p31_condition_trace_diagnostic COMMAND pathfinder_tests_p31_golden_replay_diagnostics condition)
add_test(NAME p31_baseline_mismatch_diagnostic COMMAND pathfinder_tests_p31_golden_replay_diagnostics baseline)
add_test(NAME p31_restore_plan_diagnostic COMMAND pathfinder_tests_p31_golden_replay_diagnostics restore)

# Agent reasoning tests (P39)
add_executable(pathfinder_tests_agent_reasoning
    unit/agent_reasoning/agent_reasoning_test.cpp
)

target_link_libraries(pathfinder_tests_agent_reasoning PRIVATE
    pathfinder_agent_reasoning
    pathfinder_world_interaction_types
)

add_test(NAME agent_reasoning_effect_semantics COMMAND pathfinder_tests_agent_reasoning effect_semantics_registry_validation)
add_test(NAME p41_execution_enums_and_dto COMMAND pathfinder_tests_agent_reasoning p41_execution_enums_and_dto_validation)
add_test(NAME p41_execution_registry COMMAND pathfinder_tests_agent_reasoning p41_execution_registry_validation)
add_test(NAME p41_world_effect_executor_make_torch_atomic COMMAND pathfinder_tests_agent_reasoning p41_world_effect_executor_make_torch_atomic)
add_test(NAME p41_agent_step_executor_blocks_without_torch COMMAND pathfinder_tests_agent_reasoning p41_agent_step_executor_blocks_without_torch)
add_test(NAME p41_magic_fireball_area_side_effect_dry_run COMMAND pathfinder_tests_agent_reasoning p41_magic_fireball_area_side_effect_dry_run)
add_test(NAME p41_mechanical_replication_system_pressure_dry_run COMMAND pathfinder_tests_agent_reasoning p41_mechanical_replication_system_pressure_dry_run)
add_test(NAME p41_tribe_capability_unlock_dry_run COMMAND pathfinder_tests_agent_reasoning p41_tribe_capability_unlock_dry_run)
add_test(NAME p41_diplomacy_agreement_dto_validation COMMAND pathfinder_tests_agent_reasoning p41_diplomacy_agreement_dto_validation)
add_test(NAME p41_plague_delayed_spread_dto_validation COMMAND pathfinder_tests_agent_reasoning p41_plague_delayed_spread_dto_validation)
add_test(NAME agent_reasoning_goal_hunger COMMAND pathfinder_tests_agent_reasoning goal_generation_hunger)
add_test(NAME agent_reasoning_owned_knowledge COMMAND pathfinder_tests_agent_reasoning candidate_from_owned_knowledge_only)
add_test(NAME agent_reasoning_hungry_food COMMAND pathfinder_tests_agent_reasoning hungry_agent_selects_known_safe_food)
add_test(NAME agent_reasoning_poison_rejected COMMAND pathfinder_tests_agent_reasoning poison_food_rejected)
add_test(NAME agent_reasoning_cold_prefers_fire COMMAND pathfinder_tests_agent_reasoning cold_agent_prefers_fire_when_urgent)
add_test(NAME agent_reasoning_shelter_plan COMMAND pathfinder_tests_agent_reasoning shelter_shortage_build_house_plan)
add_test(NAME agent_reasoning_build_house_chain COMMAND pathfinder_tests_agent_reasoning build_house_requires_wood_chain)
add_test(NAME agent_reasoning_dull_axe_chain COMMAND pathfinder_tests_agent_reasoning dull_axe_requires_sharpen_chain)
add_test(NAME agent_reasoning_beast_torch COMMAND pathfinder_tests_agent_reasoning beast_threat_selects_torch_repel)
add_test(NAME agent_reasoning_make_torch_reaction_rule COMMAND pathfinder_tests_agent_reasoning make_torch_uses_reaction_rule_preconditions)
add_test(NAME agent_reasoning_reaction_adapter_rules COMMAND pathfinder_tests_agent_reasoning reaction_adapter_reads_crafting_rules)
add_test(NAME agent_reasoning_cycle_detection COMMAND pathfinder_tests_agent_reasoning cycle_detection_blocks_plan)
add_test(NAME agent_reasoning_search_limit COMMAND pathfinder_tests_agent_reasoning search_limit_returns_diagnostic)
add_test(NAME agent_reasoning_projection_safe COMMAND pathfinder_tests_agent_reasoning projection_safe)

# Goal execution tests (P40)
add_executable(pathfinder_tests_goal_execution
    unit/goal_execution/goal_execution_test.cpp
)

target_link_libraries(pathfinder_tests_goal_execution PRIVATE
    pathfinder_goal_execution
    pathfinder_agent_reasoning
    pathfinder_world_interaction_types
    pathfinder_reaction
)

add_test(NAME goal_execution_stack COMMAND pathfinder_tests_goal_execution stack)
add_test(NAME goal_execution_registry COMMAND pathfinder_tests_goal_execution registry)
add_test(NAME goal_execution_dull_axe COMMAND pathfinder_tests_goal_execution dull_axe)
add_test(NAME internal_blocker_pushes_subgoal_frame COMMAND pathfinder_tests_goal_execution dull_axe)
add_test(NAME goal_execution_blocker_subgoal COMMAND pathfinder_tests_goal_execution blocker_subgoal)
add_test(NAME goal_execution_wolf_interrupt COMMAND pathfinder_tests_goal_execution wolf_interrupt)
add_test(NAME goal_execution_resume COMMAND pathfinder_tests_goal_execution resume)
add_test(NAME resume_restores_driver_state COMMAND pathfinder_tests_goal_execution resume)
add_test(NAME goal_execution_command_adapter COMMAND pathfinder_tests_goal_execution command_adapter)
add_test(NAME goal_execution_projection_safe COMMAND pathfinder_tests_goal_execution projection_safe)
add_test(NAME goal_execution_materials COMMAND pathfinder_tests_goal_execution materials)
add_test(NAME goal_execution_reaction_links COMMAND pathfinder_tests_goal_execution reaction_links)
add_test(NAME goal_execution_reaction_material_resolver_no_unknown_material_for_core_rules COMMAND pathfinder_tests_goal_execution reaction_material_resolver_no_unknown_material_for_core_rules)
add_test(NAME goal_execution_known_claims COMMAND pathfinder_tests_goal_execution known_claims)
add_test(NAME goal_execution_build_recipe COMMAND pathfinder_tests_goal_execution build_recipe)
add_test(NAME goal_execution_reservation_release COMMAND pathfinder_tests_goal_execution reservation_release)
add_test(NAME goal_execution_tick_order COMMAND pathfinder_tests_goal_execution tick_order)
add_test(NAME goal_execution_player_delay COMMAND pathfinder_tests_goal_execution player_delay)
add_test(NAME goal_execution_capability_filter COMMAND pathfinder_tests_goal_execution capability_filter)
add_test(NAME goal_execution_context_roundtrip COMMAND pathfinder_tests_goal_execution context_roundtrip)

# Content tests (P42)
add_executable(pathfinder_tests_content
    unit/content/content_loader_test.cpp
    unit/content/content_registry_test.cpp
    unit/content/content_validation_test.cpp
)

target_include_directories(pathfinder_tests_content PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

target_link_libraries(pathfinder_tests_content PRIVATE
    pathfinder_content
    pathfinder_config
    pathfinder_foundation
)

add_test(NAME content_loader COMMAND pathfinder_tests_content loader)
add_test(NAME content_registry COMMAND pathfinder_tests_content registry)
add_test(NAME content_validation COMMAND pathfinder_tests_content validation)

# World Command unit tests (P43)
add_executable(pathfinder_tests_world_command
    unit/world_command/world_command_test.cpp
    unit/world_command/world_command_dispatcher_test.cpp
)

target_link_libraries(pathfinder_tests_world_command PRIVATE
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_command_enum COMMAND pathfinder_tests_world_command enum)
add_test(NAME world_command_dto_validation COMMAND pathfinder_tests_world_command dto_validation)
add_test(NAME world_command_pipeline COMMAND pathfinder_tests_world_command pipeline)
add_test(NAME world_command_registry COMMAND pathfinder_tests_world_command registry)
add_test(NAME world_command_trace COMMAND pathfinder_tests_world_command trace)
add_test(NAME world_command_trace_isolation COMMAND pathfinder_tests_world_command trace_isolation)
add_test(NAME world_command_source_distinguish COMMAND pathfinder_tests_world_command source_distinguish)
add_test(NAME world_command_dispatcher_lookup COMMAND pathfinder_tests_world_command lookup)
add_test(NAME world_command_dispatcher_no_playbook_rules COMMAND pathfinder_tests_world_command no_playbook_rules)

# World Command integration tests (P43)
add_executable(pathfinder_tests_world_command_integration
    integration/world_command/world_command_flow_test.cpp
)

target_link_libraries(pathfinder_tests_world_command_integration PRIVATE
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_command_flow_bootstrap COMMAND pathfinder_tests_world_command_integration bootstrap)
add_test(NAME world_command_flow_sequence COMMAND pathfinder_tests_world_command_integration sequence)

# World Runtime unit tests (P44)
add_executable(pathfinder_tests_world_runtime
    unit/world_runtime/world_runtime_test.cpp
    unit/world_runtime/world_runtime_coordinate_test.cpp
    unit/world_runtime/world_runtime_grid_test.cpp
    unit/world_runtime/world_runtime_projection_test.cpp
)

target_link_libraries(pathfinder_tests_world_runtime PRIVATE
    pathfinder_world_runtime
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_runtime_smoke COMMAND pathfinder_tests_world_runtime smoke)
add_test(NAME world_runtime_coordinate_id COMMAND pathfinder_tests_world_runtime coordinate_id)
add_test(NAME world_runtime_coordinate_comparison COMMAND pathfinder_tests_world_runtime coordinate_comparison)
add_test(NAME world_runtime_enum COMMAND pathfinder_tests_world_runtime enum)
add_test(NAME world_runtime_generate_deterministic COMMAND pathfinder_tests_world_runtime generate_deterministic)
add_test(NAME world_runtime_find_actor COMMAND pathfinder_tests_world_runtime find_actor)
add_test(NAME world_runtime_move_adjacent COMMAND pathfinder_tests_world_runtime move_adjacent)
add_test(NAME world_runtime_move_blocked COMMAND pathfinder_tests_world_runtime move_blocked)
add_test(NAME world_runtime_move_not_adjacent COMMAND pathfinder_tests_world_runtime move_not_adjacent)
add_test(NAME world_runtime_move_out_of_bounds COMMAND pathfinder_tests_world_runtime move_out_of_bounds)
add_test(NAME world_runtime_visibility_update COMMAND pathfinder_tests_world_runtime visibility_update)
add_test(NAME world_runtime_time_step COMMAND pathfinder_tests_world_runtime time_step)
add_test(NAME world_runtime_actor_missing COMMAND pathfinder_tests_world_runtime actor_missing)
add_test(NAME world_runtime_inspect COMMAND pathfinder_tests_world_runtime inspect)
add_test(NAME world_runtime_entity_location COMMAND pathfinder_tests_world_runtime entity_location)
add_test(NAME world_runtime_projection_cell COMMAND pathfinder_tests_world_runtime projection_cell)
add_test(NAME world_runtime_projection_entity COMMAND pathfinder_tests_world_runtime projection_entity)
add_test(NAME world_runtime_projection_no_unknown_cells COMMAND pathfinder_tests_world_runtime projection_no_unknown_cells)
add_test(NAME world_runtime_deterministic_entity_ids COMMAND pathfinder_tests_world_runtime deterministic_entity_ids)
add_test(NAME world_runtime_origin_contains_player_entity COMMAND pathfinder_tests_world_runtime origin_contains_player_entity)

# World Runtime integration tests (P44)
add_executable(pathfinder_tests_world_runtime_integration
    integration/world_runtime/world_runtime_command_flow_main.cpp
    integration/world_runtime/world_runtime_command_flow_test.cpp
)

target_link_libraries(pathfinder_tests_world_runtime_integration PRIVATE
    pathfinder_world_command_runtime
    pathfinder_world_runtime
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_runtime_integration_smoke COMMAND pathfinder_tests_world_runtime_integration smoke)
add_test(NAME world_command_generate_world_runtime COMMAND pathfinder_tests_world_runtime_integration generate_world_runtime)
add_test(NAME world_command_move_runtime COMMAND pathfinder_tests_world_runtime_integration move_runtime)
add_test(NAME world_command_move_blocked_runtime COMMAND pathfinder_tests_world_runtime_integration move_blocked_runtime)
add_test(NAME world_command_inspect_runtime COMMAND pathfinder_tests_world_runtime_integration inspect_runtime)
add_test(NAME world_command_projection_patch_runtime COMMAND pathfinder_tests_world_runtime_integration projection_patch_runtime)
add_test(NAME world_command_pipeline_with_runtime COMMAND pathfinder_tests_world_runtime_integration pipeline_with_runtime)
add_test(NAME world_runtime_no_playbook_rules COMMAND pathfinder_tests_world_runtime_integration no_playbook_rules)
add_test(NAME world_command_wait_runtime COMMAND pathfinder_tests_world_runtime_integration wait_runtime)
add_test(NAME world_command_pipeline_projection_patch_runtime COMMAND pathfinder_tests_world_runtime_integration pipeline_projection_patch_runtime)
add_test(NAME world_command_runtime_invalid_parameters COMMAND pathfinder_tests_world_runtime_integration invalid_parameters)

# Architecture guard test: world_command must not include world_runtime headers
add_test(
    NAME architecture_world_command_no_runtime_dependency
    COMMAND bash -c "! grep -r 'pathfinder/world_runtime' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_command/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_command/"
)
add_test(NAME world_command_flow_response_completeness COMMAND pathfinder_tests_world_command_integration response_completeness)
add_test(NAME world_command_flow_invalid_command COMMAND pathfinder_tests_world_command_integration invalid_command)
add_test(NAME world_command_flow_trace_integrity COMMAND pathfinder_tests_world_command_integration trace_integrity)

# World Inventory unit tests (P45)
add_executable(pathfinder_tests_world_inventory
    unit/world_inventory/world_inventory_test.cpp
)

target_link_libraries(pathfinder_tests_world_inventory PRIVATE
    pathfinder_world_inventory
    pathfinder_world_runtime
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_inventory_owner_id COMMAND pathfinder_tests_world_inventory owner_id)
add_test(NAME world_inventory_ensure_actor_inventory COMMAND pathfinder_tests_world_inventory ensure_actor_inventory)
add_test(NAME world_inventory_pickup_from_map COMMAND pathfinder_tests_world_inventory pickup_from_map)
add_test(NAME world_inventory_drop_to_map COMMAND pathfinder_tests_world_inventory drop_to_map)
add_test(NAME world_inventory_no_duplicate_location COMMAND pathfinder_tests_world_inventory no_duplicate_location)
add_test(NAME world_inventory_quantity_invalid COMMAND pathfinder_tests_world_inventory quantity_invalid)
add_test(NAME world_inventory_quantity_insufficient COMMAND pathfinder_tests_world_inventory quantity_insufficient)
add_test(NAME world_inventory_capacity_exceeded COMMAND pathfinder_tests_world_inventory capacity_exceeded)
add_test(NAME world_inventory_state_version COMMAND pathfinder_tests_world_inventory state_version)
add_test(NAME world_inventory_stack_key_separates_states COMMAND pathfinder_tests_world_inventory stack_key_separates_states)
add_test(NAME world_inventory_non_stackable_not_merged COMMAND pathfinder_tests_world_inventory non_stackable_not_merged)
add_test(NAME world_inventory_pickup_too_far COMMAND pathfinder_tests_world_inventory pickup_too_far)
add_test(NAME world_inventory_pickup_missing_item COMMAND pathfinder_tests_world_inventory pickup_missing_item)
add_test(NAME world_inventory_drop_not_owned COMMAND pathfinder_tests_world_inventory drop_not_owned)
add_test(NAME world_inventory_drop_partial_stack COMMAND pathfinder_tests_world_inventory drop_partial_stack)
add_test(NAME world_inventory_pickup_duplicate_click_fails COMMAND pathfinder_tests_world_inventory pickup_duplicate_click_fails)
add_test(NAME world_inventory_source_proof_actor_self_only COMMAND pathfinder_tests_world_inventory source_proof_actor_self_only)
add_test(NAME world_inventory_no_default_camp_sharing COMMAND pathfinder_tests_world_inventory no_default_camp_sharing)
add_test(NAME world_inventory_pickup_requires_visible_target COMMAND pathfinder_tests_world_inventory pickup_requires_visible_target)
add_test(NAME world_inventory_container_not_empty_blocked COMMAND pathfinder_tests_world_inventory container_not_empty_blocked)
add_test(NAME world_inventory_validate_no_state_change COMMAND pathfinder_tests_world_inventory validate_no_state_change)
add_test(NAME world_inventory_pickup_merge_consistency COMMAND pathfinder_tests_world_inventory pickup_merge_consistency)
add_test(NAME world_inventory_non_stackable_runtime COMMAND pathfinder_tests_world_inventory non_stackable_runtime)
add_test(NAME world_inventory_drop_partial_stack_ground COMMAND pathfinder_tests_world_inventory drop_partial_stack_ground)
add_test(NAME world_inventory_capacity_real_slots COMMAND pathfinder_tests_world_inventory capacity_real_slots)
add_test(NAME world_inventory_full_drop_multi_entry COMMAND pathfinder_tests_world_inventory full_drop_multi_entry)
add_test(NAME world_inventory_drop_cell_invalid COMMAND pathfinder_tests_world_inventory drop_cell_invalid)
add_test(NAME world_inventory_drop_projection_new_entity COMMAND pathfinder_tests_world_inventory drop_projection_new_entity)
add_test(NAME world_inventory_pickup_partial_ground_stack_keeps_remainder COMMAND pathfinder_tests_world_inventory pickup_partial_ground_stack_keeps_remainder)
add_test(NAME world_inventory_pickup_partial_ground_stack_has_distinct_inventory_entity COMMAND pathfinder_tests_world_inventory pickup_partial_ground_stack_has_distinct_inventory_entity)
add_test(NAME world_inventory_full_drop_merge_ground_cleans_source_entity COMMAND pathfinder_tests_world_inventory full_drop_merge_ground_cleans_source_entity)
add_test(NAME world_inventory_drop_merge_ground_changed_entity_ids COMMAND pathfinder_tests_world_inventory drop_merge_ground_changed_entity_ids)
add_test(NAME world_inventory_stack_key_blocks_merge COMMAND pathfinder_tests_world_inventory stack_key_blocks_merge)
add_test(NAME world_inventory_consume_to_nowhere_reduces_stack COMMAND pathfinder_tests_world_inventory consume_to_nowhere_reduces_stack)
add_test(NAME world_inventory_consume_to_nowhere_removes_stack_at_zero COMMAND pathfinder_tests_world_inventory consume_to_nowhere_removes_stack_at_zero)
add_test(NAME world_inventory_consume_to_nowhere_insufficient_fails COMMAND pathfinder_tests_world_inventory consume_to_nowhere_insufficient_fails)
add_test(NAME world_inventory_spawn_to_inventory_adds_item COMMAND pathfinder_tests_world_inventory spawn_to_inventory_adds_item)
add_test(NAME world_inventory_spawn_to_inventory_merges_stack COMMAND pathfinder_tests_world_inventory spawn_to_inventory_merges_stack)
add_test(NAME world_inventory_spawn_to_inventory_creates_world_entity COMMAND pathfinder_tests_world_inventory spawn_to_inventory_creates_world_entity)
add_test(NAME world_inventory_spawn_to_inventory_uses_request_entity_id COMMAND pathfinder_tests_world_inventory spawn_to_inventory_uses_request_entity_id)

# World Inventory integration tests (P45)
add_executable(pathfinder_tests_world_inventory_integration
    integration/world_inventory/world_inventory_command_test.cpp
)

target_link_libraries(pathfinder_tests_world_inventory_integration PRIVATE
    pathfinder_world_inventory_command
    pathfinder_world_inventory
    pathfinder_world_command_runtime
    pathfinder_world_runtime
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_command_pickup_runtime COMMAND pathfinder_tests_world_inventory_integration pickup_runtime)
add_test(NAME world_command_drop_runtime COMMAND pathfinder_tests_world_inventory_integration drop_runtime)
add_test(NAME world_command_pickup_too_far COMMAND pathfinder_tests_world_inventory_integration pickup_too_far)
add_test(NAME world_command_pickup_missing_item COMMAND pathfinder_tests_world_inventory_integration pickup_missing_item)
add_test(NAME world_command_drop_not_owned COMMAND pathfinder_tests_world_inventory_integration drop_not_owned)
add_test(NAME world_command_inventory_projection_entries_json COMMAND pathfinder_tests_world_inventory_integration inventory_projection_entries_json)

# Architecture guard test: world_command must not include world_inventory headers
add_test(
    NAME architecture_world_command_no_inventory_dependency
    COMMAND bash -c "! grep -r 'pathfinder/world_inventory' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_command/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_command/"
)

# World Generation unit tests (P46)
add_executable(pathfinder_tests_world_generation
    unit/world_generation/world_generation_main.cpp
    unit/world_generation/world_generation_test.cpp
)

target_link_libraries(pathfinder_tests_world_generation PRIVATE
    pathfinder_world_generation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_foundation
)

add_test(NAME world_generation_smoke COMMAND pathfinder_tests_world_generation smoke)
add_test(NAME world_generation_same_seed_same_result COMMAND pathfinder_tests_world_generation same_seed_same_result)
add_test(NAME world_generation_different_seed_distribution COMMAND pathfinder_tests_world_generation different_seed_distribution)
add_test(NAME world_generation_manifest_seed_profile COMMAND pathfinder_tests_world_generation manifest_seed_profile)
add_test(NAME world_generation_cells_with_layer_key COMMAND pathfinder_tests_world_generation cells_with_layer_key)
add_test(NAME world_generation_valid_terrain COMMAND pathfinder_tests_world_generation valid_terrain)
add_test(NAME world_generation_spawn_not_blocked COMMAND pathfinder_tests_world_generation spawn_not_blocked)
add_test(NAME world_generation_resource_on_allowed_terrain COMMAND pathfinder_tests_world_generation resource_on_allowed_terrain)
add_test(NAME world_generation_resource_action_output COMMAND pathfinder_tests_world_generation resource_action_output)
add_test(NAME world_generation_resource_node_id_stable COMMAND pathfinder_tests_world_generation resource_node_id_stable)
add_test(NAME world_generation_spawn_safety_food COMMAND pathfinder_tests_world_generation spawn_safety_food)
add_test(NAME world_generation_spawn_safety_materials COMMAND pathfinder_tests_world_generation spawn_safety_materials)
add_test(NAME world_generation_spawn_safety_no_threat COMMAND pathfinder_tests_world_generation spawn_safety_no_threat)
add_test(NAME world_generation_ground_items_on_map COMMAND pathfinder_tests_world_generation ground_items_on_map)
add_test(NAME world_generation_ground_items_stack COMMAND pathfinder_tests_world_generation ground_items_stack)
add_test(NAME world_generation_no_inventory_items COMMAND pathfinder_tests_world_generation no_inventory_items)
add_test(NAME world_generation_command_uses_service COMMAND pathfinder_tests_world_generation command_uses_service)
add_test(NAME world_generation_same_seed_manifest COMMAND pathfinder_tests_world_generation same_seed_manifest)
add_test(NAME world_generation_adjacent_regions_no_collision COMMAND pathfinder_tests_world_generation adjacent_regions_no_collision)
add_test(NAME world_generation_cells_created_in_runtime COMMAND pathfinder_tests_world_generation cells_created_in_runtime)
add_test(NAME world_generation_entity_attached_to_cell COMMAND pathfinder_tests_world_generation entity_attached_to_cell)
add_test(NAME world_generation_resource_nodes_in_runtime COMMAND pathfinder_tests_world_generation resource_nodes_in_runtime)
add_test(NAME world_generation_manifest_timestamp_deterministic COMMAND pathfinder_tests_world_generation manifest_timestamp_deterministic)
add_test(NAME world_generation_generate_is_pure COMMAND pathfinder_tests_world_generation generate_is_pure)
add_test(NAME world_generation_repeated_apply_blocked COMMAND pathfinder_tests_world_generation repeated_apply_blocked)
add_test(NAME world_generation_spawn_safety_uses_tags COMMAND pathfinder_tests_world_generation spawn_safety_uses_tags)
add_test(NAME world_generation_command_handler_parsing COMMAND pathfinder_tests_world_generation command_handler_parsing)
add_test(NAME world_generation_spawn_entity_missing_cell_fails COMMAND pathfinder_tests_world_generation spawn_entity_missing_cell_fails)
add_test(NAME world_generation_ground_items_non_empty COMMAND pathfinder_tests_world_generation ground_items_non_empty)
add_test(NAME world_generation_resource_node_missing_cell_fails COMMAND pathfinder_tests_world_generation resource_node_missing_cell_fails)
add_test(NAME world_generation_resource_nodes_applied_to_existing_cells COMMAND pathfinder_tests_world_generation resource_nodes_applied_to_existing_cells)
add_test(NAME world_generation_invalid_layer_rejected COMMAND pathfinder_tests_world_generation invalid_layer_rejected)

# World Generation integration tests (P46)
add_executable(pathfinder_tests_world_generation_integration
    integration/world_generation/world_generation_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_generation_integration PRIVATE
    pathfinder_world_generation
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_generation_integration_smoke COMMAND pathfinder_tests_world_generation_integration smoke)
add_test(NAME world_generation_apply_to_runtime COMMAND pathfinder_tests_world_generation_integration apply_to_runtime)
add_test(NAME world_generation_ground_items_on_map_after_apply COMMAND pathfinder_tests_world_generation_integration ground_items_on_map)
add_test(NAME world_generation_trace_replayable COMMAND pathfinder_tests_world_generation_integration trace_replayable)
add_test(NAME world_generation_command_handler_invalid_seed COMMAND pathfinder_tests_world_generation_integration command_handler_invalid_seed)
add_test(NAME world_generation_command_handler_region_xy COMMAND pathfinder_tests_world_generation_integration command_handler_region_xy)
add_test(NAME world_generation_command_handler_integration COMMAND pathfinder_tests_world_generation_integration command_handler_integration)

# P58 World Region Math unit tests
add_executable(pathfinder_tests_world_region_math
    unit/world_generation/world_region_math_test.cpp
)

target_link_libraries(pathfinder_tests_world_region_math PRIVATE
    pathfinder_world_generation
    pathfinder_foundation
)

add_test(NAME world_region_math_floor_div COMMAND pathfinder_tests_world_region_math floor_div)
add_test(NAME world_region_math_coord_to_region COMMAND pathfinder_tests_world_region_math coord_to_region)
add_test(NAME world_region_math_covering_coords COMMAND pathfinder_tests_world_region_math covering_coords)
add_test(NAME world_region_math_covering_rect COMMAND pathfinder_tests_world_region_math covering_rect)
add_test(NAME world_region_math_cells_in_region COMMAND pathfinder_tests_world_region_math cells_in_region)
add_test(NAME world_region_math_no_overlap COMMAND pathfinder_tests_world_region_math no_overlap)

# P58 World Region Ensure integration tests
add_executable(pathfinder_tests_world_region_ensure_integration
    integration/world_generation/world_region_ensure_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_region_ensure_integration PRIVATE
    pathfinder_world_generation
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_client_runtime_bridge
    pathfinder_foundation
)

add_test(NAME world_region_ensure_idempotency COMMAND pathfinder_tests_world_region_ensure_integration idempotency)
add_test(NAME world_region_ensure_boundary_move COMMAND pathfinder_tests_world_region_ensure_integration boundary_move)
add_test(NAME world_region_ensure_negative_coord COMMAND pathfinder_tests_world_region_ensure_integration negative_coord)
add_test(NAME world_region_ensure_bootstrap COMMAND pathfinder_tests_world_region_ensure_integration bootstrap)
add_test(NAME world_region_ensure_refresh COMMAND pathfinder_tests_world_region_ensure_integration refresh)
add_test(NAME world_region_ensure_max_limit COMMAND pathfinder_tests_world_region_ensure_integration max_limit)
add_test(NAME world_region_ensure_move_guard COMMAND pathfinder_tests_world_region_ensure_integration move_guard)

# World Harvest unit tests (P47)
add_executable(pathfinder_tests_world_harvest
    unit/world_harvest/world_harvest_main.cpp
    unit/world_harvest/world_harvest_test.cpp
)

target_link_libraries(pathfinder_tests_world_harvest PRIVATE
    pathfinder_world_harvest
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_harvest_smoke COMMAND pathfinder_tests_world_harvest smoke)
add_test(NAME world_harvest_enum_to_string COMMAND pathfinder_tests_world_harvest enum_to_string)
add_test(NAME world_harvest_request_validate_basic COMMAND pathfinder_tests_world_harvest request_validate_basic)
add_test(NAME world_harvest_kind_from_command_kind COMMAND pathfinder_tests_world_harvest kind_from_command_kind)
add_test(NAME world_harvest_validate_actor_missing COMMAND pathfinder_tests_world_harvest validate_actor_missing)
add_test(NAME world_harvest_validate_node_missing COMMAND pathfinder_tests_world_harvest validate_node_missing)
add_test(NAME world_harvest_validate_node_depleted COMMAND pathfinder_tests_world_harvest validate_node_depleted)
add_test(NAME world_harvest_validate_action_mismatch COMMAND pathfinder_tests_world_harvest validate_action_mismatch)
add_test(NAME world_harvest_validate_too_far COMMAND pathfinder_tests_world_harvest validate_too_far)
add_test(NAME world_harvest_validate_not_visible COMMAND pathfinder_tests_world_harvest validate_not_visible)
add_test(NAME world_harvest_validate_tool_missing COMMAND pathfinder_tests_world_harvest validate_tool_missing)
add_test(NAME world_harvest_validate_tool_present COMMAND pathfinder_tests_world_harvest validate_tool_present)
add_test(NAME world_harvest_tool_durability_decrements COMMAND pathfinder_tests_world_harvest tool_durability_decrements)
add_test(NAME world_harvest_worn_out_tool_rejected COMMAND pathfinder_tests_world_harvest worn_out_tool_rejected)
add_test(NAME world_harvest_apply_gather_creates_output_on_map COMMAND pathfinder_tests_world_harvest apply_gather_creates_output_on_map)
add_test(NAME world_harvest_apply_chop_creates_wood_on_map COMMAND pathfinder_tests_world_harvest apply_chop_creates_wood_on_map)
add_test(NAME world_harvest_apply_consumes_one_charge COMMAND pathfinder_tests_world_harvest apply_consumes_one_charge)
add_test(NAME world_harvest_apply_depletes_node_at_zero COMMAND pathfinder_tests_world_harvest apply_depletes_node_at_zero)
add_test(NAME world_harvest_apply_output_id_deterministic COMMAND pathfinder_tests_world_harvest apply_output_id_deterministic)
add_test(NAME world_harvest_apply_failure_does_not_change_node COMMAND pathfinder_tests_world_harvest apply_failure_does_not_change_node)
add_test(NAME world_harvest_apply_failure_does_not_spawn_output COMMAND pathfinder_tests_world_harvest apply_failure_does_not_spawn_output)
add_test(NAME world_harvest_output_uses_p45_on_map_location COMMAND pathfinder_tests_world_harvest output_uses_p45_on_map_location)
add_test(NAME world_harvest_output_not_in_actor_inventory COMMAND pathfinder_tests_world_harvest output_not_in_actor_inventory)
add_test(NAME world_harvest_output_cell_contains_entity_id COMMAND pathfinder_tests_world_harvest output_cell_contains_entity_id)
add_test(NAME world_harvest_output_can_be_picked_up_by_p45_pickup COMMAND pathfinder_tests_world_harvest output_can_be_picked_up_by_p45_pickup)
add_test(NAME world_harvest_rejects_prefer_inventory_policy COMMAND pathfinder_tests_world_harvest rejects_prefer_inventory_policy)
add_test(NAME world_harvest_rejects_empty_outputs COMMAND pathfinder_tests_world_harvest rejects_empty_outputs)
add_test(NAME world_harvest_rejects_empty_output_key COMMAND pathfinder_tests_world_harvest rejects_empty_output_key)
add_test(NAME world_harvest_rejects_unknown_policy_string COMMAND pathfinder_tests_world_harvest rejects_unknown_policy_string)
add_test(NAME world_harvest_command_rejects_bad_policy_parameter COMMAND pathfinder_tests_world_harvest command_rejects_bad_policy_parameter)

# World Harvest integration tests (P47)
add_executable(pathfinder_tests_world_harvest_integration
    integration/world_harvest/world_harvest_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_harvest_integration PRIVATE
    pathfinder_world_harvest
    pathfinder_world_generation
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_harvest_integration_smoke COMMAND pathfinder_tests_world_harvest_integration smoke)
add_test(NAME world_command_gather_resource_node_runtime COMMAND pathfinder_tests_world_harvest_integration gather_resource_node_runtime)
add_test(NAME world_command_chop_resource_node_runtime COMMAND pathfinder_tests_world_harvest_integration chop_resource_node_runtime)
add_test(NAME world_command_mine_action_mismatch_fails COMMAND pathfinder_tests_world_harvest_integration mine_action_mismatch_fails)
add_test(NAME world_command_harvest_missing_tool_fails COMMAND pathfinder_tests_world_harvest_integration harvest_missing_tool_fails)
add_test(NAME world_command_harvest_depleted_node_fails COMMAND pathfinder_tests_world_harvest_integration harvest_depleted_node_fails)
add_test(NAME world_command_harvest_projection_patch_runtime COMMAND pathfinder_tests_world_harvest_integration harvest_projection_patch_runtime)
add_test(NAME world_command_harvest_events_trace_complete COMMAND pathfinder_tests_world_harvest_integration harvest_events_trace_complete)
add_test(NAME world_generation_then_gather_berry_bush COMMAND pathfinder_tests_world_harvest_integration generation_then_gather_berry_bush)
add_test(NAME world_generation_then_chop_tree_with_tool COMMAND pathfinder_tests_world_harvest_integration generation_then_chop_tree_with_tool)
add_test(NAME world_generation_then_gather_loose_stone COMMAND pathfinder_tests_world_harvest_integration generation_then_gather_loose_stone)
add_test(NAME world_command_harvest_events_use_context_tick COMMAND pathfinder_tests_world_harvest_integration harvest_events_use_context_tick)

# World Reaction unit tests (P48)
add_executable(pathfinder_tests_world_reaction
    unit/world_reaction/world_reaction_main.cpp
    unit/world_reaction/world_reaction_test.cpp
)

target_link_libraries(pathfinder_tests_world_reaction PRIVATE
    pathfinder_world_reaction
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_content
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME world_reaction_apply_craft_torch_to_inventory COMMAND pathfinder_tests_world_reaction apply_craft_torch_to_inventory)
add_test(NAME world_reaction_apply_craft_axe_to_inventory COMMAND pathfinder_tests_world_reaction apply_craft_axe_to_inventory)
add_test(NAME world_reaction_validate_input_missing COMMAND pathfinder_tests_world_reaction validate_input_missing)
add_test(NAME world_reaction_validate_state_mismatch COMMAND pathfinder_tests_world_reaction validate_state_mismatch)
add_test(NAME world_reaction_validate_quantity_insufficient COMMAND pathfinder_tests_world_reaction validate_quantity_insufficient)
add_test(NAME world_reaction_validate_catalyst_not_consumed COMMAND pathfinder_tests_world_reaction validate_catalyst_not_consumed)
add_test(NAME world_reaction_failure_does_not_pollute_state COMMAND pathfinder_tests_world_reaction failure_does_not_pollute_state)
add_test(NAME world_reaction_craft_output_entity_exists_in_world_runtime COMMAND pathfinder_tests_world_reaction craft_output_entity_exists_in_world_runtime)
add_test(NAME world_reaction_output_id_is_command_deterministic COMMAND pathfinder_tests_world_reaction output_id_is_command_deterministic)
add_test(NAME world_reaction_consumes_same_cell_map_input COMMAND pathfinder_tests_world_reaction consumes_same_cell_map_input)
add_test(NAME world_reaction_consumes_adjacent_map_input COMMAND pathfinder_tests_world_reaction consumes_adjacent_map_input)
add_test(NAME world_reaction_drop_on_map_output_spawns_entity_on_actor_cell COMMAND pathfinder_tests_world_reaction drop_on_map_output_spawns_entity_on_actor_cell)

# World Reaction integration tests (P48)
add_executable(pathfinder_tests_world_reaction_integration
    integration/world_reaction/world_reaction_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_reaction_integration PRIVATE
    pathfinder_world_reaction
    pathfinder_world_harvest
    pathfinder_world_generation
    pathfinder_world_inventory_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_world_command
    pathfinder_content
    pathfinder_foundation
)

add_test(NAME world_command_craft_reaction_key_from_target_recipe COMMAND pathfinder_tests_world_reaction_integration craft_reaction_key_from_target_recipe)
add_test(NAME world_command_craft_events_use_context_tick COMMAND pathfinder_tests_world_reaction_integration craft_events_use_context_tick)
add_test(NAME world_generation_harvest_pickup_then_craft_torch COMMAND pathfinder_tests_world_reaction_integration harvest_pickup_then_craft_torch)
add_test(NAME world_command_craft_drop_on_map_projection_patch COMMAND pathfinder_tests_world_reaction_integration craft_drop_on_map_projection_patch)
add_test(NAME world_command_craft_projection_version_advances COMMAND pathfinder_tests_world_reaction_integration craft_projection_version_advances)

# World Teaching unit tests (P50)
add_executable(pathfinder_tests_world_teaching
    unit/world_teaching/world_teaching_test.cpp
)

target_link_libraries(pathfinder_tests_world_teaching PRIVATE
    pathfinder_world_teaching
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_knowledge
    pathfinder_learning
    pathfinder_foundation
)

add_test(NAME world_teaching_enum_roundtrip COMMAND pathfinder_tests_world_teaching enum_roundtrip)
add_test(NAME world_teaching_owner_resolver_maps_actor COMMAND pathfinder_tests_world_teaching owner_resolver_maps_actor)
add_test(NAME world_teaching_eligibility_rejects_missing_source_claim COMMAND pathfinder_tests_world_teaching eligibility_rejects_missing_source_claim)
add_test(NAME world_teaching_eligibility_rejects_owner_mismatch COMMAND pathfinder_tests_world_teaching eligibility_rejects_owner_mismatch)
add_test(NAME world_teaching_eligibility_rejects_not_teachable COMMAND pathfinder_tests_world_teaching eligibility_rejects_not_teachable)
add_test(NAME world_teaching_eligibility_rejects_out_of_range COMMAND pathfinder_tests_world_teaching eligibility_rejects_out_of_range)
add_test(NAME world_teaching_loop_creates_recipient_claim COMMAND pathfinder_tests_world_teaching loop_creates_recipient_claim)
add_test(NAME world_teaching_existing_claim_reinforces_or_updates COMMAND pathfinder_tests_world_teaching existing_claim_reinforces_or_updates)
add_test(NAME world_teaching_action_gate_blocks_unknown_npc COMMAND pathfinder_tests_world_teaching action_gate_blocks_unknown_npc)
add_test(NAME world_teaching_action_gate_allows_known_action COMMAND pathfinder_tests_world_teaching action_gate_allows_known_action)
add_test(NAME world_teaching_action_gate_blocks_ai_disabled_claim COMMAND pathfinder_tests_world_teaching action_gate_blocks_ai_disabled_claim)
add_test(NAME world_teaching_action_gate_blocks_risk_without_goal COMMAND pathfinder_tests_world_teaching action_gate_blocks_risk_without_goal)

# World Teaching integration tests (P50)
add_executable(pathfinder_tests_world_teaching_integration
    integration/world_teaching/world_teaching_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_teaching_integration PRIVATE
    pathfinder_world_teaching
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_knowledge
    pathfinder_learning
    pathfinder_foundation
)

add_test(NAME world_teaching_command_player_to_npc COMMAND pathfinder_tests_world_teaching_integration teach_command_player_to_npc)
add_test(NAME world_teaching_command_npc_to_player COMMAND pathfinder_tests_world_teaching_integration teach_command_npc_to_player)
add_test(NAME world_teaching_command_out_of_range_no_patch COMMAND pathfinder_tests_world_teaching_integration teach_command_out_of_range_no_patch)
add_test(NAME world_teaching_command_missing_actor_query_port_blocks COMMAND pathfinder_tests_world_teaching_integration teach_command_missing_actor_query_port_blocks)
add_test(NAME world_teaching_handler_registers_in_registry COMMAND pathfinder_tests_world_teaching_integration teach_handler_registers_in_registry)
add_test(NAME world_teaching_teach_then_action_gate COMMAND pathfinder_tests_world_teaching_integration teach_then_action_gate)
add_test(NAME world_teaching_no_h5_dependency_scan COMMAND pathfinder_tests_world_teaching_integration no_h5_dependency_scan)
add_test(NAME world_teaching_no_content_hardcode_scan COMMAND pathfinder_tests_world_teaching_integration no_content_hardcode_scan)
# Architecture guard tests for P50
add_test(
    NAME architecture_world_teaching_no_h5_dependency
    COMMAND bash -c "! grep -riE 'h5|dialog|playable' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_teaching/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_teaching/"
)
add_test(
    NAME architecture_world_teaching_no_content_hardcode
    COMMAND bash -c "! grep -riE 'red_berry|torch|wood|beast|wolf|mushroom' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_teaching/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_teaching/"
)

# World Agent Execution unit tests (P51)
add_executable(pathfinder_tests_world_agent_execution
    unit/world_agent_execution/world_agent_execution_main.cpp
    unit/world_agent_execution/world_agent_execution_types_test.cpp
    unit/world_agent_execution/world_agent_execution_store_test.cpp
    unit/world_agent_execution/world_agent_execution_context_builder_test.cpp
    unit/world_agent_execution/world_agent_execution_knowledge_guard_test.cpp
    unit/world_agent_execution/world_agent_execution_command_compiler_test.cpp
    unit/world_agent_execution/world_agent_execution_coordinator_test.cpp
    unit/world_agent_execution/world_agent_execution_projection_test.cpp
)

target_link_libraries(pathfinder_tests_world_agent_execution PRIVATE
    pathfinder_world_agent_execution
    pathfinder_world_teaching
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_knowledge
    pathfinder_agent_reasoning
    pathfinder_goal_execution
    pathfinder_foundation
)

add_test(NAME world_agent_execution_enum_roundtrip COMMAND pathfinder_tests_world_agent_execution enum_roundtrip)
add_test(NAME world_agent_execution_context_store COMMAND pathfinder_tests_world_agent_execution context_store)
add_test(NAME world_agent_execution_context_builder COMMAND pathfinder_tests_world_agent_execution context_builder)
add_test(NAME world_agent_execution_knowledge_guard COMMAND pathfinder_tests_world_agent_execution knowledge_guard)
add_test(NAME world_agent_execution_command_compiler COMMAND pathfinder_tests_world_agent_execution command_compiler)
add_test(NAME world_agent_execution_coordinator COMMAND pathfinder_tests_world_agent_execution coordinator)
add_test(NAME world_agent_execution_projection COMMAND pathfinder_tests_world_agent_execution projection)

# World Agent Execution integration tests (P51)
add_executable(pathfinder_tests_world_agent_execution_integration
    integration/world_agent_execution/world_agent_execution_integration_main.cpp
    integration/world_agent_execution/world_agent_execution_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_agent_execution_integration PRIVATE
    pathfinder_world_agent_execution
    pathfinder_world_teaching
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_knowledge
    pathfinder_agent_reasoning
    pathfinder_goal_execution
    pathfinder_foundation
)

add_test(NAME world_agent_execution_integration COMMAND pathfinder_tests_world_agent_execution_integration integration)

# Architecture guard tests for P51
add_test(
    NAME architecture_world_agent_execution_no_h5_dependency
    COMMAND bash -c "! grep -riE 'h5|dialog|playable' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_agent_execution/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_agent_execution/"
)
add_test(
    NAME architecture_world_agent_execution_no_content_hardcode
    COMMAND bash -c "! grep -riE 'red_berry|torch|wood|beast|wolf|mushroom|companion' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_agent_execution/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_agent_execution/"
)
add_test(
    NAME architecture_world_agent_execution_no_direct_runtime_mutation
    COMMAND bash -c "! grep -riE '\.actors\[|\.entities\[|\.resource_nodes\[|\.inventories\[|\.put\(|createOrUpdateGeneratedCell|upsertGeneratedResourceNode' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_agent_execution/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_agent_execution/"
)
add_test(
    NAME architecture_world_agent_execution_no_frontend_claim_injection
    COMMAND bash -c "! grep -riE 'recipient_claim_json|learned=true|force_learn|force_teach|knowledge_json' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_agent_execution/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_agent_execution/"
)

# World Beast Ecology unit tests (P52)
add_executable(pathfinder_tests_world_beast_ecology
    unit/world_beast_ecology/world_beast_ecology_main.cpp
    unit/world_beast_ecology/world_beast_ecology_types_test.cpp
    unit/world_beast_ecology/beast_perception_test.cpp
    unit/world_beast_ecology/beast_instinct_gate_test.cpp
    unit/world_beast_ecology/beast_decision_policy_test.cpp
    unit/world_beast_ecology/beast_command_compiler_test.cpp
    unit/world_beast_ecology/beast_ecology_coordinator_test.cpp
)

target_link_libraries(pathfinder_tests_world_beast_ecology PRIVATE
    pathfinder_world_beast_ecology
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_knowledge
    pathfinder_goal_execution
)

add_test(NAME world_beast_ecology_enum_roundtrip COMMAND pathfinder_tests_world_beast_ecology enum_roundtrip)
add_test(NAME world_beast_ecology_profile_validate COMMAND pathfinder_tests_world_beast_ecology profile_validate)
add_test(NAME world_beast_ecology_perception COMMAND pathfinder_tests_world_beast_ecology perception)
add_test(NAME world_beast_ecology_instinct_gate COMMAND pathfinder_tests_world_beast_ecology instinct_gate)
add_test(NAME world_beast_ecology_policy COMMAND pathfinder_tests_world_beast_ecology policy)
add_test(NAME world_beast_ecology_compiler COMMAND pathfinder_tests_world_beast_ecology compiler)
add_test(NAME world_beast_ecology_coordinator COMMAND pathfinder_tests_world_beast_ecology coordinator)

# World Beast Ecology integration tests (P52)
add_executable(pathfinder_tests_world_beast_ecology_integration
    integration/world_beast_ecology/world_beast_ecology_integration_main.cpp
    integration/world_beast_ecology/world_beast_ecology_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_beast_ecology_integration PRIVATE
    pathfinder_world_beast_ecology
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_knowledge
    pathfinder_goal_execution
)

add_test(NAME world_beast_ecology_integration COMMAND pathfinder_tests_world_beast_ecology_integration integration)

# Architecture guard tests for P52
add_test(
    NAME architecture_world_beast_ecology_no_h5_dependency
    COMMAND bash -c "! grep -riE 'h5|dialog|playable' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_beast_ecology/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_beast_ecology/"
)
add_test(
    NAME architecture_world_beast_ecology_no_content_hardcode
    COMMAND bash -c "! grep -riE 'wolf|torch|fire|red_berry|berry|wood|beast_shadow|companion' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_beast_ecology/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_beast_ecology/"
)
add_test(
    NAME architecture_world_beast_ecology_no_direct_runtime_mutation
    COMMAND bash -c "! grep -riE '\.actors\[|\.entities\[|\.resource_nodes\[|\.inventories\[|\.put\(|createOrUpdateGeneratedCell|upsertGeneratedResourceNode' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_beast_ecology/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_beast_ecology/"
)
add_test(
    NAME architecture_world_beast_ecology_no_frontend_claim_injection
    COMMAND bash -c "! grep -riE 'recipient_claim_json|learned=true|force_learn|force_teach|knowledge_json' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_beast_ecology/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_beast_ecology/"
)

# World Learning unit tests (P49)
add_executable(pathfinder_tests_world_learning
    unit/world_learning/world_learning_main.cpp
    unit/world_learning/world_learning_test.cpp
)

target_link_libraries(pathfinder_tests_world_learning PRIVATE
    pathfinder_world_learning
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_content
    pathfinder_knowledge
    pathfinder_learning
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(NAME world_learning_enum_roundtrip COMMAND pathfinder_tests_world_learning enum_roundtrip)
add_test(NAME world_learning_extracts_experience_from_command_result COMMAND pathfinder_tests_world_learning extracts_experience_from_command_result)
add_test(NAME world_learning_resolves_template_from_reason_keys COMMAND pathfinder_tests_world_learning resolves_template_from_reason_keys)
add_test(NAME world_learning_resolves_template_by_action_effect_fallback COMMAND pathfinder_tests_world_learning resolves_template_by_action_effect_fallback)
add_test(NAME world_learning_skips_missing_template_without_world_mutation COMMAND pathfinder_tests_world_learning skips_missing_template_without_world_mutation)
add_test(NAME world_learning_rejects_unsafe_experience COMMAND pathfinder_tests_world_learning rejects_unsafe_experience)
add_test(NAME world_learning_builds_safe_feedback COMMAND pathfinder_tests_world_learning builds_safe_feedback)
add_test(NAME world_learning_direct_experience_forms_claim COMMAND pathfinder_tests_world_learning direct_experience_forms_claim)
add_test(NAME world_learning_repeated_experience_reinforces_claim COMMAND pathfinder_tests_world_learning repeated_experience_reinforces_claim)
add_test(NAME world_learning_high_risk_effect_is_normalized COMMAND pathfinder_tests_world_learning high_risk_effect_is_normalized)
add_test(NAME world_learning_no_frontend_knowledge_input COMMAND pathfinder_tests_world_learning no_frontend_knowledge_input)

# World Learning integration tests (P49)
add_executable(pathfinder_tests_world_learning_integration
    integration/world_learning/world_learning_integration_test.cpp
)

target_link_libraries(pathfinder_tests_world_learning_integration PRIVATE
    pathfinder_world_learning
    pathfinder_world_reaction
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_content
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(NAME world_learning_ext_item_reaction_command_to_knowledge COMMAND pathfinder_tests_world_learning_integration ext_item_reaction_command_to_knowledge)
add_test(NAME world_learning_craft_experience_uses_p48_knowledge_template COMMAND pathfinder_tests_world_learning_integration craft_experience_uses_p48_knowledge_template)
add_test(NAME world_learning_harvest_experience_uses_p47_output COMMAND pathfinder_tests_world_learning_integration harvest_experience_uses_p47_output)
add_test(NAME world_learning_command_result_merges_learning_events COMMAND pathfinder_tests_world_learning_integration command_result_merges_learning_events)
add_test(NAME world_learning_missing_template_does_not_fail_craft COMMAND pathfinder_tests_world_learning_integration missing_template_does_not_fail_craft)

# Client Protocol unit tests (P53)
add_executable(pathfinder_tests_client_protocol
    unit/client_protocol/client_protocol_test.cpp
)

target_link_libraries(pathfinder_tests_client_protocol PRIVATE
    pathfinder_client_protocol
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME client_protocol_enum COMMAND pathfinder_tests_client_protocol enum)
add_test(NAME client_protocol_dto_validation COMMAND pathfinder_tests_client_protocol dto_validation)
add_test(NAME client_protocol_codec_roundtrip COMMAND pathfinder_tests_client_protocol codec_roundtrip)
add_test(NAME client_protocol_projection_version COMMAND pathfinder_tests_client_protocol projection_version)
add_test(NAME client_protocol_available_command COMMAND pathfinder_tests_client_protocol available_command)
add_test(NAME client_protocol_security_request COMMAND pathfinder_tests_client_protocol security_request)
add_test(NAME client_protocol_projection_adapter COMMAND pathfinder_tests_client_protocol projection_adapter)
add_test(NAME client_protocol_codec_rejects_unknown_enum COMMAND pathfinder_tests_client_protocol codec_rejects_unknown_enum)
add_test(NAME client_protocol_snapshot_materialize COMMAND pathfinder_tests_client_protocol snapshot_materialize)
add_test(NAME client_protocol_projection_codec_full_roundtrip COMMAND pathfinder_tests_client_protocol projection_codec_full_roundtrip)
add_test(NAME client_protocol_patch_codec_full_roundtrip COMMAND pathfinder_tests_client_protocol patch_codec_full_roundtrip)

# Client Protocol integration tests (P53)
add_executable(pathfinder_tests_client_protocol_integration
    integration/client_protocol/client_protocol_integration_test.cpp
)

target_link_libraries(pathfinder_tests_client_protocol_integration PRIVATE
    pathfinder_client_protocol
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME client_protocol_bootstrap_command_patch_refresh COMMAND pathfinder_tests_client_protocol_integration bootstrap_command_patch_refresh)
add_test(NAME client_protocol_failed_command_safe_response COMMAND pathfinder_tests_client_protocol_integration failed_command_safe_response)
add_test(NAME client_protocol_stale_projection_requires_refresh COMMAND pathfinder_tests_client_protocol_integration stale_projection_requires_refresh)
add_test(NAME client_protocol_expired_option_blocked COMMAND pathfinder_tests_client_protocol_integration expired_option_blocked)
add_test(NAME client_protocol_codec_roundtrip_through_harness COMMAND pathfinder_tests_client_protocol_integration codec_roundtrip_through_harness)
add_test(NAME client_protocol_reset_creates_new_session COMMAND pathfinder_tests_client_protocol_integration reset_creates_new_session)
add_test(NAME client_protocol_available_commands_only_from_backend COMMAND pathfinder_tests_client_protocol_integration available_commands_only_from_backend)
add_test(NAME client_protocol_command_response_completeness COMMAND pathfinder_tests_client_protocol_integration command_response_completeness)
add_test(NAME client_protocol_forged_actor_ignored COMMAND pathfinder_tests_client_protocol_integration forged_actor_ignored)
add_test(NAME client_protocol_world_command_dto_wrong_actor_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_wrong_actor_blocked)
add_test(NAME client_protocol_stale_projection_no_execution COMMAND pathfinder_tests_client_protocol_integration stale_projection_no_execution)
add_test(NAME client_protocol_forged_option_id_blocked COMMAND pathfinder_tests_client_protocol_integration forged_option_id_blocked)
add_test(NAME client_protocol_old_option_after_version_advance_blocked COMMAND pathfinder_tests_client_protocol_integration old_option_after_version_advance_blocked)
add_test(NAME client_protocol_world_command_dto_illegal_source_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_illegal_source_blocked)
add_test(NAME client_protocol_command_updates_snapshot_authority COMMAND pathfinder_tests_client_protocol_integration command_updates_snapshot_authority)
add_test(NAME client_protocol_client_id_mismatch_blocked COMMAND pathfinder_tests_client_protocol_integration client_id_mismatch_blocked)
add_test(NAME client_protocol_blocked_response_codec_roundtrip COMMAND pathfinder_tests_client_protocol_integration blocked_response_codec_roundtrip)
add_test(NAME client_protocol_multi_session_version_isolation COMMAND pathfinder_tests_client_protocol_integration multi_session_version_isolation)
add_test(NAME client_protocol_unsafe_parameters_blocked COMMAND pathfinder_tests_client_protocol_integration unsafe_parameters_blocked)
add_test(NAME client_protocol_world_command_dto_generate_world_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_generate_world_blocked)
add_test(NAME client_protocol_world_command_dto_key_mismatch_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_key_mismatch_blocked)
add_test(NAME client_protocol_world_command_dto_target_mismatch_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_target_mismatch_blocked)
add_test(NAME client_protocol_world_command_dto_target_entity_forged_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_target_entity_forged_blocked)
add_test(NAME client_protocol_world_command_dto_target_coord_mismatch_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_target_coord_mismatch_blocked)
add_test(NAME client_protocol_world_command_dto_parameters_injected_blocked COMMAND pathfinder_tests_client_protocol_integration world_command_dto_parameters_injected_blocked)

# Architecture guard tests: P53 client_protocol must not contain H5V2 naming
add_test(
    NAME architecture_client_protocol_no_h5v2_naming
    COMMAND bash -c "! grep -rni 'H5V2\\|h5_v2\\|h5-v2' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/unit/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/integration/client_protocol/"
)

# Architecture guard: client_protocol must not hard-code content keys
add_test(
    NAME architecture_client_protocol_no_content_hardcode
    COMMAND bash -c "! grep -rni 'red_berry\\|berry\\|torch\\|fire\\|wolf\\|wood\\|companion\\|beast_shadow\\|mushroom' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/"
)

# Architecture guard: client_protocol must not read content JSON
add_test(
    NAME architecture_client_protocol_no_content_json
    COMMAND bash -c "! grep -rni 'content/core\\|recipes\\|reaction_rules\\|knowledge_templates\\|ContentRegistry' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/"
)

# Architecture guard: client_protocol must not inject knowledge
add_test(
    NAME architecture_client_protocol_no_knowledge_injection
    COMMAND bash -c "! grep -rni 'KnowledgeClaim\\|recipient_claim_json\\|knowledge_json\\|force_learn\\|force_teach\\|learned=true' --exclude=client_command_gateway.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/"
)

# Architecture guard: client_protocol must not directly modify runtime arrays
add_test(
    NAME architecture_client_protocol_no_runtime_mutation
    COMMAND bash -c "! grep -rni '\\.actors\\[\\|\\.entities\\[\\|\\.inventories\\[\\|\\.resource_nodes\\[\\|createOrUpdateGeneratedCell\\|upsertGeneratedResourceNode' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_protocol/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/"
)


# Client HTTP unit tests (P54)
add_executable(pathfinder_tests_client_http
    unit/client_http/client_http_test.cpp
)

target_link_libraries(pathfinder_tests_client_http PRIVATE
    pathfinder_client_http
    pathfinder_client_protocol
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME client_http_parse_get_request COMMAND pathfinder_tests_client_http parse_get_request)
add_test(NAME client_http_parse_post_json_request COMMAND pathfinder_tests_client_http parse_post_json_request)
add_test(NAME client_http_reject_path_traversal COMMAND pathfinder_tests_client_http reject_path_traversal)
add_test(NAME client_http_mime_type COMMAND pathfinder_tests_client_http mime_type)
add_test(NAME client_http_error_response_json_shape COMMAND pathfinder_tests_client_http error_response_json_shape)
add_test(NAME client_static_file_serves_index COMMAND pathfinder_tests_client_http static_file_serves_index)
add_test(NAME client_static_file_rejects_content_core COMMAND pathfinder_tests_client_http static_file_rejects_content_core)

# Client HTTP integration tests (P54)
add_executable(pathfinder_tests_client_http_integration
    integration/client_http/client_http_integration_test.cpp
)

target_link_libraries(pathfinder_tests_client_http_integration PRIVATE
    pathfinder_client_http
    pathfinder_client_protocol
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME client_http_bootstrap_roundtrip COMMAND pathfinder_tests_client_http_integration bootstrap_roundtrip)
add_test(NAME client_http_command_option_roundtrip COMMAND pathfinder_tests_client_http_integration command_option_roundtrip)
add_test(NAME client_http_refresh_roundtrip COMMAND pathfinder_tests_client_http_integration refresh_roundtrip)
add_test(NAME client_http_reset_roundtrip COMMAND pathfinder_tests_client_http_integration reset_roundtrip)
add_test(NAME client_http_stale_command_returns_full_refresh COMMAND pathfinder_tests_client_http_integration stale_command_returns_full_refresh)
add_test(NAME client_http_bad_json_returns_400_json COMMAND pathfinder_tests_client_http_integration bad_json_returns_400_json)
add_test(NAME client_http_api_get_returns_405_json COMMAND pathfinder_tests_client_http_integration api_get_returns_405_json)
add_test(NAME client_http_static_index_app_style COMMAND pathfinder_tests_client_http_integration static_index_app_style)
add_test(NAME client_http_no_content_json_exposure COMMAND pathfinder_tests_client_http_integration no_content_json_exposure)
add_test(NAME client_http_socket_bootstrap_smoke COMMAND pathfinder_tests_client_http_integration socket_bootstrap_smoke)

# Architecture guard: client_http must not contain H5 namespace
add_test(
    NAME architecture_client_http_no_h5_namespace
    COMMAND bash -c "! grep -rni 'h5_playable::\\|h5_dialog::\\|H5Playable\\|H5Dialog' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/unit/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/integration/client_http/"
)

# Architecture guard: HTTP transport/frontend files must not read content rules.
# ClientRuntimeHostFactory is the backend composition root and may wire a ContentRegistry
# into world generation/harvest services; it still must not expose raw content JSON.
add_test(
    NAME architecture_client_http_no_content_json_read
    COMMAND bash -c "! grep -rni 'content/core\|recipes\|reaction_rules\|knowledge_templates' --exclude=client_static_file_service.cpp --exclude=client_runtime_host_factory.cpp --exclude=client_runtime_host_factory.h ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/"
)

# Architecture guard: client_http must not directly modify runtime arrays
add_test(
    NAME architecture_client_http_no_runtime_array_mutation
    COMMAND bash -c "! grep -rni '\\.actors\\[\\|\\.entities\\[\\|\\.inventories\\[\\|resource_nodes\\[\\|createOrUpdateGeneratedCell\\|upsertGeneratedResourceNode' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/"
)

# Architecture guard: client_http must not inject knowledge
add_test(
    NAME architecture_client_http_no_knowledge_injection
    COMMAND bash -c "! grep -rni 'KnowledgeClaim\\|recipient_claim_json\\|knowledge_json\\|force_learn\\|force_teach\\|learned=true' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/"
)

# Architecture guard: client_http must not introduce third-party HTTP framework
add_test(
    NAME architecture_client_http_no_third_party_http_framework
    COMMAND bash -c "! grep -rni 'cpp-httplib\\|boost/beast\\|crow_all\\|crow::' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/"
)

# Architecture guard: client_http must not generate frontend rules
add_test(
    NAME architecture_client_http_no_frontend_rule_generation
    COMMAND bash -c "! grep -rni 'generateWorld\\|createWorld\\|spawnEntity\\|spawnActor' --exclude=client_runtime_host_factory.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/ ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/"
)

# Architecture guard: client_http transport boundary
add_test(
    NAME architecture_client_http_transport_boundary
    COMMAND bash -c "! grep -rni 'sys/socket\\|netinet/in\\|arpa/inet' ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/client_http_gateway.h ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/client_http_gateway.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/client_http/client_http_router.h ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_http/client_http_router_impl.cpp"
)

# ---------------------------------------------------------------------------
# P56 Client Runtime Bridge tests
# ---------------------------------------------------------------------------

add_executable(pathfinder_tests_client_runtime_bridge
    unit/client_runtime_bridge/client_runtime_bridge_test.cpp
)

target_link_libraries(pathfinder_tests_client_runtime_bridge PRIVATE
    pathfinder_client_runtime_bridge
    pathfinder_world_runtime
    pathfinder_world_command_runtime
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME client_runtime_bridge_enum COMMAND pathfinder_tests_client_runtime_bridge enum)
add_test(NAME client_runtime_bridge_dto_validation COMMAND pathfinder_tests_client_runtime_bridge dto_validation)
add_test(NAME client_runtime_bridge_projection_non_empty COMMAND pathfinder_tests_client_runtime_bridge projection_non_empty)
add_test(NAME client_runtime_bridge_projection_security COMMAND pathfinder_tests_client_runtime_bridge projection_security)
add_test(NAME client_runtime_bridge_projection_discovered COMMAND pathfinder_tests_client_runtime_bridge projection_discovered)
add_test(NAME client_runtime_bridge_movement_provider COMMAND pathfinder_tests_client_runtime_bridge movement_provider)
add_test(NAME client_runtime_bridge_movement_disabled COMMAND pathfinder_tests_client_runtime_bridge movement_disabled)
add_test(NAME client_runtime_bridge_wait_inspect_provider COMMAND pathfinder_tests_client_runtime_bridge wait_inspect_provider)
add_test(NAME client_runtime_bridge_no_handler COMMAND pathfinder_tests_client_runtime_bridge no_handler)
add_test(NAME client_runtime_bridge_diagnostics COMMAND pathfinder_tests_client_runtime_bridge diagnostics)

# P56 integration tests
add_executable(pathfinder_tests_client_runtime_bridge_integration
    integration/client_runtime_bridge/client_runtime_bridge_integration_test.cpp
)

target_link_libraries(pathfinder_tests_client_runtime_bridge_integration PRIVATE
    pathfinder_client_runtime_bridge
    pathfinder_client_runtime_host
    pathfinder_client_http
    pathfinder_client_protocol
    pathfinder_world_runtime
    pathfinder_world_command_runtime
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME client_runtime_bridge_bootstrap_non_empty COMMAND pathfinder_tests_client_runtime_bridge_integration bootstrap_non_empty)
add_test(NAME client_runtime_bridge_bootstrap_player_visible COMMAND pathfinder_tests_client_runtime_bridge_integration bootstrap_player_visible)
add_test(NAME client_runtime_bridge_bootstrap_move_options COMMAND pathfinder_tests_client_runtime_bridge_integration bootstrap_move_options)
add_test(NAME client_runtime_bridge_move_changes_position COMMAND pathfinder_tests_client_runtime_bridge_integration move_changes_position)
add_test(NAME client_runtime_bridge_move_updates_version COMMAND pathfinder_tests_client_runtime_bridge_integration move_updates_version)
add_test(NAME client_runtime_bridge_blocked_terrain_blocked COMMAND pathfinder_tests_client_runtime_bridge_integration blocked_terrain_blocked)
add_test(NAME client_runtime_bridge_forged_option_blocked COMMAND pathfinder_tests_client_runtime_bridge_integration forged_option_blocked)
add_test(NAME client_runtime_bridge_stale_version_blocked COMMAND pathfinder_tests_client_runtime_bridge_integration stale_version_blocked)
add_test(NAME client_runtime_bridge_refresh_after_move COMMAND pathfinder_tests_client_runtime_bridge_integration refresh_after_move)
add_test(NAME client_runtime_bridge_bootstrap_version_matches COMMAND pathfinder_tests_client_runtime_bridge_integration bootstrap_version_matches)
add_test(NAME client_runtime_bridge_refresh_version_matches COMMAND pathfinder_tests_client_runtime_bridge_integration refresh_version_matches)
add_test(NAME client_runtime_bridge_command_with_full_projection_version COMMAND pathfinder_tests_client_runtime_bridge_integration command_with_full_projection_version)
add_test(NAME client_runtime_bridge_cross_region_move COMMAND pathfinder_tests_client_runtime_bridge_integration cross_region_move)
add_test(NAME client_runtime_bridge_negative_cross_region_move COMMAND pathfinder_tests_client_runtime_bridge_integration negative_cross_region_move)
add_test(NAME client_runtime_bridge_npc_order_crafts_axe_after_teaching COMMAND pathfinder_tests_client_runtime_bridge_integration npc_order_crafts_axe_after_teaching)
add_test(NAME client_runtime_bridge_npc_order_torch_counters_threat_after_teaching COMMAND pathfinder_tests_client_runtime_bridge_integration npc_order_torch_counters_threat_after_teaching)
add_test(NAME client_runtime_bridge_pickup_options_exclude_predator COMMAND pathfinder_tests_client_runtime_bridge_integration pickup_options_exclude_predator)
add_test(NAME client_runtime_bridge_npc_follow_command_moves_companion COMMAND pathfinder_tests_client_runtime_bridge_integration npc_follow_command_moves_companion)
add_test(NAME client_runtime_bridge_wildlife_actor_chases_and_attacks COMMAND pathfinder_tests_client_runtime_bridge_integration wildlife_actor_chases_and_attacks)

# Architecture guard: P56 production factory must use runtime-backed bridge
add_test(
    NAME architecture_client_runtime_bridge_no_production_stub
    COMMAND bash -c "grep -rn 'RuntimeBacked' ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_runtime_host/client_runtime_host_factory.cpp"
)

# Architecture guard: client_runtime host factory must not be stub mode
add_test(
    NAME architecture_client_server_factory_runtime_backed
    COMMAND bash -c "! grep -rni 'StubForProtocolTest\|Disabled.*mode' ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_runtime_host/client_runtime_host_factory.cpp"
)

# Architecture guard: client_projection_adapter.cpp production path must not return empty success
add_test(
    NAME architecture_client_projection_adapter_no_empty_stub
    COMMAND bash -c "grep -rn 'bridge_port_' ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/client_projection_adapter.cpp"
)

# Architecture guard: client_available_command_adapter.cpp production path must not only return Wait/Inspect
add_test(
    NAME architecture_client_available_command_adapter_no_wait_inspect_stub
    COMMAND bash -c "grep -rn 'option_bridge_' ${CMAKE_CURRENT_SOURCE_DIR}/../src/client_protocol/client_available_command_adapter.cpp"
)

# World Region State tests (P59)
add_executable(pathfinder_tests_world_region_state
    unit/world_region_state/world_region_state_main.cpp
    unit/world_region_state/world_region_state_test.cpp
)

target_link_libraries(pathfinder_tests_world_region_state PRIVATE
    pathfinder_world_region_state
    pathfinder_world_runtime
    pathfinder_world_generation
    pathfinder_world_inventory
    pathfinder_foundation
)

add_test(NAME world_region_state_smoke COMMAND pathfinder_tests_world_region_state smoke)
add_test(NAME world_region_state_read_region_slice COMMAND pathfinder_tests_world_region_state read_region_slice)
add_test(NAME world_region_state_in_memory_store COMMAND pathfinder_tests_world_region_state in_memory_store)
add_test(NAME world_region_state_snapshot_builder COMMAND pathfinder_tests_world_region_state snapshot_builder)
add_test(NAME world_region_state_snapshot_roundtrip COMMAND pathfinder_tests_world_region_state snapshot_roundtrip)
add_test(NAME world_region_state_active_region_restore_rejected COMMAND pathfinder_tests_world_region_state active_region_restore_rejected)
add_test(NAME world_region_state_cross_region_entity_conflict COMMAND pathfinder_tests_world_region_state cross_region_entity_conflict)
add_test(NAME world_region_state_lifecycle_seal_restore COMMAND pathfinder_tests_world_region_state lifecycle_seal_restore)
add_test(NAME world_region_state_ensure_available COMMAND pathfinder_tests_world_region_state ensure_available)
add_test(NAME world_region_state_harvest_state_preservation COMMAND pathfinder_tests_world_region_state harvest_state_preservation)
add_test(NAME world_region_state_entity_roundtrip COMMAND pathfinder_tests_world_region_state entity_roundtrip)
add_test(NAME world_region_state_unload_guard COMMAND pathfinder_tests_world_region_state unload_guard)
add_test(NAME world_region_state_negative_coord_region COMMAND pathfinder_tests_world_region_state negative_coord_region)
add_test(NAME world_region_state_snapshot_validation COMMAND pathfinder_tests_world_region_state snapshot_validation)
add_test(NAME world_region_state_save_bridge COMMAND pathfinder_tests_world_region_state save_bridge)
add_test(NAME world_region_state_double_seal COMMAND pathfinder_tests_world_region_state double_seal)
add_test(NAME world_region_state_detach_safe_only_detaches COMMAND pathfinder_tests_world_region_state detach_safe_only_detaches)
add_test(NAME world_region_state_exploration_restore COMMAND pathfinder_tests_world_region_state exploration_restore)
add_test(NAME world_region_state_region_coord_consistency COMMAND pathfinder_tests_world_region_state region_coord_consistency)
add_test(NAME world_region_state_p45_pickup_drop_boundary COMMAND pathfinder_tests_world_region_state p45_pickup_drop_boundary)

# Architecture guard: P59 must not depend on frontend projection for snapshot
add_test(
    NAME architecture_p59_no_frontend_projection_dependency
    COMMAND bash -c "! grep -rn 'client_protocol\|client_runtime_bridge\|ClientWorldProjection' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_region_state/ ${CMAKE_CURRENT_SOURCE_DIR}/../include/pathfinder/world_region_state/"
)


# World Map Interaction tests (P60)
add_executable(pathfinder_tests_world_map_interaction
    unit/world_map_interaction/world_map_interaction_main.cpp
    unit/world_map_interaction/world_map_interaction_test.cpp
)

target_link_libraries(pathfinder_tests_world_map_interaction PRIVATE
    pathfinder_world_map_interaction
    pathfinder_world_region_state
    pathfinder_world_runtime
    pathfinder_world_generation
    pathfinder_world_inventory
    pathfinder_world_harvest
    pathfinder_client_protocol
    pathfinder_foundation
)

add_test(NAME world_map_interaction_smoke COMMAND pathfinder_tests_world_map_interaction smoke)
add_test(NAME world_map_interaction_activity_window COMMAND pathfinder_tests_world_map_interaction activity_window)
add_test(NAME world_map_interaction_lifecycle_trigger COMMAND pathfinder_tests_world_map_interaction lifecycle_trigger)
add_test(NAME world_map_interaction_map_selection COMMAND pathfinder_tests_world_map_interaction map_selection)
add_test(NAME world_map_interaction_map_projection COMMAND pathfinder_tests_world_map_interaction map_projection)

# P60 Integration tests
add_executable(pathfinder_tests_p60
    integration/p60/p60_main.cpp
    integration/p60/map_interaction_seal_restore_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p60 PRIVATE
    pathfinder_client_runtime_host
    pathfinder_world_map_interaction
    pathfinder_world_region_state
    pathfinder_world_runtime
    pathfinder_world_generation
    pathfinder_world_inventory
    pathfinder_world_harvest
    pathfinder_client_protocol
    pathfinder_client_http
    pathfinder_foundation
)

add_test(NAME p60_smoke COMMAND pathfinder_tests_p60 smoke)
add_test(NAME p60_map_interaction_seal_restore_flow COMMAND pathfinder_tests_p60 map_interaction_seal_restore_flow)
add_test(NAME p60_map_interaction_inventory_option_flow COMMAND pathfinder_tests_p60 map_interaction_inventory_option_flow)
add_test(NAME p60_map_interaction_stale_selection_no_snapshot_pollution COMMAND pathfinder_tests_p60 map_interaction_stale_selection_no_snapshot_pollution)

# Architecture guard: P60 must not directly modify runtime arrays
add_test(
    NAME architecture_p60_no_direct_runtime_array_modification
    COMMAND bash -c "! grep -rn 'cells_\.\\|entities_\.\\|actors_\.\\|resource_nodes_\.' ${CMAKE_CURRENT_SOURCE_DIR}/../src/world_map_interaction/"
)

# Camp Runtime tests (P65)
add_executable(pathfinder_tests_camp_task_graph
    unit/camp/camp_task_graph_test.cpp
)

target_link_libraries(pathfinder_tests_camp_task_graph PRIVATE
    pathfinder_camp
    pathfinder_world_command
    pathfinder_foundation
)

add_test(NAME camp_task_graph_branch_projection COMMAND pathfinder_tests_camp_task_graph)

# V3 Sandbox tests (V3.0)
add_executable(pathfinder_tests_v3_sandbox
    unit/v3_sandbox/v3_sandbox_test.cpp
)

target_compile_definitions(pathfinder_tests_v3_sandbox PRIVATE
    PATHFINDER_CONTENT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/../../content"
)

target_link_libraries(pathfinder_tests_v3_sandbox PRIVATE
    pathfinder_v3_sandbox
    pathfinder_foundation
)

add_test(NAME v3_sandbox_place_and_paint COMMAND pathfinder_tests_v3_sandbox place_and_paint)
add_test(NAME v3_sandbox_local_learning COMMAND pathfinder_tests_v3_sandbox local_learning)
add_test(NAME v3_sandbox_rotten_personal COMMAND pathfinder_tests_v3_sandbox rotten_personal)
add_test(NAME v3_sandbox_perception_no_god_view COMMAND pathfinder_tests_v3_sandbox perception_no_god_view)
add_test(NAME v3_sandbox_stone_unlock COMMAND pathfinder_tests_v3_sandbox stone_unlock)
