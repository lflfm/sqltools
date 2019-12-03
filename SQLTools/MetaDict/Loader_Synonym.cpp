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
#include "OCI8/BCursor.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace OraMetaDict 
{
    const int cn_owner       = 0;
    const int cn_name        = 1;
    const int cn_ref_owner   = 2;
    const int cn_ref_name    = 3;
    const int cn_ref_db_link = 4;

    LPSCSTR csz_syn_sttm =
		"SELECT <RULE> "
            "owner,"
            "synonym_name,"
            "table_owner,"
            "table_name,"
            "db_link "
        "FROM sys.all_synonyms "
        "WHERE owner = :owner "
            "AND synonym_name <EQUAL> :name";

void Loader::LoadSynonyms (const char* owner, const char* name, bool useLike)
{
    OCI8::EServerVersion ver = m_connect.GetVersion();
    Common::Substitutor subst;
    subst.AddPair("<EQUAL>", useLike  ? "like" : "=");
    subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
    subst << csz_syn_sttm;

    OciCursor cur(m_connect, subst.GetResult());

    cur.Bind(":owner", owner);
    cur.Bind(":name",  name);

    cur.Execute();
    while (cur.Fetch())
    {
        Synonym& synonym = m_dictionary.CreateSynonym(cur.ToString(cn_owner).c_str(), cur.ToString(cn_name).c_str());
        cur.GetString(cn_owner      , synonym.m_strOwner);
        cur.GetString(cn_name       , synonym.m_strName);
        cur.GetString(cn_ref_owner  , synonym.m_strRefOwner);
        cur.GetString(cn_ref_name   , synonym.m_strRefName);
        cur.GetString(cn_ref_db_link, synonym.m_strRefDBLink);
    }
}


}// END namespace OraMetaDict
