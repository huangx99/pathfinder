#include "pathfinder/h5_projection/projection.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::h5_projection;
using namespace pathfinder::foundation;

static ActionProjection action(std::string key, ActionAffordanceKind kind, std::string label, bool enabled = true) {
    ActionProjection item;
    item.action_key = std::move(key);
    item.affordance = kind;
    item.label = makeSafeText(item.action_key + ".label", SafeTextKind::DisplayName, std::move(label));
    item.input_text = item.label.zh_cn;
    item.enabled = enabled;
    item.command_preview_key = item.action_key + ".preview";
    return item;
}

static ObjectCardProjection objectCard(std::string ref, std::string name, std::string desc) {
    ObjectCardProjection card;
    card.object_ref_key = std::move(ref);
    card.display_name = makeSafeText(card.object_ref_key + ".name", SafeTextKind::DisplayName, std::move(name));
    card.description = makeSafeText(card.object_ref_key + ".desc", SafeTextKind::Description, std::move(desc));
    card.visibility_key = "visible";
    card.safe_tags = {"visible_object"};
    card.actions.push_back(action(card.object_ref_key + ".try_eat", ActionAffordanceKind::TryEat, "尝试食用"));
    return card;
}

static KnowledgeLineProjection knowledgeLine(std::string id, std::string subject, std::string effect, bool teachable = true) {
    KnowledgeLineProjection line;
    line.knowledge_id = std::move(id);
    line.owner_label = makeSafeText(line.knowledge_id + ".owner", SafeTextKind::DisplayName, "你");
    line.subject_label = makeSafeText(line.knowledge_id + ".subject", SafeTextKind::DisplayName, std::move(subject));
    line.predicate_label = makeSafeText(line.knowledge_id + ".predicate", SafeTextKind::DisplayName, "食用");
    line.effect_summary = makeSafeText(line.knowledge_id + ".effect", SafeTextKind::Description, std::move(effect));
    line.status_key = "active";
    line.confidence_label = makeSafeText(line.knowledge_id + ".confidence", SafeTextKind::Hint, "已确认");
    line.teachable = teachable;
    return line;
}

static ReplayStatusProjection replayStatus() {
    ReplayStatusProjection replay;
    replay.replay_key = "replay_p32_001";
    replay.status_label = makeSafeText("replay.p32.status", SafeTextKind::DebugSummary, "复盘失败");
    replay.issue_summaries.push_back(makeSafeText("replay.p32.issue.event_count", SafeTextKind::DebugSummary, "事件数量与记录不一致"));
    replay.visible_to_player = false;
    return replay;
}

static H5ProjectionSourceBundle sourceBundle() {
    H5ProjectionSourceBundle sources;
    sources.scene_title = makeSafeText("scene.p32.title", SafeTextKind::DisplayName, "先驱者营地");
    sources.scene_summary.push_back(makeSafeText("scene.p32.summary", SafeTextKind::Description, "你正在观察周围能尝试的东西。"));
    sources.object_cards.push_back(objectCard("red_berry", "红果", "一种看起来鲜亮的果实。"));
    sources.object_cards.push_back(objectCard("rotten_red_berry", "腐烂红果", "一种气味不太好的红色果实。"));
    sources.action_bar.push_back(action("teach_companion", ActionAffordanceKind::Teach, "教同伴"));
    sources.actor_knowledge.push_back(knowledgeLine("knowledge_red_berry_eat_restore", "红果", "吃后可以缓解饥饿"));
    sources.recipient_knowledge.push_back(knowledgeLine("knowledge_rotten_red_berry_eat_risk", "腐烂红果", "吃后可能让身体不舒服", false));
    ConditionSummaryProjection condition;
    condition.condition_key = "condition:test:eq:p32_safe";
    condition.summary_text = makeSafeText("condition.p32.safe", SafeTextKind::Hint, "你已经有过相关尝试。");
    condition.blocking = false;
    sources.condition_hints.push_back(condition);
    DangerHintProjection danger;
    danger.danger_key = "danger.rotten_red_berry.risk";
    danger.severity_label = makeSafeText("danger.rotten.severity", SafeTextKind::Warning, "有风险");
    danger.hint_text = makeSafeText("danger.rotten.hint", SafeTextKind::Hint, "你对这个东西有不好的经验。");
    danger.related_object_refs = {"rotten_red_berry"};
    sources.danger_hints.push_back(danger);
    sources.replay_status = replayStatus();
    sources.debug_keys = {"p32_debug_projection"};
    sources.warning_keys = {"p32_projection_ready"};
    return sources;
}

static H5ProjectionRequest request(ProjectionAudience audience = ProjectionAudience::Player) {
    H5ProjectionRequest req;
    req.request_id = "req_p32_001";
    req.session_id = "session_p32_001";
    req.audience = audience;
    req.mode = audience == ProjectionAudience::DeveloperDebug ? ProjectionMode::DebugSnapshot : ProjectionMode::Full;
    req.include_debug_trace = audience == ProjectionAudience::DeveloperDebug;
    return req;
}

static void test_enum_roundtrip() {
    assert(projectionAudienceFromString(toString(ProjectionAudience::Player)).value() == ProjectionAudience::Player);
    assert(projectionModeFromString(toString(ProjectionMode::Full)).value() == ProjectionMode::Full);
    assert(projectionSectionKindFromString(toString(ProjectionSectionKind::DangerHints)).value() == ProjectionSectionKind::DangerHints);
    assert(projectionItemKindFromString(toString(ProjectionItemKind::KnowledgeLine)).value() == ProjectionItemKind::KnowledgeLine);
    assert(actionAffordanceKindFromString(toString(ActionAffordanceKind::TryEat)).value() == ActionAffordanceKind::TryEat);
    assert(projectionBuildStatusFromString(toString(ProjectionBuildStatus::Complete)).value() == ProjectionBuildStatus::Complete);
    assert(safeTextKindFromString(toString(SafeTextKind::DisplayName)).value() == SafeTextKind::DisplayName);
    assert(projectionIssueKindFromString(toString(ProjectionIssueKind::ForbiddenKeyRejected)).value() == ProjectionIssueKind::ForbiddenKeyRejected);
    assert(projectionAudienceFromString("unknown").is_error());
}

static void test_safe_text_forbidden() {
    auto text = makeSafeText("text.hidden_truth", SafeTextKind::Hint, "隐藏真相");
    assert(text.validateBasic().is_error());
    assert(containsProjectionForbiddenKey("raw_state.debug"));
    assert(!containsProjectionForbiddenKey("knowledge.safe.summary"));
}

static void test_player_request_rejects_debug() {
    auto req = request(ProjectionAudience::Player);
    req.include_debug_trace = true;
    assert(req.validateBasic().is_error());
    req.include_debug_trace = false;
    req.requested_sections = {ProjectionSectionKind::DebugPanel};
    assert(req.validateBasic().is_error());
}

static void test_player_projection_filters_debug_and_replay() {
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::Player), sourceBundle());
    assert(result.is_ok());
    const auto& projection = result.value();
    assert(projection.header.audience == ProjectionAudience::Player);
    assert(projection.replay_status.has_value() == false);
    assert(projection.debug_keys.empty());
    assert(!projection.issues.empty());
    assert(projection.object_cards.size() == 2);
    assert(projection.object_cards[0].object_ref_key == "red_berry");
    assert(projection.object_cards[1].object_ref_key == "rotten_red_berry");
}

static void test_debug_projection_keeps_replay() {
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::DeveloperDebug), sourceBundle());
    assert(result.is_ok());
    const auto& projection = result.value();
    assert(projection.header.audience == ProjectionAudience::DeveloperDebug);
    assert(projection.replay_status.has_value());
    assert(!projection.debug_keys.empty());
}

static void test_knowledge_uses_chinese_not_debug_string() {
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::Player), sourceBundle());
    assert(result.is_ok());
    const auto& line = result.value().actor_knowledge.front();
    assert(line.subject_label.zh_cn == "红果");
    assert(line.effect_summary.zh_cn == "吃后可以缓解饥饿");
    assert(line.effect_summary.zh_cn.find("->") == std::string::npos);
}

static void test_action_enabled_is_backend_value() {
    auto sources = sourceBundle();
    sources.object_cards[0].actions[0].enabled = false;
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::Player), sources);
    assert(result.is_ok());
    assert(result.value().object_cards[0].actions[0].enabled == false);
}

static void test_danger_forbidden_rejected() {
    auto sources = sourceBundle();
    sources.danger_hints[0].warning_keys.push_back("hp_delta_hidden");
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::Player), sources);
    assert(result.is_error());
}


static void test_truncation_marks_projection() {
    auto sources = sourceBundle();
    sources.object_cards.push_back(objectCard("bitter_leaf", "苦叶", "一种带苦味的叶子。"));
    auto req = request(ProjectionAudience::Player);
    req.max_items_per_section = 2;
    auto result = H5ProjectionComposer{}.compose(req, sources);
    assert(result.is_ok());
    assert(result.value().object_cards.size() == 2);
    assert(result.value().header.truncated);
}

static void test_script_text_rejected() {
    auto text = makeSafeText("text.script", SafeTextKind::Hint, "<script>alert(1)</script>");
    assert(text.validateBasic().is_error());
}

static void test_player_debug_summary_rejected_deeply() {
    auto sources = sourceBundle();
    sources.object_cards[0].display_name = makeSafeText("debug.object.hidden", SafeTextKind::DebugSummary, "调试专用对象摘要");
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::Player), sources);
    assert(result.is_error());
}

static void test_debug_summary_allowed_for_developer() {
    auto sources = sourceBundle();
    sources.object_cards[0].display_name = makeSafeText("debug.object.summary", SafeTextKind::DebugSummary, "调试专用对象摘要");
    auto result = H5ProjectionComposer{}.compose(request(ProjectionAudience::DeveloperDebug), sources);
    assert(result.is_ok());
}

static void test_protocol_envelope() {
    auto projection = H5ProjectionComposer{}.compose(request(ProjectionAudience::Player), sourceBundle()).value();
    H5ProjectionProtocolOptions options;
    options.request_id = "req_p32_001";
    auto envelope = H5ProtocolProjectionAdapter{}.buildProjectionEnvelope(projection, options);
    assert(envelope.payload.payload_type == pathfinder::protocol::ProtocolPayloadType::H5GameProjection);
    assert(pathfinder::protocol::toString(envelope.payload.payload_type) == "h5_game_projection");
    assert(pathfinder::protocol::protocolPayloadTypeFromString("h5_game_projection").value() == pathfinder::protocol::ProtocolPayloadType::H5GameProjection);
    assert(envelope.validateBasic().is_ok());
}

static void test_replay_report_builder() {
    pathfinder::replay::GoldenReplayReport report;
    report.status = pathfinder::replay::GoldenReplayStatus::Failed;
    report.package_hash = HashValue::fromString("p32_package");
    report.deterministic_signature = HashValue::fromString("p32_signature");
    report.observed_condition_trace_keys = {"condition:test:eq:p32"};
    pathfinder::replay::ReplayDiagnosticIssue issue;
    issue.kind = pathfinder::replay::ReplayDiagnosticIssueKind::ReplayMismatch;
    issue.severity = pathfinder::replay::ReplayDiagnosticSeverity::Error;
    issue.field_path = "replay.compare.status";
    issue.expected = "match";
    issue.actual = "mismatch";
    issue.message_key = "p31.replay_mismatch";
    issue.safe_summary_text = "真实复盘结果与记录不一致";
    report.issues.push_back(issue);
    auto projection = ReplayStatusProjectionBuilder{}.fromGoldenReplayReport(report);
    assert(projection.is_ok());
    assert(projection.value().visible_to_player == false);
    assert(!projection.value().issue_summaries.empty());
    assert(!projection.value().condition_trace_summary.empty());
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string name = argv[1];
    if (name == "enum") test_enum_roundtrip();
    else if (name == "safe_text") test_safe_text_forbidden();
    else if (name == "request") test_player_request_rejects_debug();
    else if (name == "player_filter") test_player_projection_filters_debug_and_replay();
    else if (name == "debug") test_debug_projection_keeps_replay();
    else if (name == "knowledge") test_knowledge_uses_chinese_not_debug_string();
    else if (name == "action") test_action_enabled_is_backend_value();
    else if (name == "danger") test_danger_forbidden_rejected();
    else if (name == "truncation") test_truncation_marks_projection();
    else if (name == "script") test_script_text_rejected();
    else if (name == "player_debug_text") test_player_debug_summary_rejected_deeply();
    else if (name == "developer_debug_text") test_debug_summary_allowed_for_developer();
    else if (name == "protocol") test_protocol_envelope();
    else if (name == "replay") test_replay_report_builder();
    else return 2;
    std::cout << "h5_projection " << name << " passed\n";
    return 0;
}
