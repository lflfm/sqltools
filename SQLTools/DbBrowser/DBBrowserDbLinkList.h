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

    struct DbLinkEntry
    {
        string owner   ; // HIDDEN
        string db_link ; // "Db Link" 
        string username; // "Username"
        string host    ; // "Host"

        DbLinkEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<DbLinkEntry> DbLinkCollection;

    class DbLinkListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const DbLinkCollection& m_entries;

    public:
        DbLinkListAdapter (const DbLinkCollection& entries) : m_entries(entries) {}

        const DbLinkEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 3; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // string db_link ;  
            case 1: return String; // string username;  
            case 2: return String; // string host    ; 
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "Db Link";
            case 1: return "Username";
            case 2: return "Host";
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).db_link );
            case 1: return getStr(data(row).username);   
            case 2: return getStr(data(row).host    );
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).db_link , data(row2).db_link );
            case 1: return comp(data(row1).username, data(row2).username);
            case 2: return comp(data(row1).host    , data(row2).host    );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_DBLINK; }
    };


class DBBrowserDbLinkList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserDbLinkList, DbLinkCollection>;
    DbLinkCollection m_data;
    DbLinkListAdapter m_dataAdapter;

public:
	DBBrowserDbLinkList ();
	virtual ~DBBrowserDbLinkList();

    virtual const char* GetTitle () const       { return "DB Links"; };
    virtual int         GetImageIndex () const  { return IDII_DBLINK; }
    virtual const char* GetMonoType () const    { return "DATABASE LINK"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).db_link.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }
    virtual void MakeDropSql (int entry, std::string& sql) const;

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()
    virtual void OnShowInObjectView ();
};
