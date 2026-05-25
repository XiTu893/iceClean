#include "RegistryUtil.h"

namespace IceClean::Utils {

std::wstring RegistryUtil::ReadStringValue(HKEY rootKey, const std::wstring& subKey,
                                           const std::wstring& valueName) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return L"";

    DWORD dataSize = 0;
    result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, nullptr, &dataSize);
    if (result != ERROR_SUCCESS || dataSize == 0) {
        RegCloseKey(hKey);
        return L"";
    }

    // dataSize包含null终止符
    std::wstring value(dataSize / sizeof(wchar_t), L'\0');
    result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr,
                              reinterpret_cast<LPBYTE>(value.data()), &dataSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) return L"";

    // 移除末尾的null终止符
    while (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }

    return value;
}

DWORD RegistryUtil::ReadDwordValue(HKEY rootKey, const std::wstring& subKey,
                                   const std::wstring& valueName) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return 0;

    DWORD value = 0;
    DWORD dataSize = sizeof(DWORD);
    DWORD type = 0;
    result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type,
                              reinterpret_cast<LPBYTE>(&value), &dataSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || type != REG_DWORD) return 0;

    return value;
}

bool RegistryUtil::WriteStringValue(HKEY rootKey, const std::wstring& subKey,
                                    const std::wstring& valueName, const std::wstring& value) {
    HKEY hKey = nullptr;
    LONG result = RegCreateKeyExW(rootKey, subKey.c_str(), 0, nullptr,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;

    // 包含null终止符的大小
    DWORD dataSize = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(value.c_str()), dataSize);
    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

std::vector<std::wstring> RegistryUtil::EnumSubKeys(HKEY rootKey, const std::wstring& subKey) {
    std::vector<std::wstring> subKeys;

    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return subKeys;

    DWORD index = 0;
    wchar_t keyName[MAX_PATH] = {};

    while (true) {
        DWORD nameSize = MAX_PATH;
        result = RegEnumKeyExW(hKey, index, keyName, &nameSize, nullptr, nullptr, nullptr, nullptr);
        if (result != ERROR_SUCCESS) break;

        subKeys.push_back(keyName);
        ++index;
    }

    RegCloseKey(hKey);
    return subKeys;
}

std::vector<std::wstring> RegistryUtil::EnumValues(HKEY rootKey, const std::wstring& subKey) {
    std::vector<std::wstring> values;

    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return values;

    DWORD index = 0;
    wchar_t valueName[MAX_PATH] = {};

    while (true) {
        DWORD nameSize = MAX_PATH;
        result = RegEnumValueW(hKey, index, valueName, &nameSize, nullptr, nullptr, nullptr);
        if (result != ERROR_SUCCESS) break;

        values.push_back(valueName);
        ++index;
    }

    RegCloseKey(hKey);
    return values;
}

bool RegistryUtil::DeleteValue(HKEY rootKey, const std::wstring& subKey,
                               const std::wstring& valueName) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return false;

    result = RegDeleteValueW(hKey, valueName.c_str());
    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

bool RegistryUtil::KeyExists(HKEY rootKey, const std::wstring& subKey) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

} // namespace IceClean::Utils
