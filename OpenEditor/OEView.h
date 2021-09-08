/* 
    Copyright (C) 2002 Aleksey Kochetov

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __OEView_h__
#define __OEView_h__

#include <set>
#include <afxole.h>             // for Drag & Drop
#include <arg_shared.h>
#include "OpenEditor/OEContext.h"
#include "OpenEditor/OEAutocompleteCtrl.h"
#include "OpenEditor/OEViewPaintAccessories.h"

    using arg::counted_ptr;
    class COEDocument;
    namespace OpenEditor { class HighlighterBase; };
    enum SearchDirection { esdDown, esdUp, esdDefault };


class COEditorView : public CView, protected OpenEditor::EditContext
{
    // Ruler is a struct, which helpes to draw character grid
    struct Ruler
    {
        int m_Topmost,       // topmost visible line or column
            m_Count,         // number of visible lines or columns
            m_CharSize,      // size of character
            m_ClientSize,    // size of client area
            m_Indent;

        COEditorView* m_sibling; // sibling for scrolling

        int  InxToPix (int inx) const;					// screen position to pixel coordinate (inline)
        int  PosToPix (int pos) const;                  // buffer position to pixel coordinate (inline)
        bool OnScroll (UINT code, UINT pos, int last, bool beyond);  // helper function for scrolling service
    }
    m_Rulers[2];
   
    friend COEAutocompleteCtrl;
    static COEAutocompleteCtrl m_autocompleteList; // an embedded control

    static BOOL m_isOverWriteMode;     // overwrite mode for text input
    static UINT m_uWheelScrollLines;   // cached value for MS Weel support
    BOOL m_bAttached;                  // it's FALSE till context initialization
    int  m_nMaxLineLength;		       // max line length (for horisontal scroller)
    BOOL m_bSyntaxHighlight;           
    BOOL m_bMouseSelection;            // it's TRUE after WM_LBUTTONDOWN
    BOOL m_bMouseSelectionStart;       // it's TRUE after WM_LBUTTONDOWN before first WM_MOUSEMOVE
    int  m_nMouseSelStartLine;         // it's not -1 if the left border area is used for a mouse selection
    BOOL m_bMouseSelectionByWord;      //
    OpenEditor::Square m_startWord;    // 
    int  m_nDigitsInLatsLineNumber;    // the number of digits in the last line number

    BOOL m_bDragScrolling; //  data is dragged over the window and scrolling has started after a small delay

    int   m_nClicks;
    CRect m_rcClick;
    DWORD m_tmLastClick;

    BOOL m_bPaleIfLocked; // colorize locked mode
    int  m_nExecutionLine;
    int  m_nDelayedScrollLine;

    struct CurCharCache
    {
        OpenEditor::Position pos;
        std::wstring message;

        CurCharCache () { pos.line = pos.column = -1; }
        void Reset ()   { pos.line = pos.column = -1; }
    }
    m_curCharCache;

    //TODO: put m_posCache & m_selCache in struct and add reset on selection mode switch and settings like in sqltools++
    OpenEditor::Position m_posCache;
    OpenEditor::Square m_selCache;
    OpenEditor::Position m_highlightingPosCache;
    OpenEditor::SequenceId m_highlightingActionSeqCache;
    std::wstring m_highlightedText;

    int m_nFistHighlightedLine, m_nLastHighlightedLine;
    bool m_autoscrollOnHighlighting;
    
    typedef OpenEditor::LanguageSupport::Match BraceHighlighting;
    BraceHighlighting m_braceHighlighting;
    
    void HighlightBraces ();
    void SetBraceHighlighting (const BraceHighlighting&);

    OpenEditor::LineStatusMap m_statusMap;

    static const UINT SEL_MOUSE_TIMER             = (UINT)-1;
    static const UINT SEL_MOUSE_TIMER_ELAPSE      = 60;
    static const UINT DRAG_MOUSE_TIMER            = (UINT)-2;
    static const UINT DRAG_MOUSE_TIMER_ELAPSE     = 120;
    static const UINT DELAYED_SCROLL_TIMER        = (UINT)-3;
    static const UINT DELAYED_SCROLL_TIMER_ELAPSE = 125;
public:
    static const UINT DOC_TIMER_1                 = (UINT)-101;
    static const UINT DOC_TIMER_2                 = (UINT)-102;
    static const UINT DOC_TIMER_3                 = (UINT)-103;
private:
    static const int EXT_SYN_NESTED_LIMIT = 4;
    static const int EXT_SYN_LINE_INDENT  = 3;
    static const int EXT_SYN_GUTTER_WIDTH = (EXT_SYN_NESTED_LIMIT + 2) * EXT_SYN_LINE_INDENT;
    static const int LINE_STATUS_WIDTH = 4;

    int CalculateDigitsInLatsLineNumber ();
    int CalculateLeftMargin (int digits = 0);
    
    COEditorView* GetSibling (ULONG row, ULONG col);
    Ruler* GetSiblingRuler (ULONG row, ULONG col, UINT ruler);
    void AttachSibling (COEditorView*, UINT ruler);
    void DetachSibling (UINT ruler);
    void AdjustVertSibling ();
    void AdjustHorzSibling ();
    void DoPageDown ();
    void DoPageUp ();

    COEViewPaintAccessoriesPtr m_paintAccessories; // container for shared GDI objects
    counted_ptr<OpenEditor::HighlighterBase> m_highlighter;
    bool m_syntaxGutter;

    // a settings synchronization handler
    void OnSettingsChanged ();

    // Bookmarks
    void DrawLineNumber (CDC&, RECT, int line);
    void DrawBookmark (CDC&, RECT);
    void DrawRandomBookmark (CDC&, RECT, int index, bool halfclip);

    void DrawHorzRuler (CDC&, const CRect&, int start, int end);
    void DrawVertRuler (CDC&, const CRect&, int start, int end, bool lineNumbers);
    void TextOutToScreen  (CDC& dc, Ruler[2], int pos, const wchar_t* str, int len, const CRect&, bool selection);
    void TextOutToPrinter (CDC& dc, const CPoint& pt, const CSize& chSz, const wchar_t* str, int len);
    //
    //  implementations of this methods are stored in OEView_3.cpp
    //
public:
    void SetHighlighting  (int fistHighlightedLine, int lastHighlightedLine, bool updateWindow = false);
    void HideHighlighting ();
    void SetAutoscrollOnHighlighting (bool);

    void EnableSyntaxHighlight (BOOL);
    void SetPaleIfLocked (BOOL);
    void SetExecutionLine (int);
    void DelayedScrollTo (int line);
    void ScrollTo (int line);
    int  GetTopmostLine () const                    { return m_Rulers[1].m_Topmost; }
    void SetTopmostLine (int line);
    int  GetTopmostColumn () const                  { return m_Rulers[0].m_Topmost; }
    void SetTopmostColumn (int column);

    using EditContext::GetLineW;
    //using EditContext::GetLineA;
    using EditContext::GetLineLength;
    using EditContext::GetLineCount;
    using EditContext::GetLineId;
    using EditContext::FindById;

    using EditContext::GetPosition;
    using EditContext::MoveTo;
    using EditContext::InxToPos;
    using EditContext::PosToInx;

    using EditContext::GetBlockMode;
    using EditContext::SetBlockMode;
    using EditContext::IsAltColumnarMode;
    using EditContext::GetSelection;
    using EditContext::SetSelection;
    using EditContext::IsSelectionEmpty;
    using EditContext::InsertBlock;
    using EditContext::SelectAll;
    using EditContext::ClearSelection;
    
    using EditContext::GetBlockOrWordUnderCursor;

    // not implemented yet
    void SetQueueBookmark (int, bool = true);

    // cursor navigation for external usage
    // I changed the method name from Go... to Move... 
    // avoiding any impact on the existing code
    void MoveToBottom ();
    void MoveToAndCenter (OpenEditor::Position);

    virtual void GetBlock    (std::wstring&, const OpenEditor::Square* = 0) const;
    virtual void InsertBlock (const wchar_t*, bool hideSelection, bool putSelInUndo = true);
    virtual void DeleteBlock (bool putSelInUndo = true);

    // Searching
    bool RepeadSearch (SearchDirection, COEditorView*&);
    using EditContext::GetSearchText;
    using EditContext::SetSearchText;
    using EditContext::GetSearchOption;
    using EditContext::SetSearchOption;
    using EditContext::IsBackwardSearch;
    using EditContext::SetBackwardSearch;

    bool Find (const COEditorView*&, OpenEditor::FindCtx&) const;

    using EditContext::Replace;
    using EditContext::SearchBatch;

    void ShowCaret (bool show, bool dont_destroy_caret=false);
    bool GetCaretPosition (CRect&) const;

    class UndoGroup : public OpenEditor::EditContext::UndoGroup
    {
    public:
        UndoGroup (COEditorView&);
    };
    //void PushInUndoStack (Position);
    //void PushInUndoStack (EBlockMode, Square);
    using EditContext::PushInUndoStack;

protected:
    void SetCaretPosition ();
    void AdjustCaretPosition ();

    void AdjustVertScroller (int maxPos = 0);
    void AdjustHorzScroller (int maxPos = 0);
    void DoHScroll (UINT nSBCode, BOOL keyboard);
    void DoVScroll (UINT nSBCode, BOOL keyboard);

    void DoSize (UINT nType, int cx, int cy);

    OpenEditor::Position MouseToPos (CPoint, bool force = false) const;
    void SelectByMouse (CPoint);

    // override base methods
    void InvalidateWindow ();
    void InvalidateLines  (int start, int end);
    void InvalidateSquare (const OpenEditor::Square&);

    void OnChangedLines (int startLine, int endLine);
    void OnChangedLinesQuantity (int line, int quantity);
    void OnChangedLinesStatus (int, int);

protected:
    static UINT m_AltColumnarTextFormat;
public:
    static UINT GetAltColumnarTextFormat();
    void DoEditCopy (const std::wstring&, bool append = false);
    void DoEditExpandTemplate (OpenEditor::TemplatePtr templ, bool canModifyTemplate);

protected:
    //
    //  Drag & Drop implementation (see OEView_3.cpp)
    //
    struct CDragAndDropData
    {
        bool   bVisible;
        CPoint ptCaret, ptCursor;
        CSize  sCursor;
        CView* pSrcWnd, *pDstWnd;
        CDragAndDropData () { memset(this, 0, sizeof (CDragAndDropData)); }
    };

    static int cnScrollThreshold;
    static CDragAndDropData m_DragAndDropData;
    COleDropTarget m_DropTaget;

    void DrawCursor (CPoint, bool erase = false);
    bool CursorOnSelection (CPoint) const;
    void StartDrag ();
    void PositionToPoint (OpenEditor::Position, CPoint&) const;

    virtual DROPEFFECT OnDragEnter (COleDataObject*, DWORD dwKeyState, CPoint);
    virtual DROPEFFECT OnDragOver  (COleDataObject*, DWORD dwKeyState, CPoint);
    virtual BOOL OnDrop            (COleDataObject*, DROPEFFECT, CPoint);
    virtual void OnDragLeave       ();

    // comment/uncomment helper functions
    void  DoCommentText (bool comment); // uncomment on false
    static bool CommentText (const OpenEditor::EditContext&, std::wstring&);
    static bool UncommentText (const OpenEditor::EditContext&, std::wstring&);

    // printing helper
    void GetMargins (CRect& rc) const;
    void MoveToPage (CPrintInfo* pInfo, int* pMaxPage = NULL);
    void PrintHeader (CDC* pDC, CPrintInfo* pInfo);
    void PrintFooter (CDC* pDC, CPrintInfo* pInfo);

    // keyword normalization helpers
    struct NormalizeOnCharCxt 
    {
        bool matched;
        std::wstring keyword; 
        OpenEditor::Square sqr; 
    };
    bool PreNormalizeOnChar (NormalizeOnCharCxt&, wchar_t ch);
    void NormalizeOnChar    (NormalizeOnCharCxt&);
    // it's not comment and string position
    bool IsNormalizeablePos (OpenEditor::Position pos) const;
    static bool NormalizeText (const OpenEditor::EditContext&, std::wstring&);

protected: // create from serialization only
    COEditorView();
    DECLARE_DYNCREATE(COEditorView)

// Attributes
public:
    COEDocument* GetDocument();

    afx_msg void OnUpdate_Mode      (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_Pos       (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_ScrollPos (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_BlockType (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_CurChar   (CCmdUI* pCmdUI);

public:
    //{{AFX_VIRTUAL(COEditorView)
    public:
    virtual void OnInitialUpdate();
    virtual void OnDraw(CDC*) {};  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
    protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
    //}}AFX_VIRTUAL

public:
    virtual ~COEditorView();

protected:
    //{{AFX_MSG(COEditorView)
    afx_msg void OnPaint(); // moved to OEView_2.cpp
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDownImpl(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClkImpl(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnEditToggleColumnarSelection();
    afx_msg void OnEditToggleStreamSelection();
    afx_msg void OnEditToggleSelectionMode();
    afx_msg void OnUpdate_SelectionType(CCmdUI* pCmdUI);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnEditCopy();
    afx_msg void OnEditCut();
    afx_msg void OnEditPaste();
    afx_msg void OnUpdate_EditCopy(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_EditPaste(CCmdUI* pCmdUI);
    afx_msg void OnTest();
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnUpdate_EditUndo(CCmdUI* pCmdUI);
    afx_msg void OnUpdate_EditRedo(CCmdUI* pCmdUI);
    afx_msg void OnEditSelectAll();
    afx_msg void OnEditDeleteLine();
    afx_msg void OnEditDeleteWordToLeft();
    afx_msg void OnEditDeleteWordToRight();
    afx_msg void OnEditFind();
    afx_msg void OnEditReplace();
    afx_msg void OnEditFindNext();
    afx_msg void OnEditFindPrevious();
    afx_msg void OnEditFindSeletedNext();
    afx_msg void OnEditFindSeletedPrevious();
    afx_msg void OnEditBookmarkToggle();
    afx_msg void OnEditBookmarkNext();
    afx_msg void OnEditBookmarkPrev();
    afx_msg void OnEditBookmarkRemoveAll();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnUpdate_EditFindNext(CCmdUI* pCmdUI);
    afx_msg void OnEditLower();
    afx_msg void OnUpdate_SelectionOperation(CCmdUI* pCmdUI);
    afx_msg void OnEditUpper();
    afx_msg void OnBlockTabify();
    afx_msg void OnBlockUntabify();
    afx_msg void OnBlockTabifyLeading();
    afx_msg void OnEditExpandTemplate();
    afx_msg void OnEditCreateTemplate();
    afx_msg void OnOpenFileUnderCursor();
    afx_msg void OnEditIndent();
    afx_msg void OnEditUndent();
    afx_msg void OnEditSort();
    afx_msg void OnEditFindMatch();
    afx_msg void OnEditFindMatchAndSelect();
    afx_msg void OnUpdate_CommentUncomment (CCmdUI* pCmdUI);
    afx_msg void OnEditComment();
    afx_msg void OnEditUncomment();
    afx_msg void OnEditGoto();
    //}}AFX_MSG
    afx_msg void OnSetRandomBookmark       (UINT nBookmark);
    afx_msg void OnGetRandomBookmark       (UINT nBookmark);
    afx_msg void OnUpdate_GetRandomBookmark (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_BookmarkGroup     (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_BookmarkGroupRO   (CCmdUI* pCmdUI);
    afx_msg void OnEditScrollUp ();
    afx_msg void OnEditScrollDown ();
    afx_msg void OnEditScrollCenter ();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnEditCutAndAppend();
    afx_msg void OnEditCutBookmarked();
    afx_msg void OnEditCopyAndAppend();
    afx_msg void OnEditCopyBookmarked();
    afx_msg void OnEditDelete();
    afx_msg void OnEditDeleteBookmarked();
    afx_msg void OnEditSelectWord();
    afx_msg void OnEditSelectLine();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnEditCapitalize();
    afx_msg void OnEditInvertCase();
protected:
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
public:
    afx_msg void OnEditNormalizeText();
    afx_msg void OnEditDatetimeStamp();

    afx_msg void OnEditJoinLines ();         
    afx_msg void OnEditSplitLine ();         
    afx_msg void OnEditRemoveBlankLines ();   
    afx_msg void OnRemoveExcessiveBlankLines ();

    afx_msg void OnTrimLeadingSpaces ();
    afx_msg void OnTrimExcessiveSpaces ();
    afx_msg void OnTrimTrailingSpaces ();

    afx_msg void OnEditColumnInsert ();      
    afx_msg void OnEditColumnInsertNumber (); 
    afx_msg void OnEditColumnLeftJustify (); 
    afx_msg void OnEditColumnCenterJustify ();
    afx_msg void OnEditColumnRightJustify ();
    afx_msg void OnEditDupLineOrSelection ();

    afx_msg void OnUpdate_ColumnOperation (CCmdUI* pCmdUI);
    afx_msg void OnUpdate_NotInReadOnly (CCmdUI* pCmdUI);

    afx_msg void OnIdleUpdateCmdUI ();
    afx_msg void OnFilePrintPreview ();
};

inline
COEDocument* COEditorView::GetDocument()
    { return (COEDocument*)m_pDocument; }

inline
void COEditorView::InvalidateWindow ()
    { Invalidate(FALSE); }

inline
int COEditorView::Ruler::InxToPix (int inx) const
    { return inx * m_CharSize + m_Indent; }

inline
int COEditorView::Ruler::PosToPix (int pos) const
    { return (pos - m_Topmost) * m_CharSize + m_Indent; }

inline
COEditorView::
UndoGroup::UndoGroup (COEditorView& view)
: OpenEditor::EditContext::UndoGroup(view)
{
}

inline
void COEditorView::SetAutoscrollOnHighlighting (bool enable) 
    { m_autoscrollOnHighlighting = enable; }

inline
UINT COEditorView::GetAltColumnarTextFormat()
    { return m_AltColumnarTextFormat; }

//{{AFX_INSERT_LOCATION}}

#endif//__OEView_h__
