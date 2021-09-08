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


    struct ObjectEntry
    {
        string owner        ;   //"owner"         HIDDEN
        string object_name  ;   //"object_name"   "Name"
        string object_type  ;
        tm     created      ;   //"created"       "Created"
        tm     last_ddl_time;   //"last_ddl_time" "Modified"
        string status       ;   //"status"        "Status"

        int      image_index;   //hidden

        ObjectEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<ObjectEntry> ObjectCollection;

    class InvalidObjectListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const ObjectCollection& m_entries;

    public:
        InvalidObjectListAdapter (const ObjectCollection& entries) : m_entries(entries) {}

        const ObjectEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 5; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; //object_name  
            case 1: return String; //object_type  
            case 2: return Date;   //created      
            case 3: return Date;   //last_ddl_time
            case 4: return String; //status       
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Name"      ; //object_name  
            case 1: return L"Type"      ; //object_type  
            case 2: return L"Created"   ; //created      
            case 3: return L"Modifies"  ; //last_ddl_time
            case 4: return L"Status"    ; //status       
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).object_name  );
            case 1: return getStr(data(row).object_type  );
            case 2: return getStr(data(row).created      );
            case 3: return getStr(data(row).last_ddl_time);
            case 4: return getStr(data(row).status       );   
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).object_name  , data(row2).object_name  );
            case 1: return comp(data(row1).object_type  , data(row2).object_type  );
            case 2: return comp(data(row1).created      , data(row2).created      );
            case 3: return comp(data(row1).last_ddl_time, data(row2).last_ddl_time);
            case 4: return comp(data(row1).status       , data(row2).status       );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int row) const { return data(row).image_index; }
        virtual int getStateImageIndex (int row) const { return !stricmp(data(row).status.c_str(), "VALID") ? -1 : 1; }
    };


class DBBrowserInvalidObjectList : public DbBrowserList
{
protected:
    friend struct BackgroundTask_Refresh_Templ<DBBrowserInvalidObjectList, ObjectCollection>;
    friend struct BackgroundTask_RefreshInvalidObjectListEntry;
    ObjectCollection m_data;
    InvalidObjectListAdapter m_dataAdapter;

public:
	DBBrowserInvalidObjectList ();
	virtual ~DBBrowserInvalidObjectList();

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).object_name.c_str(); };
                                            
    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }
    //virtual void ApplyQuickFilter  (bool valid, bool invalid);

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

    virtual bool IsQuickFilterSupported () const  { return false; }

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

    virtual const char* GetTitle () const       { return "Invalid Objects"; };
    virtual int         GetImageIndex () const  { return IDII_INVALID_OBJECT; }
    virtual const char* GetObjectType (unsigned int inx) const  { return m_data.at(inx).object_type.c_str(); }
    virtual const char* GetMonoType () const    { return ""; }

protected:
    virtual const char* refineDoSqlForSel (const char* sttm, unsigned int dataInx);

	DECLARE_MESSAGE_MAP()
    virtual void OnCompile ();
    void OnCompileAll ();
    virtual void OnShowInObjectView ();
};



