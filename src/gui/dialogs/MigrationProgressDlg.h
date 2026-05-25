#pragma once
#include <wx/wx.h>
#include <cstdint>

namespace IceClean::Gui {

// 迁移进度对话框
class MigrationProgressDlg : public wxDialog {
public:
    MigrationProgressDlg(wxWindow* parent,
                         const wxString& title = L"迁移进度",
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxSize(480, 260));

    // 设置当前迁移的文件
    void SetCurrentFile(const wxString& file);
    // 设置进度 (0-100)
    void SetProgress(int percent);
    // 设置速度
    void SetSpeed(const wxString& speed);
    // 设置剩余时间
    void SetRemainingTime(const wxString& time);
    // 设置迁移统计
    void SetStats(int currentItem, int totalItems, uint64_t migratedSize, uint64_t totalSize);
    // 标记完成
    void SetCompleted(bool success, const wxString& message = wxEmptyString);

    // 是否已取消
    bool IsCancelled() const { return m_cancelled; }

private:
    bool m_cancelled = false;

    wxGauge* m_progressBar = nullptr;
    wxStaticText* m_fileLabel = nullptr;
    wxStaticText* m_speedLabel = nullptr;
    wxStaticText* m_timeLabel = nullptr;
    wxStaticText* m_statsLabel = nullptr;
    wxButton* m_cancelButton = nullptr;

    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
};

} // namespace IceClean::Gui
