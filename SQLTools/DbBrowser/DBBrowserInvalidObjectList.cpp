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

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DBBrowserInvalidObjectList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppGlobal.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <ActivePrimeExecutionNote.h>

using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserInvalidObjectList::DBBrowserInvalidObjectList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserInvalidObjectList::~DBBrowserInvalidObjectList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserInvalidObjectList, DbBrowserList)
    ON_COMMAND(ID_DS_COMPILE, OnCompile)
    ON_COMMAND(ID_DS_COMPILE_ALL, OnCompileAll)
END_MESSAGE_MAP()

    const int cn_owner         = 0;
    const int cn_object_name   = 1;   
    const int cn_object_type   = 2;   
    const int cn_status        = 3;
    const int cn_created       = 4;
    const int cn_last_ddl_time = 5;  

    static const char* csz_sttm =
        "SELECT"
          " owner,"        
          " object_name,"
          " object_type,"
          " status,"
          " created,"      
          " last_ddl_time"
        " FROM sys.all_objects"
          " WHERE owner = :owner"
            " AND status = 'INVALID'";

	struct BackgroundTask_RefreshInvalidObjectList 
        : BackgroundTask_Refresh_Templ<DBBrowserInvalidObjectList, ObjectCollection>
    {
        BackgroundTask_RefreshInvalidObjectList(DBBrowserInvalidObjectList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh InvalidObjectList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(csz_sttm);
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    ObjectEntry code;

                    code.owner       = cur.ToString(cn_owner      );
                    code.object_name = cur.ToString(cn_object_name);
                    code.object_type = cur.ToString(cn_object_type);
                    code.status      = cur.ToString(cn_status     );

                    cur.GetTime(cn_created,       code.created);
                    cur.GetTime(cn_last_ddl_time, code.last_ddl_time);

                    if (code.object_type == "FUNCTION")          code.image_index = IDII_FUNCTION    ;
                    else if (code.object_type == "PROCEDURE")    code.image_index = IDII_PROCEDURE   ;
                    else if (code.object_type == "PACKAGE")      code.image_index = IDII_PACKAGE     ;
                    else if (code.object_type == "PACKAGE BODY") code.image_index = IDII_PACKAGE_BODY;
                    else if (code.object_type == "TRIGGER")      code.image_index = IDII_TRIGGER     ;
                    else if (code.object_type == "VIEW")         code.image_index = IDII_VIEW        ;
                    else if (code.object_type == "SYNONYM")      code.image_index = IDII_SYNONYM     ;
                    else if (code.object_type == "TYPE")         code.image_index = IDII_TYPE        ;
                    else if (code.object_type == "TYPE_BODY")    code.image_index = IDII_TYPE_BODY   ;
                    else if (code.object_type == "JAVA SOURCE")  code.image_index = IDII_JAVA        ;
                    else if (code.object_type == "JAVA CLASS")   code.image_index = IDII_JAVA        ;
                    else code.image_index = -1;

                    m_data.push_back(code);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserInvalidObjectList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshInvalidObjectList(*this)));
        SetDirty(false);
    }
}

    static const char* csz_refresh_sttm =
        "SELECT"
          " owner,"        
          " object_name,"
          " object_type,"
          " status,"
          " created,"      
          " last_ddl_time"
        " FROM sys.all_objects"
          " WHERE owner = :owner"
          " AND object_name = :name"
          " AND object_type = :type";

	struct BackgroundTask_RefreshInvalidObjectListEntry : Task
    {
        int m_entry;
        ObjectEntry m_dataEntry;
        DBBrowserInvalidObjectList& m_list;
        string m_schema;

        BackgroundTask_RefreshInvalidObjectListEntry (DBBrowserInvalidObjectList& list, int entry)
            : Task("Refresh InvalidObjectEntry", list.GetParent()),
            m_list(list),
            m_entry(entry),
            m_dataEntry(list.m_data.at(entry))
        {
            SetSilent(true);
            m_schema = list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                OciCursor cur(connect, 1, 196);
                cur.Prepare(csz_refresh_sttm);
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type", m_dataEntry.object_type.c_str());
                cur.Bind(":name", m_dataEntry.object_name.c_str());

                cur.Execute();
                if (cur.Fetch())
                {
                    m_dataEntry.owner       = cur.ToString(cn_owner      );
                    m_dataEntry.object_name = cur.ToString(cn_object_name);
                    m_dataEntry.status      = cur.ToString(cn_status     );

                    cur.GetTime(cn_created,       m_dataEntry.created);
                    cur.GetTime(cn_last_ddl_time, m_dataEntry.last_ddl_time);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            if (!m_list.m_data.empty()) // if refresh immediatly after compile
            {
                m_list.m_data.at(m_entry) = m_dataEntry;
                m_list.OnUpdateEntry(m_entry);
            }
        }
    };


void DBBrowserInvalidObjectList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshInvalidObjectListEntry(*this, entry)));
}

//void DBBrowserInvalidObjectList::ApplyQuickFilter (bool valid, bool invalid) 
//{
//    ListCtrlManager::FilterCollection filter;
//    GetFilter(filter);
//
//    for (int i = 0, n = m_dataAdapter.getColCount(); i < n; ++i)
//        if (!stricmp(m_dataAdapter.getColHeader(i), "status"))
//        {
//            const char* val = 0;
//
//            if (valid && invalid)
//                val = "";
//            else if (valid)
//                val = "VALID";
//            else if  (invalid)
//                val = "INVALID";
//            else
//                val = "NA";
//
//            if (val != filter.at(i).value
//            || ListCtrlManager::EXACT_MATCH != filter.at(i).operation)
//            {
//                filter.at(i).value = val;
//                filter.at(i).operation = ListCtrlManager::EXACT_MATCH;
//                SetFilter(filter);
//                break;
//            }
//        }
//}

void DBBrowserInvalidObjectList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_COMPILE,     L"&Compile");                      
    pMenu->AppendMenu(MF_STRING,        ID_DS_COMPILE_ALL, L"Compile All with Dbms_&Utility");
    pMenu->AppendMenu(MF_SEPARATOR);
}

const char* DBBrowserInvalidObjectList::refineDoSqlForSel (const char* sttm, unsigned int entry)
{
    if (!strcmp(sttm, "{COMPILE}"))
    {
        if (m_data.at(entry).object_type == "PACKAGE BODY")
            return "ALTER PACKAGE \"<SCHEMA>\".\"<NAME>\" COMPILE BODY";
        else if (m_data.at(entry).object_type == "TYPE BODY")
            return "ALTER TYPE \"<SCHEMA>\".\"<NAME>\" COMPILE BODY";
        else 
            return "ALTER <TYPE> \"<SCHEMA>\".\"<NAME>\" COMPILE";
    }
    return sttm;
}

void DBBrowserInvalidObjectList::OnCompile ()
{
    DoSqlForSel("Compile selection", "{COMPILE}");
}

	struct BackgroundTask_ListCompileSchema : Task
    {
        DbBrowserList& m_list;
        string m_schema;

        BackgroundTask_ListCompileSchema (DbBrowserList& list)
            : Task("Compile Schema", list.GetParent()),
            m_list(list)
        {
            m_schema = list.GetSchema();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                string sttm = "begin Dbms_Utility.compile_schema(schema => '";
                sttm += m_schema;
                sttm += "', compile_all => false); end;";
                Global::SetStatusText("Executing DBMS_UTILITY.COMPILE_SCHEMA", TRUE);
                connect.ExecuteStatement(sttm.c_str());
            }
            catch (const OciException& x)
            {
                ostringstream msg;
                msg << "DBMS_UTILITY.COMPILE_SCHEMA failed with the following error:\n\n" << x.what();
                SetError(msg.str());
            }
        }

        void ReturnInForeground ()
        {
            m_list.Refresh(true/*force*/);
        }
    };

void DBBrowserInvalidObjectList::OnCompileAll ()
{
    BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ListCompileSchema(*this)));
}

void DBBrowserInvalidObjectList::OnShowInObjectView ()
{
    std::vector<unsigned int> data;
    GetSelEntries(data);

    if (!data.empty() && strncmp(GetObjectType(data.front()), "JAVA", sizeof("JAVA")-1) )
    {
        DbBrowserList::OnShowInObjectView();
    }
    else
    {
        MessageBeep(MB_ICONERROR);
		Global::SetStatusText("Cannot describe \"JAVA\" object(s)!");
    }
}

