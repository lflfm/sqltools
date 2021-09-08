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

#pragma once

#include <vector>
#include "ConnectData\ConnectData.h"
#include "COMMON\ListCtrlDataProvider.h"
#include "COMMON\ManagedListCtrl.h"

    class CConnectSettingsDlg;

    class ConnectInfoAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        mutable wchar_t m_buffer[128];
        const int m_buff_size = sizeof(m_buffer)/sizeof(m_buffer[0]);

        const wchar_t* getAliasStr (const ConnectEntry& entry) const { 
            if (entry.GetDirect()) {
                wcsncpy(m_buffer, entry.GetSid().c_str(), m_buff_size);
                wcsncat(m_buffer, L":", m_buff_size);
                wcsncat(m_buffer, entry.GetHost().c_str(), m_buff_size);
                m_buffer[m_buff_size-1] = 0;
                return m_buffer; 
            }
            return getStr(entry.GetAlias());
        }

        static int compAliases (const ConnectEntry& entry1, const ConnectEntry& entry2) { 
            if (int r = comp(entry1.GetDirect() ? entry1.GetSid() : entry1.GetAlias(), entry2.GetDirect() ? entry2.GetSid() : entry2.GetAlias()))
                return r;
            return comp(entry1.GetDirect() ? entry1.GetHost() : std::wstring(), entry2.GetDirect() ? entry2.GetHost() : std::wstring());
        }

        const std::vector<ConnectEntry>& m_entries;

    public:
        ConnectInfoAdapter (const std::vector<ConnectEntry>& entries) : m_entries(entries) {}

        const ConnectEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 5; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String;
            case 1: return String;
            case 2: return String;
            case 3: return Number;
            case 4: return Date;
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Tag";       
            case 1: return L"User";       
            case 2: return L"Alias";
            case 3: return L"Count";      
            case 4: return L"Last usage";    
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).GetTag());
            case 1: return getStr(data(row).GetUser());
            case 2: return getAliasStr(data(row));
            case 3: return getStr(data(row).GetUsageCounter());
            case 4: return getStrTimeT(data(row).GetLastUsage());
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return data(row).m_status != ConnectEntry::DELETED;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).GetTag(), data(row2).GetTag());
            case 1: return comp(data(row1).GetUser(), data(row2).GetUser());
            case 2: return compAliases(data(row1), data(row2));
            case 3: return comp(data(row1).GetUsageCounter(), data(row2).GetUsageCounter());
            case 4: return compTimeT(data(row1).GetLastUsage(), data(row2).GetLastUsage());
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 40 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }
    };


/////////////////////////////////////////////////////////////////////////////
// CConnectDlg dialog

class CConnectDlg : public CDialog
{
    static std::wstring m_masterPassword;
    static ConnectEntry m_last;
    static int m_sortColumn;
    static Common::ListCtrlManager::ESortDir m_sortDirection;
    static Common::ListCtrlManager::FilterCollection m_filter;

    ConnectData m_data;
    ConnectEntry m_current;
    ConnectInfoAdapter m_adapter;
    Common::CManagedListCtrl m_profiles;
    CImageList m_sortImages;

    bool showDlg (CConnectSettingsDlg&);
    void readProfiles ();
    void setupProfiles ();
    void writeData ();
    void normalizeCurrent ();
    bool doTest (bool showMessage, std::wstring& error);
    void getConnectionDisplayName (std::wstring&);

    void setupConnectionType ();

	//{{AFX_DATA(CConnectDlg)
	enum { IDD = IDD_CONNECT_DIALOG };
	//}}AFX_DATA

public:
	CConnectDlg (CWnd* pParent = NULL);

    const ConnectEntry& GetConnectEntry () const { return m_current; }

protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);
public:
    virtual INT_PTR DoModal();
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
    virtual void OnCancel();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnItemChanged_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTest();
	afx_msg void OnDelete();
	afx_msg void OnDblClk_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRClick_Profiles(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClk_DirectConnectin ();
    afx_msg void OnSettings ();
    afx_msg void OnBnClicked_Help();
    afx_msg void OnInitFile ();
    afx_msg void OnSave ();

    afx_msg void OnCleanup ();
    afx_msg void OnCopyUsernameAlias ();
    afx_msg void OnCopyUsernamePasswordAlias ();
    afx_msg void OnMakeAndCopyAlias ();
};
