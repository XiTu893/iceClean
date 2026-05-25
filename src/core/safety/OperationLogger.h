#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "models/OperationRecord.h"

namespace IceClean::Core::Safety {

class OperationLogger {
public:
    // 记录操作
    static void LogOperation(const Models::OperationRecord& record);

    // 获取最近的操作记录
    static std::vector<Models::OperationRecord> GetRecentOperations(int count = 50);

    // 清空操作日志
    static bool ClearLog();

private:
    // 获取日志文件路径
    static std::wstring GetLogFilePath();

    // 将操作记录序列化为JSON字符串
    static std::wstring SerializeRecord(const Models::OperationRecord& record);

    // 将JSON字符串反序列化为操作记录
    static Models::OperationRecord DeserializeRecord(const std::wstring& json);
};

} // namespace IceClean::Core::Safety
