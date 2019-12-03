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

namespace ConnectionTasks
{
    OCI8::EConnectionMode ToConnectionMode (int mode);
    OCI8::EConnectionMode ToConnectionMode (string mode);

    // called from the backgroung thread
    void DoConnect (
        OciConnect& connect, 
        const string& uid, const string& password, const string& alias, 
        OCI8::EConnectionMode mode, bool readOnly, bool slow, 
        bool autocommit, int commitOnDisconnect, int attempts
    );

    void DoConnect (
        OciConnect& connect,
        const string& uid, const string& password, 
        const string& host, const string& port, const string& sid, bool serviceInsteadOfSid, 
        OCI8::EConnectionMode mode, bool readOnly, bool slow, 
        bool autocommit, int commitOnDisconnect, int attempts
    );
    // called from the foreground thread
    void SubmitConnectTask (const string& uid, const string& password, const string& alias, OCI8::EConnectionMode mode, bool readOnly, bool slow, int attempts = 1);
    void SubmitConnectTask (const string& uid, const string& password, const string& host, const string& port, const string& sid, bool serviceInsteadOfSid, 
        OCI8::EConnectionMode mode, bool readOnly, bool slow, int attempts =  1
    );

    void SubmitDisconnectTask (bool exit = false);

    void SubmitReconnectTask ();

    void SubmitToggleReadOnlyTask ();

    bool GetCurrentUserAndSchema (string& user, string& schema, unsigned wait);

    void AfterClosingConnection (const string& message = string()); // called from secondary thread and notify the app

};// namespace ConnectionTasks
