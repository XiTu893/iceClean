#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class DevCacheScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"开发工具缓存"; }
    std::wstring GetDescription() const override { return L"扫描npm、pip、conda、Maven等开发工具的缓存和包文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Caution; }
    std::wstring GetIcon() const override { return L"dev_cache"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;

private:
    void ScanNpmCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanYarnCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanPnpmStore(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanNodeGyp(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanPipCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanCondaPkgs(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanMavenCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanGradleCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanGoCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanNuGetCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanCargoCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanElectronCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
};

} // namespace IceClean::Core::Scanner
