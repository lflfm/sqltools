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

#include "stdafx.h"
#include "ObjectTree_Builder.h"
#include "SQLTools.h"
#include "COMMON\StrHelpers.h"
#include <OCI8/BCursor.h>
#include "SQLWorksheet/PlsSqlParser.h"
#include "ServerBackgroundThread\TaskQueue.h"

using namespace std;
using namespace Common;
using namespace ServerBackgroundThread;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace ObjectTree {

    void parseInput (
        const std::string& input,
        std::vector<std::pair<std::string, bool> >& names
    );

    void quetyObjectDescriptors (
        OciConnect& connect,
        std::vector<std::pair<std::string, bool> > names,
        std::vector<ObjectDescriptor>& result
    );
///////////////////////////////////////////////////////////////////////////////
//
// struct ObjectDescriptor { std::string owner, name, type; };
//
// Valid input for FindObjects: name1[.name2]
//      Name can be either a sequence of A-z,0-9,_#$ or a quoted string
//      '%' and '_' are allowed for Oracle LIKE search, if name is not a quoted string,
//      but LIKE search is turned on if name contains '%' because '_' is too frequent in names
//      Currently name3 will be ignored because there is no support for package functions/procedures
//
///////////////////////////////////////////////////////////////////////////////
void FindObjects (OciConnect& connect, const std::string& input, std::vector<ObjectDescriptor>& result)
{
    std::vector<std::pair<std::string, bool> > names;

    parseInput(input, names);

    if (names.size() > 0)
        quetyObjectDescriptors(connect, names, result);
}

void parseInput (const std::string& input, std::vector<std::pair<std::string, bool> >& names)
{
    TokenMapPtr tokenMap(new TokenMap);
    tokenMap->insert(std::map<string, int>::value_type("\"", etDOUBLE_QUOTE));
    tokenMap->insert(std::map<string, int>::value_type("." , etDOT         ));
    tokenMap->insert(std::map<string, int>::value_type("@" , etAT_SIGN     ));
    tokenMap->insert(std::map<string, int>::value_type("%" , etPERCENT_SIGN));

    SimpleSyntaxAnalyser analyser;
    PlsSqlParser parser(&analyser, tokenMap);
    parser.PutLine(0, input.c_str(), input.length());
    parser.PutEOF(0);

    const std::vector<Token> tokens = analyser.GetTokens();
    std::vector<Token>::const_iterator it = tokens.begin();
    for (int i = 0; i < 2; ++i)
    {
        bool percent = false, dot = false, eof = false, quoted = false;
        int start = (it != tokens.end()) ? it->offset : 0;

        for (; it != tokens.end(); ++it)
        {
            switch (*it)
            {
            case etDOUBLE_QUOTED_STRING:
                quoted = true;
                break;
            case etPERCENT_SIGN:
                percent = true;
                break;
            case etDOT:
                dot = true;
                break;
            case etEOL:
            case etEOF:
                eof = true;
                break;
            case etAT_SIGN:
                throw Common::AppException("Find object: DB link not allowed!");
            }
            if (dot || eof) break;
        }

        int end = (it != tokens.end()) ? it->offset : input.length();

        string name(input.substr(start, end-start));
        Common::trim_symmetric(name);

        if (!quoted)
            Common::to_upper_str(name.c_str(), name);
        else
            name = name.substr(1, name.length()-2);

        // 20.03.2005 (ak) bug fix, an exception on "Find Object", if some name component is null
        if (!name.empty())
            names.push_back(make_pair(name, percent));

        TRACE("NAME: %s\n", name.c_str());

        if (dot) ++it;
        if (eof) break;
    }
}

#include <OCI8/BCursor.h>
#include "COMMON\StrHelpers.h"

static const char* cszSelectFromAllObjects =
    "SELECT <RULE> owner, object_name, object_type FROM sys.all_objects"
    " WHERE object_type in ("
    "'TABLE','VIEW','SYNONYM','SEQUENCE','PROCEDURE','FUNCTION',"
    "'PACKAGE','PACKAGE BODY','TRIGGER','TYPE','TYPE BODY')";//",'OPERATOR')"
static const char* cszOrderByClause =
    " ORDER BY owner, object_name, object_type";

static
void makeQueryByNameInCurrSchema (std::vector<std::pair<std::string, bool> > names, OciCursor& cursor)
{
    ASSERT(names.size() > 0);

    OCI8::EServerVersion ver = cursor.GetConnect().GetVersion();
    Common::Substitutor subst;
    subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
    subst.AddPair("<EQL>", names.at(0).second  ? "like" : "=");
    subst << cszSelectFromAllObjects;
    subst << (ver >= OCI8::esvServer81X
        ? " AND owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA') AND object_name <EQL> :name"
        : " AND owner = USER and object_name <EQL> :name");
    
    if (ver >= OCI8::esvServer81X)
    {
        string user, schema;
        cursor.GetConnect().GetCurrentUserAndSchema (user, schema);

        if (user == schema)
            subst << " UNION"
                " SELECT SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA') owner, synonym_name, 'SYNONYM' FROM sys.user_synonyms"
                " WHERE synonym_name <EQL> :name";
        else
            subst << " UNION"
                " SELECT owner, synonym_name, 'SYNONYM'  FROM sys.all_synonyms"
                " WHERE owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA') AND synonym_name <EQL> :name";
    }

    subst << cszOrderByClause;
    cursor.Prepare(subst.GetResult());
    cursor.Bind(":name", names.at(0).first.c_str());
    cursor.Execute();
}

static
void makeQueryByNameInPublic (std::vector<std::pair<std::string, bool> > names, OciCursor& cursor)
{
    ASSERT(names.size() > 0);

    OCI8::EServerVersion ver = cursor.GetConnect().GetVersion();
    Common::Substitutor subst;
    subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
    subst.AddPair("<EQL>", names.at(0).second  ? "like" : "=");
    subst << cszSelectFromAllObjects;
    subst << " AND owner = 'PUBLIC' AND object_name <EQL> :name";
    subst << cszOrderByClause;
    cursor.Prepare(subst.GetResult());
    cursor.Bind(":name", names.at(0).first.c_str());
    cursor.Execute();
}

static
void makeQueryBySchemaAndName (std::vector<std::pair<std::string, bool> > names, OciCursor& cursor)
{
    ASSERT(names.size() > 1);

    OCI8::EServerVersion ver = cursor.GetConnect().GetVersion();
    Common::Substitutor subst;
    subst.AddPair("<RULE>", (ver < OCI8::esvServer10X) ? "/*+RULE*/" : "");
    subst.AddPair("<EQL1>", names.at(0).second  ? "like" : "=");
    subst.AddPair("<EQL2>", names.at(1).second  ? "like" : "=");
    subst << cszSelectFromAllObjects;
    subst << " AND owner <EQL1> :schema AND object_name <EQL2> :name";
    if (ver >= OCI8::esvServer81X)
    {
        subst << " UNION"
            " SELECT owner, synonym_name, 'SYNONYM'  FROM sys.all_synonyms"
            " WHERE owner <EQL1> :schema AND synonym_name <EQL2> :name";
    }
    subst << cszOrderByClause;
    cursor.Prepare(subst.GetResult());
    cursor.Bind(":schema", names.at(0).first.c_str());
    cursor.Bind(":name", names.at(1).first.c_str());
    cursor.Execute();
}

static
void fetchObjectDescriptors (OciCursor& cursor, std::vector<ObjectDescriptor>& result)
{
    for (int rows = 0; cursor.Fetch(); rows++)
    {
        ObjectDescriptor desc;
        cursor.GetString(0, desc.owner);
        cursor.GetString(1, desc.name);
        cursor.GetString(2, desc.type);
        result.push_back(desc);

        TRACE("FOUND: %s.%s.%s\n", desc.owner.c_str(), desc.name.c_str(), desc.type.c_str());

        // problem with public.% :(
        if (rows > 300)
            throw FindObjectException("Find object: too many objects found!\nPlease use more precise criteria.");
    }
    cursor.Close();
}

///////////////////////////////////////////////////////////////////////////////
//    if it is a single name then there is no question.
//    if it is two comonent name, the question is
//    what the first component means - schema or package?
//        if it is a name with exact match (no '%' for like),
//        probably this name captured for the editor
//        so use oracle name resolving rule (double check it):
//            look at the current schema then public synomyns
//        if it is a name with widcard '%'
//        most likely it is an interactive input and the fist component is schema
//            look at this schema for objects then if nothing is found
//            thow the second component away and call the first case
///////////////////////////////////////////////////////////////////////////////
void quetyObjectDescriptors (
    OciConnect& connect,
    std::vector<std::pair<std::string, bool> > names,
    std::vector<ObjectDescriptor>& result)
{
    OciCursor cursor(connect);

    if (names.size() > 1)
    {
        makeQueryBySchemaAndName(names, cursor);
        fetchObjectDescriptors(cursor, result);
    }

    if (/*names.size() == 1 ||*/ result.empty())
    {
        makeQueryByNameInCurrSchema(names, cursor);
        fetchObjectDescriptors(cursor, result);

        if (result.empty())
        {
            makeQueryByNameInPublic(names, cursor);
            fetchObjectDescriptors(cursor, result);
        }
    }
}

};//namespace ObjectTree {