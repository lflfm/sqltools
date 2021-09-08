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

#pragma once

#include "SQLTools.h"
#include "ThreadCommunication/MessageOnlyWindow.h"

    class ActivePrimeExecutionNote : public ThreadCommunication::Note
    {
        bool m_on;

    public:
        ActivePrimeExecutionNote (bool on) : m_on(on) {}

        static void SendActivePrimeExecution (bool on)
        {
            if (ThreadCommunication::MessageOnlyWindow* pWnd = &ThreadCommunication::MessageOnlyWindow::GetWindow())
                pWnd->Send(ActivePrimeExecutionNote(on));
        }

    protected:
        virtual void Deliver () 
        {
            theApp.SetActivePrimeExecution(m_on);
        }
    };

    struct ActivePrimeExecutionOnOff
    {
        ActivePrimeExecutionOnOff ()
        {
            try { EXCEPTION_FRAME;
                ActivePrimeExecutionNote::SendActivePrimeExecution(true);
            }
            _DEFAULT_HANDLER_;
        }
        ~ActivePrimeExecutionOnOff ()
        {
            try { EXCEPTION_FRAME;
                ActivePrimeExecutionNote::SendActivePrimeExecution(false);
            }
            _DESTRUCTOR_HANDLER_;
        }
    };
