#pragma once

namespace IceClean::Core::Safety {

// 内置推荐软件种子数据（UTF-8 JSON，与仓库 docs/recommended_software.json 保持同步）
// 用途：离线首启导入 / 本地数据版本落后于种子版本时自动升级；联网后会被 GitHub 版本覆盖
const char* GetSeedJsonUtf8();

// 种子数据版本号（对应 JSON 的 version 字段；解析失败返回 0）
int GetSeedJsonVersion();

} // namespace IceClean::Core::Safety
