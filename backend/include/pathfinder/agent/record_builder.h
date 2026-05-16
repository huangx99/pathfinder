#pragma once

#include "pathfinder/agent/record_types.h"

namespace pathfinder::agent {

// AgentRecordBuilder: 从请求和结果构建记录，不执行调度、不触碰状态
class AgentRecordBuilder {
public:
    pathfinder::foundation::Result<AgentTickRecord> build(
        const AgentRecordBuildRequest& request) const;
};

} // namespace pathfinder::agent
