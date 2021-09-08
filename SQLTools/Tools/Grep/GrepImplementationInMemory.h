/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 2020 Aleksey Kochetov

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
#include "../../SQLWorksheet/DocumentProxy.h"
#include "../../SQLWorksheet/SQLWorksheetDoc.h"
#include "ThreadCommunication/MessageOnlyWindow.h"
#include <filesystem>

namespace GrepImplementation
{
    using namespace std;

    struct GetDocProxyNote : public ThreadCommunication::Note
    {
        wstring m_filepath;
        DocumentProxy* m_proxy;

        GetDocProxyNote (const wstring& filepath) 
        : m_filepath(filepath), m_proxy(0) 
        {
        }

        ~GetDocProxyNote () 
        {
            if (m_proxy)
                delete m_proxy;
        }

        virtual void Deliver () 
        {
            CWaitCursor wait;

            try { EXCEPTION_FRAME;

                if (CMultiDocTemplate* pDocTemplate = theApp.GetPLSDocTemplate())
                {
                    POSITION pos = pDocTemplate->GetFirstDocPosition();
                    while (pos != NULL)
                    {
                        if (CPLSWorksheetDoc* pdoc = dynamic_cast<CPLSWorksheetDoc*>(pDocTemplate->GetNextDoc(pos)))
                        {
                            if (!_wcsicmp(m_filepath.c_str(), pdoc->GetPathName()))
                            {
                                m_proxy =  new DocumentProxy(*pdoc);
                                return;
                            }
                        }
                    }
                }

            } _DEFAULT_HANDLER_
        }
    };


    template <typename FuncType, typename ErrorHandler>
    bool for_each_file_in_memory (const wstring& file, FuncType func, ErrorHandler handler)
    {   
        try
        {
            GetDocProxyNote note(file);

            ThreadCommunication::MessageOnlyWindow::GetWindow().Send(note);

            if (note.m_proxy)
            {
                DocumentProxy& proxy = *note.m_proxy;
                proxy.Lock(true);

                int nlines = proxy.GetLineCount();
                for (int line = 0; line < nlines; line++)
                {
                    std::wstring text;
                    proxy.GetLine(line, text);
                    func(line, text);
                }

                proxy.Lock(false);

                return true;
            }
        }
        catch (const exception& e)
        {
            handler(file, Common::wstr(e.what()));
        }

        return false;
    }

} // namespace GrepImplementation