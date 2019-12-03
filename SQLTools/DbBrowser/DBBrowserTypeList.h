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


    struct TypeCodeEntry
    {
        string owner        ;   //"owner"         HIDDEN
        string object_name  ;   //"object_name"   "Name"

        string typecode     ;
        int    attributes   ;
        int    methods      ;
        string incomplete   ;

        tm     created      ;   //"created"       "Created"
        tm     last_ddl_time;   //"last_ddl_time" "Modified"
        string status       ;   //"status"        "Status"

        TypeCodeEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<TypeCodeEntry> TypeCodeCollection;

    class TypeCodeListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {

        const TypeCodeCollection& m_entries;
        int m_imageIndex;

    public:
        TypeCodeListAdapter (const TypeCodeCollection& entries) : m_entries(entries), m_imageIndex(-1) {}

        void SetImageIndex (int imageIndex) { m_imageIndex = imageIndex; }

        const TypeCodeEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 8; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String;  //object_name  
            case 1: return String;  //typecode     
            case 2: return Number;  //attributes  
            case 3: return Number;  //methods      
            case 4: return String;  //incomplete   
            case 5: return Date;    //created      
            case 6: return Date;    //last_ddl_time
            case 7: return String;  //status       
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "Name";      //object_name  
            case 1: return "Type";      //typecode     
            case 2: return "Attrs";     //attributes  
            case 3: return "Methods";   //methods      
            case 4: return "Incomplete";//incomplete   
            case 5: return "Created";   //created      
            case 6: return "Modified";  //last_ddl_time
            case 7: return "Status";    //status       
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).object_name  );
            case 1: return getStr(data(row).typecode     );
            case 2: return getStr(data(row).attributes   );
            case 3: return getStr(data(row).methods      );
            case 4: return getStr(data(row).incomplete   );
            case 5: return getStr(data(row).created      );
            case 6: return getStr(data(row).last_ddl_time);
            case 7: return getStr(data(row).status       );
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).object_name  , data(row2).object_name  );
            case 1: return comp(data(row1).typecode     , data(row2).typecode     );
            case 2: return comp(data(row1).attributes   , data(row2).attributes   );
            case 3: return comp(data(row1).methods      , data(row2).methods      );
            case 4: return comp(data(row1).incomplete   , data(row2).incomplete   );
            case 5: return comp(data(row1).created      , data(row2).created      );
            case 6: return comp(data(row1).last_ddl_time, data(row2).last_ddl_time);
            case 7: return comp(data(row1).status       , data(row2).status       );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return m_imageIndex; }
        virtual int getStateImageIndex (int row) const { return !stricmp(data(row).status.c_str(), "VALID") ? -1 : 1; }
    };


class DBBrowserTypeCodeList : public DbBrowserList
{
protected:
    friend struct BackgroundTask_Refresh_Templ<DBBrowserTypeCodeList, TypeCodeCollection>;
    friend struct BackgroundTask_RefreshTypeCodeListEntry;
    TypeCodeCollection m_data;
    TypeCodeListAdapter m_dataAdapter;

public:
	DBBrowserTypeCodeList ();
	virtual ~DBBrowserTypeCodeList();

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).object_name.c_str(); };
                                            
    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }
    virtual void ApplyQuickFilter  (bool valid, bool invalid);

    virtual bool IsQuickFilterSupported () const  { return true; }

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

    virtual const char* GetMonoType () const = 0;

protected:
	DECLARE_MESSAGE_MAP()
    virtual void OnCompile();
};


class DBBrowserTypeList : public DBBrowserTypeCodeList
{
public:
    DBBrowserTypeList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Types"; };
    virtual int         GetImageIndex () const  { return IDII_TYPE; }
    virtual const char* GetMonoType () const    { return "TYPE"; }
};

class DBBrowserTypeBodyList : public DBBrowserTypeCodeList
{
public:
    DBBrowserTypeBodyList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Type Bodies"; };
    virtual int         GetImageIndex () const  { return IDII_TYPE_BODY; }
    virtual const char* GetMonoType () const    { return "TYPE BODY"; }

    virtual void OnCompile();
};

