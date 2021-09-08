/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2006 Aleksey Kochetov

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

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog
//      10.01.2006 - Connection dialog has been re-implemented again 
//
//      22.05.2005 - Connection dialog has been re-implemented 
//
//      1.02.2002 - Connection dialog has been re implemented 
//                  for supporting non-tnsnames connection strings. 
//                  It's slightly over engineering because of compatibility issue.
//

#include "stdafx.h"
#include <fstream>
#include "resource.h"
#include "ConnectDlg.h"
#include <COMMON/StrHelpers.h>
#include <COMMON/DlgDataExt.h>
#include <OCI8/Connect.h>
#include "Dlg\ConnectSettingsDlg.h"
#include "ConnectData\ConnectDataXmlStreamer.h"
#include <COMMON/InputDlg.h>
#include <COMMON/MyUtf.h>
#include <COMMON/AppUtilities.h>

    using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace Common;

    const static int TOOLBAT_ID = 1111;

    static const wchar_t* MB_TITLE                 = L"Connect";
    static const wchar_t* FILE_NAME                = L"connections.xml";
    static const wchar_t* DEFAULT_PASSWORD         = L"none";
    static const wchar_t* EMPTY_PASSWORD_WARNING   =
        L"You did not provide the master password."
        L"\n\nYour passwords and login script will be encrypted"
        L"\nwith the default master password \"none\".\n";

    std::wstring CConnectDlg::m_masterPassword = DEFAULT_PASSWORD;
    ConnectEntry CConnectDlg::m_last;
    int CConnectDlg::m_sortColumn = -1;
    ListCtrlManager::ESortDir CConnectDlg::m_sortDirection = ListCtrlManager::ASC;
    ListCtrlManager::FilterCollection CConnectDlg::m_filter;

    static void GetTnsEntries (std::vector<std::wstring>& entries);

CConnectDlg::CConnectDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CConnectDlg::IDD, pParent),
    m_adapter(m_data.GetConnectEntries()),
	m_profiles(m_adapter)
{
    m_current.m_status = ConnectEntry::UNDEFINED;
    SetHelpID(CConnectDlg::IDD);
}

// TODO: attach help to F1
BEGIN_MESSAGE_MAP(CConnectDlg, CDialog)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_C_PROFILES, OnItemChanged_Profiles)
    ON_BN_CLICKED(IDC_C_TEST, OnTest)
    ON_BN_CLICKED(IDC_C_DELETE, OnDelete)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_NOTIFY(NM_DBLCLK, IDC_C_PROFILES, OnDblClk_Profiles)
    ON_NOTIFY(NM_RCLICK , IDC_C_PROFILES, OnRClick_Profiles)
	ON_BN_CLICKED(IDC_C_DIRECT_CONNECTION, OnClk_DirectConnectin)
    ON_BN_CLICKED(ID_HELP, OnBnClicked_Help)
    ON_BN_CLICKED(IDC_C_SETTINGS, OnSettings)
    ON_BN_CLICKED(IDC_C_SAVE, OnSave)

    ON_COMMAND(ID_CON_CONNECT,              OnOK)                  
    ON_COMMAND(ID_CON_TEST,                 OnTest)
    ON_COMMAND(ID_CON_CLEANUP,              OnCleanup)
    ON_COMMAND(ID_CON_DELETE,               OnDelete)    
    ON_COMMAND(ID_CON_SETTINGS,             OnSettings)   
    ON_COMMAND(ID_CON_COPY_USERNAME_ALIAS,          OnCopyUsernameAlias)
    ON_COMMAND(ID_CON_COPY_USERNAME_PASSWORD_ALIAS, OnCopyUsernamePasswordAlias)
    ON_COMMAND(ID_CON_MAKE_AND_COPY_ALIAS,          OnMakeAndCopyAlias)      
END_MESSAGE_MAP()

LRESULT CConnectDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}

void CConnectDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_C_PROFILES, m_profiles);

    DDX_Text(pDX, IDC_C_TAG, m_current.m_Tag);
    DDX_Text(pDX, IDC_C_USER, m_current.m_User);
    DDX_Text(pDX, IDC_C_PASSWORD, m_current.m_DecryptedPassword);
    DDX_Text(pDX, IDC_C_CONNECT, m_current.m_Alias);
    DDX_Check(pDX, IDC_C_DIRECT_CONNECTION, m_current.m_Direct);
    DDX_CBString(pDX, IDC_C_HOST, m_current.m_Host);
    DDX_CBString(pDX, IDC_C_TCP_PORT, m_current.m_Port);
    DDX_Text(pDX, IDC_C_SID, m_current.m_Sid);
    DDX_Check(pDX, IDC_C_SERVICE_INSTEAD_OF_SID, m_current.m_UseService);
    DDX_CBIndex(pDX, IDC_C_MODE, m_current.m_Mode);
    DDX_Check(pDX, IDC_C_SAFETY, m_current.m_Safety);
    DDX_Check(pDX, IDC_C_SLOW, m_current.m_Slow);
}

bool CConnectDlg::showDlg (CConnectSettingsDlg& dlg)
{
    bool ok = false;

    dlg.m_Passwords[0] =
    dlg.m_Passwords[1]  = m_masterPassword;
    dlg.m_SavePasswords = m_data.GetSavePasswords();
    //dlg.m_ScriptOnLogin = m_data.m_ScriptOnLogin;

    if (dlg.DoModal() == IDOK)
    {
        m_masterPassword = dlg.m_Passwords[1] ;
        m_data.SetSavePasswords(dlg.m_SavePasswords);
        if (!dlg.m_SavePasswords) 
            m_data.ClearPasswords();
        //m_data.m_ScriptOnLogin  = dlg.m_ScriptOnLogin;
        ok = true;
    }

    if (m_masterPassword.empty())
    {
        MessageBox(EMPTY_PASSWORD_WARNING, MB_TITLE, MB_OK|MB_ICONEXCLAMATION);
        m_masterPassword = DEFAULT_PASSWORD;
    }

    return ok;
}

void CConnectDlg::readProfiles ()
{
    CWaitCursor wait;

    m_data.Clear();

    ConnectDataXmlStreamer streamer(FILE_NAME);

    if (streamer.fileExists()) 
    {
        streamer >> m_data;

        try
        {
            // try DEFAULT_PASSWORD password 
            // if it does not work ask user for a master password
            // TODO: after failure aks to reset all passwords
            for (int attempt = 0; !m_data.TestPassword(m_masterPassword); ++attempt)
            {
                if (attempt
                && MessageBox(L"Invalid master password. Try again?   ", MB_TITLE, MB_YESNO|MB_ICONSTOP) == IDNO)
                {
                    if (MessageBox(L"Would you like reset master password?   ", MB_TITLE, MB_YESNO|MB_ICONSTOP) == IDYES)
                    {
                        CConnectSettingsDlg dlg(this);
                        m_masterPassword.clear();
                        showDlg(dlg);
                        m_data.ResetPasswordsAndScriptOnLogin(m_masterPassword);
                        streamer.SetOverridePassword(true);
                        streamer.EnableBackup(true);
                        streamer << m_data;
                    }
                    else
                    {
	                    //EndDialog(IDCANCEL); // abort
                        AfxThrowUserException();
                    }
                }

                CPasswordDlg dlg(this);
                dlg.m_title  = L"Connect data encrypted";
                dlg.m_prompt = L"Enter password:";

                if (dlg.DoModal() == IDCANCEL)
                    continue;

                m_masterPassword = dlg.m_value;
            }
        }
        catch (const EncryptionException& e) {
            
            MessageBox(Common::wstr(e.what()).c_str(), MB_TITLE, MB_OK|MB_ICONSTOP);

            if (MessageBox(L"The file connections.xml might be corrupt."
                    L"\n\nWould you like reset master password?" 
                    L"\n\nIf you choose \"Yes\" then all encrypted data will be lost." 
                    L"\n\nPress \"No\" if you want to ignore the error and continue.  ", 
                    MB_TITLE, MB_YESNO|MB_ICONSTOP) == IDYES)
            {
                CConnectSettingsDlg dlg(this);
                m_masterPassword.clear();
                showDlg(dlg);
                m_data.ResetPasswordsAndScriptOnLogin(m_masterPassword);
                streamer.SetOverridePassword(true);
                streamer.EnableBackup(true);
                streamer << m_data;
            }
        }
    }
    //else 
    //{
    //    CConnectSettingsDlg dlg(this);
    //    m_masterPassword.clear();
    //    showDlg(dlg);
    //    m_data.ImportFromRegistry(m_masterPassword);
    //    streamer << m_data;
    //}
}

void CConnectDlg::setupProfiles ()
{
    if (m_sortColumn == -1)
    {
        m_sortColumn = m_data.GetSortColumn();
        m_sortDirection = (ListCtrlManager::ESortDir)m_data.GetSortDirection();
        
        m_filter.clear();
        if (!m_data.GetTagFilter().empty()
        || !m_data.GetUserFilter().empty()
        || !m_data.GetAliasFilter().empty()  
        || !m_data.GetUsageCounterFilter().empty()
        || !m_data.GetLastUsageFilter().empty())
        {
            m_filter.push_back(ListCtrlManager::FilterComponent(
                m_data.GetTagFilter(), 
                static_cast<ListCtrlManager::EFilterOperation>(m_data.GetTagFilterOperation())
                ));
            m_filter.push_back(ListCtrlManager::FilterComponent(
                m_data.GetUserFilter(), 
                static_cast<ListCtrlManager::EFilterOperation>(m_data.GetUserFilterOperation())
                ));
            m_filter.push_back(ListCtrlManager::FilterComponent(
                m_data.GetAliasFilter(), 
                static_cast<ListCtrlManager::EFilterOperation>(m_data.GetAliasFilterOperation())
                ));
            m_filter.push_back(ListCtrlManager::FilterComponent(
                m_data.GetUsageCounterFilter(), 
                static_cast<ListCtrlManager::EFilterOperation>(m_data.GetUsageCounterFilterOperation())
                ));
            m_filter.push_back(ListCtrlManager::FilterComponent(
                m_data.GetLastUsageFilter(), 
                static_cast<ListCtrlManager::EFilterOperation>(m_data.GetLastUsageFilterOperation())
                ));
        }
    }
    m_profiles.SetSortColumn(m_sortColumn, m_sortDirection);
    m_profiles.SetFilter(m_filter);
    m_profiles.Refresh(true);

    int last = -1;
    time_t lastUsage = 0;

    set<wstring> hosts, ports;
    ConnectData::EntriesConstIterator 
        begin = m_data.GetConnectEntries().begin(),
        end = m_data.GetConnectEntries().end();

    for (ConnectData::EntriesConstIterator it = begin; it != end; ++it)
    {
        if (!it->m_Host.empty()) 
            hosts.insert(it->m_Host);
        if (!it->m_Port.empty()) 
            ports.insert(it->m_Port);

        if (m_last.m_status != ConnectEntry::UNDEFINED)
        {
            if (ConnectEntry::IsSignatureEql(*it, m_last))
                last = it - begin;
        }
        else
        {
            if (lastUsage < it->m_LastUsage)
            {
                lastUsage = it->m_LastUsage;
                last = it - begin;
            }
        }
    }
    m_profiles.SelectEntry(last);
    {
        std::vector<std::wstring> entries;
        GetTnsEntries(entries);
        auto it = entries.begin();
        for (; it != entries.end(); ++it)
            SendDlgItemMessage(IDC_C_CONNECT, CB_ADDSTRING, 0, (LPARAM)it->c_str());
    }
    {
        auto it = hosts.begin();
        for (; it != hosts.end(); ++it)
            SendDlgItemMessage(IDC_C_HOST, CB_ADDSTRING, 0, (LPARAM)it->c_str());
    }
    {
        ports.insert(L"1521");
        auto it = ports.begin();
        for (; it != ports.end(); ++it)
            SendDlgItemMessage(IDC_C_TCP_PORT, CB_ADDSTRING, 0, (LPARAM)it->c_str());
    }

    setupConnectionType();
}

INT_PTR CConnectDlg::DoModal()
{
    try { EXCEPTION_FRAME;

        readProfiles();
    }   
    _DEFAULT_HANDLER_

    return CDialog::DoModal();
}

BOOL CConnectDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    try { EXCEPTION_FRAME;

        m_profiles.Init();
        setupProfiles ();
    }   
    _DEFAULT_HANDLER_

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CConnectDlg message handlers

void CConnectDlg::OnItemChanged_Profiles (NMHDR* pNMHDR, LRESULT* pResult) 
try {
    *pResult = 0;
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pNMHDR; 

    if (pnmv->uChanged && pnmv->uNewState & LVIS_SELECTED && pnmv->lParam >= 0)
    {
        m_data.GetConnectEntry(pnmv->lParam, m_current, m_masterPassword);
        setupConnectionType();
        UpdateData(FALSE);
    }
        
} _COMMON_DEFAULT_HANDLER_;

bool CConnectDlg::doTest (bool showMessage, std::wstring& error) 
{
    UpdateData();
    normalizeCurrent();
    UpdateData(FALSE);

    if (m_current.m_Direct)
    {
        string buff;
        OciConnect::MakeTNSString(buff, 
            Common::str(m_current.m_Host).c_str(), 
            Common::str(m_current.m_Port).c_str(), 
            Common::str(m_current.m_Sid).c_str(), 
            m_current.m_UseService);
        m_current.m_Alias = Common::wstr(buff);
    }

    try { EXCEPTION_FRAME;

        OciConnect connect;
        
        OCI8::EConnectionMode mode = OCI8::ecmDefault;
        switch (m_current.m_Mode)
        {
        case 1: mode = OCI8::ecmSysDba; break;
        case 2: mode = OCI8::ecmSysOper; break;
        }
        connect.Open(
            Common::str(m_current.m_User).c_str(), 
            Common::str(m_current.m_DecryptedPassword).c_str(), 
            Common::str(m_current.m_Alias).c_str(), 
            mode, true, true);
        connect.Close();

        if (showMessage)
        {
            std::wstring name;
            getConnectionDisplayName(name);
            MessageBeep(MB_OK);
            MessageBox((L"The connection \"" + name + L"\" is OK.     ").c_str(), 
                L"Connection test", MB_OK|MB_ICONASTERISK);
        }
    }
    catch (const OciException& x)
    {
        error = Common::wstr(x.what());

        if (showMessage)
        {
            MessageBeep((UINT)-1);
            MessageBox(error.c_str(), L"Connection test", MB_OK|MB_ICONHAND);
        }
        return false;
    }
    _COMMON_DEFAULT_HANDLER_

    return true;
}

void CConnectDlg::OnTest () 
try { EXCEPTION_FRAME;
    std::wstring dummy;
    doTest(true, dummy);
}
_DEFAULT_HANDLER_

void CConnectDlg::getConnectionDisplayName (std::wstring& name)
{
    if (!m_current.m_Direct)
        name = m_current.m_Alias + L" - " + m_current.m_User;
    else
        name = m_current.m_Host + L":" + m_current.m_Port + L":" 
            + m_current.m_Sid + L" - " + m_current.m_User;

    to_upper_str(name.c_str(), name);
}

void CConnectDlg::OnDelete () 
try { EXCEPTION_FRAME;

    int index = m_profiles.GetNextItem(-1, LVNI_SELECTED);
    if (index != -1) 
    {
        int entry = (int)m_profiles.GetItemData(index);

        std::wstring name, message;

        getConnectionDisplayName(name);

        message = L"Are you sure you want to delete \"" + name + L"\"?";

        if (MessageBox(message.c_str(), MB_TITLE, MB_YESNO) == IDYES)
        {
            m_data.MarkedDeleted(entry);
            m_profiles.OnDeleteEntry(entry);
        }
    }
}
_DEFAULT_HANDLER_

void CConnectDlg::writeData ()
{
    CWaitCursor wait;

    m_profiles.GetSortColumn(m_sortColumn, m_sortDirection);
    m_data.SetSortColumn(m_sortColumn);
    m_data.SetSortDirection(m_sortDirection);
    
    m_profiles.GetFilter(m_filter);
    if (!m_filter.empty())
    {
        ASSERT(m_filter.size() == 5);
        m_data.SetTagFilter(m_filter.at(0).value);
        m_data.SetTagFilterOperation(m_filter.at(0).operation);
        m_data.SetUserFilter(m_filter.at(1).value);
        m_data.SetUserFilterOperation(m_filter.at(1).operation);
        m_data.SetAliasFilter(m_filter.at(2).value);
        m_data.SetAliasFilterOperation(m_filter.at(2).operation);
        m_data.SetUsageCounterFilter(m_filter.at(3).value);
        m_data.SetUsageCounterFilterOperation(m_filter.at(3).operation);
        m_data.SetLastUsageFilter(m_filter.at(4).value);
        m_data.SetLastUsageFilterOperation(m_filter.at(4).operation);
    }
    else
    {
        std::wstring empty;
        m_data.SetTagFilter(empty);
        m_data.SetUserFilter(empty);
        m_data.SetAliasFilter(empty);
        m_data.SetUsageCounterFilter(empty);
        m_data.SetLastUsageFilter(empty);
    }

    ConnectDataXmlStreamer streamer(FILE_NAME);
    streamer.SetMasterPassword(Common::str(m_masterPassword));
    streamer << m_data;
}

void CConnectDlg::normalizeCurrent ()
{
    // extract tns alias from user
    string::size_type pos = m_current.m_User.find('@');
    if (pos != string::npos) 
    {
        m_current.m_Alias = m_current.m_User.substr(pos + 1);
        m_current.m_User.resize(pos);
    }

    // extract password from user
    pos = m_current.m_User.find('/');
    if (pos != string::npos) 
    {
        m_current.m_DecryptedPassword = m_current.m_User.substr(pos + 1);
        m_current.m_User.resize(pos);
    }

    if (!m_current.m_Direct) 
    {
        m_current.m_Host.clear();
        m_current.m_Port.clear();
        m_current.m_Sid.clear();
    } else {
        m_current.m_Alias.clear();
    }
}

void CConnectDlg::OnOK () 
try { EXCEPTION_FRAME;

    UpdateData();
    normalizeCurrent();
    
    int row = m_data.Find(m_current);
    if (row != -1)
        m_data.SetConnectEntry(row, m_current, m_masterPassword);
    else
        m_data.AppendConnectEntry(m_current, m_masterPassword);
    m_last = m_current;

    writeData();
    CDialog::OnOK();
}
_DEFAULT_HANDLER_;

void CConnectDlg::OnCancel()
try { EXCEPTION_FRAME;

    CDialog::OnCancel();

    m_last = m_current;

    writeData();
}
_DEFAULT_HANDLER_

void CConnectDlg::OnDblClk_Profiles (NMHDR*, LRESULT* pResult) 
{
    OnOK();
    *pResult = 0;
}

void CConnectDlg::OnRClick_Profiles (NMHDR* pNMHDR, LRESULT* pResult) 
{
    NMITEMACTIVATE* pItem = (NMITEMACTIVATE*)pNMHDR;
    if (pItem && pItem->iItem != -1)
    {
        CPoint point(pItem->ptAction);
        m_profiles.ClientToScreen(&point);

        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_CONNECT));
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);

        bool empty = (m_profiles.GetNextItem(-1, LVNI_SELECTED) == -1);
        
        pPopup->EnableMenuItem(ID_CON_CONNECT,         !empty ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
        pPopup->EnableMenuItem(ID_CON_TEST,            !empty ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
        pPopup->EnableMenuItem(ID_CON_CLEANUP,         !empty ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
        pPopup->EnableMenuItem(ID_CON_DELETE,          !empty ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
        pPopup->EnableMenuItem(ID_CON_COPY_USERNAME_ALIAS,          !empty ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
        pPopup->EnableMenuItem(ID_CON_COPY_USERNAME_PASSWORD_ALIAS, !empty ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);
        pPopup->EnableMenuItem(ID_CON_MAKE_AND_COPY_ALIAS,          !empty & m_current.m_Direct ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED);

        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

        *pResult = 1;
    }
    else
        *pResult = 0;
}

void CConnectDlg::OnClk_DirectConnectin ()
{
    UpdateData(TRUE);
    setupConnectionType();
}

void CConnectDlg::setupConnectionType ()
{
    GetDlgItem(IDC_C_CONNECT)->EnableWindow(!m_current.m_Direct);
    GetDlgItem(IDC_C_HOST)->EnableWindow(m_current.m_Direct);
    GetDlgItem(IDC_C_SID)->EnableWindow(m_current.m_Direct);
    GetDlgItem(IDC_C_TCP_PORT)->EnableWindow(m_current.m_Direct);
    //GetDlgItem(IDC_C_TNSCOPY)->EnableWindow(m_current.m_Direct);
    GetDlgItem(IDC_C_SERVICE_INSTEAD_OF_SID)->EnableWindow(m_current.m_Direct);
}

void CConnectDlg::OnSettings ()
{
    std::wstring oldMasterPassword = m_masterPassword;

    try { EXCEPTION_FRAME;

        CConnectSettingsDlg dlg(this);

        if (showDlg(dlg))
        {
            CWaitCursor wait;
            if (oldMasterPassword != m_masterPassword)
                m_data.ChangePassword(oldMasterPassword, m_masterPassword);
            ConnectDataXmlStreamer streamer(FILE_NAME);
            streamer.SetOverridePassword();
            streamer << m_data;
        }

        return;
    }
    _DEFAULT_HANDLER_

    //the simplest way of recovery after an error
    m_masterPassword = oldMasterPassword;
	EndDialog(IDCANCEL); // abort
}

void CConnectDlg::OnBnClicked_Help()
{
#if _MFC_VER <= 0x0600
#pragma message("There is no html help support for VC6!")
#else//_MFC_VER > 0x0600
    AfxGetApp()->HtmlHelp(HID_BASE_RESOURCE + m_nIDHelp);
#endif
}

bool GetOciDllPath (std::wstring& path)
{
    WCHAR fullpath[1024], *filename;
    DWORD length = SearchPath(NULL, L"OCI.DLL", NULL, sizeof(fullpath)/sizeof(fullpath[0]), fullpath, &filename);
    
    if (length > 0 && length < sizeof(fullpath)/sizeof(fullpath[0]))
    {
        path.assign(fullpath, filename - fullpath);
        return true;
    }

    return false;
}

bool GetRegValue (const std::wstring& key, const wchar_t* name, std::wstring& value)
{
    HKEY hKey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCWSTR)key.c_str(), 0, KEY_QUERY_VALUE, &hKey);

    if (lRet != ERROR_SUCCESS)
        return false;

    DWORD type;
    WCHAR buff[1024];
    DWORD length = sizeof(buff)/sizeof(buff[0]);
    lRet = RegQueryValueEx(hKey, name, NULL, &type, (LPBYTE)buff, &length);
    buff[sizeof(buff)/sizeof(buff[0])-1] = 0;

    RegCloseKey(hKey);

    if(lRet != ERROR_SUCCESS 
    || type != REG_SZ
    || length >= sizeof(buff))
        return false;

    value = buff;
    
    return true;
}

bool GetSubkeys (const std::wstring& key, vector<std::wstring>& _subkeys)
{
    vector<std::wstring> subkeys;

    HKEY hKey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS, &hKey);

    if (lRet != ERROR_SUCCESS)
        return false;
    
    for (DWORD i = 0; true; i++) 
    { 
        WCHAR buff[1024];
        DWORD length = sizeof(buff)/sizeof(buff[0])-1;
        FILETIME ftLastWriteTime;
        lRet = RegEnumKeyEx(hKey, i, buff, &length, NULL, NULL, NULL, &ftLastWriteTime);
        if (ERROR_SUCCESS != lRet) break;
        buff[sizeof(buff)/sizeof(buff[0])-1] = 0;
        subkeys.push_back(buff);
    } 

    RegCloseKey(hKey);

    swap(subkeys, _subkeys);
    return true;
}

/*
tnsnames.ora lookup works in the way like oracle does
1) search the current folder (where the program started from)
2) environment variable TNS_ADMIN
3) registry key TNS_ADMIN
4) default location
*/
bool GetTnsPath (std::wstring& path)
{
    std::wstring value;

    WCHAR buff[MAX_PATH];
    if (GetCurrentDirectory(sizeof(buff)/sizeof(buff[0])-1, buff))
    {
        buff[sizeof(buff)/sizeof(buff[0])-1] = 0;
        value = buff;

        if (value.size() && *value.rbegin() != '\\')
            value += '\\';
        
        value += L"TNSNAMES.ORA";

        if (::PathFileExists(value.c_str()))
        {
            path = value;
            return true;
        }
    }

    if (const wchar_t* tns_admin = _wgetenv(L"TNS_ADMIN"))
    {
        value = tns_admin;

        if (value.size() && *value.rbegin() != '\\')
            value += '\\';

        value += L"TNSNAMES.ORA";

        if (::PathFileExists(value.c_str()))
        {
            path = value;
            return true;
        }
    }

    std::wstring ocipath;

    if (GetOciDllPath(ocipath))
    {
        vector<std::wstring> subkeys;
        if (GetSubkeys(L"SOFTWARE\\ORACLE", subkeys))
        {
            auto it = subkeys.begin();
            for (; it != subkeys.end(); ++it)
            {
                if (!wcsnicmp(it->c_str(), L"KEY_", sizeof("KEY_")-1)
                || !wcsnicmp(it->c_str(), L"HOME", sizeof("HOME")-1))
                {
                    std::wstring key = L"SOFTWARE\\ORACLE\\" + *it;
                    
                    if (GetRegValue(key, L"ORACLE_HOME", value))
                    {
                        if (!wcsicmp(ocipath.c_str(), (value + L"\\BIN\\").c_str())) 
                        {
                            if (GetRegValue(key, L"TNS_ADMIN", value))
                                path = value + L"\\TNSNAMES.ORA";
                            else
                                path = value + L"\\NETWORK\\ADMIN\\TNSNAMES.ORA";

                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

static
void GetTnsEntries (std::vector<std::wstring>& entries)
{
    std::wstring tnsnames;
    if (!GetTnsPath(tnsnames)) return;

    std::ifstream in(tnsnames.c_str());
    string line, entry;
    int balance = 0;
    bool decription = false;

    while (getline(in, line) && balance >= 0)
    {
        for (auto it = line.begin(); it != line.end(); ++it)
        {
            if (*it == '#') break;
            
            if (!decription)
            {
                if (*it != '=')
                    entry += *it;
                else
                {
                    trim_symmetric(entry);
                    to_lower_str(entry.c_str(), entry);
                    entries.push_back(Common::wstr(entry));
                    entry.clear();
                    decription = true;
                }
            }
            else
            {
                switch (*it)
                {
                case '(': 
                    balance++; 
                    break;
                case ')': 
                    if (!--balance)
                        decription = false;
                    break;
                }
            }
        }
    }
}

void CConnectDlg::OnSave () 
try { EXCEPTION_FRAME;
    
    UpdateData();
    
    normalizeCurrent();

    int entry = m_data.Find(m_current);
    if (entry != -1)
    {
        m_data.SetConnectEntry(entry, m_current, m_masterPassword);
        m_profiles.OnUpdateEntry(entry);
        m_profiles.SelectEntry(entry);
    }
    else
    {
        m_data.AppendConnectEntry(m_current, m_masterPassword);
        m_profiles.OnAppendEntry();
    }
    m_last = m_current;

    m_profiles.Invalidate();
    UpdateData(FALSE); // if we extract password or alias from user
} 
_DEFAULT_HANDLER_

void CConnectDlg::OnCopyUsernameAlias ()
try { EXCEPTION_FRAME;

    UpdateData(TRUE);

    if (OpenClipboard() && EmptyClipboard())
    {
        std::wstring str = m_current.m_User;
        
        if (m_current.m_Direct)
            str += L"@" + m_current.m_Host + L":" + m_current.m_Port + L":" + m_current.m_Sid;
        else
            str += L"@" + m_current.m_Alias;

        if (!str.empty())
            Common::AppSetClipboardText(str.c_str(), str.length());

        CloseClipboard();
    }
} 
_DEFAULT_HANDLER_

void CConnectDlg::OnCopyUsernamePasswordAlias ()
try { EXCEPTION_FRAME;

    UpdateData(TRUE);

    if (OpenClipboard() && EmptyClipboard())
    {
        std::wstring str = m_current.m_User + L"/" + m_current.m_DecryptedPassword;
        
        if (m_current.m_Direct)
            str += L"@" + m_current.m_Host + L":" + m_current.m_Port + L":" + m_current.m_Sid;
        else
            str += L"@" + m_current.m_Alias;

        if (!str.empty())
            Common::AppSetClipboardText(str.c_str(), str.length());

        CloseClipboard();
    }
} 
_DEFAULT_HANDLER_

void CConnectDlg::OnMakeAndCopyAlias ()     
try { EXCEPTION_FRAME;

    UpdateData(TRUE);

    if (OpenClipboard() && EmptyClipboard())
    {
        std::string str;
        
        OciConnect::MakeTNSString(str, 
            Common::str(m_current.m_Host).c_str(), 
            Common::str(m_current.m_Port).c_str(), 
            Common::str(m_current.m_Sid).c_str(), 
            m_current.m_UseService);
        
        str = Common::str(m_current.m_Sid) + "=" + str;

        if (!str.empty())
        {
            std::wstring wstr = Common::wstr(str);
            Common::AppSetClipboardText(wstr.c_str(), wstr.length());
        }

        CloseClipboard();
    }
} 
_DEFAULT_HANDLER_

void CConnectDlg::OnCleanup ()
try { EXCEPTION_FRAME;

    bool found = false;
    int count = m_profiles.GetItemCount();
    int index = m_profiles.GetNextItem(-1, LVNI_SELECTED);

    while (index != -1 && index < count)
    {
        std::wstring error;
        bool ok = CConnectDlg::doTest(false, error); 

        if (!ok)
        {
            found = true;

            std::wstring name;
            getConnectionDisplayName(name);

            MessageBeep((UINT)-1);

            switch (MessageBox((L"The connection \"" + name + L"\" is invalid.     "
                L"\n\n" + error + 
                L"\n<Yes> \t- to delete,"
                L"\n<No> \t- to skip and find the next invalid or"
                L"\n<Cancel> - to stop cleanup.").c_str(), 
                L"Connection cleanup", 
                MB_YESNOCANCEL|MB_ICONEXCLAMATION))
            {
            case IDYES:
                {
                    int entry = (int)m_profiles.GetItemData(index);
                    m_data.MarkedDeleted(entry);
                    m_profiles.OnDeleteEntry(entry);
                    --count;
                }
                break;
            case IDNO:
                m_profiles.SetItemState(++index, LVIS_SELECTED|LVNI_FOCUSED, LVIS_SELECTED|LVNI_FOCUSED);
                break;
            default:
                return;
            }
        }
        else
            m_profiles.SetItemState(++index, LVIS_SELECTED|LVNI_FOCUSED, LVIS_SELECTED|LVNI_FOCUSED);

        m_profiles.EnsureVisible(index, FALSE);
        m_profiles.Update(TRUE);
        UpdateWindow();
    }

    if (!found)
    {
        MessageBeep(MB_OK);
        MessageBox(L"There is no invalid connection.", L"Connection cleanup", MB_OK|MB_ICONINFORMATION);
    }

} 
_DEFAULT_HANDLER_


