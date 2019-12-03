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
    const int cn_username    = 2;
    const int cn_host        = 3;
    const int cn_password    = 4;

    LPSCSTR csz_dbl_sttm =
		"SELECT <RULE> "
          "<OWNER>,"
          "db_link,"
          "username,"
          "host,"
          "<PASSWORD> "
        "FROM <V_DB_LINKS> "
          "WHERE db_link <EQUAL> :name"
          "<OWNER_EQL>";


void Loader::LoadDBLinks (const char* owner, const char* name, bool useLike)
{
    bool useAll = m_strCurSchema.compare(owner) ? true : false;

    Common::Substitutor subst;
    subst.AddPair("<OWNER>",      useAll ? "owner"        : "USER");
    subst.AddPair("<PASSWORD>",   useAll ? "NULL"         : "password");
    subst.AddPair("<V_DB_LINKS>", useAll ? "sys.all_db_links" : "user_db_links");
    subst.AddPair("<OWNER_EQL>",  useAll ? " AND owner = :owner" : "");
    subst.AddPair("<EQUAL>",      useLike ? "like" : "=");
    subst.AddPair("<RULE>",       m_connect.GetVersion() < OCI8::esvServer10X? "/*+RULE*/" : "");
    subst << csz_dbl_sttm;

    OciCursor cur(m_connect, subst.GetResult());
    
    if (useAll) 
        cur.Bind(":owner", owner);
    
    cur.Bind(":name",  name);

    cur.Execute();
    while (cur.Fetch())
    {
        DBLink& link = m_dictionary.CreateDBLink(cur.ToString(cn_owner).c_str(), cur.ToString(cn_name).c_str());
        cur.GetString(cn_owner,    link.m_strOwner);
        cur.GetString(cn_name,     link.m_strName);
        cur.GetString(cn_username, link.m_strUser);
        cur.GetString(cn_host,     link.m_strHost);
        cur.GetString(cn_password, link.m_strPassword);
    }
}


}// END namespace OraMetaDict
