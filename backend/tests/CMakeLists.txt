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

# P22 unit tests: h5_dialog_types
add_executable(pathfinder_tests_h5_dialog_types
    unit/h5_dialog/dialog_types_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_types PRIVATE
    pathfinder_h5_dialog
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_smoke
    COMMAND pathfinder_tests_h5_dialog_types smoke
)

add_test(
    NAME h5_dialog_intent_parse_chinese
    COMMAND pathfinder_tests_h5_dialog_types intent_parse_chinese
)

add_test(
    NAME h5_dialog_scenario_catalog
    COMMAND pathfinder_tests_h5_dialog_types scenario_catalog
)

add_test(
    NAME h5_dialog_wire_codec_escape
    COMMAND pathfinder_tests_h5_dialog_types wire_codec_escape
)

# P22 unit tests: h5_dialog_intent
add_executable(pathfinder_tests_h5_dialog_intent
    unit/h5_dialog/dialog_intent_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_intent PRIVATE
    pathfinder_h5_dialog
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_intent_observe
    COMMAND pathfinder_tests_h5_dialog_intent observe
)

add_test(
    NAME h5_dialog_intent_eat_red_berry
    COMMAND pathfinder_tests_h5_dialog_intent eat_red_berry
)

add_test(
    NAME h5_dialog_intent_teach
    COMMAND pathfinder_tests_h5_dialog_intent teach
)

add_test(
    NAME h5_dialog_intent_unknown
    COMMAND pathfinder_tests_h5_dialog_intent unknown
)

# P22 unit tests: h5_dialog_scenario
add_executable(pathfinder_tests_h5_dialog_scenario
    unit/h5_dialog/dialog_scenario_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_scenario PRIVATE
    pathfinder_h5_dialog
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_scenario_default
    COMMAND pathfinder_tests_h5_dialog_scenario default_scenario
)

add_test(
    NAME h5_dialog_scenario_find_object
    COMMAND pathfinder_tests_h5_dialog_scenario find_object
)

add_test(
    NAME h5_dialog_scenario_find_feedback
    COMMAND pathfinder_tests_h5_dialog_scenario find_feedback
)

# P22 unit tests: h5_dialog_session
add_executable(pathfinder_tests_h5_dialog_session
    unit/h5_dialog/dialog_session_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_session PRIVATE
    pathfinder_h5_dialog
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_session_create
    COMMAND pathfinder_tests_h5_dialog_session session_create
)

add_test(
    NAME h5_dialog_session_save_load
    COMMAND pathfinder_tests_h5_dialog_session session_save_load
)

# P22 unit tests: h5_dialog_learning_adapter
add_executable(pathfinder_tests_h5_dialog_learning_adapter
    unit/h5_dialog/dialog_learning_adapter_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_learning_adapter PRIVATE
    pathfinder_h5_dialog
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_learning_adapter_valid_input
    COMMAND pathfinder_tests_h5_dialog_learning_adapter valid_input
)

# P22 unit tests: h5_dialog_presenter
add_executable(pathfinder_tests_h5_dialog_presenter
    unit/h5_dialog/dialog_presenter_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_presenter PRIVATE
    pathfinder_h5_dialog
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_presenter_chinese_response
    COMMAND pathfinder_tests_h5_dialog_presenter chinese_response
)

# P22 unit tests: h5_dialog_wire_codec
add_executable(pathfinder_tests_h5_dialog_wire_codec
    unit/h5_dialog/dialog_wire_codec_test.cpp
)

target_link_libraries(pathfinder_tests_h5_dialog_wire_codec PRIVATE
    pathfinder_h5_dialog
    pathfinder_foundation
)

add_test(
    NAME h5_dialog_wire_codec_parse
    COMMAND pathfinder_tests_h5_dialog_wire_codec parse_request
)

add_test(
    NAME h5_dialog_wire_codec_encode
    COMMAND pathfinder_tests_h5_dialog_wire_codec encode_response
)

add_test(
    NAME h5_dialog_wire_codec_chinese
    COMMAND pathfinder_tests_h5_dialog_wire_codec chinese_escape
)

# P22 integration tests: learning story
add_executable(pathfinder_tests_p22_learning_story
    integration/p22/h5_dialog_learning_story_test.cpp
)

target_link_libraries(pathfinder_tests_p22_learning_story PRIVATE
    pathfinder_h5_dialog
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p22_red_berry_learning_flow
    COMMAND pathfinder_tests_p22_learning_story red_berry_learning_flow
)

add_test(
    NAME p22_teach_companion_flow
    COMMAND pathfinder_tests_p22_learning_story teach_companion_flow
)

add_test(
    NAME p22_decayed_red_berry_revision_flow
    COMMAND pathfinder_tests_p22_learning_story decayed_red_berry_revision_flow
)

add_test(
    NAME p22_non_red_object_flow
    COMMAND pathfinder_tests_p22_learning_story non_red_object_flow
)

# P22 integration tests: security
add_executable(pathfinder_tests_p22_security
    integration/p22/h5_dialog_security_test.cpp
)

target_link_libraries(pathfinder_tests_p22_security PRIVATE
    pathfinder_h5_dialog
    pathfinder_learning
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_cognition
    pathfinder_foundation
)

add_test(
    NAME p22_security_forbidden_input
    COMMAND pathfinder_tests_p22_security forbidden_input
)

add_test(
    NAME p22_security_hidden_truth
    COMMAND pathfinder_tests_p22_security hidden_truth
)

# P26 Civilization unit tests: types
add_executable(pathfinder_tests_civilization_types
    unit/civilization/civilization_types_test.cpp
)

target_link_libraries(pathfinder_tests_civilization_types PRIVATE
    pathfinder_civilization
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_event
    pathfinder_cognition
    pathfinder_foundation
)

add_test(NAME civilization_stage_enum_roundtrip COMMAND pathfinder_tests_civilization_types stage_enum_roundtrip)
add_test(NAME capability_type_enum_roundtrip COMMAND pathfinder_tests_civilization_types capability_type_enum_roundtrip)
add_test(NAME capability_maturity_enum_roundtrip COMMAND pathfinder_tests_civilization_types maturity_enum_roundtrip)
add_test(NAME capability_usability_enum_roundtrip COMMAND pathfinder_tests_civilization_types usability_enum_roundtrip)
add_test(NAME capability_change_reason_enum_roundtrip COMMAND pathfinder_tests_civilization_types change_reason_enum_roundtrip)
add_test(NAME capability_effect_target_enum_roundtrip COMMAND pathfinder_tests_civilization_types effect_target_enum_roundtrip)
add_test(NAME effect_operation_type_enum_roundtrip COMMAND pathfinder_tests_civilization_types effect_op_enum_roundtrip)
add_test(NAME civilization_forbidden_keys COMMAND pathfinder_tests_civilization_types forbidden_keys)
add_test(NAME civilization_progress_validate COMMAND pathfinder_tests_civilization_types progress_validate)
add_test(NAME capability_condition_validate COMMAND pathfinder_tests_civilization_types condition_validate)

# P26 Civilization unit tests: state resolvers
add_executable(pathfinder_tests_civilization_state
    unit/civilization/civilization_state_test.cpp
)

target_link_libraries(pathfinder_tests_civilization_state PRIVATE
    pathfinder_civilization
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_event
    pathfinder_cognition
    pathfinder_foundation
)

add_test(NAME candidate_builder_identify_edible COMMAND pathfinder_tests_civilization_state candidate_builder_identify_edible)
add_test(NAME candidate_builder_identify_danger COMMAND pathfinder_tests_civilization_state candidate_builder_identify_danger)
add_test(NAME candidate_builder_safe_foraging_prereq COMMAND pathfinder_tests_civilization_state candidate_builder_safe_foraging_prereq)
add_test(NAME requirement_evaluator_scores COMMAND pathfinder_tests_civilization_state requirement_evaluator_scores)
add_test(NAME state_resolver_unknown_to_candidate COMMAND pathfinder_tests_civilization_state state_resolver_unknown_to_candidate)
add_test(NAME state_resolver_candidate_to_emerging COMMAND pathfinder_tests_civilization_state state_resolver_candidate_to_emerging)
add_test(NAME state_resolver_emerging_to_stable COMMAND pathfinder_tests_civilization_state state_resolver_emerging_to_stable)
add_test(NAME usability_blocked_by_conflict COMMAND pathfinder_tests_civilization_state usability_blocked_by_conflict)
add_test(NAME stage_foraging_derived COMMAND pathfinder_tests_civilization_state stage_foraging_derived)
add_test(NAME effect_resolver_stable_capability COMMAND pathfinder_tests_civilization_state effect_resolver_stable_capability)

# P26 Civilization unit tests: resolver orchestrator
add_executable(pathfinder_tests_civilization_resolver
    unit/civilization/civilization_resolver_test.cpp
)

target_link_libraries(pathfinder_tests_civilization_resolver PRIVATE
    pathfinder_civilization
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_event
    pathfinder_cognition
    pathfinder_foundation
)

add_test(NAME resolver_validates_input COMMAND pathfinder_tests_civilization_resolver validates_input)
add_test(NAME resolver_builds_candidates COMMAND pathfinder_tests_civilization_resolver builds_candidates)
add_test(NAME resolver_full_pipeline COMMAND pathfinder_tests_civilization_resolver full_pipeline)
add_test(NAME resolver_deterministic COMMAND pathfinder_tests_civilization_resolver deterministic)
add_test(NAME resolver_rejects_forbidden_keys COMMAND pathfinder_tests_civilization_resolver rejects_forbidden_keys)
add_test(NAME resolver_events_on_unlock COMMAND pathfinder_tests_civilization_resolver events_on_unlock)
add_test(NAME resolver_single_success_not_stable COMMAND pathfinder_tests_civilization_resolver single_success_not_stable)
add_test(NAME resolver_stage_no_reverse_unlock COMMAND pathfinder_tests_civilization_resolver stage_no_reverse_unlock)
add_test(NAME resolver_projection_safe COMMAND pathfinder_tests_civilization_resolver projection_safe)
add_test(NAME resolver_trace_safe COMMAND pathfinder_tests_civilization_resolver trace_safe)

# P26 Civilization integration tests: capability flow
add_executable(pathfinder_tests_p26_flow
    integration/p26/civilization_capability_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p26_flow PRIVATE
    pathfinder_civilization
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_event
    pathfinder_cognition
    pathfinder_foundation
)

add_test(NAME p26_identify_edible_candidate_flow COMMAND pathfinder_tests_p26_flow identify_edible_candidate_flow)
add_test(NAME p26_identify_danger_candidate_flow COMMAND pathfinder_tests_p26_flow identify_danger_candidate_flow)
add_test(NAME p26_safe_foraging_stable_flow COMMAND pathfinder_tests_p26_flow safe_foraging_stable_flow)
add_test(NAME p26_conflict_deescalation_candidate_from_p25 COMMAND pathfinder_tests_p26_flow conflict_deescalation_candidate_from_p25)
add_test(NAME p26_capability_requires_repeated_evidence COMMAND pathfinder_tests_p26_flow requires_repeated_evidence)
add_test(NAME p26_stable_capability_emits_unlock_once COMMAND pathfinder_tests_p26_flow emits_unlock_once)
add_test(NAME p26_stage_foraging_from_stable_capabilities COMMAND pathfinder_tests_p26_flow foraging_stage)
add_test(NAME p26_blocked_by_conflict_does_not_lose_maturity COMMAND pathfinder_tests_p26_flow blocked_keeps_maturity)
add_test(NAME p26_effect_draft_from_stable_usable_capability COMMAND pathfinder_tests_p26_flow effect_draft_stable_usable)
add_test(NAME p26_deterministic_resolution COMMAND pathfinder_tests_p26_flow deterministic)

# P26 Civilization integration tests: security
add_executable(pathfinder_tests_p26_security
    integration/p26/civilization_capability_security_test.cpp
)

target_link_libraries(pathfinder_tests_p26_security PRIVATE
    pathfinder_civilization
    pathfinder_tribe
    pathfinder_knowledge
    pathfinder_memory
    pathfinder_event
    pathfinder_cognition
    pathfinder_foundation
)

add_test(NAME p26_security_rejects_unlocked_bool COMMAND pathfinder_tests_p26_security unlocked_bool)
add_test(NAME p26_security_rejects_stage_only_unlock COMMAND pathfinder_tests_p26_security stage_only_unlock)
add_test(NAME p26_security_rejects_frontend_unlock COMMAND pathfinder_tests_p26_security frontend_unlock)
add_test(NAME p26_security_rejects_hidden_truth COMMAND pathfinder_tests_p26_security hidden_truth)
add_test(NAME p26_security_rejects_raw_game_state COMMAND pathfinder_tests_p26_security raw_game_state)
add_test(NAME p26_security_rejects_hp_death_kill_loot COMMAND pathfinder_tests_p26_security hp_death_kill_loot)
add_test(NAME p26_security_rejects_direct_state_mutation COMMAND pathfinder_tests_p26_security direct_state_mutation)
add_test(NAME p26_security_rejects_duplicate_capability COMMAND pathfinder_tests_p26_security duplicate_capability)
add_test(NAME p26_security_rejects_test_only_enum COMMAND pathfinder_tests_p26_security test_only_enum)
add_test(NAME p26_security_rejects_single_success_as_stable COMMAND pathfinder_tests_p26_security single_success)

# P27 Condition expression unit tests: types
add_executable(pathfinder_tests_condition_types
    unit/condition/condition_expression_types_test.cpp
)

target_link_libraries(pathfinder_tests_condition_types PRIVATE
    pathfinder_condition
    pathfinder_foundation
)

add_test(NAME condition_ref_validation COMMAND pathfinder_tests_condition_types ref_validation)
add_test(NAME condition_value_debug_string COMMAND pathfinder_tests_condition_types value_debug_string)
add_test(NAME condition_definition_validation COMMAND pathfinder_tests_condition_types definition_validation)
add_test(NAME condition_definition_rejects_forbidden_value COMMAND pathfinder_tests_condition_types definition_rejects_forbidden_value)

# P27 Condition expression unit tests: evaluator
add_executable(pathfinder_tests_condition_evaluator
    unit/condition/condition_expression_evaluator_test.cpp
)

target_link_libraries(pathfinder_tests_condition_evaluator PRIVATE
    pathfinder_condition
    pathfinder_foundation
)

add_test(NAME condition_evaluator_object_state COMMAND pathfinder_tests_condition_evaluator object_state_canonical)
add_test(NAME condition_evaluator_civilization_numeric COMMAND pathfinder_tests_condition_evaluator civilization_numeric)
add_test(NAME condition_evaluator_combined_and COMMAND pathfinder_tests_condition_evaluator combined_and_condition)
add_test(NAME condition_context_rejects_unsafe_numeric_fields COMMAND pathfinder_tests_condition_evaluator context_rejects_unsafe_numeric_fields)
add_test(NAME condition_canonical_rejects_unsupported_op COMMAND pathfinder_tests_condition_evaluator canonical_rejects_unsupported_op)
add_test(NAME condition_source_target_domains COMMAND pathfinder_tests_condition_evaluator source_target_domains)

# P27 Condition expression unit tests: normalizer
add_executable(pathfinder_tests_condition_normalizer
    unit/condition/condition_normalizer_test.cpp
)

target_link_libraries(pathfinder_tests_condition_normalizer PRIVATE
    pathfinder_condition
    pathfinder_foundation
)

add_test(NAME condition_normalizer_legacy_mappings COMMAND pathfinder_tests_condition_normalizer legacy_mappings)
add_test(NAME condition_normalizer_unknown_legacy_fails COMMAND pathfinder_tests_condition_normalizer unknown_legacy_fails)
add_test(NAME condition_normalizer_capability_condition COMMAND pathfinder_tests_condition_normalizer capability_condition)

# P27 Condition expression integration tests
add_executable(pathfinder_tests_p27_migration_flow
    integration/p27/condition_migration_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p27_migration_flow PRIVATE
    pathfinder_knowledge
    pathfinder_condition
    pathfinder_memory
    pathfinder_foundation
)

add_test(NAME p27_knowledge_condition_normalizes COMMAND pathfinder_tests_p27_migration_flow knowledge_condition)
add_test(NAME p27_legacy_fields_combine COMMAND pathfinder_tests_p27_migration_flow combined_fields)

add_executable(pathfinder_tests_p27_gate_scan
    integration/p27/condition_gate_scan_test.cpp
)

target_link_libraries(pathfinder_tests_p27_gate_scan PRIVATE
    pathfinder_foundation
)

add_test(NAME p27_condition_authority_gate COMMAND pathfinder_tests_p27_gate_scan)

# P28 Object reaction unit tests: types
add_executable(pathfinder_tests_reaction_types
    unit/reaction/reaction_types_test.cpp
)

target_link_libraries(pathfinder_tests_reaction_types PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME reaction_enum_roundtrip COMMAND pathfinder_tests_reaction_types enum_roundtrip)
add_test(NAME reaction_input_validation COMMAND pathfinder_tests_reaction_types input_validation)
add_test(NAME reaction_pattern_rejects_dynamic_tags COMMAND pathfinder_tests_reaction_types pattern_rejects_dynamic_tags)

# P28 Object reaction unit tests: context
add_executable(pathfinder_tests_reaction_context
    unit/reaction/reaction_context_test.cpp
)

target_link_libraries(pathfinder_tests_reaction_context PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME reaction_context_builder_maps_source_target COMMAND pathfinder_tests_reaction_context)

# P28 Object reaction unit tests: resolver
add_executable(pathfinder_tests_reaction_resolver
    unit/reaction/reaction_resolver_test.cpp
)

target_link_libraries(pathfinder_tests_reaction_resolver PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME reaction_resolver_no_matching_rule COMMAND pathfinder_tests_reaction_resolver no_matching_rule)
add_test(NAME reaction_resolver_priority_stable_order COMMAND pathfinder_tests_reaction_resolver priority)
add_test(NAME reaction_resolver_produce_object_draft COMMAND pathfinder_tests_reaction_resolver produce_object_draft)
add_test(NAME reaction_resolver_data_driven_feedback COMMAND pathfinder_tests_reaction_resolver data_driven_feedback)

# P28 Object reaction integration tests
add_executable(pathfinder_tests_p28_fire_branch
    integration/p28/object_reaction_fire_branch_test.cpp
)

target_link_libraries(pathfinder_tests_p28_fire_branch PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME p28_fire_dry_branch_to_torch COMMAND pathfinder_tests_p28_fire_branch dry)
add_test(NAME p28_fire_wet_branch_condition_blocked COMMAND pathfinder_tests_p28_fire_branch wet)

add_executable(pathfinder_tests_p28_water_fire
    integration/p28/object_reaction_water_fire_test.cpp
)

target_link_libraries(pathfinder_tests_p28_water_fire PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME p28_water_extinguishes_fire COMMAND pathfinder_tests_p28_water_fire)

add_executable(pathfinder_tests_p28_gate_scan
    integration/p28/object_reaction_gate_scan_test.cpp
)

target_link_libraries(pathfinder_tests_p28_gate_scan PRIVATE
    pathfinder_foundation
)

add_test(NAME p28_reaction_gate_scan COMMAND pathfinder_tests_p28_gate_scan)


# P28.5 Configurable item/reaction pipeline unit tests: content
add_executable(pathfinder_tests_reaction_content
    unit/reaction/reaction_content_test.cpp
)

target_link_libraries(pathfinder_tests_reaction_content PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME reaction_content_valid_pack COMMAND pathfinder_tests_reaction_content valid_pack)
add_test(NAME reaction_content_rejects_dynamic_tag COMMAND pathfinder_tests_reaction_content dynamic_tag)
add_test(NAME reaction_content_rejects_missing_product COMMAND pathfinder_tests_reaction_content missing_product)
add_test(NAME reaction_content_single_object_drink COMMAND pathfinder_tests_reaction_content single_object_drink)

# P28.5 Configurable item/reaction pipeline unit tests: applier
add_executable(pathfinder_tests_reaction_applier
    unit/reaction/reaction_applier_test.cpp
)

target_link_libraries(pathfinder_tests_reaction_applier PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME reaction_applier_transform_usable_item COMMAND pathfinder_tests_reaction_applier transform)
add_test(NAME reaction_applier_produce_runtime_object COMMAND pathfinder_tests_reaction_applier produce)

# P28.5 Configurable item/reaction pipeline unit tests: learning bridge
add_executable(pathfinder_tests_reaction_learning_bridge
    unit/reaction/reaction_learning_bridge_test.cpp
)

target_link_libraries(pathfinder_tests_reaction_learning_bridge PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME reaction_learning_bridge_delivery COMMAND pathfinder_tests_reaction_learning_bridge delivery)
add_test(NAME reaction_learning_bridge_observation_flag COMMAND pathfinder_tests_reaction_learning_bridge observation_flag)

# P28.5 Configurable item/reaction pipeline integration tests
add_executable(pathfinder_tests_p28_5_pipeline
    integration/p28_5/configurable_reaction_pipeline_test.cpp
)

target_link_libraries(pathfinder_tests_p28_5_pipeline PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME p28_5_configurable_torch_learning COMMAND pathfinder_tests_p28_5_pipeline torch_learning)
add_test(NAME p28_5_configurable_drink COMMAND pathfinder_tests_p28_5_pipeline drink)

add_executable(pathfinder_tests_p28_5_learning_chain
    integration/p28_5/configurable_reaction_learning_chain_test.cpp
)

target_link_libraries(pathfinder_tests_p28_5_learning_chain PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME p28_5_repeated_learning_teaching_chain COMMAND pathfinder_tests_p28_5_learning_chain chain)
add_test(NAME p28_5_mixed_create_reinforce_teaching COMMAND pathfinder_tests_p28_5_learning_chain mixed)


add_executable(pathfinder_tests_p28_5_system_stress
    integration/p28_5/configurable_reaction_system_stress_test.cpp
)

target_link_libraries(pathfinder_tests_p28_5_system_stress PRIVATE
    pathfinder_reaction
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
    pathfinder_cognition
    pathfinder_tribe
    pathfinder_foundation
)

add_test(NAME p28_5_stress_01_full_pipeline COMMAND pathfinder_tests_p28_5_system_stress 01_full_pipeline)
add_test(NAME p28_5_stress_02_use_generated_torch COMMAND pathfinder_tests_p28_5_system_stress 02_use_generated_torch)
add_test(NAME p28_5_stress_03_missing_material_retry COMMAND pathfinder_tests_p28_5_system_stress 03_missing_material_retry)
add_test(NAME p28_5_stress_04_wet_branch_blocked COMMAND pathfinder_tests_p28_5_system_stress 04_wet_branch_blocked)
add_test(NAME p28_5_stress_05_wrong_action COMMAND pathfinder_tests_p28_5_system_stress 05_wrong_action)
add_test(NAME p28_5_stress_06_swapped_roles COMMAND pathfinder_tests_p28_5_system_stress 06_swapped_roles)
add_test(NAME p28_5_stress_07_consumed_water COMMAND pathfinder_tests_p28_5_system_stress 07_consumed_water)
add_test(NAME p28_5_stress_08_extinguish_fire COMMAND pathfinder_tests_p28_5_system_stress 08_extinguish_fire)
add_test(NAME p28_5_stress_09_craft_spear COMMAND pathfinder_tests_p28_5_system_stress 09_craft_spear)
add_test(NAME p28_5_stress_10_bind_bundle COMMAND pathfinder_tests_p28_5_system_stress 10_bind_bundle)
add_test(NAME p28_5_stress_11_knowledge_drafts COMMAND pathfinder_tests_p28_5_system_stress 11_knowledge_drafts)
add_test(NAME p28_5_stress_12_teach_reinforce COMMAND pathfinder_tests_p28_5_system_stress 12_teach_reinforce)
add_test(NAME p28_5_stress_13_mixed_teaching COMMAND pathfinder_tests_p28_5_system_stress 13_mixed_teaching)
add_test(NAME p28_5_stress_14_cognition_repeat COMMAND pathfinder_tests_p28_5_system_stress 14_cognition_repeat)
add_test(NAME p28_5_stress_15_cognition_separate COMMAND pathfinder_tests_p28_5_system_stress 15_cognition_separate)
add_test(NAME p28_5_stress_16_cognition_taught COMMAND pathfinder_tests_p28_5_system_stress 16_cognition_taught)
add_test(NAME p28_5_stress_17_tribe_knowledge COMMAND pathfinder_tests_p28_5_system_stress 17_tribe_knowledge)
add_test(NAME p28_5_stress_18_tribe_split_pressure COMMAND pathfinder_tests_p28_5_system_stress 18_tribe_split_pressure)
add_test(NAME p28_5_stress_19_member_split COMMAND pathfinder_tests_p28_5_system_stress 19_member_split)
add_test(NAME p28_5_stress_20_coordination_conflict COMMAND pathfinder_tests_p28_5_system_stress 20_coordination_conflict)

# Danger tests (P29)
add_executable(pathfinder_tests_danger_types
    unit/danger/danger_types_test.cpp
)

target_link_libraries(pathfinder_tests_danger_types PRIVATE
    pathfinder_danger
    pathfinder_reaction
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME danger_types_enum COMMAND pathfinder_tests_danger_types enum)
add_test(NAME danger_types_forbidden COMMAND pathfinder_tests_danger_types forbidden)
add_test(NAME danger_threat_profile_builder COMMAND pathfinder_tests_danger_types builder)

add_executable(pathfinder_tests_hazard_resolver
    unit/danger/hazard_resolver_test.cpp
)

target_link_libraries(pathfinder_tests_hazard_resolver PRIVATE
    pathfinder_danger
    pathfinder_reaction
    pathfinder_knowledge
    pathfinder_condition
    pathfinder_tribe
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME hazard_resolver_rotten_berry COMMAND pathfinder_tests_hazard_resolver rotten)
add_test(NAME hazard_resolver_torch_escape COMMAND pathfinder_tests_hazard_resolver torch)
add_test(NAME hazard_resolver_blocked_and_no_match COMMAND pathfinder_tests_hazard_resolver blocked)
add_test(NAME hazard_resolver_knowledge_countermeasure COMMAND pathfinder_tests_hazard_resolver knowledge)

add_executable(pathfinder_tests_danger_knowledge_bridge
    unit/danger/danger_knowledge_bridge_test.cpp
)

target_link_libraries(pathfinder_tests_danger_knowledge_bridge PRIVATE
    pathfinder_danger
    pathfinder_reaction
    pathfinder_knowledge
    pathfinder_foundation
)

add_test(NAME danger_knowledge_bridge_delivery COMMAND pathfinder_tests_danger_knowledge_bridge)

add_executable(pathfinder_tests_tribe_danger_bridge
    unit/danger/tribe_danger_bridge_test.cpp
)

target_link_libraries(pathfinder_tests_tribe_danger_bridge PRIVATE
    pathfinder_danger
    pathfinder_tribe
    pathfinder_foundation
)

add_test(NAME tribe_danger_bridge_pressure COMMAND pathfinder_tests_tribe_danger_bridge)

add_executable(pathfinder_tests_p29_danger_flow
    integration/p29/danger_exploration_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p29_danger_flow PRIVATE
    pathfinder_danger
    pathfinder_reaction
    pathfinder_knowledge
    pathfinder_condition
    pathfinder_tribe
    pathfinder_object
    pathfinder_foundation
)

add_test(NAME p29_learning_teaching_tribe_flow COMMAND pathfinder_tests_p29_danger_flow learning)
add_test(NAME p29_tool_mitigates_exploration COMMAND pathfinder_tests_p29_danger_flow tool)
add_test(NAME p29_security_rejects_forbidden_output COMMAND pathfinder_tests_p29_danger_flow security)

# Save package tests (P30)
add_executable(pathfinder_tests_save_package
    unit/save/save_package_test.cpp
)

target_link_libraries(pathfinder_tests_save_package PRIVATE
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

add_test(NAME save_package_enum COMMAND pathfinder_tests_save_package enum)
add_test(NAME save_package_forbidden_keys COMMAND pathfinder_tests_save_package forbidden)
add_test(NAME save_package_builder_validator COMMAND pathfinder_tests_save_package builder)
add_test(NAME save_package_tamper_fails COMMAND pathfinder_tests_save_package tamper)
add_test(NAME save_package_missing_snapshot_fails COMMAND pathfinder_tests_save_package missing_snapshot)
add_test(NAME save_package_version_range_fails COMMAND pathfinder_tests_save_package version)
add_test(NAME save_package_config_manifest_fails COMMAND pathfinder_tests_save_package config)
add_test(NAME save_restore_plan COMMAND pathfinder_tests_save_package restore)

add_executable(pathfinder_tests_p30_save_replay_flow
    integration/p30/save_replay_flow_test.cpp
)

target_link_libraries(pathfinder_tests_p30_save_replay_flow PRIVATE
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

add_test(NAME p30_save_package_replay_flow COMMAND pathfinder_tests_p30_save_replay_flow)



# H5 projection tests (P32)
add_executable(pathfinder_tests_h5_projection
    unit/h5_projection/projection_test.cpp
)

target_link_libraries(pathfinder_tests_h5_projection PRIVATE
    pathfinder_h5_projection
    pathfinder_replay_diagnostics
    pathfinder_protocol
    pathfinder_foundation
)

add_test(NAME h5_projection_enum COMMAND pathfinder_tests_h5_projection enum)
add_test(NAME h5_projection_safe_text COMMAND pathfinder_tests_h5_projection safe_text)
add_test(NAME h5_projection_request COMMAND pathfinder_tests_h5_projection request)
add_test(NAME h5_projection_player_filter COMMAND pathfinder_tests_h5_projection player_filter)
add_test(NAME h5_projection_debug COMMAND pathfinder_tests_h5_projection debug)
add_test(NAME h5_projection_knowledge COMMAND pathfinder_tests_h5_projection knowledge)
add_test(NAME h5_projection_action COMMAND pathfinder_tests_h5_projection action)
add_test(NAME h5_projection_danger COMMAND pathfinder_tests_h5_projection danger)
add_test(NAME h5_projection_truncation COMMAND pathfinder_tests_h5_projection truncation)
add_test(NAME h5_projection_script COMMAND pathfinder_tests_h5_projection script)
add_test(NAME h5_projection_player_debug_text COMMAND pathfinder_tests_h5_projection player_debug_text)
add_test(NAME h5_projection_developer_debug_text COMMAND pathfinder_tests_h5_projection developer_debug_text)
add_test(NAME h5_projection_protocol COMMAND pathfinder_tests_h5_projection protocol)
add_test(NAME h5_projection_replay COMMAND pathfinder_tests_h5_projection replay)


# Story tests (P37)
add_executable(pathfinder_tests_story
    unit/story/story_test.cpp
)

target_link_libraries(pathfinder_tests_story PRIVATE
    pathfinder_story
    pathfinder_h5_projection
    pathfinder_foundation
)

add_test(NAME story_enum COMMAND pathfinder_tests_story enum)
add_test(NAME story_scenario_validation COMMAND pathfinder_tests_story scenario)
add_test(NAME story_success_progression COMMAND pathfinder_tests_story success)
add_test(NAME story_failure_progression COMMAND pathfinder_tests_story failure)
add_test(NAME story_no_text_magic COMMAND pathfinder_tests_story no_text_magic)
add_test(NAME story_projection_player_safe COMMAND pathfinder_tests_story projection)

# H5 playable shell tests (P33)
add_executable(pathfinder_tests_world_interaction
    unit/world_interaction/world_interaction_test.cpp
)

target_link_libraries(pathfinder_tests_world_interaction PRIVATE
    pathfinder_world_interaction
    pathfinder_h5_dialog
)

add_test(NAME world_interaction_enum COMMAND pathfinder_tests_world_interaction enum)
add_test(NAME world_interaction_resolve_apply COMMAND pathfinder_tests_world_interaction interaction)
add_test(NAME world_interaction_agent_threat COMMAND pathfinder_tests_world_interaction agent)

add_executable(pathfinder_tests_h5_playable
    unit/h5_playable/playable_test.cpp
)

target_link_libraries(pathfinder_tests_h5_playable PRIVATE
    pathfinder_h5_playable
    pathfinder_h5_projection
    pathfinder_h5_dialog
    pathfinder_foundation
)

add_test(NAME h5_playable_enum COMMAND pathfinder_tests_h5_playable enum)
add_test(NAME h5_playable_validation COMMAND pathfinder_tests_h5_playable validation)
add_test(NAME h5_playable_bootstrap COMMAND pathfinder_tests_h5_playable bootstrap)
add_test(NAME h5_playable_learning COMMAND pathfinder_tests_h5_playable learning)
add_test(NAME h5_playable_targeted_actions COMMAND pathfinder_tests_h5_playable targeted_actions)
add_test(NAME h5_playable_danger COMMAND pathfinder_tests_h5_playable danger)
add_test(NAME h5_playable_reset COMMAND pathfinder_tests_h5_playable reset)
add_test(NAME h5_playable_codec COMMAND pathfinder_tests_h5_playable codec)
add_test(NAME h5_playable_rotten_scope COMMAND pathfinder_tests_h5_playable rotten_scope)
add_test(NAME h5_playable_frontend_gate COMMAND pathfinder_tests_h5_playable frontend_gate)
add_test(NAME h5_playable_execution_status COMMAND pathfinder_tests_h5_playable execution_status)
add_test(NAME h5_execution_status_visible_when_goal_execution_runs COMMAND pathfinder_tests_h5_playable execution_status)
add_test(NAME h5_playable_story_first_day COMMAND pathfinder_tests_h5_playable story)
add_test(NAME h5_playable_story_beast_use COMMAND pathfinder_tests_h5_playable story_beast_use)
add_test(NAME h5_playable_buttons_executable COMMAND pathfinder_tests_h5_playable buttons_executable)
add_test(NAME h5_playable_p38_world COMMAND pathfinder_tests_h5_playable p38_world)
add_test(NAME h5_playable_p38_agent COMMAND pathfinder_tests_h5_playable p38_agent)
add_test(NAME h5_playable_p38_beast_no_fake_fire COMMAND pathfinder_tests_h5_playable p38_beast_no_fake_fire)
add_test(NAME h5_playable_p38_failure COMMAND pathfinder_tests_h5_playable p38_failure)

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
add_test(NAME agent_reasoning_h5_projection_safe COMMAND pathfinder_tests_agent_reasoning h5_projection_safe)

# Goal execution tests (P40)
add_executable(pathfinder_tests_goal_execution
    unit/goal_execution/goal_execution_test.cpp
)

target_link_libraries(pathfinder_tests_goal_execution PRIVATE
    pathfinder_goal_execution
    pathfinder_agent_reasoning
    pathfinder_world_interaction
    pathfinder_world_interaction_types
    pathfinder_h5_dialog
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
add_test(NAME goal_execution_material_tags_from_dialog_runtime_snapshot COMMAND pathfinder_tests_goal_execution material_tags_from_dialog_runtime_snapshot)
add_test(NAME goal_execution_reaction_material_resolver_no_unknown_material_for_core_rules COMMAND pathfinder_tests_goal_execution reaction_material_resolver_no_unknown_material_for_core_rules)
add_test(NAME goal_execution_known_claims COMMAND pathfinder_tests_goal_execution known_claims)
add_test(NAME goal_execution_build_recipe COMMAND pathfinder_tests_goal_execution build_recipe)
add_test(NAME goal_execution_reservation_release COMMAND pathfinder_tests_goal_execution reservation_release)
add_test(NAME goal_execution_tick_order COMMAND pathfinder_tests_goal_execution tick_order)
add_test(NAME goal_execution_player_delay COMMAND pathfinder_tests_goal_execution player_delay)
add_test(NAME goal_execution_capability_filter COMMAND pathfinder_tests_goal_execution capability_filter)
add_test(NAME goal_execution_context_roundtrip COMMAND pathfinder_tests_goal_execution context_roundtrip)
