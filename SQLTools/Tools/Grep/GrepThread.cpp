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
#include <COMMON\AppGlobal.h>
#include "COMMON\AppUtilities.h"
#include "SQLTools.h"
#include "GrepThread.h"
#include "GrepDlg.h"
#include "GrepView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace Common;

/////////////////////////////////////////////////////////////////////////////
// CGrepThread

IMPLEMENT_DYNCREATE(CGrepThread, CWinThread)

CGrepThread::CGrepThread ()
: m_pViewResult(0),
  m_hFileList(0),
  m_hReadOutputPipe(0),
  m_hReadErrorPipe(0)
{
    memset(&m_procInfo, 0, sizeof m_procInfo);
}

CGrepThread::~CGrepThread ()
{
    try { 

    if (m_hReadOutputPipe) 
        CloseHandle(m_hReadOutputPipe);
    if (m_hReadErrorPipe)  
        CloseHandle(m_hReadErrorPipe);
  
    if (m_pViewResult->IsAbort()
    && m_procInfo.hProcess)
        VERIFY(TerminateProcess(m_procInfo.hProcess, 0));
  
    m_pViewResult->EndProcess();

    if (m_hFileList)  
        CloseHandle(m_hFileList);
    if (!m_strInputFile.IsEmpty()) 
        DeleteFile(m_strInputFile);

    } _DESTRUCTOR_HANDLER_
}

BEGIN_MESSAGE_MAP(CGrepThread, CWinThread)
	//{{AFX_MSG_MAP(CGrepThread)
	//}}AFX_MSG_MAP
    ON_THREAD_MESSAGE(WM_USER, OnThreadMessage)
END_MESSAGE_MAP()

void CGrepThread::RunGrep (CGrepDlg* pWndGrepDlg, CGrepView* pViewResult)
{
	m_bMathWholeWord    = pWndGrepDlg->m_bMathWholeWord;
	m_bMathCase         = pWndGrepDlg->m_bMathCase;
	m_bUseRegExpr       = pWndGrepDlg->m_bUseRegExpr;
	m_bLookInSubfolders = pWndGrepDlg->m_bLookInSubfolders;
	m_bSaveFiles        = pWndGrepDlg->m_bSaveFiles;
    m_bCollapsedList    = pWndGrepDlg->m_bCollapsedList;
	m_strFileOrType     = pWndGrepDlg->m_strFileOrType;
	m_strFolder         = pWndGrepDlg->m_strFolder;
	m_strWhatFind       = pWndGrepDlg->m_strWhatFind;

	m_pViewResult = pViewResult;
    m_pViewResult->StartProcess(this);

    PostThreadMessage(WM_USER, 0, 0);
}

void CGrepThread::OnThreadMessage (WPARAM, LPARAM)
{
    try // This block need before AfxEndThread
    {
        StringList listFiles;
        AppWalkDir(m_strFolder, m_strFileOrType, listFiles);

        if (m_pViewResult->IsAbort()) 
            AfxThrowUserException();

        CString strTempPath;
        int nBuffLength = GetTempPath(0, 0);
        char* pszBuff = strTempPath.GetBuffer(nBuffLength);
        GetTempPath(nBuffLength, pszBuff);
        strTempPath.ReleaseBuffer(nBuffLength);

        GetTempFileName(strTempPath, "in", 0, m_strInputFile.GetBuffer(MAX_PATH)); 
        m_strInputFile.ReleaseBuffer();

        m_hFileList = CreateFile(
            m_strInputFile,
            GENERIC_WRITE, FILE_SHARE_READ,
            0/*pSecurityAttributes*/, OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL| FILE_ATTRIBUTE_TEMPORARY,
            0
          );

        for(StringListIt it = listFiles.begin(); it != listFiles.end(); it++) 
        {
            DWORD dwBytes;
            *it += "\n";
            WriteFile(m_hFileList, (*it).c_str(), (*it).length(), &dwBytes, 0);
        }
        listFiles.clear();

        string strGrepApp, strGrepOpt, strBuffer;
        AppGetPath(strBuffer);
        strGrepApp = strBuffer + "\\grep.exe";

        strBuffer = " -nH";
        if (!m_bMathCase)     strBuffer += 'i';
        if (m_bMathWholeWord) strBuffer += 'w';
        strBuffer += m_bUseRegExpr  ?  'E' : 'F';

        strGrepOpt = strBuffer;
        strGrepOpt += " \"";
        strGrepOpt += m_strWhatFind;
        strGrepOpt += "\" --file-list=\"";
        strGrepOpt += (const char*)m_strInputFile;
        strGrepOpt +=  "\"";

        strBuffer = "What find: " + m_strWhatFind;
        m_pViewResult->AddEntry(strBuffer.c_str(), TRUE, TRUE);

        SECURITY_ATTRIBUTES saPipe;
        saPipe.nLength = sizeof(SECURITY_ATTRIBUTES); 
        saPipe.lpSecurityDescriptor = NULL; 
        saPipe.bInheritHandle = TRUE; 

        STARTUPINFO startInfo;
        memset(&startInfo, 0, sizeof startInfo);
        startInfo.cb = sizeof startInfo;     
        startInfo.dwFlags     = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
        startInfo.wShowWindow = SW_HIDE; 

        CreatePipe(&m_hReadOutputPipe, &startInfo.hStdOutput, &saPipe, 0);
        CreatePipe(&m_hReadErrorPipe,  &startInfo.hStdError, &saPipe, 0);
        CreateProcess(
            (LPSTR)strGrepApp.c_str(), (LPSTR)strGrepOpt.c_str(),
            NULL,          // lpProcessAttributes
            NULL,          // lpThreadAttributes
            TRUE,          // bInheritHandles
            0,             // dwCreationFlags 
            0,             // lpEnvironment
            0,             // lpCurrentDirectory
            &startInfo,    // lpStartupInfo
            &m_procInfo    // lpProcessInformation
          );

        if (!m_procInfo.hProcess) {
            MessageBeep(MB_ICONHAND);
            AfxMessageBox("Can't run grep.exe!");
        }

        CloseHandle(startInfo.hStdOutput); 
        CloseHandle(startInfo.hStdError); 

        BOOL bSuccess;
        DWORD cchReadBuffer;
        char chReadBuffer[1024];
  
        strBuffer.erase();
        for (;;) 
        { 
            if (m_pViewResult->IsAbort()) 
                AfxThrowUserException();

            bSuccess = ReadFile(m_hReadOutputPipe,     // read handle
                                chReadBuffer,          // buffer for incoming data
                                sizeof(chReadBuffer),  // number of bytes to read
                                &cchReadBuffer,        // number of bytes actually read
                                NULL);                 // no overlapped reading 
            if (bSuccess) 
            {
                for (DWORD i(0); i < cchReadBuffer; ) 
                {
                    if (m_pViewResult->IsAbort()) 
                        AfxThrowUserException();

                    for (DWORD j(0); j + i < cchReadBuffer 
                                    && chReadBuffer[j + i] != '\r'
                                    && chReadBuffer[j + i] != '\n'; j++) 
                    {
                        strBuffer += chReadBuffer[j + i];
                    }

                    if (j + i < cchReadBuffer) 
                    {
                        if (chReadBuffer[j + i] == '\r'
                        || chReadBuffer[j + i] == '\n') j++;
                        if (chReadBuffer[j + i] == '\r'
                        || chReadBuffer[j + i] == '\n') j++;

                        if (!strBuffer.empty()) 
                        {
                            m_pViewResult->AddEntry(strBuffer.c_str(), !m_bCollapsedList);
                            strBuffer.erase();
                        }
                    }
                    i += j;
                }
            }

            if (!bSuccess 
                && (GetLastError() == ERROR_BROKEN_PIPE)) 
                break;                                   // child has died
        }
    
        if (!strBuffer.empty()) 
        {
            m_pViewResult->AddEntry(strBuffer.c_str(), !m_bCollapsedList);
            strBuffer.erase();
        }

    } 
    catch (...)  // This block need before AfxEndThread
    {
    }

    AfxEndThread(0);
}

BOOL CGrepThread::InitInstance() 
{
	return TRUE;
}

