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


// LabelWithWaitBar

class LabelWithWaitBar : public CStatic
{
	//DECLARE_DYNAMIC(LabelWithWaitBar)

    CString  m_text;
    COLORREF m_textColor;
    int      m_nGradientWidth;
    int      m_nProgressPos;
    UINT     m_nTimerId;
    int      m_nTimerElapse;
    bool     m_animation;
    UINT     m_nTextPosition;

public:
	LabelWithWaitBar ();
	virtual ~LabelWithWaitBar ();

    const CString GetText() const                   { return m_text; }
    void SetText (LPCTSTR pText, COLORREF color);
    void AlignLeft ()                               { m_nTextPosition = DT_LEFT; }
    void AlignCenter ()                             { m_nTextPosition = DT_CENTER; }
    void StartAnimation (bool);
    
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	DECLARE_MESSAGE_MAP()
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};


