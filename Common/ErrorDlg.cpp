/*
    Copyright (C) 2004 Aleksey Kochetov

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
#include <afxdlgs.h>
#include <string>
#include <sstream>
#include "COMMON\ErrorDlg.h"

// from Winuser.h
#define OIC_HAND 32513

    static HANDLE hStopIcon = NULL;

using namespace std;

// CErrorDlg dialog
CErrorDlg::CErrorDlg (LPCSTR text, LPCSTR reportPrefix)
: CDialog(CErrorDlg::IDD),
m_reportPrefix(reportPrefix)
{
    string buff;
    istringstream i(text);

    while (getline(i, buff))
    {
        m_text += buff.c_str();
        m_text += "\r\n";
    }

    if (!hStopIcon)
        hStopIcon = LoadImage(NULL, MAKEINTRESOURCE(OIC_HAND), IMAGE_ICON, 0, 0, LR_SHARED);
}

void CErrorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_ERR_TEXT, m_text);
}


BEGIN_MESSAGE_MAP(CErrorDlg, CDialog)
    ON_BN_CLICKED(IDC_ERR_SAVE, OnBnClicked_ErrSave)
    ON_BN_CLICKED(IDC_ERR_COPY, OnBnClicked_ErrCopy)
END_MESSAGE_MAP()

// CErrorDlg message handlers

BOOL CErrorDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    ::SendMessage(::GetDlgItem(m_hWnd, IDC_ERR_STOP), STM_SETIMAGE, IMAGE_ICON, (LPARAM)hStopIcon);

    return TRUE;
}

void CErrorDlg::OnBnClicked_ErrSave()
{
	CFileDialog dlgFile(FALSE, NULL, NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT/* | OFN_EXPLORER*/ | OFN_ENABLESIZING,
		NULL, NULL, 0);

	CString strFilter;
	// append the "*.TXT" TXT files filter
	strFilter += "TXT Files (*.*)";
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.TXT");
	strFilter += (TCHAR)'\0';   // last string
	// append the "*.*" all files filter
	strFilter += "All Files (*.*)";
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string
    strFilter += '\0'; // close

	dlgFile.m_ofn.lpstrFilter   = strFilter;
	dlgFile.m_ofn.lpstrTitle   = "Save bug report";

    CString fileName = m_reportPrefix;
    fileName += "_";
    char buff[80];
    time_t t = time((time_t*) 0);
    struct tm* tp = localtime(&t);
    strftime(buff, sizeof(buff), "%Y%m%d%H%M", tp);
    buff[sizeof(buff)-1] = 0;
    fileName += buff;
    fileName += ".txt";

    dlgFile.m_ofn.nMaxFile = _MAX_PATH;
	dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(dlgFile.m_ofn.nMaxFile);
    fileName.ReleaseBuffer();

    if (dlgFile.DoModal() == IDOK)
    {
        HANDLE hFile = ::CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD written;
            WriteFile(hFile, (LPCSTR)m_text, m_text.GetLength(), &written, 0);
            CloseHandle(hFile);
        }
    }
}

void CErrorDlg::OnBnClicked_ErrCopy()
{
    if (::OpenClipboard(NULL))
    {
        ::EmptyClipboard();

        if (!m_text.IsEmpty())
        {
            unsigned int size = m_text.GetLength() + 1;
            HGLOBAL hDataDest = ::GlobalAlloc(GMEM_MOVEABLE, size);
            if (hDataDest)
            {
                char* dest = (char*)::GlobalLock(hDataDest);
                if (dest)
                {
                    memcpy(dest, (LPCSTR)m_text, size);
                    dest[size-1] = 0;
                }
                ::GlobalUnlock(hDataDest);
                ::SetClipboardData(CF_TEXT, hDataDest);
            }
        }

        ::CloseClipboard();
    }
}

LRESULT CErrorDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CDialog::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_

    return 0;
}
