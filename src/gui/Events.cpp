#include "Events.h"

namespace IceClean::Gui {

wxDEFINE_EVENT(wxEVT_SCAN_REQUEST, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SCAN_PROGRESS_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SCAN_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_CLEAN_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_CLEAN_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_MIGRATE_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_MIGRATE_PROGRESS, wxThreadEvent);

} // namespace IceClean::Gui
