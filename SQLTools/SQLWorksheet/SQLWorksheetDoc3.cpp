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
#include "SQLTools.h"
#include "SQLWorksheetDoc.h"
#include "COMMON/TempFilesManager.h"
#include <Common/StrHelpers.h>
#include "COMMON\AppGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPLSWorksheetDoc

/*
    check if sqlplus can be found (if not then use the current oracle home as path )

    //            SetCurrentDirectory(path);
    //            Common::Substitutor subst;
    //            subst.AddPair("$(SQLPLUS)", sqlplus.c_str());
    //            subst << GetSQLToolsSettings().GetSQLPlusExecutable().c_str();
    //            string sqlplusExecutable = subst.GetResult();
    //            if (PathFindOnPath(sqlplusExecutable.c_str()))
    //            {
    //            ....
    //            }
*/

/*
    2011.09.30 Execute in SQL*Plus taken from sqltools++ with minor changes
*/
void CPLSWorksheetDoc::OnSqlExecuteInSQLPlus ()
{
    if (!theApp.GetConnectOpen())
    {
        MessageBeep((UINT)-1);
        AfxMessageBox("Not connected to database.", MB_OK|MB_ICONSTOP);
        return;
    }

    string sqlplus = "SQLPLUS";

    bool bWriteToTempFile;
    string sFilename;
    OpenEditor::Square sel;
    m_pEditor->GetSelection(sel);
    sel.normalize();
    int nlines = m_pEditor->GetLineCount();

    bWriteToTempFile = false;

    if (sel.is_empty())
    {
        if (IsModified())
        {
            switch (AfxMessageBox("Do you want to save the file before executing SQL*Plus?", MB_YESNOCANCEL|MB_ICONQUESTION))
            {
            case IDCANCEL:
                return;
            case IDYES:
                if (DoFileSave())
                {
                    sFilename = GetPathName();
                }
                else
                    return;

                break;

            case IDNO:
                bWriteToTempFile = true;
                sel.start.line = 0;
                sel.end.line = nlines - 1;
                sel.start.column = 0;
                sel.end.column = INT_MAX;
            }
        }
        else
        {
            sFilename = GetPathName();
            if (sFilename.empty() || (sFilename == ""))
            {
                MessageBeep((UINT)-1);
                AfxMessageBox("Running an empty document in SQL*Plus is not supported.", MB_OK|MB_ICONSTOP);
                return;
            }
        }
    }
    else
    {
        bWriteToTempFile = true;
    }

    if (bWriteToTempFile)
    {
        sel.start.column = m_pEditor->PosToInx(sel.start.line, sel.start.column);
        sel.end.column   = m_pEditor->PosToInx(sel.end.line, sel.end.column);

        sFilename = TempFilesManager::CreateFile("SQL");

        if (!sFilename.empty())
        {
            HANDLE hFile = ::CreateFile(sFilename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                MessageBeep((UINT)-1);
                AfxMessageBox("Cannot open temporary file for SQL*Plus.", MB_OK|MB_ICONSTOP);
                return;
            }
            DWORD written;

            int line = sel.start.line;
            int offset = sel.start.column;
            int len;
            const char* str;
            const char lf[] = "\r\n";

            for (; line < nlines && line <= sel.end.line; line++)
            {
                m_pEditor->GetLine(line, str, len);

                if (line == sel.end.line)
                    len = min(len, sel.end.column);

                if (len > 0)
                    WriteFile(hFile, str + offset, len - offset, &written, 0);

                WriteFile(hFile, lf, sizeof(lf) - 1, &written, 0);
                
				offset = 0;
            }

            CloseHandle(hFile);
        }
        else
        {
            MessageBeep((UINT)-1);
            AfxMessageBox("Cannot generate a temporary file for SQL*Plus.", MB_OK|MB_ICONSTOP);
            return;
        }
    }

    Common::Substitutor subst;
    subst.AddPair("$(SQLPLUS)", sqlplus.c_str());
    subst.AddPair("$(USER)", theApp.GetConnectUID());
    subst.AddPair("$(PASSWORD)", theApp.GetConnectPassword());
    string sys_privs;
    switch (theApp.GetConnectMode())
    {
        case OCI8::ecmSysDba:  sys_privs = " AS SYSDBA"; break;
        case OCI8::ecmSysOper: sys_privs = " AS SYSOPER"; break;
    }
    subst.AddPair("$(SYS_PRIVS)", sys_privs);
    subst.AddPair("$(CONNECT_STRING)", theApp.GetConnectAlias());
    subst.AddPair("$(FILENAME)", string(string("\"") + sFilename + "\"").c_str());
    subst << GetSQLToolsSettings().GetSQLPlusExecutable().c_str()
        << " " << GetSQLToolsSettings().GetSQLPlusCommandLine().c_str();

    string sParameter = subst.GetResult();

    {
        CString path = bWriteToTempFile ? sFilename.c_str() : GetPathName();
        if (!path.IsEmpty())
        {
            PathRemoveFileSpec(path.LockBuffer());
            path.UnlockBuffer();
            if (!path.IsEmpty())
            {
                SetCurrentDirectory(path);
                Global::SetStatusText("Executing SQL*Plus from " + path);
            }
        }
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    if( !CreateProcess( NULL, // module name
        (LPSTR) sParameter.c_str(),// Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE                              `
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        MessageBeep((UINT)-1);
        AfxMessageBox((string("Spawning SQL*Plus failed. Command line used:\n") + sParameter).c_str(), MB_OK|MB_ICONSTOP);
        return;
    }
}