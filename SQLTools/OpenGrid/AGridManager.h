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
#ifndef __AGridManager_h__
#define __AGridManager_h__

/***************************************************************************/
/*      Purpose: Superbase grid manager class for grid component           */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~           */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

namespace OG2 /* OpenGrid v2 */
{

    enum eDirection { edHorz = 0, edVert = 1 };

    struct GridOptions
    {
        bool m_ReadOnly;
        bool m_Editing;          // - Places an edit control over the focused cell.
        bool m_AlwaysShowEditor; // - Always shows the editor in place instead of
                                 //   waiting for a keypress or F2 to display it.

        bool m_Tabs;             // + Enables the tabbing between columns.
        bool m_MoveOnEnter;      // + Use ENTER key for move forward

        bool m_ColSelect;        // + Selection and movement is done a column at a time.
        bool m_RowSelect;        // + Selection and movement is done a row at a time.
        bool m_CellFocusRect;    // + Draw focus rect around the current cell regardless m_ColSelect/m_RowSelect

        bool m_HorzScroller;
        bool m_VertScroller;
        bool m_HorzScrollerAlways; // + show scroll bar always (independed from scroll rage)
        bool m_VertScrollerAlways; // + implemented, but not supporded runtime switching

        bool m_RowSizing;        // + Allows rows to be individually resized.
        bool m_ColSizing;        // + Allows columns to be individually resized.

        bool m_FixedHorzLine;    // + Draw horizontal grid lines in the fixed cell area.
        bool m_FixedVertLine;    // + Draw veritical grid lines in the fixed cell area.
        bool m_HorzLine;         // + Draw horizontal lines between cells.
        bool m_VertLine;         // + Draw vertical lines between cells.

        bool m_ExpandLastRow;
        bool m_ExpandLastCol;
        /*
        bool RowMoving;        // - Allows rows to be moved with the mouse
        bool ColMoving;        // - Allows columns to be moved with the mouse.
        bool DrawFocusSelected;// - Draw the focused cell in the selected color.
        */
        bool RangeSelect;        // + Allow a range of cells to be selected.

        bool m_Tooltips;         // + Show cell tooltips

        bool m_RowSizingAsOne;   // + Allows all rows to be resized as one.
        bool m_ColSizingAsOne;   // + Allows all columns to be resized as one.
        
        bool m_Rotation;         // + Allows rotate grid
        bool m_ClearRecords;     // + Allows to clear records

        bool m_Wraparound;       // Allows the cursor to wrap to the first cell of the next row
        bool m_PaleColors;
    };


class AGridManager
{
public:
    GridOptions m_Options;

    AGridManager ()
    {
        memset(&m_Options, 0, sizeof m_Options);
        m_Options.m_FixedHorzLine = true;
        m_Options.m_FixedVertLine = true;
        m_Options.m_HorzLine      = true;
        m_Options.m_VertLine      = true;
        m_Options.m_Tabs          = true;
        m_Options.m_RowSizing     = true;
        m_Options.m_ColSizing     = true;
        m_Options.m_HorzScroller  = true;
        m_Options.m_VertScroller  = true;
        m_Options.m_HorzScrollerAlways = true;
        m_Options.m_VertScrollerAlways = true;
        m_Options.m_Wraparound     = true;
        m_Options.m_CellFocusRect  = false;
    }

    virtual ~AGridManager () {}

    virtual void EvOpen (CRect& rect, CSize& chSize) = 0;
    virtual void EvFontChanged (CSize& chSize)       = 0;
    virtual void EvClose ()                          = 0;
    virtual void Paint (CDC& dc, bool erase, const CRect& rect)          = 0;
    virtual void EvSize (const CSize& size, bool force = false)          = 0;
    virtual bool EvKeyDown (UINT key, UINT repeatCount, UINT flags)      = 0;
    virtual void EvLButtonDblClk (unsigned modKeys, const CPoint& point) = 0;
    virtual void EvLButtonDown   (unsigned modKeys, const CPoint& point) = 0;
    virtual void EvLButtonUp     (unsigned modKeys, const CPoint& point) = 0;
    virtual void EvMouseMove     (unsigned modKeys, const CPoint& point) = 0;
    virtual void EvScroll (eDirection, unsigned scrollCode, unsigned thumbPos) = 0;
    virtual bool IdleAction ()                                           = 0;
    virtual void EvSetFocus (bool)                                       = 0;

    virtual void Clear   () = 0;
    virtual void Refresh () = 0;
    virtual void Rotate  () = 0;

    virtual bool MoveToLeft (eDirection) = 0;
    virtual bool MoveToRight (eDirection) = 0;
    virtual bool MoveToHome (eDirection) = 0;
    virtual bool MoveToEnd (eDirection) = 0;
    virtual bool MoveToPageUp (eDirection) = 0;
    virtual bool MoveToPageDown (eDirection) = 0;

    virtual bool ScrollToLeft (eDirection) = 0;
    virtual bool ScrollToRight (eDirection) = 0;
    virtual bool ScrollToHome (eDirection) = 0;
    virtual bool ScrollToEnd (eDirection) = 0;

    virtual bool IsEditMode () const = 0;

    virtual void OnChangedRows (int start, int end) = 0;
    virtual void OnChangedCols (int start, int end) = 0;
    virtual void OnChangedRowsQuantity (int row, int quantity) = 0;
    virtual void OnChangedColsQuantity (int col, int quantity) = 0;

    virtual int AppendRow () = 0;
    virtual int InsertRow () = 0;
    virtual int DeleteRow () = 0;

/* not implemet
    virtual bool BeginEdit () = 0;
    virtual bool PostEdit () = 0;
    virtual bool CancelEdit () = 0;
not implemet */
 };

}//namespace OG2

#endif//__AGridManager_h__
