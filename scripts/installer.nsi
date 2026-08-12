; IceClean NSIS Installer Script
; Build with: makensis scripts/installer.nsi

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Application info
!define APP_NAME "IceClean"
!define APP_VERSION "1.0.0"
!define APP_PUBLISHER "XiTu"
!define APP_URL "https://github.com/XiTu893/iceClean"
!define APP_EXE "IceClean.exe"

; Installer metadata
Name "${APP_NAME} ${APP_VERSION}"
OutFile "IceClean-${APP_VERSION}-setup.exe"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKCU "Software\${APP_NAME}" ""
RequestExecutionLevel admin

; Modern UI
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\license.rtf"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "SimpChinese"

; Sections
Section "Main" SEC_MAIN
    SetOutPath "$INSTDIR"
    
    ; Main executable
    File "..\build\Release\${APP_EXE}"
    
    ; Resources
    File "..\src\gui\resources\app.manifest"
    File "..\src\gui\resources\QrReward.jpg"
    File "..\src\gui\resources\XiTu-logo.jpg"
    
    ; Start menu shortcuts
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    
    ; Registry for uninstall
    WriteRegStr HKCU "Software\${APP_NAME}" "" $INSTDIR
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "URLInfoAbout" "${APP_URL}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "NoRepair" 1
SectionEnd

; Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\app.manifest"
    Delete "$INSTDIR\QrReward.jpg"
    Delete "$INSTDIR\XiTu-logo.jpg"
    Delete "$INSTDIR\Uninstall.exe"
    
    RMDir /r "$SMPROGRAMS\${APP_NAME}"
    Delete "$DESKTOP\${APP_NAME}.lnk"
    
    RMDir "$INSTDIR"
    
    DeleteRegKey HKCU "Software\${APP_NAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd

; Functions
Function .onInit
    ; Check for admin rights
    UserInfo::GetAccountType
    Pop $0
    StrCmp $0 "Admin" 0 +2
    MessageBox MB_ICONSTOP "安装需要管理员权限，请以管理员身份运行安装程序。"
    Abort
FunctionEnd