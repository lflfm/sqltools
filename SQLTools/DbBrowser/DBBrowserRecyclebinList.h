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

    struct RecyclebinEntry
    {
        string  original_name ; // "Name"
        string  operation     ; // "Oper"
        string  type          ; // "Type"
        string  partition_name; // "Partition"
        string  ts_name       ; // "Tablespace"
        string  createtime    ; // "Created" it is varchar2 column for some reason
        string  droptime      ; // "Dropped" it is varchar2 column for some reason
        string  can_undrop    ; // "Can Undrop"
        string  can_purge     ; // "Can Purge"
        int     space         ; // "Space (blk)"

        // HIDDEN
        string  object_name; 
        string  dropscn;
        string  related; 
        string  base_object;
        string  purge_object; 

        int image_index;

        RecyclebinEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<RecyclebinEntry> RecyclebinCollection;

    class RecyclebinListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const RecyclebinCollection& m_entries;

    public:
        RecyclebinListAdapter (const RecyclebinCollection& entries) : m_entries(entries) {}

        const RecyclebinEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 10; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // "Name"
            case 1: return String; // "Oper"
            case 2: return String; // "Type"
            case 3: return String; // "Partition"
            case 4: return String; // "Tablespace"
            case 5: return String; // "Created"
            case 6: return String; // "Dropped"
            case 7: return String; // "Can Undrop"
            case 8: return String; // "Can Purge"
            case 9: return Number; // "Space (blk)"
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Name"       ;
            case 1: return L"Oper"       ;
            case 2: return L"Type"       ;
            case 3: return L"Partition"  ;
            case 4: return L"Tablespace" ;
            case 5: return L"Created"    ;
            case 6: return L"Dropped"    ;
            case 7: return L"Can Undrop" ;
            case 8: return L"Can Purge"  ;
            case 9: return L"Space (blk)";
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).original_name );
            case 1: return getStr(data(row).operation     );
            case 2: return getStr(data(row).type          );
            case 3: return getStr(data(row).partition_name);
            case 4: return getStr(data(row).ts_name       );
            case 5: return getStr(data(row).createtime    );
            case 6: return getStr(data(row).droptime      );
            case 7: return getStr(data(row).can_undrop    );
            case 8: return getStr(data(row).can_purge     );
            case 9: return getStr(data(row).space         );
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).original_name , data(row2).original_name );
            case 1: return comp(data(row1).operation     , data(row2).operation     );
            case 2: return comp(data(row1).type          , data(row2).type          );
            case 3: return comp(data(row1).partition_name, data(row2).partition_name);
            case 4: return comp(data(row1).ts_name       , data(row2).ts_name       );
            case 5: return comp(data(row1).createtime    , data(row2).createtime    );
            case 6: return comp(data(row1).droptime      , data(row2).droptime      );
            case 7: return comp(data(row1).can_undrop    , data(row2).can_undrop    );
            case 8: return comp(data(row1).can_purge     , data(row2).can_purge     );
            case 9: return comp(data(row1).space         , data(row2).space         );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int row) const { return data(row).image_index; }
    };


class DBBrowserRecyclebinList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserRecyclebinList, RecyclebinCollection>;
    friend struct BackgroundTask_ListPurge;
    friend struct BackgroundTask_ListFlashback;

    RecyclebinCollection m_data;
    RecyclebinListAdapter m_dataAdapter;

public:
	DBBrowserRecyclebinList ();
	virtual ~DBBrowserRecyclebinList();

    virtual const char* GetTitle () const       { return "Recyclebin"; };
    virtual int         GetImageIndex () const  { return IDII_RECYCLEBIN; }
    virtual const char* GetMonoType () const    { return ""; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).object_name.c_str(); };
    virtual const char* GetObjectType (unsigned int inx) const  { return m_data.at(inx).type.c_str(); }

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }
    virtual void DoDrop (int) {};

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_DS_FLASHBACK; }
	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()

    afx_msg void OnPurge();
    afx_msg void OnPurgeAll();
    afx_msg void OnFlashback();
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    virtual void OnShowInObjectView ();
};
