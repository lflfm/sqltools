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
#ifndef __CellEditor_h__
#define __CellEditor_h__

/***************************************************************************/
/*      Purpose: Cell text editor implementation for grid component        */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~           */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

namespace OG2
{

    class CellEditor : public CEdit
    {
    public:
        // For Implementation Drag & Drop
        bool CursorOnSelText (CPoint);               
        afx_msg void OnLButtonDown (UINT nFlags, CPoint);
        afx_msg void OnMouseMove   (UINT nFlags, CPoint);

	    //{{AFX_VIRTUAL(CellEditor)
	    //}}AFX_VIRTUAL

    protected:
	    //{{AFX_MSG(CellEditor)
		afx_msg void OnUndo () { Undo(); }
	    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	    afx_msg UINT OnGetDlgCode();
	    //}}AFX_MSG

	    DECLARE_MESSAGE_MAP()
    };

}//namespace OG2

#endif//__CellEditor_h__
