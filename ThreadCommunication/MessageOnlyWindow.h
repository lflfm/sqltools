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

namespace ThreadCommunication 
{ 

    // MessageOnlyWindow proviedes simple way communicate worker threads with the main thread.
    struct Note : boost::noncopyable
    {
        virtual ~Note () {}
        virtual void Deliver () = 0;
    };

class MessageOnlyWindow : private CWnd
{
public:
    static MessageOnlyWindow& GetWindow ();

    // only blocking call currently supprted
    void Send (Note&, unsigned timeout = (5 * 60 * 1000), bool nothrow = false);

private:
    //DECLARE_DYNAMIC(MessageOnlyWindow)

    static const int UM_NOTE = WM_USER + 111;
    static MessageOnlyWindow* m_pWnd;

    MessageOnlyWindow () {}
    virtual ~MessageOnlyWindow () {}

    BOOL Create ();

protected:
    afx_msg LRESULT OnNote (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

};//namespace ThreadCommunication