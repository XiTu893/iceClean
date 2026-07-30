#pragma once
#include "ScannerBase.h"
#include <vector>
#include <string>

namespace IceClean::Core::Scanner {

class AppDataScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"AppData 深度扫描"; }
    std::wstring GetDescription() const override { return L"扫描用户 AppData 目录下的临时文件、缓存、日志等可清理数据"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Caution; }
    std::wstring GetIcon() const override { return L"appdata"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;

private:
    // 各子扫描方法
    void ScanLocalTemp(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanBrowserCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanElectronCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanPackageManagerCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanIdeCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanLogFiles(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanThumbnailCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanUwpCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanDeliveryOptCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);

    // 扫描指定目录下所有文件（不区分 pattern，全部纳入）
    void ScanAllFiles(const std::wstring& dirPath, Models::ScanCategory& category,
                      const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);

    // Electron 应用路径列表
    static const std::vector<std::pair<std::wstring, std::wstring>>& GetElectronAppPaths();
};

} // namespace IceClean::Core::Scanner
