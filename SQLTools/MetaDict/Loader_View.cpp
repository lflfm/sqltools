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

// 14.04.2003 bug fix, Oracle9i and UTF8 data truncation
// 08.02.2004 bug fix, DDL reengineering: 64K limit for view increaced to 512M
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
#include "config.h"
//#pragma warning ( disable : 4786 )
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "MetaDict\MetaObjects.h"
#include "MetaDict\MetaDictionary.h"
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    const int cn_vw_owner   = 0;
    const int cn_vw_name    = 1;
    const int cn_vw_text    = 2;
    const int cn_vw_comment = 3;

    LPSCSTR csz_view_sttm =
        "SELECT <RULE> "
            "v1.owner,"
            "v1.view_name,"
            "v1.text,"
            "v2.comments "
        "FROM sys.all_views v1, sys.all_tab_comments v2 "
        "WHERE v1.view_name <EQUAL> :name "
            "AND v1.owner = :owner "
            "AND v1.owner = v2.owner "
            "AND v1.view_name = v2.table_name";

    const int cn_col_owner       = 0;
    const int cn_col_view_name   = 1;
    const int cn_col_column_name = 2;
    const int cn_col_column_id   = 3;
    const int cn_col_comment     = 4;

    LPSCSTR csz_col_sttm =
	  "SELECT <RULE> "
        "v1.owner,"
        "v1.table_name,"
        "v1.column_name,"
        "v1.column_id,"
        "v2.comments "
      "FROM sys.all_tab_columns v1, sys.all_col_comments v2, sys.all_views v3 "
      "WHERE v1.owner = :owner "
        "AND v1.table_name <EQUAL> :name "
        "AND v1.owner = v2.owner "
        "AND v1.table_name = v2.table_name "
        "AND v1.column_name = v2.column_name "
        "AND v1.owner = v3.owner "
        "AND v1.table_name = v3.view_name";

    static
    void load_views (
        OciConnect& con, Dictionary& dict,
        const char* owner, const char* name, bool useLike, size_t maxTextSize);
    static
    void load_columns (
        OciConnect& con, Dictionary& dict,
        const char* owner, const char* name, bool useLike);

void Loader::LoadViews (const char* owner, const char* name, bool useLike, bool skipConstraints)
{
#if TEST_LOADING_OVERFLOW
    load_views(m_connect, m_dictionary, owner, name, useLike, 32);
#else
    load_views(m_connect, m_dictionary, owner, name, useLike, 2 * 1024);
#endif
    load_columns(m_connect, m_dictionary, owner, name, useLike);

    if (!skipConstraints)
        loadConstraints(owner, name, true/*byTable*/, useLike);
}

    static
    void load_views (
        OciConnect& con, Dictionary& dict,
        const char* owner, const char* name, bool useLike, size_t maxTextSize)
    {

        Common::Substitutor subst;
        subst.AddPair("<RULE>", con.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst << csz_view_sttm;

        OciCursor cur(con, subst.GetResult(), useLike ? 50 : 1, maxTextSize);

        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);

        cur.Execute();
        while (cur.Fetch())
        {
            if (cur.IsRecordGood())
            {
                View& view = dict.CreateView(cur.ToString(cn_vw_owner).c_str(), 
                                             cur.ToString(cn_vw_name).c_str());

                cur.GetString(cn_vw_owner,  view.m_strOwner);
                cur.GetString(cn_vw_name,   view.m_strName);
                cur.GetString(cn_vw_text,   view.m_strText);
                cur.GetString(cn_vw_comment,view.m_strComments);
            } 
            else 
            {
                const size_t progression[] = { 64 * 1024U, 512 * 1024U };

                for (unsigned i = 0; i < sizeof(progression)/sizeof(progression[0]); i++)
                    if (maxTextSize < progression[i]) {
                        load_views(con, dict, owner, cur.ToString(cn_vw_name).c_str(), 
                            false/*useLike*/, progression[i]);
                        break;
                    }

                _CHECK_AND_THROW_(i < sizeof(progression)/sizeof(progression[0]), 
                                  "View loading error:\n text size exceed 512K!");
            }
        }
    }

    static
    void load_columns (
        OciConnect& con, Dictionary& dict,
        const char* owner, const char* name, bool useLike)
    {
        Common::Substitutor subst;
        subst.AddPair("<RULE>", con.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");
        subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
        subst << csz_col_sttm;

        OciCursor cur(con, subst.GetResult(), 50, 128);

        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);

        cur.Execute();
        while (cur.Fetch())
        {
            View& view = dict.LookupView(cur.ToString(cn_col_owner).c_str(), 
                                         cur.ToString(cn_col_view_name).c_str());
            
            int pos = cur.ToInt(cn_col_column_id) - 1;
            view.m_Columns[pos] = cur.ToString(cn_col_column_name);

            if (cur.IsGood(cn_col_comment))
            {
                if (!cur.IsNull(cn_col_comment))
                    view.m_mapColComments[pos] = cur.ToString(cn_col_comment);
            }
            else
            {
                OciCursor ovrfCur(
                        con, 
                        "SELECT comments FROM sys.all_col_comments "
                            "WHERE owner = :owner AND table_name = :name "
                                "AND column_name = :col_name",
                        1
                    );

                ovrfCur.Bind(":owner",    cur.ToString(cn_col_owner).c_str());
                ovrfCur.Bind(":name",     cur.ToString(cn_col_view_name).c_str());
                ovrfCur.Bind(":col_name", cur.ToString(cn_col_column_name).c_str());

                ovrfCur.Execute();
                ovrfCur.Fetch();
                view.m_mapColComments[pos] = ovrfCur.ToString(0);
            }
        }
    }

}// END namespace OraMetaDict
