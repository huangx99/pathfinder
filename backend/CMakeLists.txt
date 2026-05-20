cmake_minimum_required(VERSION 3.20)
project(pathfinder_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Foundation library
add_library(pathfinder_foundation STATIC
    src/foundation/error.cpp
    src/foundation/id.cpp
    src/foundation/version.cpp
    src/foundation/hash.cpp
    src/foundation/time.cpp
    src/foundation/random.cpp
)

target_include_directories(pathfinder_foundation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Config library (P1)
add_library(pathfinder_config STATIC
    src/config/validation.cpp
    src/config/package.cpp
    src/config/loader.cpp
)

target_include_directories(pathfinder_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_config PUBLIC
    pathfinder_foundation
)

# Command library (P1)
add_library(pathfinder_command STATIC
    src/command/types.cpp
    src/command/target.cpp
    src/command/action_command.cpp
    src/command/envelope.cpp
    src/command/validation.cpp
)

target_include_directories(pathfinder_command PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_command PUBLIC
    pathfinder_foundation
)

# State library (P2 + P3)
add_library(pathfinder_state STATIC
    src/state/types.cpp
    src/state/game_state.cpp
    src/state/state_change.cpp
    src/state/state_change_gate.cpp
    src/state/actor_state.cpp
    src/state/state_value.cpp
    src/state/minimal_state_applier.cpp
)

target_include_directories(pathfinder_state PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_state PUBLIC
    pathfinder_foundation
    pathfinder_config
    pathfinder_command
    pathfinder_object
    pathfinder_cognition
)

# Event library (P2)
add_library(pathfinder_event STATIC
    src/event/types.cpp
    src/event/event_record.cpp
    src/event/event_stream.cpp
)

target_include_directories(pathfinder_event PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_event PUBLIC
    pathfinder_foundation
    pathfinder_state
)

# Pipeline library (P2)
add_library(pathfinder_pipeline STATIC
    src/pipeline/types.cpp
    src/pipeline/context.cpp
    src/pipeline/result.cpp
    src/pipeline/rule_pipeline.cpp
)

target_include_directories(pathfinder_pipeline PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_pipeline PUBLIC
    pathfinder_foundation
    pathfinder_config
    pathfinder_command
    pathfinder_state
    pathfinder_event
    pathfinder_rules
)

# Object library (P3)
add_library(pathfinder_object STATIC
    src/object/types.cpp
    src/object/world_object.cpp
)

target_include_directories(pathfinder_object PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_object PUBLIC
    pathfinder_foundation
)

# Cognition library (P3 + P15)
add_library(pathfinder_cognition STATIC
    src/cognition/types.cpp
    src/cognition/cognition_state.cpp
    src/cognition/cognition_v2_types.cpp
    src/cognition/cognition_v2_state.cpp
    src/cognition/cognition_confidence.cpp
    src/cognition/cognition_evidence_builder.cpp
    src/cognition/cognition_query.cpp
)

target_include_directories(pathfinder_cognition PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_cognition PUBLIC
    pathfinder_foundation
)

# Rules library (P3)
add_library(pathfinder_rules STATIC
    src/rules/rules_smoke.cpp
    src/rules/unknown_fruit_fixture.cpp
    src/rules/eat_object_resolver.cpp
)

target_include_directories(pathfinder_rules PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_rules PUBLIC
    pathfinder_foundation
    pathfinder_command
    pathfinder_state
    pathfinder_event
    pathfinder_object
    pathfinder_cognition
)

# Replay library (P4)
add_library(pathfinder_replay STATIC
    src/replay/types.cpp
    src/replay/command_replay.cpp
    src/replay/random_replay.cpp
    src/replay/replay_runner.cpp
)

target_include_directories(pathfinder_replay PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_replay PUBLIC
    pathfinder_foundation
    pathfinder_command
    pathfinder_state
    pathfinder_event
    pathfinder_pipeline
)


# Save package library (P30)
add_library(pathfinder_save STATIC
    src/save/save_package.cpp
)

target_include_directories(pathfinder_save PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_save PUBLIC
    pathfinder_foundation
    pathfinder_state
    pathfinder_event
    pathfinder_command
    pathfinder_pipeline
    pathfinder_replay
)

# Protocol library (P4)
add_library(pathfinder_protocol STATIC
    src/protocol/types.cpp
    src/protocol/envelope.cpp
    src/protocol/command_result_adapter.cpp
)

target_include_directories(pathfinder_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_protocol PUBLIC
    pathfinder_foundation
    pathfinder_command
    pathfinder_pipeline
)

# Agent library (P5)
add_library(pathfinder_agent STATIC
    src/agent/types.cpp
    src/agent/definition.cpp
    src/agent/binding.cpp
    src/agent/observation.cpp
    src/agent/action_space.cpp
    src/agent/intent.cpp
    src/agent/command_adapter.cpp
)

target_include_directories(pathfinder_agent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent PUBLIC
    pathfinder_foundation
    pathfinder_command
)

# Agent Builders library (P6)
add_library(pathfinder_agent_builders STATIC
    src/agent/builder_types.cpp
    src/agent/observation_builder.cpp
    src/agent/action_space_builder.cpp
)

target_include_directories(pathfinder_agent_builders PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_builders PUBLIC
    pathfinder_agent
    pathfinder_state
    pathfinder_object
    pathfinder_cognition
    pathfinder_foundation
)

# Agent Runtime library (P7)
add_library(pathfinder_agent_runtime STATIC
    src/agent/policy.cpp
    src/agent/runtime_types.cpp
    src/agent/runtime.cpp
)

target_include_directories(pathfinder_agent_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_runtime PUBLIC
    pathfinder_agent
    pathfinder_agent_builders
    pathfinder_pipeline
    pathfinder_state
    pathfinder_object
    pathfinder_cognition
    pathfinder_command
    pathfinder_foundation
)

# Agent Record library (P8)
add_library(pathfinder_agent_record STATIC
    src/agent/record_types.cpp
    src/agent/record_builder.cpp
    src/agent/replay_bridge.cpp
)

target_include_directories(pathfinder_agent_record PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_record PUBLIC
    pathfinder_agent
    pathfinder_agent_runtime
    pathfinder_replay
    pathfinder_pipeline
    pathfinder_state
    pathfinder_event
    pathfinder_command
    pathfinder_foundation
)

# Agent Replay Lock library (P9)
add_library(pathfinder_agent_replay_lock STATIC
    src/agent/replay_lock.cpp
)

target_include_directories(pathfinder_agent_replay_lock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_replay_lock PUBLIC
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_replay
    pathfinder_foundation
)

# Agent Training Sample library (P9)
add_library(pathfinder_agent_training_sample STATIC
    src/agent/training_sample.cpp
)

target_include_directories(pathfinder_agent_training_sample PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_training_sample PUBLIC
    pathfinder_agent_record
    pathfinder_agent_replay_lock
    pathfinder_agent
    pathfinder_replay
    pathfinder_pipeline
    pathfinder_command
    pathfinder_foundation
)

# Agent History Query library (P10)
add_library(pathfinder_agent_history_query STATIC
    src/agent/history_query.cpp
)

target_include_directories(pathfinder_agent_history_query PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_history_query PUBLIC
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_foundation
)

# Agent Projection library (P10)
add_library(pathfinder_agent_projection STATIC
    src/protocol/agent_projection.cpp
)

target_include_directories(pathfinder_agent_projection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_projection PUBLIC
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

# Agent Debug Report library (P11)
add_library(pathfinder_agent_debug_report STATIC
    src/agent/debug_report.cpp
)

target_include_directories(pathfinder_agent_debug_report PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_debug_report PUBLIC
    pathfinder_agent_projection
    pathfinder_agent_history_query
    pathfinder_agent_replay_lock
    pathfinder_agent_training_sample
    pathfinder_agent_record
    pathfinder_agent
    pathfinder_protocol
    pathfinder_foundation
)

# Agent Debug Export library (P11)
add_library(pathfinder_agent_debug_export STATIC
    src/agent/debug_export.cpp
)

target_include_directories(pathfinder_agent_debug_export PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_debug_export PUBLIC
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

# Agent Local Export library (P12)
add_library(pathfinder_agent_local_export STATIC
    src/agent/local_export.cpp
)

target_include_directories(pathfinder_agent_local_export PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_local_export PUBLIC
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

# Agent Debug CLI library (P13)
add_library(pathfinder_agent_debug_cli_lib STATIC
    src/agent/debug_cli.cpp
)

target_include_directories(pathfinder_agent_debug_cli_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_debug_cli_lib PUBLIC
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

# Agent Debug Input library (P14)
add_library(pathfinder_agent_debug_input STATIC
    src/agent/debug_input.cpp
)

target_include_directories(pathfinder_agent_debug_input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_debug_input PUBLIC
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

# Memory library (P16 + P17)
add_library(pathfinder_memory STATIC
    src/memory/types.cpp
    src/memory/memory_record.cpp
    src/memory/memory_store.cpp
    src/memory/memory_writer.cpp
    src/memory/memory_decay.cpp
    src/memory/memory_events.cpp
    src/memory/memory_summary.cpp
    src/memory/memory_compression.cpp
    src/memory/memory_recall.cpp
)

target_include_directories(pathfinder_memory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_memory PUBLIC
    pathfinder_foundation
    pathfinder_event
    pathfinder_cognition
)

# Condition expression library (P27)
add_library(pathfinder_condition STATIC
    src/condition/condition_expression_types.cpp
    src/condition/condition_expression_context.cpp
    src/condition/condition_expression_registry.cpp
    src/condition/condition_expression_evaluator.cpp
    src/condition/condition_normalizer.cpp
    src/condition/condition_summary.cpp
    src/condition/legacy_condition_adapter.cpp
)

target_include_directories(pathfinder_condition PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_condition PUBLIC
    pathfinder_foundation
)


# Replay Diagnostics library (P31)
add_library(pathfinder_replay_diagnostics STATIC
    src/replay/replay_diagnostics.cpp
)

target_include_directories(pathfinder_replay_diagnostics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_replay_diagnostics PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_replay
    pathfinder_save
)

# Knowledge library (P18 + P19 + P20)
add_library(pathfinder_knowledge STATIC
    src/knowledge/knowledge_types.cpp
    src/knowledge/knowledge_claim.cpp
    src/knowledge/knowledge_repository.cpp
    src/knowledge/knowledge_formation.cpp
    src/knowledge/knowledge_projection.cpp
    src/knowledge/knowledge_revision.cpp
    src/knowledge/knowledge_propagation.cpp
)

target_include_directories(pathfinder_knowledge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_knowledge PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_memory
)

# Reaction library (P28)
add_library(pathfinder_reaction STATIC
    src/reaction/reaction_types.cpp
    src/reaction/reaction_rule.cpp
    src/reaction/reaction_trace.cpp
    src/reaction/reaction_context.cpp
    src/reaction/reaction_resolver.cpp
    src/reaction/reaction_fixtures.cpp
    src/reaction/reaction_content.cpp
    src/reaction/reaction_applier.cpp
    src/reaction/reaction_learning_bridge.cpp
)

target_include_directories(pathfinder_reaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_reaction PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_object
)

# Tribe library (P23/P24)
add_library(pathfinder_tribe STATIC
    src/tribe/tribe_types.cpp
    src/tribe/tribe_state.cpp
    src/tribe/tribe_coordination.cpp
    src/tribe/tribe_conflict.cpp
)

target_include_directories(pathfinder_tribe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_tribe PUBLIC
    pathfinder_foundation
    pathfinder_knowledge
)


# Danger library (P29)
add_library(pathfinder_danger STATIC
    src/danger/danger_types.cpp
    src/danger/threat_profile_builder.cpp
    src/danger/hazard_resolver.cpp
    src/danger/danger_knowledge_bridge.cpp
    src/danger/tribe_danger_bridge.cpp
)

target_include_directories(pathfinder_danger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_danger PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_reaction
    pathfinder_knowledge
    pathfinder_cognition
    pathfinder_tribe
    pathfinder_object
)

# Civilization library (P26)
add_library(pathfinder_civilization STATIC
    src/civilization/civilization_types.cpp
    src/civilization/civilization_state.cpp
)

target_include_directories(pathfinder_civilization PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_civilization PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_tribe
)

# Learning library (P21)
add_library(pathfinder_learning STATIC
    src/learning/learning_loop.cpp
)

target_include_directories(pathfinder_learning PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_learning PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_cognition
    pathfinder_memory
    pathfinder_knowledge
)

# Agent Debug CLI executable (P13)
add_executable(pathfinder_agent_debug_cli_exe
    tools/agent_debug_cli_main.cpp
)

set_target_properties(pathfinder_agent_debug_cli_exe PROPERTIES
    OUTPUT_NAME pathfinder_agent_debug_cli
)

target_link_libraries(pathfinder_agent_debug_cli_exe PRIVATE
    pathfinder_agent_debug_cli_lib
)


# World Interaction types (P38 shared DTOs)
add_library(pathfinder_world_interaction_types STATIC
    src/world_interaction/world_types.cpp
)

target_include_directories(pathfinder_world_interaction_types PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_interaction_types PUBLIC
    pathfinder_foundation
    pathfinder_knowledge
)

# Agent Reasoning library (P39)
add_library(pathfinder_agent_reasoning STATIC
    src/agent_reasoning/reasoning_types.cpp
    src/agent_reasoning/effect_semantics.cpp
    src/agent_reasoning/effect_execution.cpp
    src/agent_reasoning/reaction_planning_adapter.cpp
    src/agent_reasoning/agent_reasoner.cpp
)

target_include_directories(pathfinder_agent_reasoning PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_agent_reasoning PUBLIC
    pathfinder_foundation
    pathfinder_condition
    pathfinder_knowledge
    pathfinder_reaction
    pathfinder_world_interaction_types
)

# Goal execution library (P40)
add_library(pathfinder_goal_execution STATIC
    src/goal_execution/goal_execution_types.cpp
    src/goal_execution/goal_execution_system.cpp
)

target_include_directories(pathfinder_goal_execution PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_goal_execution PUBLIC
    pathfinder_foundation
    pathfinder_agent_reasoning
    pathfinder_world_interaction_types
    pathfinder_reaction
)

# World Interaction library (P38)
add_library(pathfinder_world_interaction STATIC
    src/world_interaction/world_services.cpp
)

target_include_directories(pathfinder_world_interaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_interaction PUBLIC
    pathfinder_foundation
    pathfinder_world_interaction_types
    pathfinder_agent_reasoning
    pathfinder_goal_execution
)

# H5 Dialog library (P22)
add_library(pathfinder_h5_dialog STATIC
    src/h5_dialog/dialog_types.cpp
    src/h5_dialog/dialog_intent.cpp
    src/h5_dialog/dialog_scenario.cpp
    src/h5_dialog/dialog_session.cpp
    src/h5_dialog/dialog_learning_adapter.cpp
    src/h5_dialog/dialog_presenter.cpp
    src/h5_dialog/dialog_turn_service.cpp
    src/h5_dialog/dialog_wire_codec.cpp
    src/h5_dialog/dialog_http_server.cpp
)

target_include_directories(pathfinder_h5_dialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_h5_dialog PUBLIC
    pathfinder_foundation
    pathfinder_world_interaction
    pathfinder_condition
    pathfinder_cognition
    pathfinder_memory
    pathfinder_knowledge
    pathfinder_learning
)


# H5 Projection library (P32)
add_library(pathfinder_h5_projection STATIC
    src/h5_projection/projection.cpp
)

target_include_directories(pathfinder_h5_projection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_h5_projection PUBLIC
    pathfinder_foundation
    pathfinder_protocol
    pathfinder_replay_diagnostics
)


# Story library (P37)
add_library(pathfinder_story STATIC
    src/story/story_types.cpp
    src/story/story_system.cpp
)

target_include_directories(pathfinder_story PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_story PUBLIC
    pathfinder_foundation
    pathfinder_h5_projection
)

# H5 Playable shell library (P33)
add_library(pathfinder_h5_playable STATIC
    src/h5_playable/playable_types.cpp
    src/h5_playable/playable_projection_mapper.cpp
    src/h5_playable/playable_turn_service.cpp
    src/h5_playable/playable_wire_codec.cpp
    src/h5_playable/playable_http_server.cpp
)

target_include_directories(pathfinder_h5_playable PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_h5_playable PUBLIC
    pathfinder_foundation
    pathfinder_world_interaction
    pathfinder_goal_execution
    pathfinder_h5_dialog
    pathfinder_h5_projection
    pathfinder_story
    pathfinder_condition
    pathfinder_knowledge
)

# H5 Playable Server executable (P33)
add_executable(pathfinder_h5_playable_server_exe
    apps/h5_playable_server_main.cpp
)

set_target_properties(pathfinder_h5_playable_server_exe PROPERTIES
    OUTPUT_NAME pathfinder_h5_playable_server
)

target_link_libraries(pathfinder_h5_playable_server_exe PRIVATE
    pathfinder_h5_playable
)

# H5 Dialog Server executable (P22)
add_executable(pathfinder_h5_dialog_server_exe
    apps/h5_dialog_server_main.cpp
)

set_target_properties(pathfinder_h5_dialog_server_exe PROPERTIES
    OUTPUT_NAME pathfinder_h5_dialog_server
)

target_link_libraries(pathfinder_h5_dialog_server_exe PRIVATE
    pathfinder_h5_dialog
)

# Tests
add_subdirectory(tests)
