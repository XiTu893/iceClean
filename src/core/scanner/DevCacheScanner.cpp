#include "DevCacheScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory DevCacheScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    ScanNpmCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanYarnCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanPnpmStore(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanNodeGyp(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanPipCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanCondaPkgs(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanMavenCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanGradleCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanGoCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanNuGetCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanCargoCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanElectronCache(category, stopFlag, progressCb);

    return category;
}

void DevCacheScanner::ScanNpmCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring npmCache = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\npm-cache");
    if (Utils::FileUtil::Exists(npmCache)) {
        ScanDirectory(npmCache, L"*", true, true, category, stopFlag, progressCb);
    }

    std::wstring npmDir = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.npm");
    if (Utils::FileUtil::Exists(npmDir)) {
        ScanDirectory(npmDir, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanYarnCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring yarnCache = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\Yarn\\Cache");
    if (Utils::FileUtil::Exists(yarnCache)) {
        ScanDirectory(yarnCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanPnpmStore(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring pnpmStore = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\pnpm-store");
    if (Utils::FileUtil::Exists(pnpmStore)) {
        ScanDirectory(pnpmStore, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanNodeGyp(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring nodeGyp = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.node-gyp");
    if (Utils::FileUtil::Exists(nodeGyp)) {
        ScanDirectory(nodeGyp, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanPipCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring pipCache = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\pip\\Cache");
    if (Utils::FileUtil::Exists(pipCache)) {
        ScanDirectory(pipCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanCondaPkgs(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring minicondaPkgs = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\miniconda3\\pkgs");
    if (Utils::FileUtil::Exists(minicondaPkgs)) {
        ScanDirectory(minicondaPkgs, L"*", true, true, category, stopFlag, progressCb);
    }

    std::wstring anacondaPkgs = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\anaconda3\\pkgs");
    if (Utils::FileUtil::Exists(anacondaPkgs)) {
        ScanDirectory(anacondaPkgs, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanMavenCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring mavenRepo = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.m2\\repository");
    if (Utils::FileUtil::Exists(mavenRepo)) {
        ScanDirectory(mavenRepo, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanGradleCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring gradleCaches = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.gradle\\caches");
    if (Utils::FileUtil::Exists(gradleCaches)) {
        ScanDirectory(gradleCaches, L"*", true, true, category, stopFlag, progressCb);
    }

    std::wstring gradleWrapper = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.gradle\\wrapper\\dists");
    if (Utils::FileUtil::Exists(gradleWrapper)) {
        ScanDirectory(gradleWrapper, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanGoCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring goModCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\go\\pkg\\mod");
    if (Utils::FileUtil::Exists(goModCache)) {
        ScanDirectory(goModCache, L"*", true, true, category, stopFlag, progressCb);
    }

    std::wstring goBuildCache = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\go");
    if (Utils::FileUtil::Exists(goBuildCache)) {
        ScanDirectory(goBuildCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanNuGetCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring nugetPkgs = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.nuget\\packages");
    if (Utils::FileUtil::Exists(nugetPkgs)) {
        ScanDirectory(nugetPkgs, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanCargoCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring cargoRegistry = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.cargo\\registry");
    if (Utils::FileUtil::Exists(cargoRegistry)) {
        ScanDirectory(cargoRegistry, L"*", true, true, category, stopFlag, progressCb);
    }
}

void DevCacheScanner::ScanElectronCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring electronCache = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\electron\\Cache");
    if (Utils::FileUtil::Exists(electronCache)) {
        ScanDirectory(electronCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

bool DevCacheScanner::IsAvailable() const {
    return Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\npm-cache")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.npm")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\Yarn\\Cache")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\pnpm-store")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.node-gyp")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\pip\\Cache")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\miniconda3\\pkgs")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\anaconda3\\pkgs")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.m2\\repository")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.gradle\\caches")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\go\\pkg\\mod")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\go")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.nuget\\packages")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\.cargo\\registry")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\electron\\Cache"));
}

} // namespace IceClean::Core::Scanner
