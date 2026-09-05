#pragma once
#include <wx/taskbar.h>
#include <functional>

namespace IceClean::Gui {

class TrayIcon : public wxTaskBarIcon {
public:
    using MenuCreator = std::function<wxMenu*()>;

    explicit TrayIcon(MenuCreator creator)
        : wxTaskBarIcon(), m_menuCreator(std::move(creator)) {}

    wxMenu* CreatePopupMenu() override {
        if (m_menuCreator) {
            return m_menuCreator();
        }
        return new wxMenu();
    }

private:
    MenuCreator m_menuCreator;
};

} // namespace IceClean::Gui
