#pragma once
#include <wx/wx.h>

namespace IceClean {

class App : public wxApp {
public:
    bool OnInit() override;
    int OnExit() override;
};

} // namespace IceClean
