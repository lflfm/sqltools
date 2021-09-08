/* 
    Copyright (C) 2002 Aleksey Kochetov

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/

// 17.01.2005 (ak) simplified settings class hierarchy 

#include "stdafx.h"
#include <string>
#include <sstream>
#include <COMMON/ExceptionHelper.h>
#include "OpenEditor/OEGeneralPage.h"
#include <Common/MyUtf.h>

    const char* COEGeneralPage::m_keymapLayoutList = "ConText;Custom;Default;EditPlus2;TextPad;UltraEdit;";

COEGeneralPage::COEGeneralPage (SettingsManager& manager)
    : CPropertyPage(COEGeneralPage::IDD)
    , m_manager(manager)
    /*, m_AllowMultipleInstances(FALSE)
    , m_NewDocOnStartup(FALSE)
    , m_MaximizeFirstDocument(FALSE)
    , m_WorkDirFollowsDoc(FALSE)
    , m_SaveCursPosAndBookmarks(FALSE)
    , m_SaveMainWindowPosition(FALSE)
    , m_keymapLayout(_T(""))
    , m_locale(_T(""))*/
{
    m_psp.dwFlags &= ~PSP_HASHELP;

    const OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
    m_AllowMultipleInstances  = settings->GetAllowMultipleInstances() ? TRUE : FALSE;
    m_NewDocOnStartup         = settings->GetNewDocOnStartup()        ? TRUE : FALSE;
    m_WorkDirFollowsDoc       = settings->GetWorkDirFollowsDocument() ? TRUE : FALSE;
    m_SaveMainWindowPosition  = settings->GetSaveMainWinPosition()    ? TRUE : FALSE;
    m_keymapLayout            = settings->GetKeymapLayout().c_str();
    m_locale                  = settings->GetLocale().c_str();
    m_Encoding                = settings->GetEncoding(); 
    m_AsciiAsUft8             = settings->GetAsciiAsUtf8() ? TRUE : FALSE;

    m_MDITabsOnTop                = settings->GetMDITabsOnTop()                ? TRUE : FALSE;
    m_MDITabsAutoColor            = settings->GetMDITabsAutoColor()            ? TRUE : FALSE;
    m_MDITabsDocumentMenuButton   = settings->GetMDITabsDocumentMenuButton()   ? TRUE : FALSE;
    m_MDITabsActiveTabCloseButton = settings->GetMDITabsActiveTabCloseButton() ? TRUE : FALSE;
    m_MDITabs3DLook               = settings->GetMDITabsRectangularLook()      ? TRUE : FALSE;
    m_MDITabsCtrlTabSwitchesToPrevActive = settings->GetMDITabsCtrlTabSwitchesToPrevActive() ? TRUE : FALSE;
}

COEGeneralPage::~COEGeneralPage()
{
}

void COEGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_OE_GEN_MULTIPLE_INST, m_AllowMultipleInstances);
    DDX_Check(pDX, IDC_OE_GEN_NEW_DOC_ON_STARTUP, m_NewDocOnStartup);
    DDX_Check(pDX, IDC_OE_GEN_WORK_DIR_FOLLOWS_DOC, m_WorkDirFollowsDoc);
    DDX_Check(pDX, IDC_OE_GEN_SAVE_MAIN_WIN_POS, m_SaveMainWindowPosition);
    
    if (!pDX->m_bSaveAndValidate)
    {
        HWND hList = ::GetDlgItem(m_hWnd, IDC_OE_GEN_KEYMAP_LAYOUT);

        if (!::SendMessage(hList, CB_GETCOUNT, 0, 0))
        {
            std::istringstream in(m_keymapLayoutList);
            std::string line;
            while (std::getline(in, line, ';'))
                ::SendMessage(hList, CB_ADDSTRING, 0, (LPARAM)Common::wstr(line).c_str());
        }
    }
    
    DDX_CBString(pDX, IDC_OE_GEN_KEYMAP_LAYOUT, m_keymapLayout);
    DDX_CBString(pDX, IDC_OE_GEN_LOCALE, m_locale);

    DDX_Check(pDX, IDC_OE_GEN_ASCII_AS_UTF8, m_AsciiAsUft8);

    if (pDX->m_bSaveAndValidate)
    {
        m_Encoding = SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_GETCURSEL);
    }
    else
    {
        SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_SETCURSEL, m_Encoding);
    }

    GetDlgItem(IDC_OE_GEN_ASCII_AS_UTF8)->EnableWindow(m_Encoding == OpenEditor::feUTF8);

    DDX_Check(pDX, IDC_OE_GEN_TABS_ON_TOP          , m_MDITabsOnTop);
    DDX_Check(pDX, IDC_OE_GEN_TABS_CLOSE_BUTTON    , m_MDITabsActiveTabCloseButton);
    DDX_Check(pDX, IDC_OE_GEN_TABS_3D_LOOK         , m_MDITabs3DLook);
    DDX_Check(pDX, IDC_OE_GEN_TABS_AUTOCOLOR       , m_MDITabsAutoColor);
    DDX_Check(pDX, IDC_OE_GEN_TABS_LIST_MENU_BUTTON, m_MDITabsDocumentMenuButton);
    DDX_Check(pDX, IDC_OE_GEN_TABS_CTRL_TAB_SWITCHES_TO_PREV_ACTIVE, m_MDITabsCtrlTabSwitchesToPrevActive);
}


BEGIN_MESSAGE_MAP(COEGeneralPage, CPropertyPage)
    ON_CBN_SELCHANGE(IDC_OE_GEN_KEYMAP_LAYOUT, ShowWarningAfterExitAndRestart)
    ON_CBN_SELCHANGE(IDC_OE_GEN_ENCODING, OnSelChangeUncoding)
END_MESSAGE_MAP()

// COEGeneralPage message handlers
void COEGeneralPage::ShowWarningAfterExitAndRestart()
{
    AfxMessageBox(L"This setting takes effect only after you exit and restart!");
}

BOOL COEGeneralPage::OnInitDialog()
{
    SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_RESETCONTENT);
    SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_ADDSTRING, 0, (LPARAM)L"ANSI (Current Windows codepage)");
    SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_ADDSTRING, 0, (LPARAM)L"UTF-8");
    SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_ADDSTRING, 0, (LPARAM)L"UTF-8 with BOM");
    SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_ADDSTRING, 0, (LPARAM)L"UTF-16 LE with BOM");

    CPropertyPage::OnInitDialog();

    return TRUE; 
}

BOOL COEGeneralPage::OnApply()
{
    try { EXCEPTION_FRAME;

        OpenEditor::GlobalSettingsPtr settings = m_manager.GetGlobalSettings();
        settings->SetAllowMultipleInstances(m_AllowMultipleInstances  ? true : false,  false /*notify*/);
        settings->SetNewDocOnStartup(m_NewDocOnStartup                ? true : false,  false /*notify*/);
        settings->SetWorkDirFollowsDocument(m_WorkDirFollowsDoc       ? true : false,  false /*notify*/);
        settings->SetSaveMainWinPosition(m_SaveMainWindowPosition     ? true : false,  false /*notify*/);
        settings->SetKeymapLayout(Common::str(m_keymapLayout), false /*notify*/);
        settings->SetLocale(Common::str(m_locale), false /*notify*/);
        settings->SetEncoding  (m_Encoding, false /*notify*/);
        settings->SetAsciiAsUtf8(m_AsciiAsUft8 ? true : false, false /*notify*/);

        settings->SetMDITabsOnTop(               m_MDITabsOnTop                ? true : false, false /*notify*/);
        settings->SetMDITabsAutoColor(           m_MDITabsAutoColor            ? true : false, false /*notify*/);
        settings->SetMDITabsDocumentMenuButton(  m_MDITabsDocumentMenuButton   ? true : false, false /*notify*/);
        settings->SetMDITabsActiveTabCloseButton(m_MDITabsActiveTabCloseButton ? true : false, false /*notify*/);
        settings->SetMDITabsRectangularLook(     m_MDITabs3DLook               ? true : false, false /*notify*/);
        settings->SetMDITabsCtrlTabSwitchesToPrevActive(m_MDITabsCtrlTabSwitchesToPrevActive ? true : false, false /*notify*/);
    }
    _OE_DEFAULT_HANDLER_;

    return TRUE;
}

void COEGeneralPage::OnSelChangeUncoding ()
{
    m_Encoding = SendDlgItemMessage(IDC_OE_GEN_ENCODING, CB_GETCURSEL);
    GetDlgItem(IDC_OE_GEN_ASCII_AS_UTF8)->EnableWindow(m_Encoding == OpenEditor::feUTF8);
}


