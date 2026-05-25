#include "OperationLogger.h"
#include "utils/JsonUtil.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace IceClean::Core::Safety {

std::wstring OperationLogger::GetLogFilePath() {
    std::wstring logDir = Utils::JsonUtil::GetLogPath();
    if (logDir.empty()) return L"";
    return logDir + L"\\operations.json";
}

static std::string OperationTypeToString(Models::OperationType type) {
    switch (type) {
        case Models::OperationType::Clean:    return "Clean";
        case Models::OperationType::Migrate:  return "Migrate";
        case Models::OperationType::Optimize: return "Optimize";
        case Models::OperationType::Restore:  return "Restore";
        default: return "Unknown";
    }
}

static Models::OperationType StringToOperationType(const std::string& str) {
    if (str == "Clean")    return Models::OperationType::Clean;
    if (str == "Migrate")  return Models::OperationType::Migrate;
    if (str == "Optimize") return Models::OperationType::Optimize;
    if (str == "Restore")  return Models::OperationType::Restore;
    return Models::OperationType::Clean;
}

void OperationLogger::LogOperation(const Models::OperationRecord& record) {
    std::wstring logPath = GetLogFilePath();
    if (logPath.empty()) return;

    // 加载现有日志
    nlohmann::json logData = Utils::JsonUtil::LoadJson(logPath);
    if (!logData.is_array()) {
        logData = nlohmann::json::array();
    }

    // 创建新的日志条目
    nlohmann::json entry;
    entry["type"] = OperationTypeToString(record.type);

    // 转换 wstring 到 UTF-8 string
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    entry["description"] = converter.to_bytes(record.description);
    entry["size"] = record.size;

    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        record.timestamp.time_since_epoch()).count();
    entry["timestamp"] = timestamp;
    entry["success"] = record.success;
    entry["details"] = converter.to_bytes(record.details);

    logData.push_back(entry);

    // 保存日志
    Utils::JsonUtil::SaveJson(logPath, logData);
}

std::vector<Models::OperationRecord> OperationLogger::GetRecentOperations(int count) {
    std::vector<Models::OperationRecord> records;

    std::wstring logPath = GetLogFilePath();
    if (logPath.empty()) return records;

    nlohmann::json logData = Utils::JsonUtil::LoadJson(logPath);
    if (!logData.is_array()) return records;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    // 从最新的记录开始读取
    int startIdx = std::max(0, static_cast<int>(logData.size()) - count);
    for (int i = startIdx; i < static_cast<int>(logData.size()); ++i) {
        const auto& entry = logData[i];

        Models::OperationRecord record;
        record.type = StringToOperationType(entry.value("type", "Clean"));
        record.description = converter.from_bytes(entry.value("description", ""));
        record.size = entry.value("size", (uint64_t)0);
        record.success = entry.value("success", false);
        record.details = converter.from_bytes(entry.value("details", ""));

        int64_t timestamp = entry.value("timestamp", (int64_t)0);
        record.timestamp = std::chrono::system_clock::time_point(
            std::chrono::seconds(timestamp));

        records.push_back(record);
    }

    return records;
}

bool OperationLogger::ClearLog() {
    std::wstring logPath = GetLogFilePath();
    if (logPath.empty()) return false;

    // 写入空数组
    nlohmann::json emptyArray = nlohmann::json::array();
    return Utils::JsonUtil::SaveJson(logPath, emptyArray);
}

} // namespace IceClean::Core::Safety
