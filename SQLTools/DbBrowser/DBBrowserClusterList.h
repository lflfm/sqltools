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

    struct ClusterEntry
    {
        string owner          ;  //HIDDEN
        string cluster_name   ;  //"Name"
        string cluster_type   ;  //"Type"
        string function       ;  //"Function"
        int    hashkeys       ;  //"Hashkeys"
        int    key_size       ;  //"Key size"
        string tablespace_name;  //"Tablespace"
        int    initial_extent ;  //"Initial"
        int    next_extent    ;  //"Next"
        int    pct_increase   ;  //"Increase"
        int    max_extents    ;  //"Max Ext"
        int    pct_free       ;  //"Pct Free"
        int    pct_used       ;  //"Pct Used"

        ClusterEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<ClusterEntry> ClusterCollection;

    class ClusterListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {
        const ClusterCollection& m_entries;

    public:
        ClusterListAdapter (const ClusterCollection& entries) : m_entries(entries) {}

        const ClusterEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 12; }

        virtual Type getColType (int col) const {
            switch (col) {
            case  0: return String; // string cluster_name   
            case  1: return String; // string cluster_type   
            case  2: return String; // string function       
            case  3: return Number; // int    hashkeys       
            case  4: return Number; // int    key_size       
            case  5: return String; // string tablespace_name
            case  6: return Number; // int    initial_extent 
            case  7: return Number; // int    next_extent    
            case  8: return Number; // int    pct_increase   
            case  9: return Number; // int    max_extents    
            case 10: return Number; // int    pct_free       
            case 11: return Number; // int    pct_used       
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case  0: return "Name";
            case  1: return "Type";
            case  2: return "Function";
            case  3: return "Hashkeys";
            case  4: return "Key size";
            case  5: return "Tablespace";
            case  6: return "Initial";
            case  7: return "Next";
            case  8: return "Increase";
            case  9: return "Max Ext";
            case 10: return "Pct Free";
            case 11: return "Pct Used";
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case  0:  return getStr(data(row).cluster_name   );
            case  1:  return getStr(data(row).cluster_type   );   
            case  2:  return getStr(data(row).function       );
            case  3:  return getStr(data(row).hashkeys       );
            case  4:  return getStr(data(row).key_size       );  
            case  5:  return getStr(data(row).tablespace_name); 
            case  6:  return getStr(data(row).initial_extent );  
            case  7:  return getStr(data(row).next_extent    );     
            case  8:  return getStr(data(row).pct_increase   );     
            case  9:  return getStr(data(row).max_extents    ); 
            case 10:  return getStr(data(row).pct_free       ); 
            case 11:  return getStr(data(row).pct_used       ); 
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case  0: return comp(data(row1).cluster_name   , data(row2).cluster_name   );
            case  1: return comp(data(row1).cluster_type   , data(row2).cluster_type   );
            case  2: return comp(data(row1).function       , data(row2).function       );
            case  3: return comp(data(row1).hashkeys       , data(row2).hashkeys       );
            case  4: return comp(data(row1).key_size       , data(row2).key_size       );
            case  5: return comp(data(row1).tablespace_name, data(row2).tablespace_name);
            case  6: return comp(data(row1).initial_extent , data(row2).initial_extent );
            case  7: return comp(data(row1).next_extent    , data(row2).next_extent    );
            case  8: return comp(data(row1).pct_increase   , data(row2).pct_increase   );
            case  9: return comp(data(row1).max_extents    , data(row2).max_extents    );
            case 10: return comp(data(row1).pct_free       , data(row2).pct_free       );
            case 11: return comp(data(row1).pct_used       , data(row2).pct_used       );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return IDII_CLUSTER; }
    };


class DBBrowserClusterList : public DbBrowserList
{
    friend struct BackgroundTask_Refresh_Templ<DBBrowserClusterList, ClusterCollection>;

    ClusterCollection m_data;
    ClusterListAdapter m_dataAdapter;

public:
	DBBrowserClusterList ();
	virtual ~DBBrowserClusterList();

    virtual const char* GetTitle () const       { return "Clusters"; };
    virtual int         GetImageIndex () const  { return IDII_CLUSTER; }
    virtual const char* GetMonoType () const    { return "CLUSTER"; }

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).cluster_name.c_str(); };

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
