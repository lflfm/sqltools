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
    16.02.2004 bug fix, temporary tables reengineering fails on 8.1.X (since build 47) 
*/

// TODO: trace and error logging (using ostream)

#include "stdafx.h"
#pragma warning ( disable : 4786 )
#include "COMMON\ExceptionHelper.h"
#include "COMMON\StrHelpers.h"
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

bool Loader::IsYes (const char* str)
{
    for (; *str && isspace(*str); str++); //skip space

    return (str[0] == 'Y' || str[0] == 'y') ? true : false;
}

bool Loader::IsYes (const string& str)
{
    string::const_iterator it = str.begin();
    for (; it != str.end() && isspace(*it); ++it); //skip space

    return (it != str.end() && (*it == 'Y' || *it == 'y')) ? true : false;
}

 void Loader::Read (OciCursor& cur, int col, Variable<int>& par)
 {
     if (cur.IsNull(col)) 
         par.SetNull();
     else 
     {
         if (cur.IsNumberField(col))
            par.Set(cur.ToInt(col));
         else
         {
             if (int val = atoi(cur.ToString(col).c_str()))
                par.Set(val);
             else
                par.SetNull();
         }
     }
 }

void Loader::Read (OciCursor& cur, int col, Variable<string>& par)
{
     if (cur.IsNull(col)) 
         par.SetNull();
     else 
         par.Set(cur.ToString(col));
}

void Loader::ReadYes (OciCursor& cur, int col, Variable<bool>& par)
{
     if (cur.IsNull(col)) 
         par.SetNull();
     else 
         par.Set(Loader::IsYes(cur.ToString(col)));
}

void Loader::Read (OciCursor& cur, int col, Variable<bool>& par, const char* trueVal)
{
     if (cur.IsNull(col)) 
         par.SetNull();
     else 
         par.Set(cur.ToString(col) == trueVal);
}

Loader::Loader (OCI8::Connect& con, Dictionary& dict)
: m_dictionary(dict),
  m_connect(con),
  OCI8::Object(con),
  m_isOpen(false)
{
}

const int cn_init_record_type       = 0;
const int cn_init_username          = 1;
const int cn_init_def_tablespace    = 2;
const int cn_init_is_curr_schema    = 3;

const int cn_init_tbs_tablespace_name = 1;
const int cn_init_tbs_initial_extent  = 3;
const int cn_init_tbs_next_extent     = 4;
const int cn_init_tbs_min_extents     = 5;
const int cn_init_tbs_max_extents     = 6;
const int cn_init_tbs_pct_increase    = 7;
const int cn_init_tbs_logging         = 8;
const int cn_init_tbs_compression     = 9;


LPSCSTR csz_init_sttm =
    "SELECT <RULE> "
        "1,username, default_tablespace, Decode(username, user, 1, 0),"
        "To_Number(NULL), To_Number(NULL), To_Number(NULL), To_Number(NULL), NULL, NULL "
    "FROM user_users " // can be used dba_users
    "UNION ALL "
	"SELECT <RULE> "
        "2, tablespace_name,"
        "NULL, initial_extent, next_extent, min_extents, max_extents, pct_increase, <LOGGING>, <DEF_TAB_COMPRESSION> "
    "FROM user_tablespaces";

// TODO: change this logic because loading thousands of tablespaces can take a while
// init private data & load shared objects (tablespaces & users) into the dictionary
void Loader::Init ()
{
    _CHECK_AND_THROW_(m_connect.IsOpen(), "Connect isn't open!");
    _CHECK_AND_THROW_(!m_isOpen, "Loader is already open!");

    if (!m_isOpen)
    {
        Common::Substitutor subst;
        subst.AddPair("<RULE>",                 (m_connect.GetVersion() < OCI8::esvServer10X)  ? "/*+RULE*/"  : "");
        subst.AddPair("<LOGGING>",              (m_connect.GetVersion() > OCI8::esvServer73X)  ? "logging"   : "'YES'");
        subst.AddPair("<DEF_TAB_COMPRESSION>",  (m_connect.GetVersion() > OCI8::esvServer9X)   ? "def_tab_compression"  : "'DISABLED'");
        subst << csz_init_sttm;

		OciCursor cur(m_connect, subst.GetResult());

        cur.Execute();
        while (cur.Fetch())
        {
            if (cur.ToInt(cn_init_record_type) == 1)
            {
                User& user = m_dictionary.CreateUser(cur.ToString(1).c_str());
                cur.GetString(cn_init_username, user.m_strName);
                cur.GetString(cn_init_def_tablespace, user.m_strDefTablespace);
//TODO: verify                
                if (cur.ToInt(cn_init_is_curr_schema) == 1)
                    m_strCurSchema = user.m_strName;
            }
            else
            {
                Tablespace& tbl = m_dictionary.CreateTablespace(cur.ToString(cn_init_tbs_tablespace_name).c_str());

                Loader::Read(cur, cn_init_tbs_tablespace_name, tbl.m_strTablespaceName);
                Loader::Read(cur, cn_init_tbs_initial_extent , tbl.m_nInitialExtent   );
                Loader::Read(cur, cn_init_tbs_next_extent    , tbl.m_nNextExtent      );
                Loader::Read(cur, cn_init_tbs_min_extents    , tbl.m_nMinExtents      );
                Loader::Read(cur, cn_init_tbs_max_extents    , tbl.m_nMaxExtents      );
                Loader::Read(cur, cn_init_tbs_pct_increase   , tbl.m_nPctIncrease     );
                Loader::Read(cur, cn_init_tbs_logging        , tbl.m_bLogging    , "LOGGING");
                Loader::Read(cur, cn_init_tbs_compression    , tbl.m_bCompression, "ENABLED");
            }
        }
    }
}

// reset state on disconnect
void Loader::Close (bool)
{
    m_isOpen = false;
}

void Loader::LoadObjects (const char* owner, const char* name, const char* type,
                          const WriteSettings& settings, bool specWithBody, bool bodyWithSpec, bool useLike)
{
    _ASSERTE(stricmp(type, "%"));

    if (!stricmp(type, "CLUSTER"))
    {
        LoadClusters(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "TABLE"))
    {
        LoadTables(owner, name, useLike);

        if (settings.m_Triggers)
            LoadTriggers(owner, name, true/*byTable*/, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "VIEW"))
    {
        LoadViews(owner, name, useLike);

        if (settings.m_ViewWithTriggers)
            LoadTriggers(owner, name, true/*byTable*/, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "TRIGGER"))
    {
        LoadTriggers(owner, name, false/*bByTable*/, useLike);

        return;
    }

    if (!stricmp(type, "PACKAGE"))
    {
        LoadPackages(owner, name, useLike);

        if (specWithBody)
            LoadPackageBodies(owner, name, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "PACKAGE BODY"))
    {
        if (bodyWithSpec)
            LoadPackages(owner, name, useLike);

        LoadPackageBodies(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "FUNCTION"))
    {
        LoadFunctions(owner, name, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "PROCEDURE"))
    {
        LoadProcedures(owner, name, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "JAVA SOURCE"))
    {
        LoadJavaSources(owner, name, useLike);

        //if (settings.m_Grants)
        //    LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "SEQUENCE"))
    {
        LoadSequences(owner, name, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "SYNONYM"))
    {
        LoadSynonyms(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "GRANTOR"))
    {
        LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "GRANTEE"))
    {
        LoadGrantByGranteies(owner, name/*grantee*/, useLike);

        return;
    }

    if (!stricmp(type, "DATABASE LINK"))
    {
        LoadDBLinks(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "SNAPSHOT LOG"))
    {
        LoadSnapshotLogs(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "TYPE"))
    {
        LoadTypes(owner, name, useLike);

        if (specWithBody)
            LoadTypeBodies(owner, name, useLike);

        if (settings.m_Grants)
            LoadGrantByGrantors(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "TYPE BODY"))
    {
        if (bodyWithSpec)
            LoadTypes(owner, name, useLike);

        LoadTypeBodies(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "INDEX"))
    {
        LoadIndexes(owner, name, useLike);

        return;
    }

    if (!stricmp(type, "CONSTRAINT"))
    {
        LoadConstraints(owner, name, useLike);
        LoadIndexes(owner, name, useLike);     // ! clarify what constraint type

        return;
    }

    _ASSERT(0);
}

}// END namespace OraMetaDict
