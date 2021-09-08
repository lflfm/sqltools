Name "SQLTools v${SQLTOOLSVER}"

!define MUI_VERSION 	"v${SQLTOOLSVER}"

!define DEST_NAME		"SQLTools 2.0"
!define SRCDIR 			"..\SQLTools"
!define EXEDIR  		"..\_output_\${SQLTOOLS_BUILDDIR}"
!define SQLHELP_PATH 	"..\SQLHelp"
!define GREP_PATH		"..\Grep"
!define LAUNCHER_PATH	"..\sqlt-launcher"

;!include "MUI.nsh"
!include "MUI2.nsh"

!define TEMP $R0

;--------------------------------
;Configuration
!ifdef NOADMIN
OutFile InstallSQLTools_${SQLTOOLSVER}_no-admin.exe
RequestExecutionLevel user
!else
OutFile InstallSQLTools_${SQLTOOLSVER}.exe
RequestExecutionLevel admin
!endif
SetCompressor bzip2

;--------------------------------
;Modern UI Configuration

!define MUI_CUSTOMPAGECOMMANDS
!define MUI_LICENSEPAGE
!define MUI_COMPONENTSPAGE
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_DIRECTORYPAGE
!define MUI_ABORTWARNING
!define MUI_UNINSTALLER
!define MUI_UNCONFIRMPAGE

;--------------------------------
;Pages
;--------------------------------
!ifdef NOADMIN
	!define MUI_WELCOMEPAGE_TITLE "Welcome to the SQLTools 2.0 Setup Wizard (for user w/o admin rights)"
!else
	!define MUI_WELCOMEPAGE_TITLE "Welcome to the SQLTools 2.0 Setup Wizard (for user with admin rights)"
!endif
!define MUI_WELCOMEPAGE_TEXT "PLEASE READ THIS INFORMATION CAREFULLY$\n$\n\
***Oracle Client (8i, 9i, 10g, 11g or 12c) 32-bit is REQUIRED***$\n$\n\
Do not install this release over an existing installation of SQLTools 1.4.X. or SQLTools 1.5.X, \
use a new folder or uninstall it the old one first otherwise you won't be able to do that later.$\n$\n\
Run the installer with the /S flag for silent installation.$\n$\n$_CLICK"
	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "${SRCDIR}\LICENSE.TXT"
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_INSTFILES

	;License page
	;LicenseText "Please read the following license before installation"
	LicenseData "${SRCDIR}\LICENSE.TXT"

	;Folder-selection page
!ifdef NOADMIN
	InstallDir "$PROFILE\Apps\${DEST_NAME}"
	InstallDirRegKey HKEY_CURRENT_USER "SOFTWARE\${DEST_NAME}" ""
!else
	InstallDir "$PROGRAMFILES\${DEST_NAME}"
	InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${DEST_NAME}" ""
!endif

	;InstType "Normal (recommended for fresh installation and upgrade)"
	;InstType "Full upgrade (with replacing all user settings)"

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

	;Descriptions
	LangString DESC_SecCopyProgramFiles   ${LANG_ENGLISH} "Copy SQLTools.exe and other required components to the application folder."
	LangString DESC_SecCreateMenuGroup 	  ${LANG_ENGLISH} "Crete SQLTools menu group in Windows menu."
	LangString DESC_SecCreateDesktopIcon  ${LANG_ENGLISH} "Create SQLTools shortcut on your descktop."
	LangString DESC_SecQuickLaunchIcon    ${LANG_ENGLISH} "Create SQLTools quick launch shortcut."

;--------------------------------
;General
Icon "${SRCDIR}\Res\InstallSQLTools14.ico"
UninstallIcon "${SRCDIR}\Res\Uninstall.ico"

;--------------------------------
;Installer Sections

Section "!Program files (required)" SecCopyProgramFiles
	SectionIn 1 2 RO

	SetOutPath "$INSTDIR"
	File "${SQLHELP_PATH}\sqlqkref.chm"
	File /oname=SQLToolsU.chm "${EXEDIR}\sqltools.chm"
	File /oname=SQLToolsU.exe "${EXEDIR}\SQLTools.exe"
	File "${SRCDIR}\README.TXT"
	File "${SRCDIR}\LICENSE.TXT"
	File "${SRCDIR}\History.TXT"

!ifdef NOADMIN
	;nothing
!else
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\${DEST_NAME}" "" "$INSTDIR"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${DEST_NAME}" "DisplayName" "${DEST_NAME} (remove only)"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${DEST_NAME}" "UninstallString" '"$INSTDIR\uninst.exe"'
!endif
	WriteUninstaller "$INSTDIR\uninst.exe"

;	CreateDirectory $INSTDIR\Samples
;	SetOutPath $INSTDIR\Samples

;	File "..\Samples\substitution.sql"

	CreateDirectory "$INSTDIR\Launcher"
	SetOutPath "$INSTDIR\Launcher"
	File "${LAUNCHER_PATH}\sqlt-launcher.exe"
	IfFileExists "sqlt-launcher.xml" SqltLauncherXmlExists
	File "${LAUNCHER_PATH}\sqlt-launcher.xml"
SqltLauncherXmlExists:

	Exec 'hh $INSTDIR\sqltoolsu.chm::/init_setup.htm'

SectionEnd


Section "Menu Group" SecCreateMenuGroup
	SectionIn 1 2

!ifdef NOADMIN
	SetShellVarContext current
!else
	SetShellVarContext all
!endif
    SetOutPath "$SMPROGRAMS\${DEST_NAME}"
    WriteINIStr "$SMPROGRAMS\${DEST_NAME}\SQLTools Home Page.url" "InternetShortcut" "URL" "http://www.sqltools.net"
    WriteINIStr "$SMPROGRAMS\${DEST_NAME}\Download SQLTools Tutorial.url" "InternetShortcut" "URL" "https://sourceforge.net/projects/sqlt/files/Tutorial/Version%200.1%20%28EN%2CPL%2CFR%2CSP%2CHU%29/"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\Uninstall SQLTools.lnk" "$INSTDIR\uninst.exe"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\SQLTools.lnk" "$INSTDIR\SQLToolsU.exe"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\README.lnk" "$INSTDIR\README.TXT"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\LICENSE.lnk" "$INSTDIR\LICENSE.TXT"
    CreateShortCut "$SMPROGRAMS\${DEST_NAME}\History.lnk" "$INSTDIR\History.txt"
    SetOutPath $INSTDIR
SectionEnd

Section "Desktop Icon" SecCreateDesktopIcon
	SectionIn 1 2
!ifdef NOADMIN
	SetShellVarContext current
!else
	SetShellVarContext all
!endif
    CreateShortCut "$DESKTOP\${DEST_NAME}.lnk" "$INSTDIR\SQLToolsU.exe"
SectionEnd

Section "Quick Launch Icon" SecQuickLaunchIcon
	SectionIn 1 2
!ifdef NOADMIN
	SetShellVarContext current
!else
	SetShellVarContext all
!endif
    CreateShortCut "$QUICKLAUNCH\${DEST_NAME}.lnk" "$INSTDIR\SQLToolsU.exe"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCopyProgramFiles}  $(DESC_SecCopyProgramFiles)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCreateMenuGroup}   $(DESC_SecCreateMenuGroup)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecCreateDesktopIcon} $(DESC_SecCreateDesktopIcon)
	!insertmacro MUI_DESCRIPTION_TEXT ${SecQuickLaunchIcon}   $(DESC_SecQuickLaunchIcon)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section Uninstall

	Delete "$INSTDIR\sqlnet.log"
	Delete "$INSTDIR\uninst.exe"
	Delete "$INSTDIR\sqlqkref.chm"
	Delete "$INSTDIR\SQLToolsU.chm"
	Delete "$INSTDIR\SQLToolsU.exe"
	Delete "$INSTDIR\ReadMe.txt"
	Delete "$INSTDIR\License.txt"
	Delete "$INSTDIR\History.txt"

;	Delete "$INSTDIR\Samples\substitution.sql"
;	RMDir  "$INSTDIR\Samples"

	Delete "$INSTDIR\Launcher\sqlt-launcher.exe"
	Delete "$INSTDIR\Launcher\sqlt-launcher.xml"
	RMDir  "$INSTDIR\Launcher"

!ifdef NOADMIN
	SetShellVarContext current
!else
	SetShellVarContext all
!endif
	Delete "$SMPROGRAMS\${DEST_NAME}\*.lnk"
	Delete "$SMPROGRAMS\${DEST_NAME}\*.url"
	RMDir  "$SMPROGRAMS\${DEST_NAME}"
	Delete "$DESKTOP\${DEST_NAME}.lnk"
	Delete "$QUICKLAUNCH\${DEST_NAME}.lnk"

!ifdef NOADMIN
	DeleteRegKey /ifempty HKEY_CURRENT_USER "SOFTWARE\${DEST_NAME}"
!else
	DeleteRegKey /ifempty HKEY_LOCAL_MACHINE "SOFTWARE\${DEST_NAME}"
	DeleteRegKey /ifempty HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${DEST_NAME}"
!endif
	RMDir "$INSTDIR"

SectionEnd ; end of uninstall section
