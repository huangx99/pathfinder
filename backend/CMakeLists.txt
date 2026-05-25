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

# Content library (P42)
add_library(pathfinder_content STATIC
    src/content/content_types.cpp
    src/content/content_registry.cpp
    src/content/content_validation.cpp
    src/content/json_content_loader.cpp
)

target_include_directories(pathfinder_content PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

target_link_libraries(pathfinder_content PUBLIC
    pathfinder_foundation
    pathfinder_config
)

# World Runtime library (P44)
add_library(pathfinder_world_runtime STATIC
    src/world_runtime/world_runtime_types.cpp
    src/world_runtime/world_grid_runtime.cpp
    src/world_runtime/world_projection_adapter.cpp
    src/world_runtime/world_command_runtime_bridge.cpp
)

target_include_directories(pathfinder_world_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_runtime PUBLIC
    pathfinder_foundation
    pathfinder_world_command
)

# World Command library (P43 only, no runtime dependency)
add_library(pathfinder_world_command STATIC
    src/world_command/world_command_types.cpp
    src/world_command/world_command_registry.cpp
    src/world_command/world_command_dispatcher.cpp
    src/world_command/world_command_projection.cpp
    src/world_command/world_command_trace.cpp
    src/world_command/world_command_pipeline.cpp
    src/world_command/handlers/noop_command_handler.cpp
    src/world_command/handlers/wait_command_handler.cpp
    src/world_command/handlers/inspect_command_handler.cpp
    src/world_command/handlers/system_tick_command_handler.cpp
    src/world_command/handlers/generate_world_command_handler.cpp
)

target_include_directories(pathfinder_world_command PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_command PUBLIC
    pathfinder_foundation
)

# World Generation library (P46 + P57)
add_library(pathfinder_world_generation STATIC
    src/world_generation/world_generation_types.cpp
    src/world_generation/world_generation_rng.cpp
    src/world_generation/terrain_generator.cpp
    src/world_generation/terrain_noise_sampler.cpp
    src/world_generation/terrain_noise_field_builder.cpp
    src/world_generation/terrain_classifier.cpp
    src/world_generation/terrain_connectivity_analyzer.cpp
    src/world_generation/terrain_connectivity_repairer.cpp
    src/world_generation/resource_distribution_generator.cpp
    src/world_generation/spawn_safety_planner.cpp
    src/world_generation/world_generation_service.cpp
    src/world_generation/world_generation_applier.cpp
    src/world_generation/world_generation_projection_bridge.cpp
    src/world_generation/generate_world_command_handler.cpp
    src/world_generation/world_region_math.cpp
    src/world_generation/world_region_ensure_types.cpp
    src/world_generation/world_region_ensure_service.cpp
    src/world_generation/move_target_region_guard.cpp
)

target_include_directories(pathfinder_world_generation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_generation PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_content
)

# World Command Runtime library (P44 runtime-aware handlers, depends on both)
add_library(pathfinder_world_command_runtime STATIC
    src/world_runtime/world_runtime_command_handlers.cpp
)

target_include_directories(pathfinder_world_command_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_command_runtime PUBLIC
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_generation
)

# World Inventory library (P45)
add_library(pathfinder_world_inventory STATIC
    src/world_inventory/world_inventory_types.cpp
    src/world_inventory/world_inventory_runtime.cpp
    src/world_inventory/inventory_projection_adapter.cpp
)

target_include_directories(pathfinder_world_inventory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_inventory PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
)

# World Harvest library (P47)
add_library(pathfinder_world_harvest STATIC
    src/world_harvest/world_harvest_types.cpp
    src/world_harvest/resource_harvest_service.cpp
    src/world_harvest/harvest_command_handler.cpp
    src/world_harvest/harvest_projection_bridge.cpp
)

target_include_directories(pathfinder_world_harvest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_harvest PUBLIC
    pathfinder_foundation
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_world_command
    pathfinder_content
)

# World Reaction library (P48)
add_library(pathfinder_world_reaction STATIC
    src/world_reaction/world_reaction_types.cpp
    src/world_reaction/world_reaction_service.cpp
    src/world_reaction/craft_command_handler.cpp
    src/world_reaction/world_reaction_projection_bridge.cpp
)

target_include_directories(pathfinder_world_reaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_reaction PUBLIC
    pathfinder_foundation
    pathfinder_content
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_world_command
)

# World Teaching library (P50)
add_library(pathfinder_world_teaching STATIC
    src/world_teaching/world_teaching_types.cpp
    src/world_teaching/world_teaching_owner_resolver.cpp
    src/world_teaching/world_teaching_eligibility_service.cpp
    src/world_teaching/world_teaching_loop_bridge.cpp
    src/world_teaching/world_teaching_store_port.cpp
    src/world_teaching/world_teaching_projection_bridge.cpp
    src/world_teaching/teach_command_handler.cpp
    src/world_teaching/npc_basic_action_knowledge_gate.cpp
)

target_include_directories(pathfinder_world_teaching PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_teaching PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_knowledge
    pathfinder_learning
    pathfinder_memory
    pathfinder_cognition
    pathfinder_condition
    pathfinder_content
)

# World Agent Execution library (P51)
add_library(pathfinder_world_agent_execution STATIC
    src/world_agent_execution/world_agent_execution_types.cpp
    src/world_agent_execution/world_agent_execution_context_store.cpp
    src/world_agent_execution/world_agent_context_builder.cpp
    src/world_agent_execution/world_agent_goal_selector.cpp
    src/world_agent_execution/world_agent_reasoning_bridge.cpp
    src/world_agent_execution/agent_action_knowledge_guard.cpp
    src/world_agent_execution/agent_plan_command_compiler.cpp
    src/world_agent_execution/agent_execution_coordinator.cpp
    src/world_agent_execution/world_agent_projection_bridge.cpp
)

target_include_directories(pathfinder_world_agent_execution PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_agent_execution PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
    pathfinder_knowledge
    pathfinder_world_teaching
    pathfinder_agent_reasoning
    pathfinder_goal_execution
)

# World Region State library (P59)
add_library(pathfinder_world_region_state STATIC
    src/world_region_state/world_region_state_types.cpp
    src/world_region_state/in_memory_world_region_state_store.cpp
    src/world_region_state/world_region_snapshot_builder.cpp
    src/world_region_state/world_region_snapshot_restorer.cpp
    src/world_region_state/world_grid_runtime_apply_port.cpp
    src/world_region_state/world_region_unload_guard.cpp
    src/world_region_state/world_region_lifecycle_service.cpp
    src/world_region_state/world_region_save_bridge.cpp
    src/world_region_state/world_region_ensure_service_adapter.cpp
)

target_include_directories(pathfinder_world_region_state PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_region_state PUBLIC
    pathfinder_foundation
    pathfinder_world_runtime
    pathfinder_world_generation
    pathfinder_world_inventory
)

# World Beast Ecology library (P52)
add_library(pathfinder_world_beast_ecology STATIC
    src/world_beast_ecology/world_beast_ecology_types.cpp
    src/world_beast_ecology/beast_perception_builder.cpp
    src/world_beast_ecology/beast_instinct_gate.cpp
    src/world_beast_ecology/beast_decision_policy.cpp
    src/world_beast_ecology/beast_command_compiler.cpp
    src/world_beast_ecology/beast_interrupt_projector.cpp
    src/world_beast_ecology/beast_projection_bridge.cpp
    src/world_beast_ecology/beast_ecology_coordinator.cpp
)

target_include_directories(pathfinder_world_beast_ecology PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_beast_ecology PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_knowledge
    pathfinder_goal_execution
)

# World Learning library (P49)
add_library(pathfinder_world_learning STATIC
    src/world_learning/world_learning_types.cpp
    src/world_learning/world_experience_extractor.cpp
    src/world_learning/world_knowledge_learning_service.cpp
    src/world_learning/world_knowledge_projection_bridge.cpp
    src/world_learning/world_knowledge_store_port.cpp
    src/world_learning/world_knowledge_template_resolver.cpp
    src/world_learning/world_learning_loop_bridge.cpp
    src/world_learning/world_safe_feedback_builder.cpp
)

target_include_directories(pathfinder_world_learning PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_learning PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_content
    pathfinder_knowledge
    pathfinder_learning
    pathfinder_memory
    pathfinder_cognition
    pathfinder_condition
)

# Camp Runtime library (P66)
add_library(pathfinder_camp STATIC
    src/camp/camp_content.cpp
    src/camp/camp_runtime.cpp
    src/camp/register_camp_core_rules.cpp
    src/camp/rules/wait_tick_rule.cpp
    src/camp/rules/player_resource_rule.cpp
    src/camp/rules/knowledge_card_rule.cpp
    src/camp/rules/npc_task_rule.cpp
    src/camp/rules/scholar_event_rule.cpp
)

target_include_directories(pathfinder_camp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

get_filename_component(PATHFINDER_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
target_compile_definitions(pathfinder_camp PRIVATE
    PATHFINDER_CONTENT_ROOT="${PATHFINDER_REPO_ROOT}/content"
)

target_link_libraries(pathfinder_camp PUBLIC
    pathfinder_foundation
    pathfinder_world_command
)

# Client Runtime Bridge library (P56)
add_library(pathfinder_client_runtime_bridge STATIC
    src/client_runtime_bridge/client_runtime_bridge_types.cpp
    src/client_runtime_bridge/client_runtime_projection_bridge.cpp
    src/client_runtime_bridge/client_runtime_command_option_bridge.cpp
    src/client_runtime_bridge/client_world_region_ensure_adapter.cpp
)

target_include_directories(pathfinder_client_runtime_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_client_runtime_bridge PUBLIC
    pathfinder_foundation
    pathfinder_world_runtime
    pathfinder_world_command
    pathfinder_world_generation
    pathfinder_world_region_state
    pathfinder_world_harvest
    pathfinder_world_inventory
    pathfinder_world_reaction
)

# World Map Interaction library (P60)
add_library(pathfinder_world_map_interaction STATIC
    src/world_map_interaction/world_map_interaction_types.cpp
    src/world_map_interaction/region_activity_window_service.cpp
    src/world_map_interaction/region_lifecycle_trigger_service.cpp
    src/world_map_interaction/client_map_selection_service.cpp
    src/world_map_interaction/client_map_projection_adapter.cpp
)

target_include_directories(pathfinder_world_map_interaction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

target_link_libraries(pathfinder_world_map_interaction PUBLIC
    pathfinder_foundation
    pathfinder_world_runtime
    pathfinder_world_generation
    pathfinder_world_region_state
    pathfinder_world_inventory
    pathfinder_world_harvest
    pathfinder_client_protocol
)

# Client Protocol library (P53)
add_library(pathfinder_client_protocol STATIC
    src/client_protocol/client_protocol_types.cpp
    src/client_protocol/client_protocol_codec.cpp
    src/client_protocol/client_projection_adapter.cpp
    src/client_protocol/client_patch_contract.cpp
    src/client_protocol/client_available_command_adapter.cpp
    src/client_protocol/client_session_gateway.cpp
    src/client_protocol/client_command_gateway.cpp
    src/client_protocol/client_protocol_harness.cpp
)

target_include_directories(pathfinder_client_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

target_link_libraries(pathfinder_client_protocol PUBLIC
    pathfinder_foundation
    pathfinder_world_command
    pathfinder_client_runtime_bridge
    pathfinder_world_map_interaction
)

# World Modules library (P64)
add_library(pathfinder_world_modules STATIC
    src/world_modules/core/world_module_utils.cpp
    src/world_modules/core/world_module_context.cpp
    src/world_modules/core/world_use_command_router.cpp
    src/world_modules/knowledge/actor_knowledge_inspect_module.cpp
    src/world_modules/bootstrap/default_playable_world_bootstrap.cpp
    src/world_modules/core/default_world_modules.cpp
    src/world_beast_ecology/beast_runtime_module.cpp
    src/world_modules/follow/world_follow_module.cpp
    src/world_modules/npc_work/world_npc_work_module.cpp
)

target_include_directories(pathfinder_world_modules PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_modules PUBLIC
    pathfinder_foundation
    pathfinder_client_protocol
    pathfinder_content
    pathfinder_knowledge
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_command_runtime
    pathfinder_world_generation
    pathfinder_world_inventory
    pathfinder_world_inventory_command
    pathfinder_world_harvest
    pathfinder_world_reaction
    pathfinder_world_teaching
    pathfinder_world_beast_ecology
)

# Client HTTP library (P54)
add_library(pathfinder_client_http STATIC
    src/client_http/client_http_types.cpp
    src/client_http/client_static_file_service.cpp
    src/client_http/client_http_gateway.cpp
    src/client_http/client_http_router_impl.cpp
    src/client_http/client_socket_http_transport.cpp
    src/client_http/client_http_server.cpp
)

target_include_directories(pathfinder_client_http PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

target_link_libraries(pathfinder_client_http PUBLIC
    pathfinder_foundation
    pathfinder_client_protocol
    pathfinder_world_command
)

# Client Runtime Host library (P64)
add_library(pathfinder_client_runtime_host STATIC
    src/client_runtime_host/client_runtime_host_factory.cpp
)

target_include_directories(pathfinder_client_runtime_host PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann
)

target_link_libraries(pathfinder_client_runtime_host PUBLIC
    pathfinder_foundation
    pathfinder_client_protocol
    pathfinder_client_http
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_command_runtime
    pathfinder_world_generation
    pathfinder_world_map_interaction
    pathfinder_world_inventory_command
    pathfinder_world_reaction
    pathfinder_world_learning
    pathfinder_world_teaching
    pathfinder_world_beast_ecology
    pathfinder_world_modules
    pathfinder_content
)

# Client Server executable (P54)
add_executable(pathfinder_client_server_exe
    apps/client_server_main.cpp
)

set_target_properties(pathfinder_client_server_exe PROPERTIES
    OUTPUT_NAME pathfinder_client_server
)

target_link_libraries(pathfinder_client_server_exe PRIVATE
    pathfinder_client_runtime_host
    pathfinder_client_http
    pathfinder_client_protocol
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_command_runtime
    pathfinder_world_generation
    pathfinder_world_inventory_command
)

# World Inventory Command library (P45 runtime-aware inventory handlers)
add_library(pathfinder_world_inventory_command STATIC
    src/world_inventory/world_inventory_command_handlers.cpp
)

target_include_directories(pathfinder_world_inventory_command PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_world_inventory_command PUBLIC
    pathfinder_world_command
    pathfinder_world_runtime
    pathfinder_world_inventory
)

# Tests
add_subdirectory(tests)
