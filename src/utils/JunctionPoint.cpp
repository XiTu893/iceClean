#include "JunctionPoint.h"
#include <windows.h>
#include <cstring>
#include <vector>

namespace IceClean::Utils {

// REPARSE_POINT结构定义(用于Junction)
#pragma pack(push, 1)
struct JUNCTION_REPARSE_DATA_BUFFER {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
    } u;
};
#pragma pack(pop)

#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT (0xA0000003L)
#endif

#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef FSCTL_GET_REPARSE_POINT
#define FSCTL_GET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef FSCTL_DELETE_REPARSE_POINT
#define FSCTL_DELETE_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

bool JunctionPoint::Create(const std::wstring& junctionPath, const std::wstring& targetPath) {
    // 确保目标路径存在
    DWORD attrs = GetFileAttributesW(targetPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    // 创建目录(如果不存在)
    if (!CreateDirectoryW(junctionPath.c_str(), nullptr)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }
    }

    // 打开目录获取句柄
    HANDLE hDir = CreateFileW(
        junctionPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        RemoveDirectoryW(junctionPath.c_str());
        return false;
    }

    // 准备目标路径(Junction需要\??\前缀)
    std::wstring nativeTarget = L"\\??\\" + targetPath;
    if (nativeTarget.back() != L'\\') {
        nativeTarget += L'\\';
    }

    // 准备显示路径
    std::wstring printName = targetPath;
    if (printName.back() != L'\\') {
        printName += L'\\';
    }

    // 计算所需缓冲区大小
    USHORT substituteNameLength = static_cast<USHORT>(nativeTarget.size() * sizeof(WCHAR));
    USHORT printNameLength = static_cast<USHORT>(printName.size() * sizeof(WCHAR));
    USHORT reparseDataLength = sizeof(ULONG) + sizeof(USHORT) * 5 +
                               substituteNameLength + printNameLength;

    std::vector<BYTE> buffer(sizeof(JUNCTION_REPARSE_DATA_BUFFER) + substituteNameLength + printNameLength, 0);
    auto* reparseData = reinterpret_cast<JUNCTION_REPARSE_DATA_BUFFER*>(buffer.data());

    reparseData->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    reparseData->ReparseDataLength = reparseDataLength;
    reparseData->Reserved = 0;

    auto& mountPoint = reparseData->u.MountPointReparseBuffer;
    mountPoint.SubstituteNameOffset = 0;
    mountPoint.SubstituteNameLength = substituteNameLength;
    mountPoint.PrintNameOffset = substituteNameLength;
    mountPoint.PrintNameLength = printNameLength;

    std::memcpy(mountPoint.PathBuffer, nativeTarget.c_str(), substituteNameLength);
    std::memcpy(reinterpret_cast<BYTE*>(mountPoint.PathBuffer) + substituteNameLength,
                printName.c_str(), printNameLength);

    // 设置重解析点
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hDir,
        FSCTL_SET_REPARSE_POINT,
        reparseData,
        static_cast<DWORD>(sizeof(ULONG) * 2 + sizeof(USHORT) + reparseDataLength),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    CloseHandle(hDir);

    if (!success) {
        RemoveDirectoryW(junctionPath.c_str());
        return false;
    }

    return true;
}

bool JunctionPoint::Remove(const std::wstring& junctionPath) {
    // 打开Junction点
    HANDLE hDir = CreateFileW(
        junctionPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr
    );

    if (hDir == INVALID_HANDLE_VALUE) return false;

    // 准备删除重解析点的缓冲区
    JUNCTION_REPARSE_DATA_BUFFER reparseData = {};
    reparseData.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    reparseData.ReparseDataLength = 0;

    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hDir,
        FSCTL_DELETE_REPARSE_POINT,
        &reparseData,
        REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    CloseHandle(hDir);

    if (!success) return false;

    // 删除空目录
    return RemoveDirectoryW(junctionPath.c_str()) != 0;
}

bool JunctionPoint::IsJunction(const std::wstring& path) {
    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(path.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    FindClose(hFind);

    return (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 &&
           findData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT;
}

std::wstring JunctionPoint::GetTarget(const std::wstring& junctionPath) {
    HANDLE hDir = CreateFileW(
        junctionPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr
    );

    if (hDir == INVALID_HANDLE_VALUE) return L"";

    // 分配足够大的缓冲区
    std::vector<BYTE> buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 0);
    auto* reparseData = reinterpret_cast<JUNCTION_REPARSE_DATA_BUFFER*>(buffer.data());

    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hDir,
        FSCTL_GET_REPARSE_POINT,
        nullptr,
        0,
        reparseData,
        static_cast<DWORD>(buffer.size()),
        &bytesReturned,
        nullptr
    );

    CloseHandle(hDir);

    if (!success || reparseData->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
        return L"";
    }

    auto& mountPoint = reparseData->u.MountPointReparseBuffer;

    // 读取替代名称(实际路径)
    std::wstring substituteName(
        mountPoint.PathBuffer + mountPoint.SubstituteNameOffset / sizeof(WCHAR),
        mountPoint.SubstituteNameLength / sizeof(WCHAR)
    );

    // 去掉\??\前缀
    if (substituteName.starts_with(L"\\??\\")) {
        substituteName = substituteName.substr(4);
    }

    // 去掉末尾的反斜杠
    while (substituteName.size() > 3 && substituteName.back() == L'\\') {
        substituteName.pop_back();
    }

    return substituteName;
}

} // namespace IceClean::Utils
