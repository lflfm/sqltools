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

// MessageOnlyWindow.cpp : implementation file
//

#include "stdafx.h"
#include "afxmt.h"
#include "MessageOnlyWindow.h"

namespace ThreadCommunication
{
// MessageOnlyWindow

    MessageOnlyWindow* MessageOnlyWindow::m_pWnd = 0;

//IMPLEMENT_DYNAMIC(MessageOnlyWindow, CWnd)

BOOL MessageOnlyWindow::Create ()
{
    CString className = AfxRegisterWndClass(NULL);
    return CreateEx(0, className, "MessageOnlyWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, 0);
}

BEGIN_MESSAGE_MAP(MessageOnlyWindow, CWnd)
    ON_MESSAGE(UM_NOTE, OnNote)
END_MESSAGE_MAP()

void MessageOnlyWindow::Send (Note& note, unsigned timeout /*= 10000*/, bool nothrow /*= false*/)
{
    void* ptr = &note;

    //::SendMessage(m_hWnd, UM_NOTE, 0xFFFFFFFF ^ reinterpret_cast<WPARAM>(ptr), reinterpret_cast<LPARAM>(ptr));

    if (!::SendMessageTimeout(m_hWnd, UM_NOTE, 0xFFFFFFFF ^ reinterpret_cast<WPARAM>(ptr), reinterpret_cast<LPARAM>(ptr), SMTO_NOTIMEOUTIFNOTHUNG, timeout, 0)
    && !nothrow)
        throw Common::AppException(string("MessageOnlyWindow::Send timeout exceeded,\n delivering ") + typeid(note).name());
}

// MessageOnlyWindow message handlers

LRESULT MessageOnlyWindow::OnNote (WPARAM wParam, LPARAM lParam)
{
    if (wParam == (0xFFFFFFFF ^ lParam))
    {
        if (Note* note = reinterpret_cast<Note*>(lParam))
        {
            try { EXCEPTION_FRAME;
            
                note->Deliver();

                return 1L;
            }
	        //_DEFAULT_HANDLER_
            _COMMON_DEFAULT_HANDLER_
        }

    }
    return 0L;
}

MessageOnlyWindow& MessageOnlyWindow::GetWindow ()
{
    CCriticalSection cs;
    CSingleLock lk(&cs, TRUE);

    if (!m_pWnd)
    {
        m_pWnd = new MessageOnlyWindow;
        m_pWnd->Create();
    }

    return *m_pWnd; 
}

};//namespace ThreadCommunication
