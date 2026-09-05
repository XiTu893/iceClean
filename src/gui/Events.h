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
// 迁移扫描进度事件（迁移面板专用，worker 线程通过 wxQueueEvent 投递）
wxDECLARE_EVENT(wxEVT_MIGRATION_SCAN_PROGRESS, wxThreadEvent);
// 迁移扫描请求事件（携带阈值参数）
wxDECLARE_EVENT(wxEVT_MIGRATION_SCAN_REQUEST, wxThreadEvent);
// 清理完成事件
wxDECLARE_EVENT(wxEVT_CLEAN_COMPLETE, wxThreadEvent);
// 清理进度事件
wxDECLARE_EVENT(wxEVT_CLEAN_PROGRESS, wxThreadEvent);
// 迁移完成事件
wxDECLARE_EVENT(wxEVT_MIGRATE_COMPLETE, wxThreadEvent);
// 迁移进度事件
wxDECLARE_EVENT(wxEVT_MIGRATE_PROGRESS, wxThreadEvent);

// ==== 通用操作进度事件 ====
// 操作进度更新事件（统一用于Debloat/Privacy/系统文件等操作）
wxDECLARE_EVENT(wxEVT_OPERATION_PROGRESS_UPDATE, wxThreadEvent);
// 操作完成事件
wxDECLARE_EVENT(wxEVT_OPERATION_COMPLETE, wxThreadEvent);
// 操作请求取消事件
wxDECLARE_EVENT(wxEVT_OPERATION_CANCEL, wxThreadEvent);

// ==== 通用导航事件 ====
// 导航选择改变事件（由卡片点击发出， 携带目标页面索引）
wxDECLARE_EVENT(wxEVT_NAV_SELECTION_CHANGED, wxCommandEvent);

} // namespace IceClean::Gui