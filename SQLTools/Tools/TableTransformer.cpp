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
#include "SQLTools.h"
#include "TableTransformer.h"
#include "COMMON\ExceptionHelper.h"
#include "MetaDict\TextOutput.h"
#include "MetaDict\MetaDictionary.h"
#include "MetaDict\Loader.h"
#include "SQLWorksheet/SQLWorksheetDoc.h"
#include "OCI8/ACursor.h"
#include "OCI8/BCursor.h"
#include "ServerBackgroundThread\TaskQueue.h"

using namespace OraMetaDict;
using namespace ServerBackgroundThread;

// 01.12.2003 bug fix, table transformer helper generates unique name only once
// 20.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk).

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTableTransformer::CTableTransformer(CWnd* pParent /*=NULL*/)
: CDialog(CTableTransformer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTableTransformer)
	//}}AFX_DATA_INIT
}


void CTableTransformer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTableTransformer)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_TT_SCHEMA, m_wndSchemaList);
	DDX_Control(pDX, IDC_TT_TABLE, m_wndTableList);
	DDX_CBString(pDX, IDC_TT_SCHEMA, m_strSchema);
	DDX_CBString(pDX, IDC_TT_TABLE, m_strTable);
}


BEGIN_MESSAGE_MAP(CTableTransformer, CDialog)
	//{{AFX_MSG_MAP(CTableTransformer)
	ON_CBN_SELCHANGE(IDC_TT_SCHEMA, OnSelChangeSchema)
    ON_NOTIFY(CSchemaList::NM_LIST_LOADED, IDC_TT_SCHEMA, OnSchemaListLoaded)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTableTransformer message handlers

  LPSCSTR cszTablesSttm =
    "SELECT table_name FROM sys.all_tables"
    " WHERE owner = :owner";

	struct BackgroundTask_PopulateTableList : Task
    {
        HWND m_hwndTableList;
        string m_schema;
        vector<string> m_tables;

        BackgroundTask_PopulateTableList (CComboBox& wndTableList, const string& schema)
            : Task("Populate Table List", 0),
            m_hwndTableList((HWND)wndTableList),
            m_schema(schema)
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                OciCursor cur(connect, 50, 196);
                cur.Prepare(cszTablesSttm);
                cur.Bind(":owner", m_schema.c_str());

                cur.Execute();
                while (cur.Fetch())
                    m_tables.push_back(cur.ToString(0));

                //Sleep(3000);
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            CWaitCursor wait;

            if (::IsWindow(m_hwndTableList))
            {
                vector<string>::const_iterator it = m_tables.begin();

                for (; it != m_tables.end(); ++it)
                    ::SendMessage(m_hwndTableList, CB_ADDSTRING, 0, (LPARAM)it->c_str());

                if (((int)::SendMessage(m_hwndTableList, CB_GETCOUNT, 0, 0)) > 0)
                {
                    ::SendMessage(m_hwndTableList, CB_SETCURSEL, 0, 0);
                    ::EnableWindow(::GetDlgItem(::GetParent(m_hwndTableList), IDOK), TRUE);
                }
            }
        }
    };

void CTableTransformer::OnSelChangeSchema()
{
    try { EXCEPTION_FRAME;

        CWaitCursor wait;

        UpdateData();
        m_wndTableList.ResetContent();

        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_PopulateTableList(m_wndTableList, (LPCSTR)m_strSchema)));

        ::EnableWindow(::GetDlgItem(m_hWnd, IDOK), FALSE);
    }
    _DEFAULT_HANDLER_
}

void CTableTransformer::OnSchemaListLoaded (NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    OnSelChangeSchema();

    m_wndTableList.SelectString(0, AfxGetApp()->GetProfileString("TableTransformer", "TableName"));

    *pResult = 0;
}

BOOL CTableTransformer::OnInitDialog()
{
	CDialog::OnInitDialog();

    //OnSelChangeSchema();

    m_wndTableList.SelectString(0, AfxGetApp()->GetProfileString("TableTransformer", "TableName"));

	return TRUE;
}

  LPSCSTR cszNameSttm =
    "SELECT '1' FROM sys.all_objects"
    " WHERE owner = :owner AND object_name = :name";


// 01.12.2003 bug fix, table transformer helper generates unique name only once
void generate_unique_name (OciConnect& connect,
                           const string& owner, const string& _orgName,
                           string& tmpName)
{
    string orgName;
    orgName.assign(_orgName.c_str(), min(26U, _orgName.size()));

    OciCursor cursor(connect, cszNameSttm, 1);

    OciStringVar name(30+1);

    cursor.Bind(":name",  name);
    cursor.Bind(":owner", owner.c_str(), owner.size());

    bool isUnique = false;

    for (int i(0); i < 100 && !isUnique; i++)
    {
        char buffer[30+1];
        _snprintf(buffer, sizeof(buffer), "%s_$%02d", orgName.c_str(), i);
        buffer[sizeof(buffer)-1] = 0;
        name.Assign(buffer);

        cursor.Execute();

        if (!cursor.Fetch())
            isUnique = true;
    }

    name.GetString(tmpName);

    _CHECK_AND_THROW_(isUnique, "Unique name generation error!");
}

	struct BackgroundTask_LoadTableForTransformation : Task
    {
        string m_schema, m_table;
        string m_text;

        BackgroundTask_LoadTableForTransformation (const string& schema, const string& table)
            : Task("Load Table for Transformation", 0),
            m_schema(schema), 
            m_table(table)
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                Dictionary dict;
                Loader loader(connect, dict);
                loader.Init();

                loader.LoadTables(m_schema.c_str(), m_table.c_str(), false/*bUseLike*/);
                loader.LoadTriggers(m_schema.c_str(), m_table.c_str(), true/*byTable*/, false/*bUseLike*/);

                loader.LoadRefFkConstraints(m_schema.c_str(), m_table.c_str(), false/*bUseLike*/);
                loader.LoadGrantByGrantors(m_schema.c_str(), m_table.c_str(), false/*bUseLike*/);

                WriteSettings settings;
                // Common settings
                settings.m_Comments               = true;  // table, view & column comments
                settings.m_Grants                 = true;  // grants
                settings.m_LowerNames             = true;  // safe lowercase object names
                settings.m_ShemaName              = false; // schema as object names prefix
                settings.m_SQLPlusCompatibility   = true;  // view do not have no white space lines, truncate end-line white space
                // Table Specific
                settings.m_CommentsAfterColumn    = false; // column comments as end line remark
                settings.m_CommentsPos            = 0;     // end line comments position
                settings.m_Constraints            = true;  // constraints (table writing)
                settings.m_Indexes                = true;  // indexes (table writing)
                settings.m_NoStorageForConstraint = false; // no storage for primary key & unique key
                settings.m_StorageClause          = 1;     // table & index storage clause
                settings.m_TableDefinition        = true;  // short table definition (table writing)
                settings.m_Triggers               = true;  // table triggers (table writing)

                Table& table = dict.LookupTable(m_schema.c_str(), m_table.c_str());
        //        _CHECK_AND_THROW_(table.m_strClusterName.empty(),
        //                        "Table write error:\nCluster tables not supported!");

                TextOutputInMEM out(true, 32*1024);

                out.PutLine("PROMPT ================================================================");
                out.PutLine("PROMPT        <<< SQLTools Table Transformation Helper V1.1 >>>        ");
                out.PutLine("PROMPT    CAUTION:                                                     ");
                out.PutLine("PROMPT      No warranty of any kind is expressed or implied. You       ");
                out.PutLine("PROMPT      use at your own risk. The author will not be liable for    ");
                out.PutLine("PROMPT      data loss, damages, loss of profits or any other kind      ");
                out.PutLine("PROMPT      of loss while using or misusing this software.             ");
                out.PutLine("PROMPT    USAGE:                                                       ");
                out.PutLine("PROMPT      You must modify and execute each statement step by step.   ");
                out.PutLine("PROMPT ================================================================");
                out.NewLine();
                {   
                    out.PutLine("PROMPT LOCK table in exclusive mode");
                    out.PutLine("PROMPT   The following lock does not prevent from accessing the table while it's being rebult.");
                    out.PutLine("PROMPT   It helps to make sure that the table is not in use at this particular moment");
                    out.PutLine("PROMPT   and NOWAIT helps to do it w/o waiting. Hello TOAD developers!");
                    out.PutLine("PROMPT If ERROR then STOP and REPEAT before SUCCESS!");
                    out.Put("LOCK TABLE ");
                    out.PutOwnerAndName(table.m_strOwner, table.m_strName, settings.m_ShemaName);
                    out.PutLine(" IN EXCLUSIVE MODE NOWAIT");
                    out.PutLine("/");
                    out.NewLine();
                }

                string bakName;
                generate_unique_name(connect, table.m_strOwner, table.m_strName, bakName);

                {   out.PutLine("PROMPT RENAME table");
                    out.Put("RENAME ");
                    out.PutOwnerAndName(table.m_strOwner, table.m_strName, settings.m_ShemaName);
                    out.Put(" TO ");
                    out.PutOwnerAndName(table.m_strOwner, bakName, settings.m_ShemaName);
                    out.NewLine();
                    out.PutLine("/");
                    out.NewLine();
                    out.NewLine();
                }

                {   out.PutLine("PROMPT DROP triggers");

                    set<string>::const_iterator it(table.m_setTriggers.begin()),
                                                   end(table.m_setTriggers.end());
                    for (it; it != end; it++) {
                        Trigger& trigger = dict.LookupTrigger((*it).c_str());

                        out.Put("DROP TRIGGER ");
                        out.PutOwnerAndName(trigger.m_strOwner, trigger.m_strName,
                                            settings.m_ShemaName
                                            || trigger.m_strOwner != trigger.m_strTableOwner);
                        out.NewLine();
                        out.PutLine("/");
                        out.NewLine();
                    }
                    out.NewLine();
                }

                {   out.PutLine("PROMPT DROP primary, unique and named constraints");

                    set<string>::const_iterator
                        it(table.m_setConstraints.begin()),
                        end(table.m_setConstraints.end());

                    for (it; it != end; it++)
                    {
                        Constraint& constraint =
                            dict.LookupConstraint(table.m_strOwner.c_str(), (*it).c_str());

                        if (constraint.m_strType[0] == 'P' || constraint.m_strType[0] == 'U'
                          || !constraint.IsGenerated())
                        {

                            out.Put("ALTER TABLE ");
                            out.PutOwnerAndName(table.m_strOwner, bakName, settings.m_ShemaName);
                            out.NewLine();
                            out.Put("  DROP CONSTRAINT ");
                            out.SafeWriteDBName(constraint.m_strName);
                            if (constraint.m_strType[0] == 'P' || constraint.m_strType[0] == 'U')
                                out.Put(" CASCADE");
                            out.NewLine();
                            out.PutLine("/");
                            out.NewLine();

                        }
                    }
                    out.NewLine();
                }

                {   out.PutLine("PROMPT DROP indexes");

                    set<string>::const_iterator it(table.m_setIndexes.begin()),
                                                end(table.m_setIndexes.end());
                    for (it; it != end; it++) {
                        Index& index = dict.LookupIndex((*it).c_str());

                        if (index.m_strOwner == index.m_strTableOwner) {
                            try {
                                Constraint& constraint = dict.LookupConstraint((*it).c_str());
                                if (constraint.m_strType[0] == 'P' || constraint.m_strType[0] == 'U')
                                    continue;
                            } catch (const XNotFound&) {}
                        }

                        out.Put("DROP INDEX ");
                        out.PutOwnerAndName(index.m_strOwner, index.m_strName,
                                            settings.m_ShemaName
                                            || index.m_strOwner != index.m_strTableOwner);
                        out.NewLine();
                        out.PutLine("/");
                        out.NewLine();
                    }
                    out.NewLine();
                }

                {   out.Put("PROMPT CREATE ");
                    out.PutOwnerAndName(table.m_strOwner, table.m_strName, settings.m_ShemaName);
                    out.NewLine();

                    table.WriteDefinition(out, settings);
                    out.NewLine();
                }

                {   out.PutLine("PROMPT TRANSFER data into new table");

                    list<string> columns;
                    {
                        Table::ColumnContainer::const_iterator
                            it(table.m_Columns.begin()), end(table.m_Columns.end());

                        for (; it != end; it++)
                            columns.push_back(it->second->m_strColumnName);
                    }

                    out.Put("INSERT /*+append*/ INTO ");
                    out.PutOwnerAndName(table.m_strOwner, table.m_strName, settings.m_ShemaName);
                    out.Put(" (");
                    out.NewLine();
                    out.WriteColumns(columns, 2);
                    out.Put(")");
                    out.NewLine();
                    out.PutLine("(");
                    out.MoveIndent(2);
                    out.PutLine("SELECT");
                    out.WriteColumns(columns, 2);
                    out.PutIndent();
                    out.Put("FROM ");
                    out.PutOwnerAndName(table.m_strOwner, bakName, settings.m_ShemaName);
                    out.NewLine();
                    out.MoveIndent(-2);
                    out.PutLine(")");
                    out.PutLine("/");
                    out.NewLine();
                    out.NewLine();
                }

                {   out.PutLine("PROMPT CREATE indexes");
                    table.WriteIndexes(out, settings);
                    out.NewLine();
                }

                {   out.PutLine("PROMPT CREATE constraints");
                    table.WriteConstraints(out, settings, 'C');
                    table.WriteConstraints(out, settings, 'P');
                    table.WriteConstraints(out, settings, 'U');
                    table.WriteConstraints(out, settings, 'R');
                    out.NewLine();
                }

                {   out.PutLine("PROMPT CREATE dependent foreign key constraints");
                    ConstraintMapConstIter it, end;
                    dict.InitIterators(it, end);
                    for (; it != end; it++) {
                        const Constraint& constraint = *it->second;
                        if (constraint.m_strType[0] == 'R'
                        && (constraint.m_strTableName != table.m_strName
                           || constraint.m_strOwner != table.m_strOwner)) {
                            try {
                                Constraint& r_constraint = dict.LookupConstraint(constraint.m_strROwner.c_str(),
                                                                                 constraint.m_strRConstraintName.c_str());

                                if (r_constraint.m_strOwner == table.m_strOwner
                                && r_constraint.m_strTableName == table.m_strName)
                                    constraint.Write(out, settings);

                            } catch (const XNotFound&) {}
                        }
                    }
                    out.NewLine();
                }

                {   out.PutLine("PROMPT CREATE triggers");
                    table.WriteTriggers(out, settings);
                    out.NewLine();
                }

                {   out.PutLine("PROMPT CREATE comments");
                    table.WriteComments(out, settings);
                    out.NewLine();
                }

                {   out.PutLine("PROMPT GRANT privileges");
                    GrantorMapConstIter it, end;
                    dict.InitIterators(it, end);
                    for (; it != end; it++) {
                        const Grantor& grantor = *it->second;
                        grantor.Write(out, settings);
                    }
                }

                m_text = string(out.GetData(), out.GetLength());
            }
            catch (const exception& x)
            {
                SetError(x.what());
            }
        }

        void ReturnInForeground ()
        {
            CWaitCursor wait;

            try { EXCEPTION_FRAME;

                if (CDocTemplate* pDocTemplate = theApp.GetPLSDocTemplate())
                {
                    if (CPLSWorksheetDoc* pDoc = (CPLSWorksheetDoc*)pDocTemplate->OpenDocumentFile(NULL))
                    {
                        ASSERT_KINDOF(CPLSWorksheetDoc, pDoc);

                        pDoc->SetText(m_text.c_str(), m_text.length());
                        pDoc->SetTitle(("Transform " + m_table).c_str());

                        // 22.03.2004 bug fix, CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
                        pDoc->DefaultFileFormat();
                    }
                }

            } _DEFAULT_HANDLER_
        }
    };

void CTableTransformer::OnOK()
{
    UpdateData();
    AfxGetApp()->WriteProfileString("TableTransformer", "TableName", m_strTable);

	_ASSERTE(!m_strSchema.IsEmpty() || !m_strTable.IsEmpty());

    if (!m_strSchema.IsEmpty() && !m_strTable.IsEmpty())
        BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_LoadTableForTransformation((LPCSTR)m_strSchema, (LPCSTR)m_strTable)));

	CDialog::OnOK();
}

//static 
void CTableTransformer::MakeScript (const string& schema, const string& table)
{
    BkgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_LoadTableForTransformation(schema, table)));
}


LRESULT CTableTransformer::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _DEFAULT_HANDLER_

    return 0;
}
