#include "ScanResultPanel.h"
#include "DeepCleanPanel.h"
#include "gui/Events.h"
#include "gui/controls/ThemeManager.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(ScanResultPanel, wxPanel)
wxEND_EVENT_TABLE()

ScanResultPanel::ScanResultPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

ScanResultPanel::~ScanResultPanel() = default;

void ScanResultPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"深度清理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 描述
    auto* descLabel = new wxStaticText(this, wxID_ANY,
        L"深度清理可释放更多空间，包括系统压缩、休眠文件关闭、隐私清理和注册表修复");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    descLabel->Wrap(600);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 深度清理面板
    m_deepCleanPanel = new DeepCleanPanel(this);
    mainSizer->Add(m_deepCleanPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);

    // 转发深度清理面板的事件到上层
    m_deepCleanPanel->Bind(wxEVT_CLEAN_PROGRESS, [this](wxThreadEvent& event) {
        wxPostEvent(GetParent(), event);
    });

    SetSizer(mainSizer);
}

void ScanResultPanel::SetScanResult(const IceClean::Models::ScanResult& result) {
    m_result = result;
    // 扫描结果现在由首页DashboardPanel处理，此处仅保留数据兼容
}

std::vector<std::wstring> ScanResultPanel::GetSelectedPaths() const {
    // 扫描结果路径现在由首页DashboardPanel处理
    return {};
}

uint64_t ScanResultPanel::GetSelectedSize() const {
    return 0;
}

} // namespace IceClean::Gui
