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

    struct ViewEntry
    {
        string owner        ; // HIDDEN
        string view_name    ; // "Name"
        int    text_length  ; // "Text Length"
        tm     created      ; // "Created"
        tm     last_ddl_time; // "Modified"
        string status       ; // "Status"

        ViewEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<ViewEntry> ViewCollection;

    class ViewListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const ViewCollection& m_entries;

    public:
        ViewListAdapter (const ViewCollection& entries) : m_entries(entries) {}

        const ViewEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 5; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // string view_name     ;
            case 1: return Number; // int    text_length   ;
            case 2: return Date  ; // tm     created       ;
            case 3: return Date  ; // tm     last_ddl_time ;
            case 4: return String; // string status        ;
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "Name";
            case 1: return "Text Length";
            case 2: return "Created";
            case 3: return "Modified";
            case 4: return "Status";
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).view_name    );
            case 1: return getStr(data(row).text_length  );   
            case 2: return getStr(data(row).created      );
            case 3: return getStr(data(row).last_ddl_time);
            case 4: return getStr(data(row).status       );  
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).view_name    , data(row2).view_name    );
            case 1: return comp(data(row1).text_length  , data(row2).text_length  );
            case 2: return comp(data(row1).created      , data(row2).created      );
            case 3: return comp(data(row1).last_ddl_time, data(row2).last_ddl_time);
            case 4: return comp(data(row1).status       , data(row2).status       );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_VIEW; }
        virtual int getStateImageIndex (int row) const { return !stricmp(data(row).status.c_str(), "VALID") ? -1 : 1; }
    };


class DBBrowserViewList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserViewList, ViewCollection>;
    friend struct BackgroundTask_RefreshViewListEntry;
    ViewCollection m_data;
    ViewListAdapter m_dataAdapter;

public:
	DBBrowserViewList ();
	virtual ~DBBrowserViewList();

    virtual const char* GetTitle () const       { return "Views"; };
    virtual int         GetImageIndex () const  { return IDII_VIEW; }
    virtual const char* GetMonoType () const   { return "VIEW"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).view_name.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }
    virtual void ApplyQuickFilter  (bool valid, bool invalid);
    virtual bool IsQuickFilterSupported () const  { return true; }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()
    void OnCompile();
    afx_msg void OnQuery ();
};
