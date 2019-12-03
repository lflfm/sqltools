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
#ifndef __AGridSource_h__
#define __AGridSource_h__

/***************************************************************************/
/*      Purpose: Superbase data source class for grid component            */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~            */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

#include <ostream>
#include <COMMON/NVector.h>
#include "AGridManager.h"

namespace OG2 /* OpenGrid v2 */
{
    enum ESelect   { esNone, esBorder, esRect };
    enum EFixType  { efNone, efFirst, efLast };
    enum ECopyWhat { 
        ecFieldNames,
        ecDragTopLeftCorner,
        ecColumn, 
        ecColumnHeader,
        ecDragColumnHeader,
        ecRow, 
        ecRowHeader,
        ecDragRowHeader,
        ecDataCell, 
        ecEverything,
    };

    struct DrawCellContexts
    {
        int      m_Col, m_Row;
        CRect    m_Rect;
        CSize    m_CellMargin;
        CDC*     m_Dc;
        COLORREF m_TextColor, m_BkColor[2],
                 m_FixTextColor, m_FixBkColor, m_FixShdColor,
                 m_LightColor, m_UnusedColor,
                 m_HightLightColor, m_HightLightTextColor;
        CBrush   m_BkBrush[2], m_FixBrush, m_HightLightBrush;
        CPen     m_HightLightPen;
        ESelect  m_Select;
        EFixType m_Type[2];
        CRect    m_PaintRect;
        int      m_CharSize[2];

        bool IsDataCell  () const { return (m_Type[0] == efNone && m_Type[1] == efNone); }
        bool IsFixCell   () const { return (m_Type[0] != efNone || m_Type[1] != efNone); }
        bool IsCornerCell() const { return (m_Type[0] != efNone && m_Type[1] != efNone); }
    };

    //struct TooltipCellContexts
    //{
    //    int      m_Col, m_Row;
    //    CRect    m_Rect;
    //    CPoint   m_MousePos;
    //    EFixType m_Type[2];
    //    bool     m_CellRecall;

    //    bool IsDataCell  () const { return (m_Type[0] == efNone && m_Type[1] == efNone); }
    //    bool IsFixCell   () const { return (m_Type[0] != efNone || m_Type[1] != efNone); }
    //    bool IsCornerCell() const { return (m_Type[0] != efNone && m_Type[1] != efNone); }
    //};

class AGridSource
{
public:
    AGridSource ();
    virtual ~AGridSource ()                          {}

    virtual void EvAttach (AGridManager*);
    virtual void EvDetach (AGridManager*);

    bool IsEmpty () const                               { return m_empty;  }
    virtual void SetEmpty (bool empty)                  { m_empty = empty; }
    virtual bool CanModify () const = 0;
    
    // implementation of the following methods MUST call notification methods
    virtual void Clear ()           = 0;
    // the follow methods operate with rows only
    virtual int  Append ()          = 0;
    virtual int  Insert (int row)   = 0;
    virtual void Delete (int row)   = 0;
    virtual void DeleteAll ()       = 0;

    // Cell management
    virtual int GetCount         (eDirection) const = 0;
    virtual int GetFirstFixCount (eDirection) const = 0;
    virtual int GetLastFixCount  (eDirection) const = 0;

    virtual int GetFirstFixSize  (int pos, eDirection) const = 0;
    virtual int GetLastFixSize   (int pos, eDirection) const = 0;
    virtual int GetSize          (int pos, eDirection) const = 0;

    virtual void PaintCell (DrawCellContexts&) const = 0;
    virtual void CalculateCell (DrawCellContexts&, size_t maxTextLength) const = 0;

    virtual CWnd* BeginEdit  (int, int, CWnd*, const CRect&) = 0; // get editor for grid window
    virtual void  PostEdit   () = 0;  // event for commit edit result &
    virtual void  CancelEdit () = 0;  // end edit session

    // clipboard operation support
    virtual void DoEditCopy  (int, int, int, int, ECopyWhat, int format) = 0;
    virtual void DoEditPaste () = 0;
    virtual void DoEditCut   () = 0;

    // drag & drop support
    virtual BOOL       DoDragDrop (int, int, int, int, ECopyWhat) = 0;
    virtual DROPEFFECT DoDragOver (COleDataObject*, DWORD dwKeyState) = 0;
    virtual BOOL       DoDrop     (COleDataObject*, DROPEFFECT)       = 0;

    virtual int ExportText (std::ostream&, int row = -1, int nrows = -1, int col = -1, int ncols = -1, int format = -1) const = 0; // returns actual format

    // tooltips support
    //virtual const char* GetTooltipString (TooltipCellContexts&) const = 0;

protected:
    // notification attached grid managers
    void Notify_ChangedRows (int, int);
    void Notify_ChangedCols (int, int);
    void Notify_ChangedRowsQuantity (int, int);
    void Notify_ChangedColsQuantity (int, int);

    class NotificationDisabler
    {
    public:
        NotificationDisabler (AGridSource*);
        ~NotificationDisabler ()                { Enable(); }
        void Enable ();
    private:
        AGridSource* m_pOwner;
    };

protected:
    bool m_empty;
    bool m_disableNotifications;
    Common::nvector<AGridManager*> m_GridManagers;
};

}//namespace OG2

#endif//__AGridSource_h__

