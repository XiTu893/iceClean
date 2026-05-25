#include "RestorePointManager.h"
#include <windows.h>
#include <srrestoreptapi.h>

#pragma comment(lib, "SrClient.lib")

namespace IceClean::Core::Safety {

bool RestorePointManager::CreateRestorePoint(const std::wstring& description) {
    RESTOREPOINTINFOW rpInfo = {};
    rpInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
    rpInfo.dwRestorePtType = APPLICATION_UNINSTALL;
    rpInfo.llSequenceNumber = 0;

    // 截断描述到MAX_DESC长度
    wcsncpy_s(rpInfo.szDescription, description.c_str(), _TRUNCATE);

    STATEMGRSTATUS mgrStatus = {};

    // 开始还原点
    if (!SRSetRestorePointW(&rpInfo, &mgrStatus)) {
        return false;
    }

    // 保存序列号，用于结束还原点
    rpInfo.llSequenceNumber = mgrStatus.llSequenceNumber;
    rpInfo.dwEventType = END_SYSTEM_CHANGE;

    // 结束还原点
    if (!SRSetRestorePointW(&rpInfo, &mgrStatus)) {
        return false;
    }

    return true;
}

bool RestorePointManager::IsSystemRestoreAvailable() {
    // 尝试创建一个临时还原点来测试系统还原是否可用
    // 使用 MODIFY_SETTINGS 类型，这样不会创建真正的还原点
    RESTOREPOINTINFOW rpInfo = {};
    rpInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
    rpInfo.dwRestorePtType = MODIFY_SETTINGS;
    rpInfo.llSequenceNumber = 0;
    wcscpy_s(rpInfo.szDescription, L"IceClean Availability Check");

    STATEMGRSTATUS mgrStatus = {};

    if (!SRSetRestorePointW(&rpInfo, &mgrStatus)) {
        return false;
    }

    // 取消还原点
    rpInfo.llSequenceNumber = mgrStatus.llSequenceNumber;
    rpInfo.dwEventType = END_SYSTEM_CHANGE;
    rpInfo.dwRestorePtType = CANCELLED_OPERATION;

    SRSetRestorePointW(&rpInfo, &mgrStatus);
    return true;
}

} // namespace IceClean::Core::Safety
