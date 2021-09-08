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
#include "DbBrowser\DBBrowserClusterList.h"
#include "OCI8/BCursor.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "COMMON\StrHelpers.h"
#include <ActivePrimeExecutionNote.h>

using namespace OraMetaDict;
using namespace ServerBackgroundThread;


DBBrowserClusterList::DBBrowserClusterList()
: DbBrowserList(m_dataAdapter),
m_dataAdapter(m_data)
{
}

DBBrowserClusterList::~DBBrowserClusterList()
{
}

BEGIN_MESSAGE_MAP(DBBrowserClusterList, DbBrowserList)
END_MESSAGE_MAP()

    const int cn_owner           = 0;
    const int cn_cluster_name    = 1;
    const int cn_cluster_type    = 2; 
    const int cn_function        = 3;  
    const int cn_hashkeys        = 4;
    const int cn_key_size        = 5; 
    const int cn_tablespace_name = 6;  
    const int cn_initial_extent  = 7; 
    const int cn_next_extent     = 8;  
    const int cn_pct_increase    = 9;     
    const int cn_max_extents     = 10;    
    const int cn_pct_free        = 11;
    const int cn_pct_used        = 12;


    static const char* csz_sttm =
        "SELECT <RULE>"
        " owner,"          
        " cluster_name,"   
        " cluster_type,"   
        " function,"       
        " hashkeys,"       
        " key_size,"       
        " tablespace_name,"
        " initial_extent," 
        " next_extent,"    
        " pct_increase,"   
        " max_extents,"    
        " pct_free,"       
        " pct_used"       
        " FROM sys.all_clusters WHERE owner = :owner";

    struct BackgroundTask_RefreshClusterList 
        : BackgroundTask_Refresh_Templ<DBBrowserClusterList, ClusterCollection>
    {
        BackgroundTask_RefreshClusterList(DBBrowserClusterList& list) 
            : BackgroundTask_Refresh_Templ(list, "Refresh ClusterList") {}

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
                    ClusterEntry cluster;

                    cluster.owner           = cur.ToString(cn_owner          );
                    cluster.cluster_name    = cur.ToString(cn_cluster_name   );
                    cluster.cluster_type    = cur.ToString(cn_cluster_type   );
                    cluster.function        = cur.ToString(cn_function       );
                    cluster.hashkeys        = cur.ToInt   (cn_hashkeys       );
                    cluster.key_size        = cur.ToInt   (cn_key_size       );
                    cluster.tablespace_name = cur.ToString(cn_tablespace_name);
                    cluster.initial_extent  = cur.ToInt   (cn_initial_extent );
                    cluster.next_extent     = cur.ToInt   (cn_next_extent    );
                    cluster.pct_increase    = cur.ToInt   (cn_pct_increase   );
                    cluster.max_extents     = cur.ToInt   (cn_max_extents    );
                    cluster.pct_free        = cur.ToInt   (cn_pct_free       );
                    cluster.pct_used        = cur.ToInt   (cn_pct_used       );

                    m_data.push_back(cluster);
                }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }
    };

void DBBrowserClusterList::Refresh (bool force) 
{
    if (IsDirty() || force)
    {
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_RefreshClusterList(*this)));
        SetDirty(false);
    }
}

void DBBrowserClusterList::RefreshEntry (int /*entry*/) 
{
}

void DBBrowserClusterList::ExtendContexMenu (CMenu* pMenu)
{
    pMenu->DeleteMenu(ID_SQL_DESCRIBE, MF_BYCOMMAND);
}

void DBBrowserClusterList::OnShowInObjectView ()
{
    MessageBeep((UINT)-1);
}
