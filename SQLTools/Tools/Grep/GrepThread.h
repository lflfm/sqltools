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

#ifndef __GREPTHREAD_H__
#define __GREPTHREAD_H__
#pragma once

class CGrepDlg;
class CGrepView;

class CGrepThread : public CWinThread
{
    DECLARE_DYNCREATE(CGrepThread)
protected:
    CGrepThread();
    ~CGrepThread();

public:
    BOOL	m_MatchWholeWord;
    BOOL	m_MatchCase;
    BOOL	m_RegExp;
    BOOL	m_LookInSubfolders;
    BOOL	m_SearchInMemory;
    BOOL    m_CollapsedList;
    CString	m_MaskList;
    CString	m_FolderList;
    CString	m_SearchText;

    CGrepView* m_pViewResult;

public:
    void RunGrep (CGrepDlg*, CGrepView*);

    //{{AFX_VIRTUAL(CGrepThread)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CGrepThread)
    //}}AFX_MSG
    afx_msg void OnThreadMessage (WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

#endif//__GREPTHREAD_H__
