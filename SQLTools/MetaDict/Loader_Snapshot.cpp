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

#include "stdafx.h"
//#pragma warning ( disable : 4786 )
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "COMMON\StrHelpers.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    using namespace Oci7;

#define LPSCSTR static const char*

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
        "SELECT /*+RULE*/ "
          "l.log_owner,"
          "l.master,"
          "t.tablespace_name,"
          "t.pct_free,"
          "t.pct_used," 
          "t.ini_trans," 
          "t.max_trans," 
          "t.initial_extent,"
          "t.next_extent," 
          "t.min_extents," 
          "t.max_extents," 
          "t.pct_increase, "
          "t.freelists,"
          "t.freelist_groups "
        "FROM <V_SNAPSHOT_LOGS> l, sys.all_tables t "
        "WHERE l.log_owner = t.owner "
          "AND l.log_table = t.table_name "
          "AND l.log_owner = :owner AND l.master <EQUAL> :name";


void Loader::LoadSnapshots (const char* owner, const char* name, bool useLike)
{
    bool useAll = m_strCurSchema.compare(owner) ? true : false;

    Common::Substitutor subst;
    subst.AddPair("<V_SNAPSHOT_LOGS>", 
                  useAll ? (m_isVer8 ? "sys.all_snapshot_logs" 
                                     : "dba_snapshot_logs")
                         : "user_snapshot_logs");

    subst.AddPair("<EQUAL>", useLike ? "like" : "=");
    subst << csz_log_sttm;

    AutoDefCursor cur(m_connect, 1024, 4 * 1024);
    cur.Open();
    cur.Parse(subst.GetResult());
    cur.Bind(":owner", owner);
    cur.Bind(":name",  name);

    cur.Execute();
    while (cur.Fetch())
    {
        Snapshot& snapshot 
            = m_dictionary.CreateSnapshot(cur[cn_tbl_owner], cur[cn_tbl_table_name]);
        
        cur[cn_tbl_tablespace].GetTextString(snapshot.m_strTablespaceName);
        snapshot.m_nInitialExtent  = cur[cn_tbl_initial_extent];
        snapshot.m_nNextExtent     = cur[cn_tbl_next_extent];
        snapshot.m_nMinExtents     = cur[cn_tbl_min_extents];
        snapshot.m_nMaxExtents     = cur[cn_tbl_max_extents];
        snapshot.m_nPctIncrease    = cur[cn_tbl_pct_increase];
        
        snapshot.m_nFreeLists      = cur[cn_tbl_freelists];
        snapshot.m_nFreeListGroups = cur[cn_tbl_freelist_groups];
        snapshot.m_nPctFree        = cur[cn_tbl_pct_free];
        snapshot.m_nPctUsed        = cur[cn_tbl_pct_used];
        snapshot.m_nIniTrans       = cur[cn_tbl_ini_trans];
        snapshot.m_nMaxTrans       = cur[cn_tbl_max_trans];

        cur[cn_tbl_owner     ].GetTextString(snapshot.m_strOwner);
        cur[cn_tbl_table_name].GetTextString(snapshot.m_strName);
    }
}


}// END namespace OraMetaDict
