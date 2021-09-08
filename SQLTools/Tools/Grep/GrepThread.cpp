/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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
#include "GrepImplementation.h"
#include "GrepImplementationInMemory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;
    using namespace Common;
    using namespace GrepImplementation;

/////////////////////////////////////////////////////////////////////////////
// CGrepThread

IMPLEMENT_DYNCREATE(CGrepThread, CWinThread)

CGrepThread::CGrepThread ()
: m_pViewResult(0)
{
}

CGrepThread::~CGrepThread ()
{
    try { 
        m_pViewResult->EndProcess();
    } _DESTRUCTOR_HANDLER_
}

BEGIN_MESSAGE_MAP(CGrepThread, CWinThread)
    //{{AFX_MSG_MAP(CGrepThread)
    //}}AFX_MSG_MAP
    ON_THREAD_MESSAGE(WM_USER, OnThreadMessage)
END_MESSAGE_MAP()

void CGrepThread::RunGrep (CGrepDlg* pWndGrepDlg, CGrepView* pViewResult)
{
    m_MatchWholeWord    = pWndGrepDlg->m_MatchWholeWord;
    m_MatchCase         = pWndGrepDlg->m_MatchCase;
    m_RegExp            = pWndGrepDlg->m_RegExp;
    m_LookInSubfolders  = pWndGrepDlg->m_LookInSubfolders;
    m_SearchInMemory    = pWndGrepDlg->m_SearchInMemory;
    m_CollapsedList     = pWndGrepDlg->m_CollapsedList;
    m_MaskList          = pWndGrepDlg->m_MaskList.IsEmpty() ? "*.*" : pWndGrepDlg->m_MaskList;
    m_FolderList        = pWndGrepDlg->m_finalFolderList.c_str();
    m_SearchText        = pWndGrepDlg->m_SearchText;

    m_pViewResult = pViewResult;
    m_pViewResult->StartProcess(this);

    PostThreadMessage(WM_USER, 0, 0);
}

void CGrepThread::OnThreadMessage (WPARAM, LPARAM)
{
    int totalFiles = 0, matchingFiles = 0, totalMatchingLines = 0;
    set<wstring> processedFiles;

    try { EXCEPTION_FRAME;
    
        if (m_pViewResult->IsAbort()) 
            AfxThrowUserException();

        {
            wostringstream out;
            out << L"Find all: \"" << (const wchar_t*)m_SearchText << L"\"";
            if (m_LookInSubfolders) out << L", Subfolders";
            if (m_RegExp) out << L", RegExp";
            if (m_MatchCase) out << L", MatchCase";
            if (m_MatchWholeWord) out << L", MathWholeWord";
            m_pViewResult->AddInitialInfo(out.str());
        }

        regexp_cxt what;
        what.compile((const wchar_t*)m_SearchText, !m_RegExp, !m_MatchCase, m_MatchWholeWord);

        for_each_folder(
            (const wchar_t*)m_FolderList, 
            (const wchar_t*)m_MaskList,
            m_LookInSubfolders ? true : false, 
            [&](const wstring& filename) 
            { 
                if (processedFiles.find(filename) != processedFiles.end())
                {
                    m_pViewResult->AddInfo(L"The file is already processed: " + filename);
                    return;
                }
                processedFiles.insert(filename);

                if (m_pViewResult->IsAbort()) 
                    AfxThrowUserException();

                int matchingLines = 0;
                bool matchFound = false;

                bool in_memory = false;
                
                if (m_SearchInMemory)
                    in_memory = for_each_file_in_memory (
                        filename, 
                        [&](int line, const wstring& text) 
                        {
                            if (m_pViewResult->IsAbort()) 
                                AfxThrowUserException();

                            int start, end;
                            if (what.find(text.c_str(), text.length(), start, end))
                            {
                                matchingLines++;
                                matchFound = true;
                                totalMatchingLines++;
                                m_pViewResult->AddFoundMatch(filename, text, line, start, end, matchingLines, totalMatchingLines, !m_CollapsedList, true/*in memory*/);
                            }
                        },
                        [&](const wstring& file, const wstring& err)
                        {
                            wostringstream out;
                            out << L"Error while processing the file \"" << file << L"\" : " << err;
                            m_pViewResult->AddError(out.str());
                        }
                    );

                if (!in_memory)
                    for_each_file(
                        filename, 
                        [&](int line, const wstring& text) 
                        {
                            if (m_pViewResult->IsAbort()) 
                                AfxThrowUserException();

                            int start, end;
                            if (what.find(text.c_str(), text.length(), start, end))
                            {
                                matchingLines++;
                                matchFound = true;
                                totalMatchingLines++;
                                m_pViewResult->AddFoundMatch(filename, text, line, start, end, matchingLines, totalMatchingLines, !m_CollapsedList);
                            }
                        },
                        [&](const wstring& file, const wstring& err)
                        {
                            wostringstream out;
                            out << L"Error while processing the file \"" << file << L"\" : " << err;
                            m_pViewResult->AddError(out.str());
                        }
                    ); 

                if (matchFound)
                    matchingFiles++;
                totalFiles++;
            },
            [&](const wstring& folder, const wstring& err)
            {
                
                wostringstream out;
                out << L"Error while processing the folder \"" << folder << L"\" : " << err;
                m_pViewResult->AddError(out.str());
            }
        );

    } 
    catch (CUserException*)
    {
        m_pViewResult->AddError(L"User cancelled search.");
    }
    catch (std::exception& e)
    {
        m_pViewResult->AddError(Common::wstr(e.what()));
    }
    _OE_DEFAULT_HANDLER_;

    {
        wostringstream out;
        out << L"Matching lines: " << totalMatchingLines << L"  Matching files: " << matchingFiles << L"  Total files searched: " << totalFiles;
        m_pViewResult->AddInfo(out.str());
    }

    AfxEndThread(0);
}

BOOL CGrepThread::InitInstance() 
{
    return TRUE;
}

