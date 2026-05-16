# config module dev notes

## P1 任务记录

### TASK-P1-001: 创建 config 目录与 CMake 目标

状态: 完成

创建内容:
- backend/include/pathfinder/config/
- backend/src/config/
- backend/tests/unit/config/

验证:
- cmake -S backend -B build/backend 通过
- cmake --build build/backend 通过
- ctest 全部 20 个测试通过 (10 foundation + 4 config + 6 command)

### TASK-P1-002: 实现 ConfigValidationIssue / ConfigValidationReport

状态: 完成

实现内容:
- ConfigValidationSeverity: info / warning / error / fatal
- ConfigValidationIssue: severity, code, message, file_path, json_path, line, column
- ConfigValidationReport: addIssue, hasErrors, hasFatal, issueCount, issues
- toString(ConfigValidationSeverity)

测试:
- 空 report 没有错误
- warning 不算错误
- error 算错误
- fatal 同时算 fatal 和 error
- issueCount 正确
- 字段保持正确

### TASK-P1-003: 实现 ConfigPackage 元信息

状态: 完成

实现内容:
- ConfigPackageId: 基于 StrongId<ConfigPackIdTag>
- ConfigVersion: 字符串包装
- SchemaVersion: 字符串包装
- ConfigFileEntry: path, category, required
- ConfigPackage: package_id, config_version, schema_version, files, locale_files, test_files, checksum
- validateBasic(): 检查 ID 非空、版本非空、schema 非空、文件路径非空、必需文件列表非空

测试:
- 合法 package 通过基础校验
- 缺 package_id 失败
- 缺 config_version 失败
- 缺 schema_version 失败
- 空文件路径失败
- 没有必需配置文件失败

### TASK-P1-004: 实现 ConfigLoader 文件读取空壳

状态: 完成

实现内容:
- LoadedConfigFile: path, content, content_hash
- ConfigLoadRequest: root_path, relative_files
- ConfigLoadResult: files, validation_report
- ConfigLoader::loadFiles(request): 只读取文件文本、计算内容 hash、基础文件校验
- isPathSafe(): 拒绝路径穿越和绝对路径

测试:
- 读取存在文件成功
- 读取不存在文件返回 error issue
- 空文件返回 warning
- 相同内容 hash 稳定
- 路径穿越 ../ 拒绝
- 绝对路径拒绝
