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

// 2018-01-01 improvement, RULE hint is used for databases prior to 10g only

#include "stdafx.h"
#include "SQLTools.h"
#include "DbBrowser\DBBrowserDbLinkList.h"
#include "OCI8/BCursor.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\AppGlobal.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <ActivePrimeExecutionNote.h>

using namespace OraMetaDict;
using namespace Common;
using namespace ServerBackgroundThread;

DBBrowserDbLinkList::DBBrowserDbLinkList()
: DbBrowserList(m_dataAdapter, true),
m_dataAdapter(m_data)
{
}

DBBrowserDbLinkList::~DBBrowserDbLinkList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserDbLinkList, DbBrowserList)
END_MESSAGE_MAP()

    const int cn_owner    = 0;
    const int cn_db_link  = 1;
    const int cn_username = 2;   
    const int cn_host     = 3;

    static const char* csz_sttm =
        "SELECT <RULE> owner, db_link, username, host FROM sys.all_db_links WHERE owner = :owner";


	struct BackgroundTask_RefreshDbLinkList 
        : BackgroundTask_Refresh_Templ<DBBrowserDbLinkList, DbLinkCollection>
    {
        BackgroundTask_RefreshDbLinkList(DBBrowserDbLinkList& list)
            : BackgroundTask_Refresh_Templ(list, "Refresh DbLinkList")
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                ActivePrimeExecutionOnOff onOff;

                Common::Substitutor subst;
                OCI8::EServerVersion servVer = connect.GetVersion();
                subst.AddPair("<RULE>", (servVer < OCI8::esvServer10X) ? "/*+RULE*/" : "");
                subst << csz_sttm;

                OciCursor cur(connect, 50, 196);
                cur.Prepare(subst.GetResult());
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                {
                    DbLinkEntry dblink;

                    dblink.owner    = cur.ToString(cn_owner   );
                    dblink.db_link  = cur.ToString(cn_db_link );
                    dblink.username = cur.ToString(cn_username);
                    dblink.host     = cur.ToString(cn_host    );

                    m_data.push_back(dblink);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserDbLinkList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshDbLinkList(*this)));
        SetDirty(false);
    }
}

void DBBrowserDbLinkList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserDbLinkList::MakeDropSql (int entry, std::string& sql) const
{
    Common::Substitutor subst;
    subst.AddPair("<PUBLIC>", !strcmp(GetSchema(), "PUBLIC") ? "PUBLIC" : "");
    subst.AddPair("<NAME>", GetObjectName(entry));
    subst <<  ("DROP <PUBLIC> DATABASE LINK \"<NAME>\"");
    sql = subst.GetResult();
}

void DBBrowserDbLinkList::ExtendContexMenu (CMenu* pMenu)
{
    pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
}

void DBBrowserDbLinkList::OnShowInObjectView ()
{
    MessageBeep((UINT)-1);
}
