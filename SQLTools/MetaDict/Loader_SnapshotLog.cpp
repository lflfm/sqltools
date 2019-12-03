/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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

// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters
// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
//#pragma warning ( disable : 4786 )
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "COMMON\StrHelpers.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    const int cn_tbl_owner           = 0;
    const int cn_tbl_table_name      = 1;
    const int cn_tbl_tablespace      = 2;
    const int cn_tbl_pct_free        = 3;
    const int cn_tbl_pct_used        = 4;
    const int cn_tbl_ini_trans       = 5;
    const int cn_tbl_max_trans       = 6;
    const int cn_tbl_initial_extent  = 7;
    const int cn_tbl_next_extent     = 8;
    const int cn_tbl_min_extents     = 9;
    const int cn_tbl_max_extents     = 10;
    const int cn_tbl_pct_increase    = 11;
    const int cn_tbl_freelists       = 12;
    const int cn_tbl_freelist_groups = 13;

    LPSCSTR csz_log_sttm =
		"SELECT <RULE> "
          "l.log_owner,"
          "l.master,"
          "t.tablespace_name,"
          "t.pct_free,"
          "Nvl(t.pct_used, 30)," 
          "t.ini_trans," 
          "t.max_trans," 
          "t.initial_extent,"
          "t.next_extent," 
          "t.min_extents," 
          "t.max_extents," 
          "t.pct_increase, "
          "Nvl(t.freelists, 1),"
          "Nvl(t.freelist_groups, 1) "
        "FROM <V_SNAPSHOT_LOGS> l, sys.all_tables t "
        "WHERE l.log_owner = t.owner "
          "AND l.log_table = t.table_name "
          "AND l.log_owner = :owner AND l.master <EQUAL> :name";


void Loader::LoadSnapshotLogs (const char* owner, const char* name, bool useLike)
{
    bool useAll = m_strCurSchema.compare(owner) ? true : false;

    Common::Substitutor subst;
    subst.AddPair("<RULE>", m_connect.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");
    subst.AddPair("<V_SNAPSHOT_LOGS>", 
            useAll ? (m_connect.GetVersion() >= OCI8::esvServer80X ? "sys.all_snapshot_logs" 
                : "dba_snapshot_logs")
                         : "user_snapshot_logs");

    subst.AddPair("<EQUAL>", useLike ? "like" : "=");
    subst << csz_log_sttm;

    OciCursor cur(m_connect, subst.GetResult());
    
    cur.Bind(":owner", owner);
    cur.Bind(":name",  name);

    cur.Execute();
    while (cur.Fetch())
    {
        SnapshotLog& log 
            = m_dictionary.CreateSnapshotLog(cur.ToString(cn_tbl_owner).c_str(), 
                                             cur.ToString(cn_tbl_table_name).c_str());

        cur.GetString(cn_tbl_owner,      log.m_strOwner);
        cur.GetString(cn_tbl_table_name, log.m_strName);
        
        Loader::Read(cur, cn_tbl_tablespace     , log.m_strTablespaceName);          
        Loader::Read(cur, cn_tbl_initial_extent , log.m_nInitialExtent   );
        Loader::Read(cur, cn_tbl_next_extent    , log.m_nNextExtent      );
        Loader::Read(cur, cn_tbl_min_extents    , log.m_nMinExtents      );
        Loader::Read(cur, cn_tbl_max_extents    , log.m_nMaxExtents      );
        Loader::Read(cur, cn_tbl_pct_increase   , log.m_nPctIncrease     );
        Loader::Read(cur, cn_tbl_freelists      , log.m_nFreeLists       );
        Loader::Read(cur, cn_tbl_freelist_groups, log.m_nFreeListGroups  );
        Loader::Read(cur, cn_tbl_pct_free       , log.m_nPctFree         );
        Loader::Read(cur, cn_tbl_pct_used       , log.m_nPctUsed         );
        Loader::Read(cur, cn_tbl_ini_trans      , log.m_nIniTrans        );
        Loader::Read(cur, cn_tbl_max_trans      , log.m_nMaxTrans        );
    }
}


}// END namespace OraMetaDict
