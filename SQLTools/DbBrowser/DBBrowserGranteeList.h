/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

#include "DbBrowser\DbBrowserList.h"
#include "COMMON\ListCtrlDataProvider.h"

    struct GranteeEntry
    {
        string grantee ; // "Grantee" 

        GranteeEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<GranteeEntry> GranteeCollection;

    class GranteeListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const GranteeCollection& m_entries;

    public:
        GranteeListAdapter (const GranteeCollection& entries) : m_entries(entries) {}

        const GranteeEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 1; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // string grantee;  
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Grantee";
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).grantee);
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).grantee, data(row2).grantee);
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_GRANTEE; }
    };


class DBBrowserGranteeList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserGranteeList, GranteeCollection>;
    GranteeCollection m_data;
    GranteeListAdapter m_dataAdapter;

public:
	DBBrowserGranteeList ();
	virtual ~DBBrowserGranteeList();

    virtual const char* GetTitle () const       { return "Grantee"; };
    virtual int         GetImageIndex () const  { return IDII_GRANTEE; }
    virtual const char* GetMonoType () const    { return "GRANTEE"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).grantee.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()
    virtual void OnShowInObjectView ();
};
