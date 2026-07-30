#pragma once
#include <wx/wx.h>
#include "core/optimizer/SystemFileManager.h"

namespace IceClean::Gui {

class SystemFileManagerPanel : public wxPanel {
public:
    SystemFileManagerPanel(wxWindow* parent);

    void RefreshItems();

private:
    void OnDisableHibernation(wxCommandEvent& event);
    void OnSetHibernationSmall(wxCommandEvent& event);
    void OnOptimizePageFile(wxCommandEvent& event);
    void OnCleanWindowsOld(wxCommandEvent& event);
    void OnEmptyRecycleBin(wxCommandEvent& event);
    void OnCleanWinSxS(wxCommandEvent& event);

    wxStaticText* CreateInfoCard(const std::wstring& title, const std::wstring& desc,
                                  wxWindow* parent, wxBoxSizer* sizer);

    Core::Optimizer::SystemFileManager m_manager;

    wxStaticText* m_hibernationSize = nullptr;
    wxStaticText* m_hibernationStatus = nullptr;
    wxButton* m_hibernationBtn = nullptr;
    wxButton* m_hibernationSmallBtn = nullptr;

    wxStaticText* m_pagefileSize = nullptr;
    wxButton* m_pagefileBtn = nullptr;

    wxStaticText* m_winOldSize = nullptr;
    wxButton* m_winOldBtn = nullptr;

    wxStaticText* m_recycleBinSize = nullptr;
    wxButton* m_recycleBinBtn = nullptr;

    wxStaticText* m_winSxSInfo = nullptr;
    wxButton* m_winSxsBtn = nullptr;
};

} // namespace IceClean::Gui
