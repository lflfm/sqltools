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

// 14.04.2003 bug fix, Oracle9i and UTF8 data truncation
// 26.10.2003 the serious bug since 1.4.1 build 37, a procedure/function/package code line can be truncated if its length > 128
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).
// 12.09.2011 bug fix, DDL reengineering fails on wrapped pl/sql procedure because all_source.text might contain multiple lines 
// 2011.10.02 bug fix, DDL reengineering fails on altered Oracle OBJECTS/TYPES
// 2011.10.03 bug fix, DDL reengineering fails on source lines longer than 128 chars
// 2017-12-31 bug fix, Oracle client version was used to control RULE hint

#include "stdafx.h"
#include <strstream>
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
    const int cn_owner = 0;
    const int cn_name  = 1;
    const int cn_type  = 2;
    const int cn_line  = 3;
    const int cn_text  = 4;

    LPSCSTR csz_enum_sttm =
      "SELECT DISTINCT "
        "owner,"
        "object_name,"
        "object_type "
      "FROM sys.all_objects "
      "WHERE owner = :owner "
        "AND object_name LIKE :name "
        "AND object_type = :type";

    LPSCSTR csz_src_sttm =
      "SELECT "
        "owner,"
        "name,"
        "type,"
        "line,"
        "text "
      "FROM sys.all_source "
      "WHERE owner = :owner "
        "AND name = :name "
        "AND type = :type "
        "<LINE>"
      "ORDER BY line";

    LPSCSTR csz_src_sttm_type_9i =
        "SELECT "
            "owner,"
            "name,"
            "type,"
            "line,"
            "text "
        "FROM "
        "( "
            "SELECT v.*,"
                "Row_Number() OVER (PARTITION BY line ORDER BY Length(text) DESC) rn "
            "FROM sys.all_source v "
            "WHERE owner = :owner "
                "AND name = :name "
                "AND type = :type "
                "<LINE>"
        ") "
        "WHERE rn = 1 "
        "ORDER BY line";

    static
    void load_code (Dictionary& dict, OciConnect& con,
                    const char* owner, const char* name, const char* type);


    void Loader::loadPlsCodes (const char* owner, const char* name, const char* type, bool useLike)
    {
        if (!useLike)
        {
            load_code(m_dictionary, m_connect, owner, name, type);
        }
        else
        {
            OciCursor cur(m_connect, csz_enum_sttm);

            cur.Bind(":owner", owner);
            cur.Bind(":name",  name);
            cur.Bind(":type",  type);

            cur.Execute();
            while (cur.Fetch())
            {
                load_code(m_dictionary, m_connect, owner, cur.ToString(cn_name).c_str(), type);
            }
        }
    }

    static
    void load_code (Dictionary& dict, OciConnect& con,
                    const char* owner, const char* name, const char* type)
    {
        OCI8::EServerVersion servVer = con.GetVersion();

        const char* sttm = (servVer == OCI8::esvServer9X && !stricmp(type, "TYPE")) 
            ? csz_src_sttm_type_9i : csz_src_sttm;

        Common::Substitutor subst;
        subst.AddPair("<LINE>", "");
        //subst.AddPair("<RULE>", con.GetVersion() < OCI8::esvServer10X? "/*+RULE*/" : "");
        subst << sttm;

#if TEST_LOADING_OVERFLOW
        OciCursor cur(con, subst.GetResult(), 1000, 32);
#else
        OciCursor cur(con, subst.GetResult(), 1000, 192);
#endif
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);
        cur.Bind(":type",  type);

        cur.Execute();

        string buff;
        std::ostringstream out;
        bool is_empty = true; 

        while (cur.Fetch())
        {
            is_empty = false;

            if (cur.IsGood(cn_text)) 
            {
                cur.GetString(cn_text, buff);
            } 
            else // overflow of small buffer -> reread in big buffer
            {   
                Common::Substitutor ovrfSubst;
                ovrfSubst.AddPair("<LINE>", "AND line = :line ");
                ovrfSubst.AddPair("<RULE>", con.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");
                ovrfSubst << sttm;

                OciCursor ovrfCur(con, ovrfSubst.GetResult(), 1, 4 * 1024);

                ovrfCur.Bind(":owner", owner);
                ovrfCur.Bind(":name",  name);
                ovrfCur.Bind(":type",  type);
                ovrfCur.Bind(":line",  cur.ToInt(cn_line));

                ovrfCur.Execute();
                ovrfCur.Fetch();
                _CHECK_AND_THROW_(ovrfCur.IsGood(0), "Code loading error:\nUnknown Error!");
	            // 26.10.2003 the serious bug since 1.4.1 build 37, a procedure/function/package code line can be truncated if its length > 128
                ovrfCur.GetString(cn_text, buff);
            }

            /*
            // 26.10.2003 workaround for ..., removed trailing '\n' and '\r' for code lines
            int length = buff.length();
            for (; length > 0; length--) 
            {
                char ch = buff[length-1];
                if (ch != '\n' && ch != '\r')
                    break;
            }
            buff.resize(length);

            out << buff << endl;
            */

            // 12.09.2011 bug fix, DDL reengineering fails on wrapped pl/sql procedure because all_source.text might contain multiple lines 
            string line;
            std::istrstream is(buff.c_str(), buff.length());
            
            for (int i = 0; getline(is, line); i++)
            {
                if (i) 
                    out << endl;

                if (!line.empty() && *line.rbegin() == '\r')
                    line.resize(line.length() - 1);

                out << line;
            }

            if (!buff.empty() && (*buff.rbegin() == '\r' || *buff.rbegin() == '\n'))
                out << endl;
        }

        if (!is_empty)
        {
            PlsCode* p_code = 0;

            if (!stricmp(type, "FUNCTION"))
                p_code = &dict.CreateFunction(owner, name);
            else if (!stricmp(type, "PROCEDURE"))
                p_code = &dict.CreateProcedure(owner, name);
            else if (!stricmp(type, "PACKAGE"))
                p_code = &dict.CreatePackage(owner, name);
            else if (!stricmp(type, "PACKAGE BODY"))
                p_code = &dict.CreatePackageBody(owner, name);
            else if (!stricmp(type, "TYPE"))
                p_code = &dict.CreateType(owner, name);
            else if (!stricmp(type, "TYPE BODY"))
                p_code = &dict.CreateTypeBody(owner, name);
            else if (!stricmp(type, "JAVA SOURCE"))
                p_code = &dict.CreateJavaSource(owner, name);
            else
                _CHECK_AND_THROW_(0, "Code loading error:\nUnknown Error!");

            p_code->m_strOwner = owner;
            p_code->m_strName  = name;
            p_code->m_strType  = type;
            p_code->m_strText  = out.str();
        }
    }

}// END namespace OraMetaDict
