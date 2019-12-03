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

#pragma once
#include <oci.h>
#include "OCI8/Connect.h"
#include "OCI8/Datatypes.h"

namespace OCI8
{
///////////////////////////////////////////////////////////////////////////////
// OCI8::Statement
///////////////////////////////////////////////////////////////////////////////

    enum EStatementType
    {
        StmtUnknown = 0,
        StmtSelect  = OCI_STMT_SELECT,
        StmtUpdate  = OCI_STMT_UPDATE,
        StmtDelete  = OCI_STMT_DELETE,
        StmtInsert  = OCI_STMT_INSERT,
        StmtCreate  = OCI_STMT_CREATE,
        StmtDrop    = OCI_STMT_DROP,
        StmtAlter   = OCI_STMT_ALTER,
        StmtBegin   = OCI_STMT_BEGIN,
        StmtDeclare = OCI_STMT_DECLARE,
        StmtExplain = 15,
        StmtMerge   = 16,
        StmtRollback= 17,
        StmtCommit  = 21,
    };


class Statement : public Object
{
public:
    Statement (Connect&, const char* sttm, int prefetch = 100);
    Statement (Connect&, int prefetch = 100);
    ~Statement ();

	void Prepare (const char* sttm);
    void Execute (ub4 iters = 0, bool guaranteedSafe = false);

    bool Fetch (sword* pstatus = 0);
    bool Fetch (ub4 rows, ub4& outrows, sword* pstatus = 0);

    void Define (int pos, Variable& var);
    void Define (int pos, HostArray& arr);

    void Bind (int pos, Variable& var);
    void Bind (int pos, HostArray& arr);
    void Bind (const char*, Variable& var);
    void Bind (const char*, HostArray& arr);

    bool IsOpen () const { return m_sttmp ? true : false; }
    virtual void Close (bool purge = false);

    Connect&    GetConnect () const     { return m_connect; }
    OCIStmt*    GetOCIStmt () const     { return m_sttmp; }
    OCIError*   GetOCIError () const    { return m_connect.GetOCIError();  }

    bool IsReadOnly ();
    EStatementType GetType ();
    ub4 GetRowCount ();
    ub2 GetSQLFunctionCode ();
    const char* GetSQLFunctionDescription ();
    bool PossibleCompilationErrors ();
    ub2 GetParseErrorOffset ();

    void EnableAutocommit (bool enable) { m_autocommit = enable; }
    bool IsAutocommitEnabled () const { return m_autocommit; }

    bool IsStateInitialized ();
    bool IsStateExecuted ();
    bool IsStateEndOfFetch ();

protected:
    Connect& m_connect;
    OCIStmt* m_sttmp;
    int m_prefetch;
    bool m_autocommit;
    string m_text;

    OCIEnv*    GetOCIEnv ()     { return m_connect.GetOCIEnv(); }
    OCISvcCtx* GetOCISvcCtx ()  { return m_connect.GetOCISvcCtx(); }

    void CHECK_ALLOC (sword status) { m_connect.CHECK_ALLOC(status); }
    void CHECK (sword status)       { m_connect.CHECK(status, m_text.c_str()); }
    void CHECK_INTERRUPT ()         { m_connect.CHECK_INTERRUPT(); }

private:
    // copy-constraction & assign-operation is not supported
    Statement (const Statement&);
    void operator = (const Statement&);
};

///////////////////////////////////////////////////////////////////////////////
};