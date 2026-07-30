#pragma once
#include <wx/wx.h>
#include <memory>
#include "models/ScanResult.h"
#include "models/CleanItem.h"
#include "models/MigrationItem.h"

namespace IceClean::Gui {

// 扫描请求事件（由面板发出，触发扫描）
wxDECLARE_EVENT(wxEVT_SCAN_REQUEST, wxThreadEvent);
// 扫描进度更新事件（由扫描器发出，通知UI更新进度）
wxDECLARE_EVENT(wxEVT_SCAN_PROGRESS_UPDATE, wxThreadEvent);
// 停止扫描请求事件（由面板发出，请求停止扫描）
wxDECLARE_EVENT(wxEVT_SCAN_STOP, wxThreadEvent);
// 扫描完成事件
wxDECLARE_EVENT(wxEVT_SCAN_COMPLETE, wxThreadEvent);
// 清理完成事件
wxDECLARE_EVENT(wxEVT_CLEAN_COMPLETE, wxThreadEvent);
// 清理进度事件
wxDECLARE_EVENT(wxEVT_CLEAN_PROGRESS, wxThreadEvent);
// 迁移完成事件
wxDECLARE_EVENT(wxEVT_MIGRATE_COMPLETE, wxThreadEvent);
// 迁移进度事件
wxDECLARE_EVENT(wxEVT_MIGRATE_PROGRESS, wxThreadEvent);

// ==== 通用操作进度事件（Phase 1 新增） ====
// 操作进度更新事件（统一用于Debloat/Privacy/系统文件等操作）
wxDECLARE_EVENT(wxEVT_OPERATION_PROGRESS_UPDATE, wxThreadEvent);
// 操作完成事件
wxDECLARE_EVENT(wxEVT_OPERATION_COMPLETE, wxThreadEvent);
// 操作请求取消事件
wxDECLARE_EVENT(wxEVT_OPERATION_CANCEL, wxThreadEvent);

} // namespace IceClean::Gui
