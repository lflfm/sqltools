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

    struct TriggerEntry
    {
        string owner            ; // HIDDEN
        string trigger_name     ; // "Name"      
        string table_owner      ; // "Table Owner"
        string table_name       ; // "Table"     
        string trigger_type     ; // "Type"      
        string triggering_event ; // "Event"     
        string when_clause      ; // "When"     
        tm     created          ; // "Created"   
        tm     last_ddl_time    ; // "Modified" 
        string enabled          ; // "Enabled"
        string status           ; // "Status"

        TriggerEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<TriggerEntry> TriggerCollection;

    class TriggerListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const TriggerCollection& m_entries;

    public:
        TriggerListAdapter (const TriggerCollection& entries) : m_entries(entries) {}

        const TriggerEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 10; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // string trigger_name     ;
            case 1: return String; // string table_owner      ;
            case 2: return String; // string table_name       ;
            case 3: return String; // string trigger_type     ;
            case 4: return String; // string triggering_event ;
            case 5: return String; // string when_clause      ;
            case 6: return Date;   // tm     created          ;
            case 7: return Date;   // tm     last_ddl_time    ;
            case 8: return String; // string enabled           ;
            case 9: return String; // string status           ;
            }
            return String;
        }

        virtual const wchar_t* getColHeader (int col) const {
            switch (col) {
            case 0: return L"Name"       ; 
            case 1: return L"Table Owner"; 
            case 2: return L"Table"      ; 
            case 3: return L"Type"       ; 
            case 4: return L"Event"      ; 
            case 5: return L"When"       ; 
            case 6: return L"Created"    ; 
            case 7: return L"Modified"   ; 
            case 8: return L"Enabled"    ; 
            case 9: return L"Status"     ; 
            }
            return L"Unknown";
        }

        virtual const wchar_t* getString (int row, int col) const {
            switch (col) {                             
            case 0: return getStr(data(row).trigger_name    );
            case 1: return getStr(data(row).table_owner     );   
            case 2: return getStr(data(row).table_name      );
            case 3: return getStr(data(row).trigger_type    );
            case 4: return getStr(data(row).triggering_event);  
            case 5: return getStr(data(row).when_clause     );  
            case 6: return getStr(data(row).created         );  
            case 7: return getStr(data(row).last_ddl_time   );  
            case 8: return getStr(data(row).enabled         );  
            case 9: return getStr(data(row).status          );  
            }
            return L"Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).trigger_name    , data(row2).trigger_name    );
            case 1: return comp(data(row1).table_owner     , data(row2).table_owner     );
            case 2: return comp(data(row1).table_name      , data(row2).table_name      );
            case 3: return comp(data(row1).trigger_type    , data(row2).trigger_type    );
            case 4: return comp(data(row1).triggering_event, data(row2).triggering_event);
            case 5: return comp(data(row1).when_clause     , data(row2).when_clause     );
            case 6: return comp(data(row1).created         , data(row2).created         );
            case 7: return comp(data(row1).last_ddl_time   , data(row2).last_ddl_time   );
            case 8: return comp(data(row1).enabled         , data(row2).enabled         );
            case 9: return comp(data(row1).status          , data(row2).status          );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_TRIGGER; }
        virtual int getStateImageIndex (int row) const 
        { 
            int v = !stricmp(data(row).status.c_str(), "VALID") ? 0 : 1;
            int e = !stricmp(data(row).enabled.c_str(), "ENABLED") ? 0 : 2;
            return (!v && !e) ? -1 : (v | e);
        }
    };


class DBBrowserTriggerList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserTriggerList, TriggerCollection>;
    friend struct BackgroundTask_RefreshTriggerListEntry;
    TriggerCollection m_data;
    TriggerListAdapter m_dataAdapter;

public:
	DBBrowserTriggerList ();
	virtual ~DBBrowserTriggerList();

    virtual const char* GetTitle () const       { return "Triggers"; };
    virtual int         GetImageIndex () const  { return IDII_TRIGGER; }
    virtual const char* GetMonoType () const    { return "TRIGGER"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).trigger_name.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }
    virtual void ApplyQuickFilter  (bool valid, bool invalid);
    virtual bool IsQuickFilterSupported () const  { return true; }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()
    afx_msg void OnCompile();
    afx_msg void OnEnable();
    afx_msg void OnDisable();
};
