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

// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
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

// 04.08.02 bug fix: debug privilege for 9i
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).

namespace OraMetaDict 
{
    const int cn_grantee      = 0;
    const int cn_table_schema = 1;
    const int cn_table_name   = 2;
    const int cn_column_name  = 3;
    const int cn_privilege    = 4;
    const int cn_grantable    = 5;

    LPSCSTR csz_grant_sttm =
        "SELECT /*+NO_MERGE <RULE>*/ "
            "grantee,"
            "table_schema,"
            "table_name,"
            "NULL,"
            "privilege,"
            "grantable "
        "FROM sys.all_tab_privs "
        "WHERE table_schema <EQUAL> :owner " 
            "AND <NAME> <EQUAL> :name "         // <NAME> may be table_name or grantee
        "UNION ALL "
        "SELECT /*+NO_MERGE <RULE>*/ "
            "grantee,"
            "table_schema,"
            "table_name,"
            "column_name,"
            "privilege,"
            "grantable "
        "FROM sys.all_col_privs "
        "WHERE table_schema <EQUAL> :owner " 
            "AND <NAME> <EQUAL> :name";         // <NAME> may be table_name or grantee

    static
    void load_grant_by (Dictionary& dict, OciConnect& con,
                        const char* owner, const char* name, 
                        bool useLike, bool byGrantor);

void Loader::LoadGrantByGrantors (const char* owner, const char* name, bool useLike)
{
    load_grant_by(m_dictionary, m_connect, owner, name, useLike, true/*byGrantor*/);
}

void Loader::LoadGrantByGranteies (const char* owner, const char* grantee, bool useLike)
{
    load_grant_by(m_dictionary, m_connect, owner, grantee, useLike, false/*byGrantor*/);
}

    static
    void load_grant_by (Dictionary& dict, OciConnect& con,
                        const char* owner, const char* name, 
                        bool useLike, bool byGrantor)
    {
        Common::Substitutor subst;
        subst.AddPair("<EQUAL>", useLike   ? "like" : "=");
        subst.AddPair("<NAME>",  byGrantor ? "table_name" : "grantee");
        subst.AddPair("<RULE>", con.GetVersion() < OCI8::esvServer10X? "RULE" : "");
        subst << csz_grant_sttm;
        
        OciCursor cur(con, subst.GetResult(), 1000);

        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);

        cur.Execute();
        while (cur.Fetch())
        {
            GrantContainer* p_container = 0;
            try {
                if (byGrantor)
                    p_container = &dict.LookupGrantor(cur.ToString(cn_table_schema).c_str());
                else
                    p_container = &dict.LookupGrantee(cur.ToString(cn_grantee).c_str());
            }
            catch (const XNotFound&) {
                if (byGrantor)
                    p_container = &dict.CreateGrantor(cur.ToString(cn_table_schema).c_str());
                else
                    p_container = &dict.CreateGrantee(cur.ToString(cn_grantee).c_str());
            }

            cur.GetString(byGrantor ? cn_table_schema : cn_grantee, p_container->m_strName);

            //p_container->m_strName = p_container->m_strOwner;

            Grant* p_grant = 0;

            try { 
                p_grant = &p_container->LookupGrant(cur.ToString(!byGrantor ? cn_table_schema : cn_grantee).c_str(), 
                                                    cur.ToString(cn_table_name).c_str()); 
            }
            catch (const XNotFound&) { 
                p_grant = &p_container->CreateGrant(cur.ToString(!byGrantor ? cn_table_schema : cn_grantee).c_str(),
                                                    cur.ToString(cn_table_name).c_str()); 
            }

            cur.GetString(cn_table_schema, p_grant->m_strOwner);
            cur.GetString(cn_table_name  , p_grant->m_strName);
            cur.GetString(cn_grantee     , p_grant->m_strGrantee);

            string priv;
            cur.GetString(cn_privilege, priv);
            bool nWithAdmin = Loader::IsYes(cur.ToString(cn_grantable));

            if (cur.IsNull(cn_column_name)) 
            {
                set<string>& privileges = (!nWithAdmin) ? p_grant->m_privileges : p_grant->m_privilegesWithAdminOption;
                privileges.insert(priv);
            } 
            else 
            {
                string key;
                cur.GetString(cn_column_name, key);
                map<string,set<string> >& colPrivileges = (!nWithAdmin) ? p_grant->m_colPrivileges : p_grant->m_colPrivilegesWithAdminOption;
                colPrivileges[priv].insert(key);
            }
        }
    }

}// END namespace OraMetaDict
