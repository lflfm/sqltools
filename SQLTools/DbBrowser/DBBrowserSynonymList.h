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

    struct SynonymEntry
    {
        string owner        ; // HIDDEN
        string synonym_name ; // "Synonym"
        string table_owner  ; // "Owner", 
        string table_name   ; // "Object",
        string db_link      ; // "DB Link"
        string status       ; // "Status"

        SynonymEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<SynonymEntry> SynonymCollection;

    class SynonymListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const SynonymCollection& m_entries;

    public:
        SynonymListAdapter (const SynonymCollection& entries) : m_entries(entries) {}

        const SynonymEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 5; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; // string synonym_name;
            case 1: return String; // string table_owner ;
            case 2: return String; // string table_name  ;
            case 3: return String; // string db_link     ;
            case 4: return String; // string status      ;
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "Synonym";
            case 1: return "Owner"  ;
            case 2: return "Object" ;
            case 3: return "DB Link";
            case 4: return "Status" ;
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).synonym_name);
            case 1: return getStr(data(row).table_owner );   
            case 2: return getStr(data(row).table_name  );
            case 3: return getStr(data(row).db_link     );
            case 4: return getStr(data(row).status      );  
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).synonym_name, data(row2).synonym_name);
            case 1: return comp(data(row1).table_owner , data(row2).table_owner );
            case 2: return comp(data(row1).table_name  , data(row2).table_name  );
            case 3: return comp(data(row1).db_link     , data(row2).db_link     );
            case 4: return comp(data(row1).status      , data(row2).status      );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_SYNONYM; }
        virtual int getStateImageIndex (int row) const 
        { 
            return !stricmp(data(row).status.c_str(), "VALID") ? -1
                : !stricmp(data(row).status.c_str(), "NA") ? 2 : 1;
        }
    };


class DBBrowserSynonymList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserSynonymList, SynonymCollection>;
    friend struct BackgroundTask_RefreshSynonymListEntry;
    SynonymCollection m_data;
    SynonymListAdapter m_dataAdapter;

public:
	DBBrowserSynonymList ();
	virtual ~DBBrowserSynonymList();

    virtual const char* GetTitle () const       { return "Synonyms"; };
    virtual int         GetImageIndex () const  { return IDII_SYNONYM; }
    virtual const char* GetMonoType () const    { return "SYNONYM"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).synonym_name.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }
    virtual void ApplyQuickFilter  (bool valid, bool invalid);
    virtual bool IsQuickFilterSupported () const  { return true; }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()
    virtual void OnCompile();
};
