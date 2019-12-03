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
#include "DbBrowser\DBBrowserGranteeList.h"
#include "OCI8/BCursor.h"
#include "ServerBackgroundThread\TaskQueue.h"

using namespace OraMetaDict;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserGranteeList::DBBrowserGranteeList()
: DbBrowserList(m_dataAdapter),
m_dataAdapter(m_data)
{
}

DBBrowserGranteeList::~DBBrowserGranteeList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserGranteeList, DbBrowserList)
END_MESSAGE_MAP()

    const int cn_grantee  = 0;

    static const char* csz_sttm =
        " SELECT /*+NO_MERGE*/"
        " DISTINCT grantee"
        " FROM sys.all_tab_privs"
        " WHERE table_schema = :owner"
        " UNION"
        " SELECT /*+NO_MERGE*/"
        " DISTINCT grantee"
        " FROM sys.all_col_privs"
        " WHERE table_schema = :owner";

	struct BackgroundTask_RefreshGranteeList 
        : BackgroundTask_Refresh_Templ<DBBrowserGranteeList, GranteeCollection>
    {
        BackgroundTask_RefreshGranteeList(DBBrowserGranteeList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh GranteeList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 50, 196);
                cur.Prepare(csz_sttm);
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    GranteeEntry grantee;

                    grantee.grantee = cur.ToString(cn_grantee);

                    m_data.push_back(grantee);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserGranteeList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshGranteeList(*this)));
        SetDirty(false);
    }
}

void DBBrowserGranteeList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserGranteeList::ExtendContexMenu (CMenu* pMenu)
{
    pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
    pMenu->DeleteMenu(ID_EDIT_DELETE,  MF_BYCOMMAND);
    pMenu->DeleteMenu(ID_SQL_LOAD_WITH_DBMS_METADATA, MF_BYCOMMAND);
    pMenu->DeleteMenu(ID_EDIT_DELETE_WORD_TO_RIGHT,   MF_BYCOMMAND);
}

void DBBrowserGranteeList::OnShowInObjectView ()
{
    MessageBeep((UINT)-1);
}
