#include "pathfinder/h5_playable/playable_turn_service.h"

namespace pathfinder::h5_playable {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using namespace pathfinder::h5_projection;

namespace {

PlayableFeedbackTone toneFromDialog(const pathfinder::h5_dialog::DialogResponseDto& response) {
    if (response.decision == pathfinder::h5_dialog::DialogTurnDecision::Rejected ||
        response.decision == pathfinder::h5_dialog::DialogTurnDecision::Failed) return PlayableFeedbackTone::Warning;
    if (response.reply_text.find("不适") != std::string::npos || response.reply_text.find("危险") != std::string::npos) return PlayableFeedbackTone::Danger;
    if (response.reply_text.find("形成") != std::string::npos || response.reply_text.find("缓解") != std::string::npos || response.reply_text.find("同伴听取") != std::string::npos) return PlayableFeedbackTone::Positive;
    return PlayableFeedbackTone::Neutral;
}

pathfinder::h5_dialog::DialogResponseDto bootstrapDialogResponse(const std::string& session_id, uint64_t turn_index) {
    pathfinder::h5_dialog::DialogResponseDto response;
    response.session_id = session_id;
    response.decision = pathfinder::h5_dialog::DialogTurnDecision::RepliedOnly;
    response.reply_text = "你醒来了。周围的一切都还等待你理解。";
    response.state_summary_lines.push_back("回合：" + std::to_string(turn_index));
    response.state_summary_lines.push_back("当前地点：林地边缘");
    return response;
}

} // namespace

Result<H5PlayableResponse> H5PlayableTurnService::bootstrap(const H5PlayableBootstrapRequest& request) {
    auto request_result = request.validateBasic();
    if (request_result.is_error()) return Result<H5PlayableResponse>::fail(request_result.errors());
    if (request.reset) {
        auto reset_result = dialog_service_.resetSession(request.session_id);
        if (reset_result.is_error()) return Result<H5PlayableResponse>::fail(reset_result.errors());
    }
    auto state_result = dialog_service_.loadOrCreateSessionSnapshot(request.session_id);
    if (state_result.is_error()) return Result<H5PlayableResponse>::fail(state_result.errors());
    auto state = state_result.value();
    auto dialog_response = bootstrapDialogResponse(request.session_id, state.turn_index);
    return buildResponse(dialog_response, state, PlayableFeedbackTone::System, request.reset ? "reset" : "bootstrap");
}

Result<H5PlayableResponse> H5PlayableTurnService::handleTurn(const H5PlayableRequest& request) {
    auto request_result = request.validateBasic();
    if (request_result.is_error()) return Result<H5PlayableResponse>::fail(request_result.errors());

    if (request.input_kind == PlayableInputKind::Reset) {
        H5PlayableBootstrapRequest bootstrap_request;
        bootstrap_request.session_id = request.session_id;
        bootstrap_request.reset = true;
        return bootstrap(bootstrap_request);
    }

    pathfinder::h5_dialog::DialogRequestDto dialog_request;
    dialog_request.session_id = request.session_id;
    dialog_request.client_turn = request.client_turn;
    dialog_request.input_text = request.input_text;
    if (dialog_request.input_text.empty() && request.action_key.has_value()) {
        dialog_request.input_text = *request.action_key;
    }

    auto detailed_result = dialog_service_.handleDetailed(dialog_request);
    if (detailed_result.is_error()) return Result<H5PlayableResponse>::fail(detailed_result.errors());
    const auto detailed = detailed_result.value();
    return buildResponse(detailed.response, detailed.state, toneFromDialog(detailed.response), "turn_" + std::to_string(request.client_turn));
}

Result<H5PlayableResponse> H5PlayableTurnService::buildResponse(
    const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
    const pathfinder::h5_dialog::DialogSessionState& session_state,
    PlayableFeedbackTone tone,
    const std::string& request_key) const {
    auto source_result = mapper_.buildSourceBundle(dialog_response, session_state);
    if (source_result.is_error()) return Result<H5PlayableResponse>::fail(source_result.errors());

    H5ProjectionRequest projection_request;
    projection_request.request_id = request_key + "_" + session_state.session_id;
    projection_request.session_id = session_state.session_id;
    projection_request.audience = ProjectionAudience::Player;
    projection_request.mode = ProjectionMode::Full;
    projection_request.max_items_per_section = 64;
    projection_request.language = "zh_cn";

    auto projection_result = composer_.compose(projection_request, source_result.value());
    if (projection_result.is_error()) return Result<H5PlayableResponse>::fail(projection_result.errors());

    H5PlayableResponse response;
    response.ok = dialog_response.decision != pathfinder::h5_dialog::DialogTurnDecision::Failed;
    response.session_id = session_state.session_id;
    response.server_turn = session_state.turn_index;
    response.tone = tone;
    response.reply_text = makeSafeText("playable.reply." + request_key, SafeTextKind::Feedback, playableSafeReplyText(dialog_response));
    response.projection = std::move(projection_result.value());
    response.issues = response.projection.issues;

    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<H5PlayableResponse>::fail(valid.errors());
    return Result<H5PlayableResponse>::ok(std::move(response));
}

} // namespace pathfinder::h5_playable
