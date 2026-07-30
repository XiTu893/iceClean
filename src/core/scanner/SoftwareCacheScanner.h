#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class SoftwareCacheScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"软件缓存"; }
    std::wstring GetDescription() const override { return L"扫描微信、QQ、迅雷等常用软件的缓存文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Caution; }
    std::wstring GetIcon() const override { return L"software_cache"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;

private:
    void ScanWeChatCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanQQCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanThunderCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanVideoAppCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanWPSCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanDingTalkCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanYoudaoCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanElectronAppCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
    void ScanPackageManagerCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb);
};

} // namespace IceClean::Core::Scanner
