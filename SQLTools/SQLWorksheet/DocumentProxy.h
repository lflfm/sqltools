/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2016 Aleksey Kochetov

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

#include "ThreadCommunication/MessageOnlyWindow.h"
#include "SQLWorksheetDoc.h"

class CPLSWorksheetDoc;
namespace ErrorLoader { struct comp_error; };

class DocumentProxy //: nocopy!
{
    CPLSWorksheetDoc& m_doc;
    HWND m_hMainDocWnd;
    ThreadCommunication::MessageOnlyWindow& m_msgWnd;

    void send (ThreadCommunication::Note&);
public:
    struct document_destroyed : std::exception { document_destroyed(); };

    DocumentProxy (CPLSWorksheetDoc& doc);
    
    bool IsOk ();
    void TestDocumnet ();

    void SetActivePrimeExecution (bool on);

    void SetHighlighting  (int fistHighlightedLine, int lastHighlightedLine, bool updateWindow = false);

    void GoTo (int line);
    void PutError (const string& str, int line =-1, int pos =-1, bool skip = false);
    void PutMessage (const string& str, int line =-1, int pos =-1);
    void PutDbmsMessage (const string& str, int line =-1, int pos =-1);
    void AddStatementToHistory (time_t startTime, const std::string& duration, const string& connectDesc, const string& sqlSttm);
    void RefreshBindView (const vector<string>& data);
    void LoadStatistics (const vector<int>& data);
    void AdjustErrorPosToLastLine (int& line, int& col);

    void OnSqlConnect ();
    void OnSqlDisconnect ();
    string GetDisplayConnectionString ();

    void PutErrors (vector<ErrorLoader::comp_error>&);

    int  GetLineCount ();
    void GetLine (int line, std::wstring&);
    bool AskIfUserWantToStopOnError ();
    bool InputValue (const string& prompt, string& value);
    void DoSqlExplainPlan (const string&);
    void Lock (bool lock);
    void SetExecutionStarted (clock64_t at, int line);
    void ScrollTo (int line);

    void CheckUserCancel ();
};

