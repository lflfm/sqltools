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


    struct CodeEntry
    {
        string owner        ;   //"owner"         HIDDEN
        string object_name  ;   //"object_name"   "Name"
        tm     created      ;   //"created"       "Created"
        tm     last_ddl_time;   //"last_ddl_time" "Modified"
        string status       ;   //"status"        "Status"

        CodeEntry () : deleted(false) {}
        bool deleted;
    };

    typedef std::vector<CodeEntry> CodeCollection;

    class CodeListAdapter : public Common::ListCtrlDataProvider, Common::ListCtrlDataProviderHelper
    {

        const CodeCollection& m_entries;
        int m_imageIndex;

    public:
        CodeListAdapter (const CodeCollection& entries) : m_entries(entries), m_imageIndex(-1) {}

        void SetImageIndex (int imageIndex) { m_imageIndex = imageIndex; }

        const CodeEntry& data (int row) const { return m_entries.at(row); }

        virtual int getRowCount () const { return (int)m_entries.size(); }

        virtual int getColCount () const { return 4; }

        virtual Type getColType (int col) const {
            switch (col) {
            case 0: return String; //object_name  
            case 1: return Date;   //created      
            case 2: return Date;   //last_ddl_time
            case 3: return String; //status       
            }
            return String;
        }

        virtual const char* getColHeader (int col) const {
            switch (col) {
            case 0: return "Name"      ; //object_name  
            case 1: return "Created"   ; //created      
            case 2: return "Modifies"  ; //last_ddl_time
            case 3: return "Status"    ; //status       
            }
            return "Unknown";
        }

        virtual const char* getString (int row, int col) const {
            switch (col) {
            case 0: return getStr(data(row).object_name  );
            case 1: return getStr(data(row).created      );
            case 2: return getStr(data(row).last_ddl_time);
            case 3: return getStr(data(row).status       );   
            }
            return "Unknown";
        }

        bool IsVisibleRow (int row) const {
            return !data(row).deleted;
        }

        virtual int compare (int row1, int row2, int col) const {
            switch (col) {
            case 0: return comp(data(row1).object_name  , data(row2).object_name  );
            case 1: return comp(data(row1).created      , data(row2).created      );
            case 2: return comp(data(row1).last_ddl_time, data(row2).last_ddl_time);
            case 3: return comp(data(row1).status       , data(row2).status       );
            }
            return 0;
        }

        virtual int GetMinDefColWidth (int col) const { 
            return !col ? 200 : Common::ListCtrlDataProvider::GetMinDefColWidth(col);
        }

        virtual int getImageIndex (int /*row*/) const { return m_imageIndex; }
        virtual int getStateImageIndex (int row) const { return !stricmp(data(row).status.c_str(), "VALID") ? -1 : 1; }
    };


class DBBrowserCodeList : public DbBrowserList
{
protected:
    friend struct BackgroundTask_Refresh_Templ<DBBrowserCodeList, CodeCollection>;
    friend struct BackgroundTask_RefreshCodeListEntry;
    CodeCollection m_data;
    CodeListAdapter m_dataAdapter;

public:
	DBBrowserCodeList ();
	virtual ~DBBrowserCodeList();

    virtual unsigned int GetTotalEntryCount () const            { return m_data.size(); }
    virtual const char* GetObjectName (unsigned int inx) const  { return m_data.at(inx).object_name.c_str(); };
                                            
    virtual void Refresh      (bool force = false);
    virtual void RefreshEntry (int entry);
    virtual void ClearData ()                                   { m_data.clear(); }
    virtual void ApplyQuickFilter  (bool valid, bool invalid);

    virtual void MarkAsDeleted (int entry) { m_data.at(entry).deleted = true; }

    virtual bool IsQuickFilterSupported () const  { return true; }

    virtual UINT GetDefaultContextCommand (bool /*bAltPressed*/) const { return ID_SQL_LOAD; }
	virtual void ExtendContexMenu (CMenu* pMenu);

    virtual const char* GetMonoType () const = 0;

protected:
	DECLARE_MESSAGE_MAP()
    virtual void OnCompile();
};


class DBBrowserProcedureList : public DBBrowserCodeList
{
public:
    DBBrowserProcedureList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Procedures"; };
    virtual int         GetImageIndex () const  { return IDII_PROCEDURE; }
    virtual const char* GetMonoType () const    { return "PROCEDURE"; }
};

class DBBrowserFunctionList : public DBBrowserCodeList
{
public:
    DBBrowserFunctionList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Functions"; };
    virtual int         GetImageIndex () const  { return IDII_FUNCTION; }
    virtual const char* GetMonoType () const    { return "FUNCTION"; }
};

class DBBrowserPackageList : public DBBrowserCodeList
{
public:
    DBBrowserPackageList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Packages"; };
    virtual int         GetImageIndex () const  { return IDII_PACKAGE; }
    virtual const char* GetMonoType () const    { return "PACKAGE"; }
};

class DBBrowserPackageBodyList : public DBBrowserCodeList
{
public:
    DBBrowserPackageBodyList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Package Bodies"; };
    virtual int         GetImageIndex () const  { return IDII_PACKAGE_BODY; }
    virtual const char* GetMonoType () const    { return "PACKAGE BODY"; }

    virtual void OnCompile();
};

class DBBrowserJavaList : public DBBrowserCodeList
{
public:
    DBBrowserJavaList () 
    { 
        m_dataAdapter.SetImageIndex(GetImageIndex()); 
    }

    virtual const char* GetTitle () const       { return "Java"; };
    virtual int         GetImageIndex () const  { return IDII_JAVA; }
    virtual const char* GetMonoType () const    { return "JAVA SOURCE"; }

    virtual void ExtendContexMenu (CMenu* pMenu);
    virtual void OnShowInObjectView () {};
};

//class DBBrowserTypeList : public DBBrowserCodeList
//{
//public:
//    DBBrowserTypeList () 
//    { 
//        m_dataAdapter.SetImageIndex(GetImageIndex()); 
//    }
//
//    virtual const char* GetTitle () const       { return "Types"; };
//    virtual int         GetImageIndex () const  { return IDII_TYPE; }
//    virtual const char* GetMonoType () const   { return "TYPE"; }
//};
//
//class DBBrowserTypeBodyList : public DBBrowserCodeList
//{
//public:
//    DBBrowserTypeBodyList () 
//    { 
//        m_dataAdapter.SetImageIndex(GetImageIndex()); 
//    }
//
//    virtual const char* GetTitle () const       { return "Type Bodies"; };
//    virtual int         GetImageIndex () const  { return IDII_TYPE_BODY; }
//    virtual const char* GetMonoType () const    { return "TYPE BODY"; }
//
//    virtual void OnCompile();
//};

