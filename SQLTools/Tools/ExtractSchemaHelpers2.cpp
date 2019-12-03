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
#include <set>
#include <string>
#include "SQLTools.h"
#include <COMMON\AppGlobal.h>
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\Loader.h"
#include "ExtractSchemaDlg.h"
#include "ExtractSchemaHelpers.h"
#include "ExtractDDLSettingsXmlStreamer.h"
#include <COMMON\ErrorDlg.h>
#include <OCI8/BCursor.h>

using namespace std;
using namespace Common;
using namespace OraMetaDict;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace ExtractSchemaHelpers
{

string GetFullPath (ExtractDDLSettings& settings)
{
    return GetRootFolder(settings) + '\\' + GetSubdirName(settings);
}

string GetRootFolder (ExtractDDLSettings& settings)
{
    string folder = settings.m_strFolder;
        Common::trim_symmetric(folder);

    if (folder.size() && (*(folder.rbegin()) == '\\'))
        folder.resize(folder.size() - 1);

    if (settings.m_UseDbAlias 
    && settings.m_UseDbAliasAs == 2 
    && !settings.m_strDbAlias.empty())
        folder += '\\' + settings.m_strDbAlias;

    return folder;
}

string GetSubdirName (ExtractDDLSettings& settings)
{
    string name;
    
    if (settings.m_UseDbAlias && !settings.m_strDbAlias.empty())
    {
        switch (settings.m_UseDbAliasAs)
        {
        case 0: name = settings.m_strDbAlias + '_' + settings.m_strSchema; break;
        case 1: name = settings.m_strSchema + '_' + settings.m_strDbAlias; break;
        case 2: name = settings.m_strSchema;                               break;
        }
    }
    else
        name = settings.m_strSchema;

    return name;
}

void MakeFolders (string& path, ExtractDDLSettings& settings)
{
    string folder = GetRootFolder(settings);

    if (folder.size())
    {
        list<string> listDir;
        // check existent path part
        while (!folder.empty() && *(folder.rbegin()) != ':'
        && GetFileAttributes(folder.c_str()) == 0xFFFFFFFF) {
            listDir.push_front(folder);

            size_t len = folder.find_last_of('\\');
            if (len != string::npos)
                folder.resize(len);
            else
                break;
        }

        // create non-existent path part
        list<string>::const_iterator it(listDir.begin()), end(listDir.end());
        for (; it != end; it++)
            _CHECK_AND_THROW_(CreateDirectory((*it).c_str(), NULL), "Can't create folder!");
    }

    path = GetFullPath(settings);

    if (GetFileAttributes(path.c_str()) != 0xFFFFFFFF) {
        string strMessage;
        strMessage = "Folder \"" + path + "\" already exists.\nDo you want to remove it?";

        if (AfxMessageBox(strMessage.c_str(), MB_YESNO) == IDNO)
            AfxThrowUserException();

//        m_MainPage.SetStatusText("Removing folders...");
        _CHECK_AND_THROW_(AppDeleteDirectory(path.c_str()), "Can't remove folder!");
    }

    _CHECK_AND_THROW_(CreateDirectory(path.c_str(), NULL), "Can't create folder!");

    set<string> folders;
    folders.insert(settings.m_TableFolder      ); 
    folders.insert(settings.m_TriggerFolder    ); 
    folders.insert(settings.m_ViewFolder       ); 
    folders.insert(settings.m_SequenceFolder   ); 
    folders.insert(settings.m_TypeFolder       ); 
    folders.insert(settings.m_TypeBodyFolder   ); 
    folders.insert(settings.m_FunctionFolder   ); 
    folders.insert(settings.m_ProcedureFolder  ); 
    folders.insert(settings.m_PackageFolder    ); 
    folders.insert(settings.m_PackageBodyFolder); 
    folders.insert(settings.m_GrantFolder      ); 

    set<string>::const_iterator it = folders.begin();
    for (; it != folders.end(); ++it)
        _CHECK_AND_THROW_(CreateDirectory((path + "\\" + *it).c_str(), NULL), "Can't create folder!");
}

void NextAction (OciConnect* connect, HWND hStatusWnd, char* text)
{
    if (connect)
        connect->CHECK_INTERRUPT();

    if (IsWindow(hStatusWnd)) 
    {
        ::SetWindowText(hStatusWnd, text);
        UpdateWindow(hStatusWnd);
    }
}

void LoadSchema (OciConnect& connect, Dictionary& dict, HWND hStatusWnd, ExtractDDLSettings& settings)
{
    Loader loader(connect, dict);
    loader.Init();

    if (settings.m_ExtractTables) {
        NextAction(&connect, hStatusWnd, "Loading tables...");
        loader.LoadTables(settings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (settings.m_ExtractViews) {
        NextAction(&connect, hStatusWnd, "Loading views...");
        loader.LoadViews(settings.m_strSchema.c_str(), "%", true/*useLike*/, settings.m_ExtractTables/*skipConstraints*/);
    }

    if (settings.m_ExtractTriggers) {
        NextAction(&connect, hStatusWnd, "Loading triggers...");
        loader.LoadTriggers(settings.m_strSchema.c_str(), "%", false/*byTable*/, true/*useLike*/);
    }

    if (settings.m_ExtractSequences) {
        NextAction(&connect, hStatusWnd, "Loading sequences...");
        loader.LoadSequences(settings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (settings.m_ExtractCode) {
        NextAction(&connect, hStatusWnd, "Loading types,functions,procedures && packages...");
        loader.LoadTypes        (settings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadTypeBodies   (settings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadFunctions    (settings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadProcedures   (settings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadPackages     (settings.m_strSchema.c_str(), "%", true/*useLike*/);
        loader.LoadPackageBodies(settings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (settings.m_ExtractSynonyms) {
        NextAction(&connect, hStatusWnd, "Loading Synonyms...");
        loader.LoadSynonyms(settings.m_strSchema.c_str(), "%", true/*useLike*/);
    }

    if (settings.m_ExtractGrantsByGrantee) {
        NextAction(&connect, hStatusWnd, "Loading Grants...");
        loader.LoadGrantByGrantors(settings.m_strSchema.c_str(), "%", true/*useLike*/);
    }
}

void OverrideTablespace (Dictionary& dict, ExtractDDLSettings& settings)
{
    if (!settings.m_strTableTablespace.empty())
    {
        settings.m_AlwaysPutTablespace = TRUE;
        {
            TableMapConstIter it, end;
            dict.InitIterators(it, end);
            for (; it != end; it++)
                it->second->m_strTablespaceName.Set(settings.m_strTableTablespace);
        }
        {
            ClusterMapConstIter it, end;
            dict.InitIterators(it, end);
            for (; it != end; it++)
                it->second->m_strTablespaceName.Set(settings.m_strTableTablespace);
        }
    }
    if (!settings.m_strIndexTablespace.empty())
    {
        settings.m_AlwaysPutTablespace = TRUE;
        {
            IndexMapConstIter it, end;
            dict.InitIterators(it, end);
            for (; it != end; it++)
                it->second->m_strTablespaceName.Set(settings.m_strIndexTablespace);
        }
    }
}

void WriteSchema (Dictionary& dict, vector<const DbObject*>& views, HWND hStatusWnd, string& path, string& error_log, ExtractDDLSettings& settings)
{
    WriteContext context(path, "DO.sql", settings);

    try 
    {
        NextAction(NULL, hStatusWnd, "Write data to disk...");

        if (settings.m_ExtractSynonyms) {
            NextAction(NULL, hStatusWnd, "Writing synonyms...");
            context.SetDefExt("sql");
            context.SetSubDir("");
            context.BeginSingleStream("Synonyms");
            dict.EnumSynonyms(write_object, &context);
            context.EndSingleStream();
        }

        if (settings.m_ExtractCode) {
            NextAction(NULL, hStatusWnd, "Writing types...");

            context.SetDefExt("sql");
            context.SetSubDir("");
            context.BeginSingleStream("Types");
            dict.EnumTypes(write_incopmlete_type, &context);
            context.EndSingleStream();

            context.SetDefExt(settings.m_TypeExt);
            context.SetSubDir(settings.m_TypeFolder);
            dict.EnumTypes(write_object, &context);
        }

        if (settings.m_ExtractTables) {
            NextAction(NULL, hStatusWnd, "Writing tables...");
            context.SetDefExt(settings.m_TableExt);
            context.SetSubDir(settings.m_TableFolder);
            if (settings.m_GroupByDDL) {
                dict.EnumTables(write_table_definition,     &context);
                dict.EnumTables(write_table_indexes,        &context);
                dict.EnumTables(write_table_chk_constraint, &context);
                dict.EnumTables(write_table_unq_constraint, &context);
                dict.EnumTables(write_table_fk_constraint,  &context);
            } else {
                settings.m_Constraints = true;
                settings.m_Indexes     = true;
                dict.EnumTables(write_object, &context);
            }
        }

        if (settings.m_ExtractSequences) {
            NextAction(NULL, hStatusWnd, "Writing sequences...");
            context.SetDefExt(settings.m_SequenceExt);
            context.SetSubDir(settings.m_SequenceFolder);
            dict.EnumSequences(write_object, &context);
        }

        if (settings.m_ExtractCode) {

            NextAction(NULL, hStatusWnd, "Writing functions...");
            context.SetDefExt(settings.m_FunctionExt);
            context.SetSubDir(settings.m_FunctionFolder);
            dict.EnumFunctions(write_object, &context);

            NextAction(NULL, hStatusWnd, "Writing procedures...");
            context.SetDefExt(settings.m_ProcedureExt);
            context.SetSubDir(settings.m_ProcedureFolder);
            dict.EnumProcedures(write_object, &context);

            NextAction(NULL, hStatusWnd, "Writing packages...");
            context.SetDefExt(settings.m_PackageExt);
            context.SetSubDir(settings.m_PackageFolder);
            dict.EnumPackages(write_object, &context);

        }

        if (settings.m_ExtractViews) {
            NextAction(NULL, hStatusWnd, "Writing views...");
            context.SetDefExt(settings.m_ViewExt);
            context.SetSubDir(settings.m_ViewFolder);

            if (settings.m_OptimalViewOrder && !views.empty())
            {
                vector<const DbObject*>::const_iterator it = views.begin();
                for (; it != views.end(); it++)
                    write_object((DbObject&)**it, &context);
            } else
                dict.EnumViews(write_object, &context);
        }

        if (settings.m_ExtractCode) {
            NextAction(NULL, hStatusWnd, "Writing types...");
            context.SetDefExt(settings.m_TypeBodyExt);
            context.SetSubDir(settings.m_TypeBodyFolder);
            dict.EnumTypeBodies(write_object, &context);
        }

        if (settings.m_ExtractCode) {
            NextAction(NULL, hStatusWnd, "Writing packages...");
            context.SetDefExt(settings.m_PackageBodyExt);
            context.SetSubDir(settings.m_PackageBodyFolder);
            dict.EnumPackageBodies(write_object, &context);
        }

        if (settings.m_ExtractTriggers) {
            NextAction(NULL, hStatusWnd, "Writing triggers...");
            context.SetDefExt(settings.m_TriggerExt);
            context.SetSubDir(settings.m_TriggerFolder);
            dict.EnumTriggers(write_object, &context);
        }

        if (settings.m_ExtractGrantsByGrantee) {
            NextAction(NULL, hStatusWnd, "Writing grants...");
            context.SetDefExt(settings.m_GrantExt);
            context.SetSubDir(settings.m_GrantFolder);
            dict.EnumGrantors(write_grantee, &context);
        }
    } 
    catch (...)
    {
        context.Cleanup();
        throw;
    }

    context.ErrorLog(error_log);
}

/// helper class ////////////////////////////////////////////////////////////

    class DpdBuilder
    {
    public:
        DpdBuilder (const Dictionary&, vector<const DbObject*>&);

        void AddDependency (const char* owner, const char* name,
                            const char* ref_owner, const char* ref_name);

        void Unwind ();

    private:
        struct DpdNode
        {
            DbObject& m_object;                   // dictionary object reference
            int     m_refCount;                   // number of dependent objects
            std::vector<DpdNode*> m_dependencies; // used "OWNER.NAME" format
            bool    m_lock;                       // algorithm checking support

            DpdNode (DbObject& object);
            void AddDependency (DpdNode& node);
        };

        typedef counted_ptr<DpdNode>       DpdNodePtr;
        typedef map<string, DpdNodePtr>    DpdNodeMap;
        typedef DpdNodeMap::iterator       DpdNodeMapIter;
        typedef DpdNodeMap::const_iterator DpdNodeMapConstIter;

        void unwind (DpdNode& node);

        int m_Cycle, m_MaxCycle;
        DpdNodeMap m_mapNodes;
        vector<const DbObject*>& m_vecResult;
    };

/////////////////////////////////////////////////////////////////////////////

    inline
    DpdBuilder::
    DpdNode::DpdNode (DbObject& object)
    : m_object(object),
      m_refCount(0),
      m_lock(false) {}

    inline
    void DpdBuilder::
    DpdNode::AddDependency (DpdNode& node)
    {
        m_dependencies.push_back(&node);
        node.m_refCount++;
    }

    DpdBuilder::DpdBuilder (const Dictionary& dict, vector<const DbObject*>& result)
    : m_Cycle(0),
    m_MaxCycle(10000),
    m_vecResult(result)
    {
        ViewMapConstIter it, end;
        dict.InitIterators (it, end);

        for (; it != end; it++) {
            counted_ptr<DpdNode> ptr(new DpdNode(*(*it).second.get()));
            m_mapNodes.insert(DpdNodeMap::value_type(((*it).first), ptr));
        }
    }

    void DpdBuilder::AddDependency (const char* owner, const char* name,
                                    const char* ref_owner, const char* ref_name)
    {
        DpdNodeMapIter it;
        char key[cnDbKeySize];

        it = m_mapNodes.find(make_key(key, owner, name));
        if (it == m_mapNodes.end()) throw XNotFound(key);
        DpdNode& node1 = *(*it).second.get();

        it = m_mapNodes.find(make_key(key, ref_owner, ref_name));

        if (it != m_mapNodes.end()) // ref_obj can be not accessible
        {
            DpdNode& node2 = *(*it).second.get();
            node1.AddDependency(node2);
        }
    }

    void DpdBuilder::unwind (DpdNode& node)
    {
        _CHECK_AND_THROW_(m_Cycle++ < m_MaxCycle, "Unwind dependencies algorithm error!");

        std::vector<DpdNode*>::iterator
            it(node.m_dependencies.begin()),
            end(node.m_dependencies.end());

        for (; it != end; it++)
            unwind(**it);

        if (node.m_lock) return;
        m_vecResult.push_back(&node.m_object);
        node.m_lock = true;
    }

    void DpdBuilder::Unwind ()
    {
        DpdNodeMapIter it(m_mapNodes.begin()), end(m_mapNodes.end());
        for (; it != end; it++)
            if ((*it).second.get()->m_refCount == 0)
                unwind(*(*it).second.get());
    }


/// optimal view creation order builder /////////////////////////////////////

    LPSCSTR cszDpdSttm =
        "SELECT owner, name, referenced_owner, referenced_name FROM sys.all_dependencies"
          " WHERE owner = :p_owner"
            " AND referenced_owner = :p_owner"
            " AND type = 'VIEW'"
            " AND referenced_type = 'VIEW'";

    const int cn_owner      = 0;
    const int cn_name       = 1;
    const int cn_ref_owner  = 2;
    const int cn_ref_name   = 3;

void OptimizeViewCreationOrder (
    OciConnect& connect, Dictionary& dict, HWND hStatusWnd,
    vector<const DbObject*>& result,
    string& error_log, ExtractDDLSettings& settings
)
{
    if (settings.m_ExtractViews && settings.m_OptimalViewOrder) 
    {
        NextAction(&connect, hStatusWnd, "Optimizing view creation order...");
    
        try
        {
            DpdBuilder builder(dict, result);

            OciCursor cursor(connect, cszDpdSttm, 256);
            cursor.Bind(":p_owner", settings.m_strSchema.c_str());

            cursor.Execute();
            while (cursor.Fetch())
                builder.AddDependency(
                    cursor.ToString(cn_owner).c_str(),
                    cursor.ToString(cn_name).c_str(),
                    cursor.ToString(cn_ref_owner).c_str(),
                    cursor.ToString(cn_ref_name).c_str());

            builder.Unwind();
        }
        catch (const std::exception& x) { 
            error_log += string("Optimal view reorder failed with ") + x.what() + "\n"; 
        }
    }
}

}; //namespace ExtractSchemaHelpers
