/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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

#include "LabelWithWaitBar.h"

// ConnectionBar

class CConnectionLabel : public CStatic
{
    CString  m_text;
    COLORREF m_textColor;

public:
    const CString GetText() const { return m_text; }

    void SetConnectionDescription (LPCSTR pText, COLORREF color);
    void SetColor (COLORREF color);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};

class ConnectionBar : public CToolBar
{
    /*
	DECLARE_DYNAMIC(ConnectionBar)
    */

    CSize m_btnDelta;
    LabelWithWaitBar m_wndLabel;

public:
	ConnectionBar();
	virtual ~ConnectionBar();

    const CString GetText() const { return m_wndLabel.GetText(); }

    BOOL Create (CWnd* pParent, UINT id);

    void SetConnectionDescription (LPCSTR pText, COLORREF color)
        { m_wndLabel.SetText(pText, color); }

    void SetColor (COLORREF color)
        { m_wndLabel.SetText(m_wndLabel.GetText(), color); }

    void StartAnimation (bool animation)
        { m_wndLabel.StartAnimation(animation); }

protected:
    /*
	DECLARE_MESSAGE_MAP()
    */
};


