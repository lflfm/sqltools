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

#include "stdafx.h"
#include "SQLTools.h"
#include "ConnectionTasks.h"
#include "OCI8\Connect.h"
#include <OCI8\ACursor.h>
#include "ThreadCommunication\MessageOnlyWindow.h"
#include "ServerBackgroundThread\TaskQueue.h"
#include <COMMON\InputDlg.h>
#include <Common\AppGlobal.h>

//TODO#2: decide if AfxMessageBox if safe to use from background thread

namespace ConnectionTasks
{
    enum ETransaction { etUnknown, etNo, etYes, etConnectionLost };

    static ETransaction check_open_trasaction (OciConnect& connect);
    static bool before_closing_connect (OciConnect& connect, bool autoCommit, int commitOnDisconnect);
    static void after_opening_connect (OciConnect&); // open connection in secondary thread and notify the app

    /*
    All tasks will purge the secondary queue and initiate connect/disconnect

    the procedure to connect from script runner 

    the procedure to disconnect from script runner 

    the task to connect for ui and command line:
      BG: check for open transaction
        FG: user confirmation
      BG: disconnect
      BG: connect
        FG: if wrong assword go back and show dialog 

    the task to disconnect
      BG: check for open transaction
        FG: user confirmation
      BG: disconnect

    the task to switch read-only attibute
      BG: check for open transaction
        FG: user confirmation
      BG: commit/rollback and switch

    do_conect calls diconnect first
    */

static
ETransaction check_open_trasaction (OciConnect& connect)
{
    ETransaction transaction = etUnknown;

    if (!connect.IsDatabaseOpen())
    {
        transaction = etNo;
    }
    else if (connect.GetVersion() >= OCI8::esvServer80X)
    {
        try
        {
            OciStatement cursor(connect);
            OciStringVar transaction_id(100);
            cursor.Prepare("BEGIN :transaction_id := dbms_transaction.local_transaction_id; END;");
            cursor.Bind(":transaction_id", transaction_id);
	        cursor.Execute(1, true);
            transaction = transaction_id.IsNull() ?  etNo : etYes;
        }
        catch (const OciException& x)
        {
            if (!connect.IsOpen())
            {
                MessageBeep(MB_ICONSTOP);
                AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                transaction = etConnectionLost;
            }
            else // most likely connection was lost but it was not detected by some reason
            {
                try
                {
                    OciAutoCursor cursor(connect);
                    cursor.Prepare("SELECT * FROM sys.dual");
	                cursor.Execute();
	                cursor.Fetch();
                }
                catch (const OciException& x)
                {
                    if (!connect.IsOpen())
                    {
                        MessageBeep(MB_ICONSTOP);
                        AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                        transaction = etConnectionLost;
                    }
                }
            }
        }
    }

    return transaction;
}


static 
bool before_closing_connect (OciConnect& connect, bool autoCommit, int commitOnDisconnect)
{
    try { EXCEPTION_FRAME;

        if (connect.IsOpen() && !autoCommit)
        {
            if (connect.IsReadOnly())
            {
                connect.Rollback();
                return true;
            }

            switch (commitOnDisconnect)
            {
            case 0: connect.Rollback(); return true; // always rollback
            case 1: connect.Commit();   return true; // always commit
            }

            ETransaction transaction = check_open_trasaction(connect);

            if (transaction == etConnectionLost)
                return true;

            if (transaction != etNo)
            {
                int retVal = AfxMessageBox(
                        (transaction == etUnknown)
                            ? "Auto commit is disabled and the current session may have an open transaction."
                            "\nDo you want to perform a commit before closing this session?"
                            : "SQLTools detected that the current session has an open transaction."
                            "\nDo yo want to perform commit before closing this session?",
                        MB_YESNOCANCEL|MB_ICONEXCLAMATION);

                switch (retVal)
                {
                case IDYES:
                    connect.Commit();
                    break;
                case IDNO:
                    connect.Rollback();
                    break;
                default:
                    AfxThrowUserException();
                }
            }
        }
    }
    _DEFAULT_HANDLER_

    return true;
}

    struct OnDisconnectNote : public ThreadCommunication::Note
    {
        string m_message;

        OnDisconnectNote (const string& message)
            : m_message(message)
        {
        }

        virtual void Deliver () 
        {
            theApp.SetConnectOpen         (false);
            theApp.SetConnectSlow         (false);
            theApp.SetDatabaseOpen        (false);
            //do not change theApp.SetCanReconnect();
            theApp.SetConnectReadOnly     (false);

            theApp.SetConnectUID     ("");
            theApp.SetConnectPassword("");
            theApp.SetConnectAlias   ("");
            theApp.SetConnectMode    (OCI8::ecmDefault);

            theApp.SetConnectDisplayString(string());
            theApp.SetUser                (string());
            theApp.SetCurrentSchema       (string());
            theApp.SetServerVersionStr    (string());
            theApp.SetConnectGlobalName   (string());
            theApp.SetServerVersion       (OCI8::esvServerUnknown);
            theApp.SetBreakHandler(arg::counted_ptr<OCI8::BreakHandler>(0));
            theApp.SetActivePrimeExecution(false);

            theApp.AfterClosingConnect();

            Global::SetStatusText(m_message.c_str());
        }
    };

void AfterClosingConnection (const string& message)
{
    ThreadCommunication::MessageOnlyWindow::GetWindow().Send(OnDisconnectNote(message));
}

    struct OnConnectNote : public ThreadCommunication::Note
    {
        APP_DECLARE_PROPERTY(bool, ConnectOpen);
        APP_DECLARE_PROPERTY(bool, ConnectSlow);
        APP_DECLARE_PROPERTY(bool, DatabaseOpen);
        APP_DECLARE_PROPERTY(bool, CanReconnect);
        APP_DECLARE_PROPERTY(bool, ConnectReadOnly);
        APP_DECLARE_PROPERTY(string, ConnectUID);
        APP_DECLARE_PROPERTY(string, ConnectPassword);
        APP_DECLARE_PROPERTY(string, ConnectAlias);
        APP_DECLARE_PROPERTY(OCI8::EConnectionMode, ConnectMode);
        APP_DECLARE_PROPERTY(string, ConnectDisplayString);
        APP_DECLARE_PROPERTY(string, User);
        APP_DECLARE_PROPERTY(string, CurrentSchema);
        APP_DECLARE_PROPERTY(string, ConnectGlobalName);
        APP_DECLARE_PROPERTY(string, ServerVersionStr);
        APP_DECLARE_PROPERTY(OCI8::EServerVersion, ServerVersion);
        arg::counted_ptr<OCI8::BreakHandler> m_connectBreakHandler;

        OnConnectNote (OciConnect& connect)
        {
            SetConnectOpen     (connect.IsOpen());
            SetConnectSlow     (connect.IsSlow());
            SetDatabaseOpen    (connect.IsDatabaseOpen());
            SetCanReconnect    (string(connect.GetUID()) != "" && string(connect.GetAlias()) != "");
            SetConnectReadOnly (connect.IsReadOnly());

            SetConnectUID     (connect.GetUID     ());
            SetConnectPassword(connect.GetPassword());
            SetConnectAlias   (connect.GetAlias   ());
            SetConnectMode    (connect.GetMode    ());

            SetConnectDisplayString(connect.GetDisplayString(true));
            connect.GetCurrentUserAndSchema(m_User, m_CurrentSchema);
            SetConnectGlobalName(connect.IsDatabaseOpen() ? connect.GetGlobalName() : "UNKNOWN");

            SetServerVersionStr(connect.GetVersionStr());
            SetServerVersion   (connect.GetVersion());

            m_connectBreakHandler = connect.CreateBreakHandler();
        }

        virtual void Deliver () 
        {
            theApp.SetConnectOpen         (GetConnectOpen         ());
            theApp.SetConnectSlow         (GetConnectSlow         ());
            theApp.SetDatabaseOpen        (GetDatabaseOpen        ());
            theApp.SetCanReconnect        (GetCanReconnect        ());
            theApp.SetConnectReadOnly     (GetConnectReadOnly     ());

            theApp.SetConnectUID          (GetConnectUID     ());
            theApp.SetConnectPassword     (GetConnectPassword());
            theApp.SetConnectAlias        (GetConnectAlias   ());
            theApp.SetConnectMode         (GetConnectMode    ());

            theApp.SetConnectDisplayString(GetConnectDisplayString());
            theApp.SetUser                (GetUser                ());
            theApp.SetCurrentSchema       (GetCurrentSchema       ());
            theApp.SetServerVersionStr    (GetServerVersionStr    ());
            theApp.SetConnectGlobalName   (GetConnectGlobalName   ());
            theApp.SetServerVersion       (GetServerVersion       ());
            
            theApp.SetBreakHandler        (m_connectBreakHandler);

            theApp.AfterOpeningConnect();
        }
    };

static
void after_opening_connect (OciConnect&  connect)
{
    ThreadCommunication::MessageOnlyWindow::GetWindow().Send(OnConnectNote(connect));
}

    struct ConnectTask : ServerBackgroundThread::Task
    {
        string m_uid, m_password, m_alias;
        bool m_direct,  m_serviceInsteadOfSid; 
        string m_host, m_port, m_sid; 
        OCI8::EConnectionMode m_mode;
        bool m_readOnly, m_slow;
        int m_keepAlivePeriod;
        bool m_autocommit;
        int m_commitOnDisconnect;
        int m_attempts;
        string m_error;

        ConnectTask (const string& uid, const string& password, const string& alias, OCI8::EConnectionMode mode, bool readOnly, bool slow, int attempts)
            : ServerBackgroundThread::Task("Connect", 0),
            m_uid(uid), m_password(password), m_alias(alias),
            m_direct(false),  m_serviceInsteadOfSid(false), 
            m_mode(mode), m_readOnly(readOnly), m_slow(slow),
            m_attempts(attempts)
        {
            m_keepAlivePeriod    = GetSQLToolsSettings().GetKeepAlivePeriod();
            m_autocommit         = GetSQLToolsSettings().GetAutocommit();
            m_commitOnDisconnect = GetSQLToolsSettings().GetCommitOnDisconnect();
        }

        ConnectTask (const string& uid, const string& password, const string& host, const string& port, const string& sid, bool serviceInsteadOfSid, 
            OCI8::EConnectionMode mode, bool readOnly, bool slow, int attempts
        )
            : ServerBackgroundThread::Task("Connect", 0),
            m_uid(uid), m_password(password),
            m_host(host), m_port(port), m_sid(sid), 
            m_direct(true),  m_serviceInsteadOfSid(serviceInsteadOfSid), 
            m_mode(mode), m_readOnly(readOnly), m_slow(slow),
            m_attempts(attempts)
        {
            m_keepAlivePeriod    = GetSQLToolsSettings().GetKeepAlivePeriod();
            m_autocommit         = GetSQLToolsSettings().GetAutocommit();
            m_commitOnDisconnect = GetSQLToolsSettings().GetCommitOnDisconnect();
        }

        virtual void DoInBackground (OciConnect& connect);
        virtual void ReturnInForeground ();
    };

    struct PasswordDlgNote : public ThreadCommunication::Note
    {
        string m_prompt, m_password;
        bool m_retVal;

        PasswordDlgNote () : m_retVal(false)
        {
        }

        virtual void Deliver () 
        {
            Common::CPasswordDlg dlg;
            dlg.m_prompt = m_prompt;
            m_retVal = (dlg.DoModal() == IDOK) ? true : false;
            m_password = dlg.m_value;
        }
    };

void DoConnect (
    OciConnect& connect,
    const string& uid, 
    const string& _password, 
    const string& alias, 
    const string& host, const string& port, const string& sid, bool serviceInsteadOfSid,
    bool direct,
    OCI8::EConnectionMode mode, 
    bool readOnly, 
    bool slow, 
    bool autocommit, int commitOnDisconnect,
    int attempts /*= 1*/
)
{
    if (!before_closing_connect(connect, autocommit, commitOnDisconnect))
        throw Common::AppException("User canceled conection!");

    connect.Close();
    AfterClosingConnection();

    for (int i = 0; i < attempts; i++)
    {
        string password = _password;

	    try
	    {
            if (!uid.empty() && password.empty())
            {
                PasswordDlgNote note;
                note.m_prompt = "Enter password for \"" + uid; 
                if (!direct) 
                    note.m_prompt += '@' + alias;
                else
                    note.m_prompt += host + ':' + port + ':' + sid;
                note.m_prompt += "\":";

                ThreadCommunication::MessageOnlyWindow::GetWindow().Send(note);

                if (!note.m_retVal)
                    throw Common::AppException("User canceled conection!");

                password = note.m_password;
            }

            if (!direct)
                connect.Open(uid.c_str(), password.c_str(), alias.c_str(), mode, readOnly, slow);
            else
                connect.Open(uid.c_str(), password.c_str(), host.c_str(), port.c_str(), sid.c_str(), serviceInsteadOfSid, mode, readOnly, slow);

            break;
		}
		catch (const OciException& x)
		{
            connect.Close(true);

            MessageBeep(MB_ICONSTOP);
			AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);

            if (x == 1017) // invalid password
                password.clear(); // let's try again
            else
                break;
		}
    }

    if (connect.IsOpen())
        after_opening_connect(connect);
}

void DoConnect (
    OciConnect& connect, 
    const string& uid, const string& password, const string& alias, 
    OCI8::EConnectionMode mode, bool readOnly, bool slow, 
    bool autocommit, int commitOnDisconnect, int attempts
)
{
    DoConnect (
        connect,
        uid, 
        password, 
        alias, 
        string(), string(), string(), false,
        false,
        mode, 
        readOnly, 
        slow, 
        autocommit, commitOnDisconnect,
        attempts
    );
}

void DoConnect (
    OciConnect& connect, 
    const string& uid, const string& password, const string& host, const string& port, const string& sid, bool serviceInsteadOfSid, 
    OCI8::EConnectionMode mode, bool readOnly, bool slow, 
    bool autocommit, int commitOnDisconnect, int attempts
)
{
    DoConnect (
        connect,
        uid, 
        password, 
        string(), 
        host, port, sid, serviceInsteadOfSid,
        true,
        mode, 
        readOnly, 
        slow, 
        autocommit, commitOnDisconnect,
        attempts
    );
}

void ConnectTask::DoInBackground (OciConnect& connect)
{
    try
    {
        DoConnect (
            connect,
            m_uid, 
            m_password, 
            m_alias, 
            m_host, m_port, m_sid, m_serviceInsteadOfSid,
            m_direct,
            m_mode, 
            m_readOnly, 
            m_slow, 
            m_autocommit, m_commitOnDisconnect,
            m_attempts
        );
    }
    // downgrade this exception and hadle them in normal flow
    catch (OCI8::Exception& x)
    {
        m_error = x.what();
    }
    catch (Common::AppException& x)
    {
        m_error = x.what();
    }
}

void ConnectTask::ReturnInForeground ()
{
    if (!m_error.empty())
    {
        MessageBeep((UINT)-1);
        AfxMessageBox(m_error.c_str(), MB_OK|MB_ICONSTOP);
        theApp.OnSqlConnect();
    }
    // after_opening_connect will update the app status faster than thru request queue processing
}


OCI8::EConnectionMode ToConnectionMode (int mode)
{
    switch (mode)
    {
    case 1: return OCI8::ecmSysDba;
    case 2: return OCI8::ecmSysOper;
    }
    return OCI8::ecmDefault;
}

OCI8::EConnectionMode ToConnectionMode (string mode)
{
    if (!stricmp(mode.c_str(), "SYSDBA"))       return OCI8::ecmSysDba;
    else if (!stricmp(mode.c_str(), "SYSOPER")) return OCI8::ecmSysOper;
    return OCI8::ecmDefault;
}

void SubmitConnectTask (const string& uid, const string& password, const string& alias, OCI8::EConnectionMode mode, bool readOnly, bool slow, int attempts)
{
    ServerBackgroundThread::FrgdRequestQueue::Get()
        .Push(ServerBackgroundThread::TaskPtr(new ConnectTask(uid, password, alias, mode, readOnly, slow, attempts)));
}

void SubmitConnectTask (const string& uid, const string& password, const string& host, const string& port, const string& sid, bool serviceInsteadOfSid, OCI8::EConnectionMode mode, bool readOnly, bool slow, int attempts)
{
    ServerBackgroundThread::FrgdRequestQueue::Get()
        .Push(ServerBackgroundThread::TaskPtr(new ConnectTask(uid, password, host, port, sid, serviceInsteadOfSid, mode, readOnly, slow, attempts)));
}

    struct DisconnectTask : ServerBackgroundThread::Task
    {
        bool m_exit;
        bool m_autocommit;
        int  m_commitOnDisconnect;

        DisconnectTask (bool exit) 
            : ServerBackgroundThread::Task("Disconnect", 0),
            m_exit(exit)
        {
            m_autocommit         = GetSQLToolsSettings().GetAutocommit();
            m_commitOnDisconnect = GetSQLToolsSettings().GetCommitOnDisconnect();
        }

        virtual void DoInBackground (OciConnect& connect) 
        { 
            if (connect.IsOpen())
            {
                if (!before_closing_connect(connect, m_autocommit, m_commitOnDisconnect))
                    return;

                try
                {
                    connect.Close();
                }
                catch (const OciException& x)
                {
                    connect.Close(true);

                    MessageBeep(MB_ICONSTOP);
                    AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                }
            }
            // call it regardless because connection can be lost earlier
            AfterClosingConnection();
        }

        virtual void ReturnInForeground ()
        {
            if (m_exit)
                AfxGetMainWnd()->PostMessage(WM_CLOSE);
        }
    };

void SubmitDisconnectTask (bool exit /*= false*/)
{
    ServerBackgroundThread::FrgdRequestQueue::Get()
        .Push(ServerBackgroundThread::TaskPtr(new DisconnectTask(exit)));
}

    struct ReconnectTask : ServerBackgroundThread::Task
    {
        ReconnectTask () 
            : ServerBackgroundThread::Task("Reconnect", 0) 
        {
        }

        virtual void DoInBackground (OciConnect& connect) 
        { 
            if (connect.IsOpen())
            {
                try
                {
                    OciStatement sttm(connect, "begin null; end;");
                    sttm.Execute(1, true);
                }
                catch (const OciException&)
                {
                }
            }
            
            if (!connect.IsOpen())
            {
                try
		        {
                    connect.Reconnect();
                    after_opening_connect(connect);
                }
                catch (const OciException& x)
                {
                    MessageBeep(MB_ICONSTOP);
                    AfxMessageBox(x.what(), MB_OK|MB_ICONSTOP);
                }
            }
        }
    };

void SubmitReconnectTask ()
{
    ServerBackgroundThread::FrgdRequestQueue::Get()
        .Push(ServerBackgroundThread::TaskPtr(new ReconnectTask));
    //TODO#2: submit same for background connection
}

    struct ToggleReadOnlyTask : ServerBackgroundThread::Task
    {
        bool m_autocommit;
        int  m_commitOnDisconnect;
        bool m_readOnly;

        ToggleReadOnlyTask () 
            : ServerBackgroundThread::Task("Toggle ReadOnly", 0),
            m_readOnly(false)
        {
            m_autocommit         = GetSQLToolsSettings().GetAutocommit();
            m_commitOnDisconnect = GetSQLToolsSettings().GetCommitOnDisconnect();
        }

        virtual void DoInBackground (OciConnect& connect) 
        { 
            if (!connect.IsReadOnly() && !m_autocommit)
            {
                bool check_needed = true;

                switch (m_commitOnDisconnect)
                {
                case 0: // always rollback
                    connect.Rollback(); 
                    check_needed = false;
                    break;
                case 1: // always commit
                    connect.Commit();   
                    check_needed = false; 
                    break;
                }

                if (check_needed)
                {
                    ETransaction transaction = check_open_trasaction(connect);

                    if (transaction != etNo && transaction != etConnectionLost)
                    {
                        int retVal = AfxMessageBox(
                                (transaction == etUnknown)
                                    ? "Auto commit is disabled and the current session may have an open transaction."
                                    "\nDo you want to perform a commit before switching to READ-ONLY mode?"
                                    : "SQLTools detected that the current session has an open transaction."
                                    "\nDo yo want to perform commit before switching to READ-ONLY mode?",
                                MB_YESNOCANCEL|MB_ICONEXCLAMATION);

                        switch (retVal)
                        {
                        case IDYES:
                            connect.Commit();
                            break;
                        case IDNO:
                            connect.Rollback();
                            break;
                        default:
                            AfxThrowUserException();
                        }
                    }
                }
            }

            connect.SetReadOnly(!connect.IsReadOnly());
            m_readOnly = connect.IsReadOnly();

            //ServerBackgroundThread::BkgdRequestQueue::Get()
            //    .Push(ServerBackgroundThread::TaskPtr(new SrvBkSetReadOnly(connect.IsReadOnly())));
        }

        virtual void ReturnInForeground ()
        {
            theApp.SetConnectReadOnly(m_readOnly);
            theApp.AfterToggleReadOnly();
        }
    };

void SubmitToggleReadOnlyTask ()
{
    ServerBackgroundThread::FrgdRequestQueue::Get()
        .Push(ServerBackgroundThread::TaskPtr(new ToggleReadOnlyTask));
    //TODO#2: submit same for background connection
}

	struct GetCurrentUserAndSchemaTask : ServerBackgroundThread::Task
    {
        string m_user, m_schema;
        CEvent m_event;

        GetCurrentUserAndSchemaTask ()
            : Task("Get current user and schema", 0)
        {
            SetSilent(true);
        }

        void DoInBackground (OciConnect& connect)
        {
            connect.GetCurrentUserAndSchema(m_user, m_schema);
            m_event.SetEvent();
        }
    };



bool GetCurrentUserAndSchema (string& user, string& schema, unsigned wait)
{
    ServerBackgroundThread::TaskPtr task(new GetCurrentUserAndSchemaTask());
    GetCurrentUserAndSchemaTask* _task = static_cast<GetCurrentUserAndSchemaTask*>(task.get());
    _task->m_event.ResetEvent();

    ServerBackgroundThread::BkgdRequestQueue::Get().Push(task);

    if (WaitForSingleObject(_task->m_event, wait) == WAIT_OBJECT_0)
    {
        user   = _task->m_user;
        schema = _task->m_schema;
        return true;
    }

    return false;
}

};// namespace ConnectionTasks
