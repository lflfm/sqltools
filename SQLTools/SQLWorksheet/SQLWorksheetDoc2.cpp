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

// 05.06.2003 small improvement, thousand delemiters for cost,cardinality and bytes in Explain Plan viewer 
// 07.12.2003 bug fix, explain plan result may contain duplicated records if you press F9 too fast
// 07.12.2003 bug fix, explain plan table is not cleared after fetching result
// 2014-05-11 bug fix, read-only connection does not allow explain plan

#include "stdafx.h"
#include "SQLTools.h"
#include "SQLWorksheetDoc.h"
#include <SQLWorksheet\CommandParser.h>
#include "MainFrm.h"
#include "COMMON\StrHelpers.h"
#include "COMMON\ExceptionHelper.h"
#include "SQLToolsSettings.h"
#include <OCI8/BCursor.h>
#include "ExplainPlanView2.h"
#include "ExplainPlanTextView.h"
#include "ExplainPlanSource.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include "DocumentProxy.h"

using namespace ServerBackgroundThread;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc

static const char expl_xplan[] = "select plan_table_output from table(dbms_xplan.display('<PLAN_TABLE>', :sttm_id, 'ALL'))";

static const char expl_xplan_display_cursor[] = "select plan_table_output from table(dbms_xplan.display_cursor(:sql_id, :child_number, 'ALLSTATS LAST'))";

LPSCSTR expl_sttm = 
    "SELECT * "
        "FROM <PLAN_TABLE> "
            "WHERE statement_id = :sttm_id "
            "ORDER BY id, position";

static const char expl_clear[] = "DELETE FROM <PLAN_TABLE> WHERE statement_id = :sttm_id";

//TODO#2: move in SqlPlanPerformerImpl::DoExecuteSql

	struct BackgroundTask_ExplainPlan : Task
    {
        CPLSWorksheetDoc& m_doc;
        DocumentProxy m_proxy;
        string m_text;
        string m_plan_table;
        string m_text_plan_output;
        counted_ptr<ExplainPlanSource> m_source;

        BackgroundTask_ExplainPlan (CPLSWorksheetDoc& doc, const string& text) 
            : Task("Explain Plan", 0),
            m_doc(doc),
            m_proxy(doc),
            m_text(text),
            m_source(new ExplainPlanSource)
        {
            m_plan_table = GetSQLToolsSettings().GetPlanTable();
        }

        void DoInBackground (OciConnect& connect)
        {
            try
            {
                m_proxy.SetActivePrimeExecution(true);

                srand((UINT)time(NULL));
        
                char sttm_id[80];
                _snprintf(sttm_id, sizeof(sttm_id), "SQLTOOLS.%d", rand());
                sttm_id[sizeof(sttm_id)-1] = 0;

                Common::Substitutor subst;
                subst.AddPair("<PLAN_TABLE>", m_plan_table);
                subst.AddPair("<STTM_ID>", sttm_id);
                subst << "EXPLAIN PLAN SET STATEMENT_ID = \'<STTM_ID>\' INTO <PLAN_TABLE> FOR ";

                OCI8::Statement sttm(connect);
                sttm.Prepare((std::string(subst.GetResult()) + m_text).c_str());
                sttm.Execute(1, true);

                subst.ResetContent();
                subst.ResetResult();
                subst.AddPair("<PLAN_TABLE>", m_plan_table);
                subst << expl_sttm;

                OciCursor explain_curs(connect, subst.GetResult());
                explain_curs.Bind(":sttm_id", sttm_id);

                explain_curs.Execute();

                int col_statement_id      = -1;
                int col_plan_id           = -1;
                int col_timestamp         = -1;
                int col_remarks           = -1;
                int col_operation         = -1;
                int col_options           = -1;
                int col_object_node       = -1;
                int col_object_owner      = -1;
                int col_object_name       = -1;
                int col_object_alias      = -1;
                int col_object_instance   = -1;
                int col_object_type       = -1;
                int col_optimizer         = -1;
                int col_search_columns    = -1;
                int col_id                = -1;
                int col_parent_id         = -1;
                int col_depth             = -1;
                int col_position          = -1;
                int col_cost              = -1;
                int col_cardinality       = -1;
                int col_bytes             = -1;
                int col_other_tag         = -1;
                int col_partition_start   = -1;
                int col_partition_stop    = -1;
                int col_partition_id      = -1;
                int col_other             = -1;
                int col_other_xml         = -1;
                int col_distribution      = -1;
                int col_cpu_cost          = -1;
                int col_io_cost           = -1;
                int col_temp_space        = -1;
                int col_access_predicates = -1;
                int col_filter_predicates = -1;
                int col_projection        = -1;
                int col_time              = -1;
                int col_qblock_name       = -1;

                int ncount = explain_curs.GetFieldCount();
                for (int i = 0; i < ncount; ++i)
                {
                    const string& name = explain_curs.GetFieldName(i);

                         if (!stricmp(name.c_str(), "statement_id"     )) col_statement_id      = i;
                    else if (!stricmp(name.c_str(), "plan_id"          )) col_plan_id           = i;
                    else if (!stricmp(name.c_str(), "timestamp"        )) col_timestamp         = i;
                    else if (!stricmp(name.c_str(), "remarks"          )) col_remarks           = i;
                    else if (!stricmp(name.c_str(), "operation"        )) col_operation         = i;
                    else if (!stricmp(name.c_str(), "options"          )) col_options           = i;
                    else if (!stricmp(name.c_str(), "object_node"      )) col_object_node       = i;
                    else if (!stricmp(name.c_str(), "object_owner"     )) col_object_owner      = i;
                    else if (!stricmp(name.c_str(), "object_name"      )) col_object_name       = i;
                    else if (!stricmp(name.c_str(), "object_alias"     )) col_object_alias      = i;
                    else if (!stricmp(name.c_str(), "object_instance"  )) col_object_instance   = i;
                    else if (!stricmp(name.c_str(), "object_type"      )) col_object_type       = i;
                    else if (!stricmp(name.c_str(), "optimizer"        )) col_optimizer         = i;
                    else if (!stricmp(name.c_str(), "search_columns"   )) col_search_columns    = i;
                    else if (!stricmp(name.c_str(), "id"               )) col_id                = i;
                    else if (!stricmp(name.c_str(), "parent_id"        )) col_parent_id         = i;
                    else if (!stricmp(name.c_str(), "depth"            )) col_depth             = i;
                    else if (!stricmp(name.c_str(), "position"         )) col_position          = i;
                    else if (!stricmp(name.c_str(), "cost"             )) col_cost              = i;
                    else if (!stricmp(name.c_str(), "cardinality"      )) col_cardinality       = i;
                    else if (!stricmp(name.c_str(), "bytes"            )) col_bytes             = i;
                    else if (!stricmp(name.c_str(), "other_tag"        )) col_other_tag         = i;
                    else if (!stricmp(name.c_str(), "partition_start"  )) col_partition_start   = i;
                    else if (!stricmp(name.c_str(), "partition_stop"   )) col_partition_stop    = i;
                    else if (!stricmp(name.c_str(), "partition_id"     )) col_partition_id      = i;
                    else if (!stricmp(name.c_str(), "other"            )) col_other             = i;
                    else if (!stricmp(name.c_str(), "other_xml"        )) col_other_xml         = i;
                    else if (!stricmp(name.c_str(), "distribution"     )) col_distribution      = i;
                    else if (!stricmp(name.c_str(), "cpu_cost"         )) col_cpu_cost          = i;
                    else if (!stricmp(name.c_str(), "io_cost"          )) col_io_cost           = i;
                    else if (!stricmp(name.c_str(), "temp_space"       )) col_temp_space        = i;
                    else if (!stricmp(name.c_str(), "access_predicates")) col_access_predicates = i;
                    else if (!stricmp(name.c_str(), "filter_predicates")) col_filter_predicates = i;
                    else if (!stricmp(name.c_str(), "projection"       )) col_projection        = i;
                    else if (!stricmp(name.c_str(), "time"             )) col_time              = i;
                    else if (!stricmp(name.c_str(), "qblock_name"      )) col_qblock_name       = i;
                }


                while (explain_curs.Fetch()) 
                {
                    plan_table_record rec;

                    if (col_statement_id      != -1) explain_curs.GetString(col_statement_id      , rec.statement_id      ); 
                    if (col_timestamp         != -1) explain_curs.GetString(col_timestamp         , rec.timestamp         ); 
                    if (col_remarks           != -1) explain_curs.GetString(col_remarks           , rec.remarks           ); 
                    if (col_operation         != -1) explain_curs.GetString(col_operation         , rec.operation         ); 
                    if (col_options           != -1) explain_curs.GetString(col_options           , rec.options           ); 
                    if (col_object_node       != -1) explain_curs.GetString(col_object_node       , rec.object_node       ); 
                    if (col_object_owner      != -1) explain_curs.GetString(col_object_owner      , rec.object_owner      ); 
                    if (col_object_name       != -1) explain_curs.GetString(col_object_name       , rec.object_name       ); 
                    if (col_object_alias      != -1) explain_curs.GetString(col_object_alias      , rec.object_alias      ); 
                    if (col_object_type       != -1) explain_curs.GetString(col_object_type       , rec.object_type       ); 
                    if (col_optimizer         != -1) explain_curs.GetString(col_optimizer         , rec.optimizer         ); 
                    if (col_other_tag         != -1) explain_curs.GetString(col_other_tag         , rec.other_tag         ); 
                    if (col_partition_start   != -1) explain_curs.GetString(col_partition_start   , rec.partition_start   ); 
                    if (col_partition_stop    != -1) explain_curs.GetString(col_partition_stop    , rec.partition_stop    ); 
                    if (col_other             != -1) explain_curs.GetString(col_other             , rec.other             ); 
                    if (col_other_xml         != -1) explain_curs.GetString(col_other_xml         , rec.other_xml         ); 
                    if (col_distribution      != -1) explain_curs.GetString(col_distribution      , rec.distribution      ); 
                    if (col_access_predicates != -1) explain_curs.GetString(col_access_predicates , rec.access_predicates ); 
                    if (col_filter_predicates != -1) explain_curs.GetString(col_filter_predicates , rec.filter_predicates ); 
                    if (col_projection        != -1) explain_curs.GetString(col_projection        , rec.projection        ); 
                    if (col_qblock_name       != -1) explain_curs.GetString(col_qblock_name       , rec.qblock_name       );
 
                    if (col_plan_id           != -1) rec.plan_id         = explain_curs.ToInt64(col_plan_id         , LLONG_MAX); 
                    if (col_object_instance   != -1) rec.object_instance = explain_curs.ToInt64(col_object_instance , LLONG_MAX); 
                    if (col_search_columns    != -1) rec.search_columns  = explain_curs.ToInt64(col_search_columns  , LLONG_MAX); 
                    if (col_id                != -1) rec.id              = explain_curs.ToInt64(col_id              , LLONG_MAX); 
                    if (col_parent_id         != -1) rec.parent_id       = explain_curs.ToInt64(col_parent_id       , LLONG_MAX); 
                    if (col_depth             != -1) rec.depth           = explain_curs.ToInt64(col_depth           , LLONG_MAX); 
                    if (col_position          != -1) rec.position        = explain_curs.ToInt64(col_position        , LLONG_MAX); 
                    if (col_cost              != -1) rec.cost            = explain_curs.ToInt64(col_cost            , LLONG_MAX); 
                    if (col_cardinality       != -1) rec.cardinality     = explain_curs.ToInt64(col_cardinality     , LLONG_MAX); 
                    if (col_bytes             != -1) rec.bytes           = explain_curs.ToInt64(col_bytes           , LLONG_MAX); 
                    if (col_partition_id      != -1) rec.partition_id    = explain_curs.ToInt64(col_partition_id    , LLONG_MAX); 
                    if (col_cpu_cost          != -1) rec.cpu_cost        = explain_curs.ToInt64(col_cpu_cost        , LLONG_MAX); 
                    if (col_io_cost           != -1) rec.io_cost         = explain_curs.ToInt64(col_io_cost         , LLONG_MAX); 
                    if (col_temp_space        != -1) rec.temp_space      = explain_curs.ToInt64(col_temp_space      , LLONG_MAX); 
                    if (col_time              != -1) rec.time            = explain_curs.ToInt64(col_time            , LLONG_MAX); 

                    m_source->Append(rec);
                }

                explain_curs.Close();

                if (connect.GetVersion() >= OCI8::esvServer9X)
		        {
			        subst.ResetContent();
			        subst.ResetResult();
			        subst.AddPair("<PLAN_TABLE>", GetSQLToolsSettings().GetPlanTable());
			        subst << expl_xplan;

			        explain_curs.Prepare(subst.GetResult());
			        explain_curs.Bind(":sttm_id", sttm_id);

			        explain_curs.Execute();
			        while (explain_curs.Fetch()) 
			        {
                        string buff;
				        explain_curs.GetString(0, buff);
				        m_text_plan_output += buff;
				        m_text_plan_output += "\r\n";
			        }

	                explain_curs.Close();
		        }
            }
            catch (const OciException& x)
            {
                SetError(x.what());
            }
            catch (const DocumentProxy::document_destroyed& x)
            {
                SetError(x.what());
                m_proxy.SetActivePrimeExecution(false);
                throw;
            }
            catch (...)
            {
                m_proxy.SetActivePrimeExecution(false);
                throw;
            }

            m_proxy.SetActivePrimeExecution(false);
        }

        void ReturnInForeground ()
        {
            if (m_proxy.IsOk()) // check if document was not closed
            {
                m_doc.GetExplainPlanView()->SetSource(m_source);
	            m_doc.GetExplainPlanTextView()->SetWindowText(Common::wstr(m_text_plan_output).c_str());
                m_doc.ActivatePlanTab();
            }
        }
    };

void CPLSWorksheetDoc::DoSqlExplainPlan (const string& text)
{
    FrgdRequestQueue::Get().Push(TaskPtr(new BackgroundTask_ExplainPlan(*this, text)));
}
