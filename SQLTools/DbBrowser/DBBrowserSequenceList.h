/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015, 2018 Aleksey Kochetov

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

    struct SequenceEntry
    {
        string owner        ; // HIDDEN
        string sequence_name; // "Name"
        string last_number  ; // "Last Number"
        string min_value    ; // "Min Value"
        string max_value    ; // "Max Value"
        string increment_by ; // "Interval"
        string cache_size   ; // "Cache"
        string cycle_flag   ; // "Cycle"
        string order_flag   ; // "Order"
        string generated    ;
        string created      ;
        string last_ddl_time;

        SequenceEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<SequenceEntry> SequenceCollection;

    class SequenceListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const SequenceCollection& m_entries;

        static int compOraIntStr (const std::string& s1, const std::string& s2) 
        {  
            int len = max(s1.size(), s2.size());
            string t1(len-s1.size(), '0');t1 += s1;
            string t2(len-s2.size(), '0'); t2 += s2;
            return stricmp(t1.c_str(), t2.c_str()); 
        }

    public:
        SequenceListAdapter (const SequenceCollection& entries) : m_entries(entries) {}

        const SequenceEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 11; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; //string sequence_name;
            case 1: return Number; //__int64 last_number; 
            case 2: return Number; //__int64 min_value;   
            case 3: return Number; //__int64 max_value;   
            case 4: return Number; //__int64 increment_by;
            case 5: return Number; //string cache_size;   
            case 6: return String; //string cycle_flag;   
            case 7: return String; //string order_flag;   
            case 8: return String;// string generated;    
            case 9: return String;// string created;      
            case 10: return String;// string last_ddl_time;
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "Name"        ;
            case 1: return "Last Number" ;
            case 2: return "Min Value"   ;
            case 3: return "Max Value"   ;
            case 4: return "Interval"    ;
            case 5: return "Cache"       ;
            case 6: return "Cycle"       ;
            case 7: return "Order"       ;
            case 8: return "Generated"   ;
            case 9: return "Created"     ;
            case 10:return "Modified"    ;
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).sequence_name);
            case 1: return getStr(data(row).last_number  );   
            case 2: return getStr(data(row).min_value    );
            case 3: return getStr(data(row).max_value    );
            case 4: return getStr(data(row).increment_by );  
            case 5: return getStr(data(row).cache_size   );     
            case 6: return getStr(data(row).cycle_flag   ); 
            case 7: return getStr(data(row).order_flag   );  
            case 8: return getStr(data(row).generated    );    
            case 9: return getStr(data(row).created      );      
            case 10:return getStr(data(row).last_ddl_time);
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).sequence_name         , data(row2).sequence_name);
            case 1: return compOraIntStr(data(row1).last_number  , data(row2).last_number  );
            case 2: return compOraIntStr(data(row1).min_value    , data(row2).min_value    );
            case 3: return compOraIntStr(data(row1).max_value    , data(row2).max_value    );
            case 4: return compOraIntStr(data(row1).increment_by , data(row2).increment_by );
            case 5: return compOraIntStr(data(row1).cache_size   , data(row2).cache_size   );
            case 6: return comp(data(row1).cycle_flag            , data(row2).cycle_flag   );
            case 7: return comp(data(row1).order_flag            , data(row2).order_flag   );
            case 8: return comp(data(row1).generated             , data(row2).generated    );
            case 9: return comp(data(row1).created               , data(row2).created      );
            case 10:return comp(data(row1).last_ddl_time         , data(row2).last_ddl_time);
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_SEQUENCE; }
    };


class DBBrowserSequenceList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserSequenceList, SequenceCollection>;
    SequenceCollection m_data;
    SequenceListAdapter m_dataAdapter;

public:
	DBBrowserSequenceList ();
	virtual ~DBBrowserSequenceList();

    virtual const char* GetTitle () const       { return "Sequences"; };
    virtual int         GetImageIndex () const  { return IDII_SEQUENCE; }
    virtual const char* GetMonoType () const    { return "SEQUENCE"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).sequence_name.c_str(); };

    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

	virtual void ExtendContexMenu (CMenu* pMenu);

protected:
	DECLARE_MESSAGE_MAP()
};
