#include "JunctionPoint.h"
#include <windows.h>
#include <cstring>
#include <vector>

namespace IceClean::Utils {

// Mount Point 重解析数据（平铺布局，与 NTFS FSCTL_SET_REPARSE_POINT 规范一致）
struct MOUNTPOINT_REPARSE {
    DWORD ReparseTag;
    WORD  ReparseDataLength;
    WORD  Reserved;
    WORD  SubstituteNameOffset;
    WORD  SubstituteNameLength;
    WORD  PrintNameOffset;
    WORD  PrintNameLength;
    WCHAR PathBuffer[1];
};

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

namespace {

constexpr DWORD kGenericHeaderSize = sizeof(DWORD) + sizeof(WORD) + sizeof(WORD); // Tag+DataLen+Reserved = 8

HANDLE OpenReparseHandle(const std::wstring& path, DWORD access) {
    return CreateFileW(path.c_str(), access,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       nullptr, OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                       nullptr);
}

} // namespace

bool JunctionPoint::Create(const std::wstring& junctionPath, const std::wstring& targetPath) {
    // 目标必须是已存在的目录
    DWORD attrs = GetFileAttributesW(targetPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    // 创建目录（不存在时）
    if (!CreateDirectoryW(junctionPath.c_str(), nullptr)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            return false;
        }
    }

    HANDLE hDir = OpenReparseHandle(junctionPath, GENERIC_WRITE);
    if (hDir == INVALID_HANDLE_VALUE) {
        RemoveDirectoryW(junctionPath.c_str());
        return false;
    }

    // Substitute 名称需 \??\ 前缀；两者均以反斜杠结尾、null 终止
    std::wstring substitute = L"\\??\\" + targetPath;
    if (substitute.back() != L'\\') substitute += L'\\';
    std::wstring print = targetPath;
    if (print.back() != L'\\') print += L'\\';

    const WORD subBytes  = static_cast<WORD>(substitute.size() * sizeof(WCHAR)); // 不含 null
    const WORD printBytes = static_cast<WORD>(print.size() * sizeof(WCHAR));     // 不含 null

    // 布局：[sub][null][print][null]
    const WORD subOffset  = 0;
    const WORD printOffset = static_cast<WORD>(subBytes + sizeof(WCHAR));
    const WORD dataLength  = static_cast<WORD>(subBytes + sizeof(WCHAR) +
                                               printBytes + sizeof(WCHAR) +
                                               sizeof(WORD) * 4);

    const DWORD inputSize = kGenericHeaderSize + dataLength;
    std::vector<BYTE> buffer(inputSize + 16, 0);
    auto* rp = reinterpret_cast<MOUNTPOINT_REPARSE*>(buffer.data());

    rp->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    rp->ReparseDataLength = dataLength;
    rp->Reserved = 0;
    rp->SubstituteNameOffset = subOffset;
    rp->SubstituteNameLength = subBytes;
    rp->PrintNameOffset = printOffset;
    rp->PrintNameLength = printBytes;

    memcpy(rp->PathBuffer, substitute.c_str(), subBytes + sizeof(WCHAR));
    memcpy(reinterpret_cast<BYTE*>(rp->PathBuffer) + subBytes + sizeof(WCHAR),
           print.c_str(), printBytes + sizeof(WCHAR));

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(hDir, FSCTL_SET_REPARSE_POINT, rp, inputSize,
                              nullptr, 0, &bytesReturned, nullptr);
    CloseHandle(hDir);

    if (!ok) {
        RemoveDirectoryW(junctionPath.c_str());
        return false;
    }
    return true;
}

bool JunctionPoint::Remove(const std::wstring& junctionPath) {
    HANDLE hDir = OpenReparseHandle(junctionPath, GENERIC_WRITE);
    if (hDir == INVALID_HANDLE_VALUE) return false;

    MOUNTPOINT_REPARSE rp{};
    rp.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    rp.ReparseDataLength = 0;
    rp.Reserved = 0;

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(hDir, FSCTL_DELETE_REPARSE_POINT, &rp,
                              kGenericHeaderSize, nullptr, 0, &bytesReturned, nullptr);
    CloseHandle(hDir);

    if (!ok) return false;
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
    HANDLE hDir = OpenReparseHandle(junctionPath, GENERIC_READ);
    if (hDir == INVALID_HANDLE_VALUE) return L"";

    std::vector<BYTE> buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE, 0);
    auto* rp = reinterpret_cast<MOUNTPOINT_REPARSE*>(buffer.data());

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(hDir, FSCTL_GET_REPARSE_POINT, nullptr, 0,
                              rp, static_cast<DWORD>(buffer.size()),
                              &bytesReturned, nullptr);
    CloseHandle(hDir);

    if (!ok || rp->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
        return L"";
    }

    std::wstring substitute(
        rp->PathBuffer + rp->SubstituteNameOffset / sizeof(WCHAR),
        rp->SubstituteNameLength / sizeof(WCHAR));

    // 去掉\??\前缀
    if (substitute.starts_with(L"\\??\\")) {
        substitute = substitute.substr(4);
    }

    // 去掉末尾的反斜杠
    while (substitute.size() > 3 && substitute.back() == L'\\') {
        substitute.pop_back();
    }

    return substitute;
}

} // namespace IceClean::Utils
