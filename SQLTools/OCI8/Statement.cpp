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
#include "OCI8/Statement.h"
#include "OCI8/SQLFunctionCodes.h"

// 27.06.2004 bug fix, Autocommit does not work
// 28.09.2006 (ak) bXXXXXXX: poor fetch performance because prefetch rows disabled

#ifdef OCI_TRACE_STACK_ON_THROW
#define OCI_RAISE(x)	 do { Common::TraceStackOnThrow(); throw (x); } while(false)
#else
#define OCI_RAISE(x)	 do { throw (x); } while(false)
#endif

namespace OCI8
{
    // 10.09.2003 bug fix, 8.0.5 compatibility
    const int OCI80X_ATTR_PARSE_ERROR_OFFSET = 128;

///////////////////////////////////////////////////////////////////////////////
// OCI8::Statement
///////////////////////////////////////////////////////////////////////////////

Statement::Statement (Connect& connect, const char* sttm, int prefetch)
    : Object(connect),
    m_connect(connect),
    m_sttmp(0),
    m_prefetch(prefetch),
    m_autocommit(false)
{
    Prepare(sttm);
}

Statement::Statement (Connect& connect, int prefetch)
    : Object(connect),
    m_connect(connect),
    m_sttmp(0),
    m_prefetch(prefetch),
    m_autocommit(false)
{
}

Statement::~Statement ()
{
    try { EXCEPTION_FRAME;

        Close();
    }
    _DESTRUCTOR_HANDLER_;
}

void Statement::Close (bool /*purge*/)
{
    if (m_sttmp)
        OCIHandleFree(m_sttmp, OCI_HTYPE_STMT);

    m_sttmp = 0;
}

void Statement::Prepare (const char* sttm)
{
    _ASSERTE(!m_sttmp);

    CHECK_INTERRUPT();

    m_text = sttm;

    CHECK_ALLOC(OCIHandleAlloc(GetOCIEnv(), (dvoid**)&m_sttmp, OCI_HTYPE_STMT, 0, 0));
    //8171  1315603 when fetching NULL data.
    //              ie: indp / rcodep may be incorrect.
    //              Workaround: Set OCI_ATTR_PREFETCH_ROWS to 0
    //              This problem was introduced in 8.1.6
    if (Connect::GetClientVersion() >= ecvClient9X)
        CHECK(OCIAttrSet(m_sttmp, OCI_HTYPE_STMT, &m_prefetch, 0, OCI_ATTR_PREFETCH_ROWS, GetOCIError()));
    CHECK(OCIStmtPrepare(m_sttmp, GetOCIError(), (oratext*)sttm, strlen(sttm), OCI_NTV_SYNTAX, OCI_DEFAULT));
}

void Statement::Execute (ub4 iters, bool guaranteedSafe)
{
    ASSERT_EXCEPTION_FRAME;
    CHECK_INTERRUPT();

    if (!guaranteedSafe
    && m_connect.IsReadOnly()
    && !IsReadOnly())
        OCI_RAISE(Exception(0, "OCI8::Statement::Execute: Only SELECT is allowed for Read-Only connection!"));

    // 27.06.2004 bug fix, Autocommit does not work
    sword status = OCIStmtExecute(GetOCISvcCtx(), m_sttmp, GetOCIError(), iters, 
        0, (OCISnapshot*)NULL, (OCISnapshot*)NULL, m_autocommit ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT);

    switch (status)
    {
    case OCI_SUCCESS:
    case OCI_SUCCESS_WITH_INFO:
        break;
    default:
        CHECK(status);
    }

    m_connect.SetLastExecutionClockTime();
}

bool Statement::Fetch (sword* pstatus)
{
    CHECK_INTERRUPT();

    bool retVal = true;

    sword status = OCIStmtFetch(m_sttmp, GetOCIError(), 1, OCI_FETCH_NEXT, OCI_DEFAULT);

    if (pstatus)
        *pstatus = status;

    switch (status)
    {
    case OCI_NO_DATA:
        retVal = false;
    case OCI_SUCCESS:
    case OCI_SUCCESS_WITH_INFO:
        break;
    default:
        CHECK(status);
    }

    m_connect.SetLastExecutionClockTime();

    return retVal;
}

bool Statement::Fetch (ub4 rows, ub4& outrows, sword* pstatus)
{
    CHECK_INTERRUPT();

    bool retVal = true;

    sword status = OCIStmtFetch(m_sttmp, GetOCIError(), rows, OCI_FETCH_NEXT, OCI_DEFAULT);

    if (pstatus)
        *pstatus = status;

    switch (status)
    {
    case OCI_NO_DATA:
        retVal = false;
    case OCI_SUCCESS:
    case OCI_SUCCESS_WITH_INFO:
        break;
    default:
        CHECK(status);
    }

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &outrows, (ub4 *)0, OCI_ATTR_ROW_COUNT, GetOCIError()));

    m_connect.SetLastExecutionClockTime();

    return retVal;
}

void Statement::Define (int pos, Variable& var)
{
    CHECK_INTERRUPT();
    var.Define(*this, pos);
}

void Statement::Define (int pos, HostArray& arr)
{
    CHECK_INTERRUPT();
    arr.Define(*this, pos);
}

void Statement::Bind (int pos, Variable& var)
{
    CHECK_INTERRUPT();
    var.Bind(*this, pos);
}

void Statement::Bind (int pos, HostArray& arr)
{
    CHECK_INTERRUPT();
    arr.Bind(*this, pos);
}

void Statement::Bind (const char* name, Variable& var)
{
    CHECK_INTERRUPT();
    var.Bind(*this, name);
}

void Statement::Bind (const char* name, HostArray& arr)
{
    CHECK_INTERRUPT();
    arr.Bind(*this, name);
}

bool Statement::IsReadOnly ()
{
    ub2 type;
    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &type, (ub4 *)0, OCI_ATTR_STMT_TYPE, GetOCIError()));

    switch (type)
    {
    case StmtSelect:
    case StmtExplain:
        return true;
    }

    return false;
}

EStatementType Statement::GetType ()
{
    ub2 type;
    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &type, (ub4 *)0, OCI_ATTR_STMT_TYPE, GetOCIError()));

    switch (type)
    {
    case StmtUnknown: // it can be comment either on table or column
    case StmtSelect:
    case StmtUpdate:
    case StmtDelete:
    case StmtInsert:
    case StmtCreate:
    case StmtDrop:
    case StmtAlter:
    case StmtBegin:
    case StmtDeclare:
    case StmtExplain:
    case StmtMerge:
    case StmtRollback:
    case StmtCommit:
        return static_cast<EStatementType>(type);
        break;
    default:
        _ASSERTE(0);
    }

    return StmtUnknown;
}

ub4 Statement::GetRowCount ()
{
    ub4 rows;
    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &rows, (ub4 *)0, OCI_ATTR_ROW_COUNT, GetOCIError()));
    return rows;
}

ub2 Statement::GetSQLFunctionCode ()
{
    ub2 fncode;
    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &fncode, (ub4 *)0, OCI_ATTR_SQLFNCODE, GetOCIError()));
    return fncode;
}

const char* Statement::GetSQLFunctionDescription ()
{
    return OCI8::GetSQLFunctionDescription(GetSQLFunctionCode());
}

ub2 Statement::GetParseErrorOffset ()
{
    ub2 offset;
    // 10.09.2003 bug fix, 8.0.5 compatibility
    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &offset, (ub4 *)0, 
        Connect::GetClientVersion() == ecvClient80X ? OCI80X_ATTR_PARSE_ERROR_OFFSET : OCI_ATTR_PARSE_ERROR_OFFSET, GetOCIError()));
    return offset;
}


bool Statement::IsStateInitialized ()
{
    ub4 state;

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &state, (ub4 *)0, OCI_ATTR_STMT_STATE, GetOCIError()));

    return state == OCI_STMT_STATE_INITIALIZED;
}

bool Statement::IsStateExecuted ()
{
    ub4 state;

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &state, (ub4 *)0, OCI_ATTR_STMT_STATE, GetOCIError()));

    return state == OCI_STMT_STATE_EXECUTED;
}

bool Statement::IsStateEndOfFetch ()
{
    ub4 state;

    CHECK(OCIAttrGet(m_sttmp, OCI_HTYPE_STMT, &state, (ub4 *)0, OCI_ATTR_STMT_STATE, GetOCIError()));

    return state == OCI_STMT_STATE_END_OF_FETCH;
}

bool Statement::PossibleCompilationErrors ()
{
    return OCI8::PossibleCompilationErrors(GetSQLFunctionCode());
}

///////////////////////////////////////////////////////////////////////////////
};