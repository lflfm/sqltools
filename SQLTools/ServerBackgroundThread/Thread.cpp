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
#include "ServerBackgroundThread/Thread.h"
#include "ServerBackgroundThread/TaskQueue.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include <OCI8/Statement.h>
#include <ConnectionTasks.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace ThreadCommunication;

namespace ServerBackgroundThread
{
    struct ResultQueueNotEmptyNote : Note
    {
        virtual void Deliver () { theApp.OnServerBackground_ResultQueueNotEmpty(); }
    };

    __declspec(thread) int g_keepAlivePeriod = 0;

    inline
    void keep_connection_alive (OciConnect& connect)
    {
        if (g_keepAlivePeriod && connect.IsOpen())
        {
            if (clock64_t lastExeClock = connect.GetLastExecutionClockTime())
            {
                double interval = double(clock64() - lastExeClock) / CLOCKS_PER_SEC / 60;
                
                if (interval > g_keepAlivePeriod)
                {
                    OciStatement sttm(connect, "begin null; end;");
                    sttm.Execute(1, true);
                }
            }
        }
    }

UINT ThreadProc (LPVOID)
{
    try { EXCEPTION_FRAME;
            
        TaskQueue& requestQueue = BkgdRequestQueue::Get();
        TaskQueue& resultQueue  = BkgdResultQueue::Get();
        
        OciConnect connect;
        bool queueEmpty = true;

        // add exit
        while (1)
        {
            try { EXCEPTION_FRAME;
            
                if (requestQueue.GetSize() > 0
                || requestQueue.WaitForNewRequest(1000))
                {
                    if (requestQueue.IsShuttedDown())
                    {
                        // check how properly destoy singleton
                        // TODO#2: send a task to destroy the result queue
                        BkgdRequestQueue::Destroy();
                        break;
                    }

                    TaskPtr taskPtr = requestQueue.Pop();

                    if (taskPtr.get())
                    {
                        if (queueEmpty && !requestQueue.IsShuttedDown())
                        {
                            queueEmpty = false;
                            if (CWnd* pMainWnd = theApp.GetMainWnd())
                                if (*pMainWnd)
                                    pMainWnd->PostMessage(CSQLToolsApp::UM_REQUEST_QUEUE_NOT_EMPTY);
                        }

                        bool is_open = connect.IsOpen();

                        try
                        {
                            taskPtr->DoInBackground(connect);
                        }
                        catch (const std::exception& x) 
                        { 
                            taskPtr->SetError(x.what()); 
                        } 
                        catch (CUserException*)
                        {
                            taskPtr->SetError("User canceled operation!"); 
                        }
                        catch (...)
                        {
                            taskPtr->SetError("Unknown error."); 
                        }
                        
                        resultQueue.Push(taskPtr);

                        if (is_open && !connect.IsOpen())
                            ConnectionTasks::AfterClosingConnection("Connection lost!");

                        //MessageOnlyWindow::GetWindow().Send(ResultQueueNotEmptyNote());

                        if (!requestQueue.IsShuttedDown() && !requestQueue.GetSize()) 
                        {
                            queueEmpty = true;
                            if (theApp.GetMainWnd() && IsWindow(*theApp.GetMainWnd()))
                                theApp.GetMainWnd()->PostMessage(CSQLToolsApp::UM_REQUEST_QUEUE_EMPTY);
                        }
                    }

                    requestQueue.ResetLast();
                } // wait if

                if (!requestQueue.IsShuttedDown() 
                && resultQueue.GetSize() > 0) // messaging is not reliable so sending notificaton until the queue is empty 
                    MessageOnlyWindow::GetWindow().Send(ResultQueueNotEmptyNote(), 100/*timeout*/, true/*nothrow*/);

                try 
                {
                    keep_connection_alive(connect); 
                }
                catch (...)
                {
                    if (!connect.IsOpen())
                        ConnectionTasks::AfterClosingConnection("Keep Alive process: Connection lost!"); // connection is lost so what
                    else
                        throw;
                }
            }
            _COMMON_DEFAULT_HANDLER_
        } // while(1)
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}

};