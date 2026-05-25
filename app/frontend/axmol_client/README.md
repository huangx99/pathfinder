# Pathfinder Axmol Client

沙盒客户端。前端只负责输入与展示，玩法状态由后端正式引擎管线处理：`ClientSessionGateway` / `ClientCommandGateway` / `WorldCommandPipeline`。

本地单机版不走网络、不走 HTTP JSON，也不能直接绕过 `available_commands` 修改玩法状态。
