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

/*
    2018-03-30 bug fix, compatibility with non enterprise version of 8i
*/

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DBBrowserTypeList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <ActivePrimeExecutionNote.h>

using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserTypeCodeList::DBBrowserTypeCodeList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserTypeCodeList::~DBBrowserTypeCodeList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserTypeCodeList, DbBrowserList)
    ON_COMMAND(ID_DS_COMPILE, OnCompile)
END_MESSAGE_MAP()

    const int cn_owner         = 0;
    const int cn_object_name   = 1;   
    const int cn_typecode      = 2;
    const int cn_attributes    = 3;
    const int cn_methods       = 4;
    const int cn_incomplete    = 5;
    const int cn_status        = 6;
    const int cn_created       = 7;
    const int cn_last_ddl_time = 8;  

    static const char* csz_sttm_8i =
        "SELECT "
            "v1.owner,"        
            "v1.type_name,"
            "v1.typecode," 
            "v1.attributes," 
            "v1.methods," 
            "v1.incomplete,"
            "v2.status,"
            "v2.created,"      
            "v2.last_ddl_time "
        "FROM sys.all_types v1, sys.all_objects v2 "
            "WHERE v2.owner = :owner "
                "AND v2.object_type = :type "
                "AND v2.owner = v1.owner "
                "AND v1.type_name = v2.object_name"
    ;

    static const char* csz_sttm =
        "SELECT * FROM ("
            "SELECT "
                "v1.owner,"        
                "v1.type_name,"
                "v1.typecode," 
                "v1.attributes," 
                "v1.methods," 
                "v1.incomplete,"
                "v2.status,"
                "v2.created,"      
                "v2.last_ddl_time,"
                "Row_Number() OVER (PARTITION BY v2.owner, v2.object_name, v2.object_type ORDER BY v2.last_ddl_time DESC) rn "
            "FROM sys.all_types v1, sys.all_objects v2 "
                "WHERE v2.owner = :owner "
                    "AND v2.object_type = :type "
                    "AND v2.owner = v1.owner "
                    "AND v1.type_name = v2.object_name"
        ") WHERE rn = 1"
    ;

	struct BackgroundTask_RefreshTypeCodeList 
        : BackgroundTask_Refresh_Templ<DBBrowserTypeCodeList, TypeCodeCollection>
    {
        BackgroundTask_RefreshTypeCodeList (DBBrowserTypeCodeList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh TypeList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(connect.GetVersion() < OCI8::esvServer9X ? csz_sttm_8i : csz_sttm);
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type", m_monoType.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    TypeCodeEntry code;

                    code.owner       = cur.ToString(cn_owner      );
                    code.object_name = cur.ToString(cn_object_name);
                    code.status      = cur.ToString(cn_status     );

                    code.typecode   = cur.ToString(cn_typecode  );
                    code.attributes = cur.ToInt   (cn_attributes);
                    code.methods    = cur.ToInt   (cn_methods   );
                    code.incomplete = cur.ToString(cn_incomplete);

                    cur.GetTime(cn_created,       code.created);
                    cur.GetTime(cn_last_ddl_time, code.last_ddl_time);

                    m_data.push_back(code);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserTypeCodeList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshTypeCodeList(*this)));
        SetDirty(false);
    }
}

	struct BackgroundTask_RefreshTypeCodeListEntry : Task
    {
        int m_entry;
        TypeCodeEntry m_dataEntry;
        DBBrowserTypeCodeList& m_list;
        string m_schema, m_monoType;

        BackgroundTask_RefreshTypeCodeListEntry (DBBrowserTypeCodeList& list, int entry)
            : Task("Refres TypeEntry", list.GetParent()),
            m_list(list),
            m_entry(entry),
            m_dataEntry(list.m_data.at(entry))
        {
            SetSilent(true);
            m_schema = list.GetSchema();
            m_monoType = list.GetMonoType();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                OciCursor cur(connect, 1, 196);
                cur.Prepare((string(connect.GetVersion() < OCI8::esvServer9X ? csz_sttm_8i : csz_sttm) + " and type_name = :name").c_str());
                cur.Bind(":owner", m_schema.c_str());
                cur.Bind(":type", m_monoType.c_str());
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

void DBBrowserTypeCodeList::RefreshEntry (int entry) 
{
    BkgdRequestQueue::Get().PushInFront(TaskPtr(new BackgroundTask_RefreshTypeCodeListEntry(*this, entry)));
}

void DBBrowserTypeCodeList::ApplyQuickFilter (bool valid, bool invalid) 
{
    ListCtrlManager::FilterCollection filter;
    GetFilter(filter);

    for (int i = 0, n = m_dataAdapter.getColCount(); i < n; ++i)
        if (!wcsicmp(m_dataAdapter.getColHeader(i), L"status"))
        {
            const wchar_t* val = 0;

            if (valid && invalid)
                val = L"";
            else if (valid)
                val = L"VALID";
            else if  (invalid)
                val = L"INVALID";
            else
                val = L"NA";

            if (val != filter.at(i).value
            || ListCtrlManager::EXACT_MATCH != filter.at(i).operation)
            {
                filter.at(i).value = val;
                filter.at(i).operation = ListCtrlManager::EXACT_MATCH;
                SetFilter(filter);
                break;
            }
        }
}

void DBBrowserTypeCodeList::ExtendContexMenu (CMenu* pMenu)
{
    UINT grayed = IsSelectionEmpty() ? MF_GRAYED : 0;

    pMenu->AppendMenu(MF_STRING|grayed, ID_DS_COMPILE, L"&Compile");                      
    pMenu->AppendMenu(MF_SEPARATOR);
}

void DBBrowserTypeCodeList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER <TYPE> \"<SCHEMA>\".\"<NAME>\" COMPILE");
}

void DBBrowserTypeBodyList::OnCompile ()
{
    DoSqlForSel("Compile selection", "ALTER TYPE \"<SCHEMA>\".\"<NAME>\" COMPILE BODY");
}

