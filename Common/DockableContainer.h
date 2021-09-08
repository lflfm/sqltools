/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2019 Aleksey Kochetov

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


// DockableContainer

class DockableContainer : public CDockablePane
{
    DECLARE_DYNAMIC(DockableContainer)

public:
    DockableContainer();
    virtual ~DockableContainer();

    void SetClient (CWnd* pClient)              { m_pClient = pClient; }
    void SetCanBeTabbedDocument (BOOL can)      { m_bCanBeTabbedDocument = can; }
    BOOL GetCanBeTabbedDocument () const        { return m_bCanBeTabbedDocument; }
    virtual BOOL CanBeTabbedDocument() const    { return m_bCanBeTabbedDocument; }
    BOOL IsMDITabbed () const                   { return m_bIsMDITabbed; }

protected:
    CWnd* m_pClient;
    BOOL m_bCanBeTabbedDocument;

    DECLARE_MESSAGE_MAP()
public:
    virtual afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
};


