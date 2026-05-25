#include "ShellUtil.h"
#include <shlobj.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

namespace IceClean::Utils {

bool ShellUtil::EmptyRecycleBin() {
    // 传入nullptr表示清空所有驱动器上的回收站
    // SHERB_NOCONFIRMATION: 不显示确认对话框
    // SHERB_NOPROGRESSUI: 不显示进度对话框
    // SHERB_NOSOUND: 完成后不播放声音
    HRESULT hr = SHEmptyRecycleBinW(nullptr, nullptr,
        SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);

    // S_FALSE表示回收站已经为空
    return SUCCEEDED(hr);
}

uint64_t ShellUtil::GetRecycleBinSize() {
    SHQUERYRBINFO rbInfo = {};
    rbInfo.cbSize = sizeof(rbInfo);

    // 传入nullptr表示查询所有驱动器上的回收站
    HRESULT hr = SHQueryRecycleBinW(nullptr, &rbInfo);
    if (FAILED(hr)) return 0;

    // i64Size是回收站中所有项目占用的总大小
    ULARGE_INTEGER size;
    size.QuadPart = rbInfo.i64Size;
    return size.QuadPart;
}

bool ShellUtil::OpenInExplorer(const std::wstring& path) {
    // 使用ShellExecute打开资源管理器并选中/定位到路径
    HINSTANCE result = ShellExecuteW(
        nullptr,
        L"explore",
        path.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    // ShellExecute返回值大于32表示成功
    return reinterpret_cast<INT_PTR>(result) > 32;
}

} // namespace IceClean::Utils
