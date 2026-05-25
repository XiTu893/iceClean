#pragma once
#include <wx/wx.h>
#include <memory>
#include "models/ScanResult.h"
#include "models/CleanItem.h"
#include "models/MigrationItem.h"

namespace IceClean::Gui {

// 扫描完成事件
wxDECLARE_EVENT(wxEVT_SCAN_COMPLETE, wxThreadEvent);
// 扫描进度事件
wxDECLARE_EVENT(wxEVT_SCAN_PROGRESS, wxThreadEvent);
// 清理完成事件
wxDECLARE_EVENT(wxEVT_CLEAN_COMPLETE, wxThreadEvent);
// 清理进度事件
wxDECLARE_EVENT(wxEVT_CLEAN_PROGRESS, wxThreadEvent);
// 迁移完成事件
wxDECLARE_EVENT(wxEVT_MIGRATE_COMPLETE, wxThreadEvent);
// 迁移进度事件
wxDECLARE_EVENT(wxEVT_MIGRATE_PROGRESS, wxThreadEvent);

} // namespace IceClean::Gui
