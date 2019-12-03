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

// 22.04.2002 "Load DDL script" fails on table with trigger which body is bigger then 4K
// 14.04.2003 bug fix, Oracle9i and UTF8 data truncation
// 08.02.2004 bug fix, DDL reengineering: 64K limit for trigger increaced to 512M
// 17.06.2005 B#1119107 - SYS. & dba_ views for DDL reverse-engineering (tMk)
// 19.01.2006 B#1404162 - Trigger with CALL - unsupported syntax (tMk)
// 22.10.2006 B#XXXXXXX - (ak) compatibily issue with Oracle & and 8.0.X

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
    LPSCSTR csz_trg_sttm =
		"SELECT <RULE> "
            "owner,"
            "trigger_name,"
            "table_owner,"
            "table_name,"
            "referencing_names,"
            "when_clause,"
            "description,"
            "trigger_body,"
            "status,"
            "<ACTION_TYPE>,"
			"<BASE_OBJECT_TYPE> "
			//"trigger_type,"
			//"triggering_event,"
        "FROM sys.all_triggers "
        "WHERE <OWNER> = :owner "       // may be "owner" or "table_owner"
//#pragma message ("   Loss of functionality - dll and database trigger support")
//            "AND table_name IS NOT NULL " 
            "AND <NAME> <EQUAL> :name"; // may be "trigger_name" or "table_name"

    const int cn_owner             = 0;
    const int cn_trigger_name      = 1;
    const int cn_table_owner       = 2;
    const int cn_table_name        = 3;
    const int cn_ref_names         = 4;
    const int cn_when_clause       = 5;
    const int cn_description       = 6;
    const int cn_trigger_body      = 7;
    const int cn_status            = 8;
	const int cn_action_type       = 9;
	const int cn_base_object_type  = 10;

    static
    void load_trigger (
        OciConnect& con, Dictionary& dict,
        const char* owner, const char* name, bool byTable, bool useLike, size_t maxTextSize
        );

void Loader::LoadTriggers (const char* owner, const char* name, bool byTable, bool useLike)
{
#if TEST_LOADING_OVERFLOW
    load_trigger(m_connect, m_dictionary, owner, name, byTable, useLike, 32);
#else
    load_trigger(m_connect, m_dictionary, owner, name, byTable, useLike, 1024);
#endif
}

    static
    void load_trigger (
        OciConnect& con, Dictionary& dict,
        const char* owner, const char* name, bool byTable, bool useLike, size_t maxTextSize
        )
    {
        Common::Substitutor subst;
        subst.AddPair("<EQUAL>", useLike   ? "like" : "=");
        subst.AddPair("<OWNER>", byTable  ? "table_owner" : "owner");
        subst.AddPair("<NAME>",  byTable  ? "table_name"  : "trigger_name");
        subst.AddPair("<RULE>", con.GetVersion() < OCI8::esvServer10X ? "/*+RULE*/" : "");

        subst.AddPair("<ACTION_TYPE>", con.GetVersion() > OCI8::esvServer80X ? "action_type" : "NULL");
        subst.AddPair("<BASE_OBJECT_TYPE>", con.GetVersion() > OCI8::esvServer9X ? "base_object_type" : "'TABLE'");
        
        subst << csz_trg_sttm;

        OciCursor cur(con, subst.GetResult(), useLike ? 32 : 1, maxTextSize);
        
        cur.Bind(":owner", owner);
        cur.Bind(":name",  name);

        cur.Execute();
        while (cur.Fetch())
        {
            if (cur.IsRecordGood())
            {
//                _CHECK_AND_THROW_(!cur.IsNull(cn_table_name) ,"Trigger loading error:\nunsupporded trigger type!");

                Trigger& trigger = dict.CreateTrigger(cur.ToString(cn_owner).c_str(), 
                                                      cur.ToString(cn_trigger_name).c_str());

                cur.GetString(cn_owner,       trigger.m_strOwner);
                cur.GetString(cn_trigger_name,trigger.m_strName);
                cur.GetString(cn_table_owner, trigger.m_strTableOwner);
                cur.GetString(cn_table_name,  trigger.m_strTableName);
                cur.GetString(cn_description, trigger.m_strDescription);
                cur.GetString(cn_when_clause, trigger.m_strWhenClause);
                cur.GetString(cn_ref_names,   trigger.m_strRefNames);
                cur.GetString(cn_trigger_body,trigger.m_strTriggerBody);
                cur.GetString(cn_status,      trigger.m_strStatus);
				cur.GetString(cn_action_type, trigger.m_strActionType);
				cur.GetString(cn_base_object_type, trigger.m_strBaseObjectType);

				if (trigger.m_strActionType == "CALL") {
					// m_strTriggerBody contains for example "proc_testcall(:NEW.x);" and
					// this is not accepted syntax for Oracle (ORA-00911 - bad character ;)!!! 
					//
					// I think that this is a bug of DB, but I can't find it on MetaLink.
					// We must truncate last character to get a 'proper' syntax.
					if(*trigger.m_strTriggerBody.rbegin() == ';') {
						trigger.m_strTriggerBody.resize(trigger.m_strTriggerBody.size()-1);
					}
				}

                string strKey;
                strKey = trigger.m_strOwner + '.' + trigger.m_strName;

                try 
                {
                    Table& table = dict.LookupTable(trigger.m_strTableOwner.c_str(), trigger.m_strTableName.c_str());
                    table.m_setTriggers.insert(strKey);
                } 
                catch (const XNotFound&) 
                {
                    try 
                    {
                        View& view = dict.LookupView(trigger.m_strTableOwner.c_str(), trigger.m_strTableName.c_str());
                        view.m_setTriggers.insert(strKey);

                    } catch (const XNotFound&) {}
                }
            } 
            else 
            {
                const size_t progression[] = { 64 * 1024U, 512 * 1024U };

                for (unsigned i = 0; i < sizeof(progression)/sizeof(progression[0]); i++)
                    if (maxTextSize < progression[i]) {
                        load_trigger(con, dict, owner, cur.ToString(cn_trigger_name).c_str(),
                                    false/*byTable*/, false/*useLike*/, progression[i]);
                        break;
                    }

                _CHECK_AND_THROW_(i < sizeof(progression)/sizeof(progression[0]), 
                                  "Trigger loading error:\n text size exceed 512K!");
            }
        }
    }

}// END namespace OraMetaDict
