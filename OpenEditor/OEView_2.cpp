/* 
    Copyright (C) 2002-2005 Aleksey Kochetov

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

/*
    28.03.2003 improvement, optimization of GDI resources usage - it's a very critical issue for Win95/Win98
    29.06.2003 bug fix, selected text foreground color cannot be changed for "Text" class
    11.10.2004 improvement, Ruler for columns has been added
    11.10.2004 improvement, Columnar markers has been added (can be defined in class property page)
    16.04.2005 improvement, optimization of DrawHorzRuler/DrawVertRuler
    2016.05.10 improvement, implemented Read-Only mode
    2016.06.12 improvement, Highlighting for changed lines
*/

#include "stdafx.h"
namespace MyMemDC 
{
#include "MemDC.h"
};
using namespace MyMemDC;

#include "OpenEditor/OEDocument.h"
#include "OpenEditor/OEView.h"
#include "OpenEditor/OEHighlighter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace OpenEditor;

/////////////////////////////////////////////////////////////////////////////
// COEditorView drawing

    // arguments should be normalized
    // it's necessary to decrement blk.end.column
    inline
    bool _is_block_visible (const Square& scr, const Square& blk, EBlockMode mode)
    {
        switch (mode)
        {
        case ebtColumn:
            return intersect_not_null(scr, blk);
        case ebtStream:
            if (blk.start.line == blk.end.line) return intersect_not_null(scr, blk);// 1-line block
        }
        return scr.line_in_rect(blk.start.line) || blk.line_in_rect(scr.start.line);
    }

void COEditorView::DrawLineNumber (CDC& dc, RECT rc, int line)
{
    rc.right -= (m_Rulers[0].m_CharSize*3)/4 + LINE_STATUS_WIDTH;

    char buff[40];
    itoa(line + 1, buff, 10);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(m_paintAccessories->m_COLOR_BTNTEXT);
    
    CFont* pOrgFont = m_paintAccessories->SelectFontEx(dc, 0);
    DrawTextA(dc.m_hDC, buff, strlen(buff), &rc, DT_RIGHT|DT_TOP);
    dc.SelectObject(pOrgFont);
}

void COEditorView::DrawBookmark (CDC& dc, RECT rc)
{
    rc.left  += 2; 
    rc.right -= 2 + LINE_STATUS_WIDTH;
    CBrush* pOrgBrush = dc.SelectObject(&m_paintAccessories->m_BmkBrush);
    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_BtnShadowPen/*m_BmkPen*/);
    dc.RoundRect(&rc, CPoint(3, 3));
    dc.SelectObject(pOrgBrush);
    dc.SelectObject(pOrgPen);
}

static // move it in Common namespace
void fill_gradient (HDC hDC, LPCRECT lpRect, COLORREF colorStart, COLORREF colorFinish, BOOL bHorz/* = TRUE*/)
{
    // this will make 2^6 = 64 fountain steps
    int nShift = 6;
    int nSteps = 1 << nShift;

    RECT r2;
    r2.top = lpRect->top;
    r2.left = lpRect->left;
    r2.right = lpRect->right;
    r2.bottom = lpRect->bottom;

    int nHeight = lpRect->bottom - lpRect->top;
    int nWidth = lpRect->right - lpRect->left;

    for (int i = 0; i < nSteps; i++)
    {
        // do a little alpha blending
        BYTE bR = (BYTE) ((GetRValue(colorStart) * (nSteps - i) +
                   GetRValue(colorFinish) * i) >> nShift);
        BYTE bG = (BYTE) ((GetGValue(colorStart) * (nSteps - i) +
                   GetGValue(colorFinish) * i) >> nShift);
        BYTE bB = (BYTE) ((GetBValue(colorStart) * (nSteps - i) +
                   GetBValue(colorFinish) * i) >> nShift);

        HBRUSH hBrush = ::CreateSolidBrush(RGB(bR, bG, bB));
        
        // then paint with the resulting color

        if (!bHorz)
        {
            r2.top = lpRect->top + ((i * nHeight) >> nShift);
            r2.bottom = lpRect->top + (((i + 1) * nHeight) >> nShift);
            if ((r2.bottom - r2.top) > 0)
                ::FillRect(hDC, &r2, hBrush);
        }
        else
        {
            r2.left = lpRect->left + ((i * nWidth) >> nShift);
            r2.right = lpRect->left + (((i + 1) * nWidth) >> nShift);
            if ((r2.right - r2.left) > 0)
                ::FillRect(hDC, &r2, hBrush);
        } //if
        
        if (NULL != hBrush)
        {
            ::DeleteObject(hBrush);
            hBrush = NULL;
        } //if
    } //for
} //End FillGradient

void COEditorView::DrawRandomBookmark (CDC& dc, RECT rc, int index, bool halfclip)
{
    rc.left  += 2; 
    rc.right -= 2 + LINE_STATUS_WIDTH;

    if (halfclip)
    {
        rc.left = rc.right / 2;
        dc.IntersectClipRect(&rc);
        rc.left = 0;
    }

    CBrush* pOrgBrush = dc.SelectObject(&m_paintAccessories->m_RndBmkBrush);
    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_BtnShadowPen/*m_BmkPen*/);
    dc.RoundRect(&rc, CPoint(3, 3));
    dc.SelectObject(pOrgBrush);
    dc.SelectObject(pOrgPen);

    if (halfclip)
    {
        if (halfclip)
            dc.SelectClipRgn(NULL);
        
        RECT _rc = rc;
        ::InflateRect(&_rc, -3, -1);
        fill_gradient(dc, &_rc, m_paintAccessories->m_BmkBackground, m_paintAccessories->m_RndBmkBackground, TRUE);
    }

    char buff[40];
    itoa(index, buff, 10);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(m_paintAccessories->m_RndBmkForeground);

    //CFont* pOrgFont = m_paintAccessories->SelectFont(dc, GetSettings().GetLineNumbers() ? HighlighterBase::efmBold : HighlighterBase::efmNormal);
    CFont* pOrgFont = m_paintAccessories->SelectFontEx(dc, HighlighterBase::efmNormal);

    DrawTextA(dc.m_hDC, buff, strlen(buff), &rc, DT_CENTER|DT_TOP);
    dc.SelectObject(pOrgFont);
}

void COEditorView::DrawHorzRuler (CDC& realDc, const CRect& rc, int start, int end)
{
    MyMemDC::CMemDC dc(&realDc, &rc);
    dc.FillRect(&rc, &m_paintAccessories->m_BtnFaceBrush);

    COLORREF bkColor = dc.SetBkColor(m_paintAccessories->m_COLOR_BTNFACE);
    COLORREF txtColor = dc.SetTextColor(m_paintAccessories->m_COLOR_BTNTEXT);
    CFont* pOrgFont = dc.SelectObject(&m_paintAccessories->m_RulerFont);
    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_BmkPen);

    dc.MoveTo(rc.left, rc.bottom-1);
    dc.LineTo(rc.right, rc.bottom-1);

    for (int i = start; i <= end; i++)
    {
        int column = m_Rulers[0].m_Topmost + i;

        int x = m_Rulers[0].m_Indent + m_Rulers[0].m_CharSize * i;

        int hight = (!(column % 5)) ? 4 : 2;

        dc.MoveTo(x, rc.bottom-1-hight);
        dc.LineTo(x, rc.bottom-1);

        if (!(column % 10))
        {
            char buffer[80];
            itoa(column/*+1*/, buffer, 10);
            int buflen = strlen(buffer);
            int width = buflen * m_paintAccessories->m_RulerCharSize.cx;
            DrawTextA(dc.m_hDC, buffer, buflen, 
                CRect(x - width, 0, x + width, m_paintAccessories->m_RulerCharSize.cy), 
                DT_CENTER|DT_TOP
            );
        }
    }

    dc.SetBkColor(bkColor);
    dc.SetTextColor(txtColor);
    dc.SelectObject(pOrgFont);
    dc.SelectObject(pOrgPen);
}

#pragma warning ( push ) 
#pragma warning ( disable : 4701 ) 
void COEditorView::DrawVertRuler (CDC& realDc, const CRect& rc, int start, int end, bool lineNumbers)
{
    MyMemDC::CMemDC dc(&realDc, &rc);
    COLORREF bkColor = dc.SetBkColor(m_paintAccessories->m_COLOR_BTNHIGHLIGHT);
    COLORREF txtColor = dc.SetTextColor(m_paintAccessories->m_COLOR_BTNFACE);

    RECT stdGutterRc, extGutterRc;
    stdGutterRc.left   = 0;
    stdGutterRc.right  = m_Rulers[0].m_Indent;
    stdGutterRc.top    = rc.top;
    stdGutterRc.bottom = rc.bottom;

    CPen* pOrgPen = dc.SelectObject(&m_paintAccessories->m_BtnShadowPen);

    LanguageSupportPtr ls = GetLanguageSupport();
    bool syntaxExtendedSupport = m_syntaxGutter && ls.get() && ls->HasExtendedSupport(); // it can be a memeber
    if (syntaxExtendedSupport) 
    {
        ls->GetLineStatus(m_statusMap, m_Rulers[1].m_Topmost, m_Rulers[1].m_Count + 1);

        extGutterRc = stdGutterRc;
        stdGutterRc.right -= EXT_SYN_GUTTER_WIDTH;
        extGutterRc.left   = stdGutterRc.right;
        dc.FillRect(&extGutterRc, dc.GetHalftoneBrush());
        dc.FillSolidRect(&stdGutterRc, m_paintAccessories->m_COLOR_BTNFACE);
        extGutterRc.right--;
        stdGutterRc.right--;

        dc.MoveTo(extGutterRc.right, extGutterRc.top);
        dc.LineTo(extGutterRc.right, extGutterRc.bottom);
    }
    else
    {
        dc.FillRect(&stdGutterRc, dc.GetHalftoneBrush());
        stdGutterRc.right--;
    }
    dc.MoveTo(stdGutterRc.right, stdGutterRc.top);
    dc.LineTo(stdGutterRc.right, stdGutterRc.bottom);
    dc.SelectObject(pOrgPen);

    dc.SetBkColor(bkColor);
    dc.SetTextColor(txtColor);

    int lim = min(GetLineCount() - m_Rulers[1].m_Topmost, end);

    for (int i = start; i < lim; i++)
    {
        int line = m_Rulers[1].m_Topmost + i;
        stdGutterRc.top    = m_Rulers[1].m_Indent + m_Rulers[1].m_CharSize * i;
        stdGutterRc.bottom = stdGutterRc.top + m_Rulers[1].m_CharSize;

        RandomBookmark bookmarkId;
        bool isBmkLine = IsBookmarked(line, eBmkGroup1);
        bool isRandomBmkLine = IsRandomBookmarked(line, bookmarkId);
        ELineStatus status = GetLineStatus(line);

        if (status != elsNothing)
        {
            CRect gtRc = stdGutterRc;
            gtRc.right -= 1;
            gtRc.left  = gtRc.right - LINE_STATUS_WIDTH;
            dc.FillSolidRect(gtRc, 
                    status == elsUpdated ? RGB(255, 255, 0) 
                        : status == elsRevertedBevoreSaved ? RGB(255, 215, 0) 
                            : RGB(124, 252, 0));
        }

        if (isRandomBmkLine)
        {
            if (isBmkLine) DrawBookmark(dc, stdGutterRc);
            DrawRandomBookmark(dc, stdGutterRc, bookmarkId.GetId(), isBmkLine);
        }
        else
        {
            if (isBmkLine) DrawBookmark(dc, stdGutterRc);
            if (lineNumbers) DrawLineNumber(dc, stdGutterRc, line);
        }
        //TODO#2: move in procedure
        if (m_nExecutionLine != -1 && m_nExecutionLine == line)
        {
            //TODO#2: maybe to add the color attributes?
            CBrush* pOrgBrush = dc.SelectObject(&m_paintAccessories->m_RndBmkBrush);
            CPen* pOrgPen2 = dc.SelectObject(&m_paintAccessories->m_RndBmkPen);

            POINT pt[3] = { 
                { stdGutterRc.left + 2, stdGutterRc.top + 2 },
                { stdGutterRc.left + 2, stdGutterRc.bottom - 2 },
                { min((stdGutterRc.bottom - stdGutterRc.top) / 2, stdGutterRc.right - stdGutterRc.left - 4), (stdGutterRc.bottom + stdGutterRc.top) / 2 }
            };
            dc.Polygon(pt, sizeof(pt)/sizeof(pt[0]));

            dc.SelectObject(pOrgBrush);
            dc.SelectObject(pOrgPen2);
        }

        if (syntaxExtendedSupport) 
        {
            CPen* pOrgPen2 = dc.SelectObject(&m_paintAccessories->m_BtnShadowPen);

            extGutterRc.top    = stdGutterRc.top;
            extGutterRc.bottom = stdGutterRc.bottom;

            if (m_statusMap[line].syntaxError)
                dc.FillSolidRect(&extGutterRc, m_paintAccessories->m_ErrorHighlightingBackground);

            if (int level = m_statusMap[line].level)
            {
                int lim2 = min(EXT_SYN_NESTED_LIMIT, level);
                for (int i2 = 1; i2 <= lim2; i2++) {
                    dc.MoveTo(extGutterRc.left + i2 * EXT_SYN_LINE_INDENT, extGutterRc.top);
                    dc.LineTo(extGutterRc.left + i2 * EXT_SYN_LINE_INDENT, extGutterRc.bottom);

                    if (level - m_statusMap[line].beginCounter < i2) {
                        dc.MoveTo(extGutterRc.left + i2 * EXT_SYN_LINE_INDENT, extGutterRc.top);
                        dc.LineTo(extGutterRc.left + (i2+1) * EXT_SYN_LINE_INDENT, extGutterRc.top);
                    }
                    if (level - m_statusMap[line].endCounter < i2) {
                        dc.MoveTo(extGutterRc.left + i2 * EXT_SYN_LINE_INDENT, extGutterRc.bottom-1);
                        dc.LineTo(extGutterRc.left + (i2+1) * EXT_SYN_LINE_INDENT, extGutterRc.bottom-1);
                    }
                }
            }
            dc.SelectObject(pOrgPen2);
        }
    }
}
#pragma warning ( pop ) 

    inline
    int get_text_width (CDC& dc, const wchar_t* str, int len)
    {
        CSize sz;
        GetTextExtentPoint32(dc.m_hDC, str, len, &sz);
        return sz.cx;
    }

inline
void COEditorView::TextOutToScreen (CDC& dc, Ruler rulers[2], int pos, const wchar_t* str, int len, const CRect& rcBar, bool selection)
{
    int expectedWidth =  rulers[0].m_CharSize * len;
    int actualWidth = get_text_width(dc, str, len);
    if (actualWidth - expectedWidth > rulers[0].m_CharSize/3 
    || Common::is_right_to_left_lang(str[0]))
    {
        CRect rc; 
        rc.top = rcBar.top;
        rc.bottom = rcBar.bottom -1;
        for (int charInx = 0; charInx < len; charInx++)
        {
            rc.left = rulers[0].PosToPix(pos + charInx);
            rc.right = rulers[0].PosToPix(pos + charInx + 1) - 1;
            int charWidth = get_text_width(dc, str + charInx, 1);

            if (charWidth - rulers[0].m_CharSize > rulers[0].m_CharSize/3 
            || Common::is_right_to_left_lang(str[charInx]))
            {
                CBrush* pOrgBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
                CPen* pOrgdPen = dc.SelectObject(selection
                    ? &m_paintAccessories->m_SelTextForegroundPen
                    : &m_paintAccessories->m_TextForegroundPen);
                //dc.Rectangle(CRect(rc.left + rc.Width()/5, rc.top + rc.Height()/4, rc.right - rc.Width()/5, rc.bottom - rc.Height()/4));
                dc.Rectangle(CRect(rc.left + 1, rc.top + 2, rc.right - 1, rc.bottom - 2));
                dc.SelectObject(pOrgdPen);
                dc.SelectObject(pOrgBrush);
            }
            else
                dc.TextOut(rulers[0].PosToPix(pos + charInx), rcBar.top, str + charInx, 1);
        }
    }
    else if (actualWidth != expectedWidth)
    {
        for (int charInx = 0; charInx < len; charInx++)
            TextOut(dc.m_hDC, rulers[0].PosToPix(pos + charInx), rcBar.top, str + charInx, 1);

        //std::vector<int> dsp(len * 2);
        //for (int charInx = 0; charInx < len; charInx++)
        //{
        //    dsp[2 * charInx] = rulers[0].m_CharSize;
        //    dsp[2 * charInx + 1] = 0;
        //}
        //dc.ExtTextOut(rulers[0].PosToPix(pos), rcBar.top, ETO_PDY, 0, str, len, dsp.data());
    }
    else
        TextOut(dc.m_hDC, rulers[0].PosToPix(pos), rcBar.top, str, len);
}

void COEditorView::OnPaint ()
{
    CPaintDC dc(this);

    if (dc.m_ps.rcPaint.left == dc.m_ps.rcPaint.right
    || dc.m_ps.rcPaint.top == dc.m_ps.rcPaint.bottom)
        return;

    if (!m_bAttached)
    {
        ::FillRect(dc, &dc.m_ps.rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH));
        return;
    }

    int maxLineLength = 0;
    const Settings& settings = GetSettings();
    bool locked = IsLocked() && m_bPaleIfLocked;

    Square scr;
    scr.start.line   = max(0, (int)(dc.m_ps.rcPaint.top - m_Rulers[1].m_Indent) / m_Rulers[1].m_CharSize);
    scr.end.line     = max(0, (int)(dc.m_ps.rcPaint.bottom - m_Rulers[1].m_Indent - 1) / m_Rulers[1].m_CharSize);
    scr.start.column = max(0, (int)(dc.m_ps.rcPaint.left - m_Rulers[0].m_Indent) / m_Rulers[0].m_CharSize);
    scr.end.column   = max(0, (int)(dc.m_ps.rcPaint.right - m_Rulers[0].m_Indent - 1) / m_Rulers[0].m_CharSize);

    if (dc.m_ps.rcPaint.left < m_Rulers[0].m_Indent)
    {
        CRect rc(dc.m_ps.rcPaint);
        rc.right = m_Rulers[0].m_Indent;
        rc.top   = m_Rulers[1].m_Indent;
        DrawVertRuler(dc, rc, scr.start.line, scr.end.line+1, settings.GetLineNumbers());
    }

    if (dc.m_ps.rcPaint.top < m_Rulers[1].m_Indent)
    {
        CRect rc(dc.m_ps.rcPaint);
        rc.bottom = m_Rulers[1].m_Indent;
        DrawHorzRuler(dc, rc, scr.start.column, scr.end.column+1);
    }

    if (dc.m_ps.rcPaint.right >= m_Rulers[0].m_Indent
    && dc.m_ps.rcPaint.bottom >= m_Rulers[1].m_Indent)
    {
        Square blk;
        GetSelection(blk);
        bool blockExist    = !blk.is_empty();
        EBlockMode blkMode = GetBlockMode();

        if (blockExist)
        {
            blk.normalize();

            blk.start.line   -= m_Rulers[1].m_Topmost;
            blk.end.line     -= m_Rulers[1].m_Topmost;
            blk.start.column -= m_Rulers[0].m_Topmost;
            blk.end.column   -= m_Rulers[0].m_Topmost;

            if (blkMode == ebtColumn)
                blk.normalize_columns();
        }

        blockExist = blockExist && _is_block_visible(scr, blk, blkMode);

        HighlighterBase* phighlighter = m_bSyntaxHighlight ? m_highlighter.get() : 0;

        LineTokenizer tokenizer(
            settings.GetVisibleSpaces(),
            GetTabSpacing(), 
            phighlighter ? phighlighter->GetDelimiters() : GetDelimiters()
            );

        if (phighlighter)
        {
            int status(0), quoteId(0);
            bool parsing(false);
            ScanMultilineQuotes(scr.start.line + m_Rulers[1].m_Topmost, status, quoteId, parsing);
            phighlighter->SetMultilineQuotesState(status, quoteId, parsing);
        }

        CRect rcLine, rcBar;
        rcLine.top    = rcBar.top    = 0;
        rcLine.bottom = rcBar.bottom = m_Rulers[1].m_CharSize;
        rcLine.left   = max(dc.m_ps.rcPaint.left,  (long)m_Rulers[0].m_Indent);
        rcLine.right  = max(dc.m_ps.rcPaint.right, (long)m_Rulers[0].m_Indent);

        CDC memDc, memSelDc;
        CBitmap bitmap, selBitmap;
        CBitmap *pOrgBitmap = 0, *pOrgSelBitmap = 0;

        memDc.CreateCompatibleDC(&dc);
        bitmap.CreateCompatibleBitmap(&dc, rcLine.right - rcLine.left, m_Rulers[1].m_CharSize);
        pOrgBitmap = memDc.SelectObject(&bitmap);
        memDc.SetBkMode(TRANSPARENT);
        memDc.SetViewportOrg(-rcLine.left, 0);

        CPen* pOrgdMemDcPen = memDc.SelectObject(&m_paintAccessories->m_RulerPen);
        memDc.SetROP2(R2_NOTXORPEN);

        bool columnMarkers = settings.GetColumnMarkers();
        const vector<int>& columnMarkersSet = settings.GetColumnMarkersSet();
        bool indentGuide = settings.GetIndentGuide();
        int indentSpacing = GetIndentSpacing();

        if (blockExist)
        {
            memSelDc.CreateCompatibleDC(&dc);
            selBitmap.CreateCompatibleBitmap(&dc, rcLine.right - rcLine.left, m_Rulers[1].m_CharSize);
            pOrgSelBitmap = memSelDc.SelectObject(&selBitmap);
            memSelDc.SetBkMode(TRANSPARENT);
            memSelDc.SetTextColor(m_paintAccessories->m_SelTextForeground);
            memSelDc.SetViewportOrg(-rcLine.left, 0);
        }

        if (!phighlighter)
        {
            m_paintAccessories->SelectTextFont(memDc);
            memDc.SetTextColor(m_paintAccessories->m_TextForeground);

            if (blockExist)
            {
                m_paintAccessories->SelectTextFont(memSelDc);
                // 29.06.2003 bug fix, selected text foreground color cannot be changed for "Text" class
                memSelDc.SetTextColor(m_paintAccessories->m_SelTextForeground);
            }
        }

        int endFileLine = GetLineCount() - m_Rulers[1].m_Topmost;
        int curLine = GetPosition().line -  m_Rulers[1].m_Topmost;

        for (int i(scr.start.line); i <= scr.end.line; i++)
        {
            int firstWordPos = -1;

            COLORREF color = 0;

            if (i >= (m_nFistHighlightedLine - m_Rulers[1].m_Topmost) 
            && i <= (m_nLastHighlightedLine - m_Rulers[1].m_Topmost))
                color = m_paintAccessories->m_HighlightingBackground;
            else
                if (i == curLine)
                    color = m_paintAccessories->m_CurrentLineBackground;
                else
                    color = m_paintAccessories->m_TextBackground;

            memDc.FillSolidRect(&rcLine, color);

            for (int j(0); j < sizeof(m_braceHighlighting.line)/sizeof(m_braceHighlighting.line[0]); j++)
                if (m_braceHighlighting.line[j] != -1
                && (m_braceHighlighting.line[j] - m_Rulers[1].m_Topmost) == i)
                {
                    rcBar.left   = m_Rulers[0].InxToPix(m_braceHighlighting.offset[j] - m_Rulers[0].m_Topmost);
                    rcBar.right  = m_Rulers[0].InxToPix(m_braceHighlighting.offset[j] - m_Rulers[0].m_Topmost + m_braceHighlighting.length[j]);
                    if (!m_braceHighlighting.broken)
                        memDc.FillSolidRect(&rcBar, m_paintAccessories->m_HighlightingBackground);
                    else
                        memDc.FillSolidRect(&rcBar, m_paintAccessories->m_ErrorHighlightingBackground);
                }

            if (blockExist && blk.line_in_rect(i))
            {
                switch (blkMode)
                {
                case ebtColumn:
                    rcBar.left   = m_Rulers[0].InxToPix(blk.start.column);
                    rcBar.right  = m_Rulers[0].InxToPix(blk.end.column);
                    break;
                case ebtStream:
                default: // because of warning
                    if (blk.start.line == i)
                        rcBar.left = m_Rulers[0].InxToPix(blk.start.column);
                    else
                        rcBar.left = rcLine.left;

                    if (blk.end.line == i)
                        rcBar.right = m_Rulers[0].InxToPix(blk.end.column);
                    else
                        rcBar.right = rcLine.right;
                    break;
                }
                if (rcBar.left == rcBar.right) rcBar.right++; // make it visible

                memSelDc.FillSolidRect(&rcBar, m_paintAccessories->m_SelTextBackground);

                //COLORREF color = 0;

                //if (i >= (m_nFistHighlightedLine - m_Rulers[1].m_Topmost) 
                //&& i <= (m_nLastHighlightedLine - m_Rulers[1].m_Topmost))
                //    color = m_paintAccessories->m_HighlightingBackground;
                //else
                //    color = m_paintAccessories->m_SelTextBackground;

                //memSelDc.FillSolidRect(&rcBar, color);
            }
            else
                rcBar.left = rcBar.right = -1;

            if (i < endFileLine)
            {
                OEStringW lineBuff;
                GetLineW(i + m_Rulers[1].m_Topmost, lineBuff);
                const wchar_t* currentLine = lineBuff.data(); 
                int currentLineLength = lineBuff.length();

                // recalculation for maximum of visible line lengthes
                if (currentLineLength)
                {
                    int length = inx2pos(currentLine, currentLineLength, currentLineLength - 1);
                    if (maxLineLength < length)
                        maxLineLength = length;
                }

                if (phighlighter)
                {
                    phighlighter->NextLine(currentLine, currentLineLength);
                    tokenizer.EnableProcessSpaces(phighlighter->IsSpacePrintable() || m_paintAccessories->m_TextFont & 0x04);
                }
                else
                {
                    tokenizer.EnableProcessSpaces(m_paintAccessories->m_TextFont & 0x04 ? true : false);
                }

                tokenizer.StartScan(currentLine, currentLineLength);

                while (!tokenizer.Eol())
                {
                    int pos, len;
                    const wchar_t* str;

                    tokenizer.GetCurentWord(str, pos, len);

                    if (firstWordPos == -1) 
                        firstWordPos = pos;

                    if (phighlighter)
                    {
                        phighlighter->NextWord(str, len, pos);
                        tokenizer.EnableProcessSpaces(phighlighter->IsSpacePrintable() || m_paintAccessories->m_TextFont & 0x04);
                    }

                    if (pos <= scr.end.column + m_Rulers[0].m_Topmost)
                    {
                        if (pos + len > scr.start.column + m_Rulers[0].m_Topmost
                        && pos <= scr.end.column + m_Rulers[0].m_Topmost)
                        {
                            // bug fix, very long lines w/o space or delimiters are shown as blank
                            if (pos < m_Rulers[0].m_Topmost)
                            {
                                int shift = m_Rulers[0].m_Topmost - pos;
                                pos += shift;
                                str += shift;
                                len -= shift;
                                ASSERT(len >= 0);
                                if (len < 0) 
                                    len = 0;
                            }
                            if (len > m_Rulers[0].m_Count+1)
                                len = m_Rulers[0].m_Count+1;

                            if (!blockExist || (rcBar.left == rcBar.right)
                            || !(pos >= blk.start.column + m_Rulers[0].m_Topmost
                              && pos + len <= blk.end.column + m_Rulers[0].m_Topmost))
                            {
                                if (phighlighter)
                                {
                                    m_paintAccessories->SelectFontEx(memDc, phighlighter->GetFontIndex());
                                    memDc.SetTextColor(phighlighter->GetTextColor());
                                }

                                // Highligting MatchingText {
                                if (m_highlightedText.length() > 0
                                && (int)m_highlightedText.length() == len
                                && !wcsnicmp(m_highlightedText.c_str(), str, len))
                                {
                                    CRect rc = rcBar;
                                    rc.left = m_Rulers[0].PosToPix(pos);
                                    rc.right = m_Rulers[0].PosToPix(pos + len);
                                    memDc.FillSolidRect(&rc, m_paintAccessories->m_HighlightingBackground);
                                }
                                // Highligting MatchingText }
                                
                                TextOutToScreen(memDc, m_Rulers, pos, str, len, rcBar, false);
                            }

                            if (blockExist && (rcBar.left != rcBar.right)
                                && ((blkMode != ebtColumn)
                                    || ((blkMode == ebtColumn)
                                        && pos + len > blk.start.column + m_Rulers[0].m_Topmost
                                        && pos <= blk.end.column + m_Rulers[0].m_Topmost
                                       )
                                   )
                               )
                            {
                                if (phighlighter)
                                {
                                    m_paintAccessories->SelectFontEx(memSelDc, phighlighter->GetFontIndex());
                                }
                                //TextOut(memSelDc.m_hDC, m_Rulers[0].PosToPix(pos), rcLine.top, str, len);
                                TextOutToScreen(memSelDc, m_Rulers, pos, str, len, rcBar, true);
                            }
                        }
                    }
                    tokenizer.Next();
                }
            }
            else if (i == endFileLine && settings.GetEOFMark())
            {
                static const char str[] = ">> EOF <<";

                if (sizeof(str)-1 > m_Rulers[0].m_Topmost)
                {
                    m_paintAccessories->SelectTextFont(memDc);
                    memDc.SetTextColor(m_paintAccessories->m_EOFForeground);
                    TextOutA(memSelDc.m_hDC, m_Rulers[0].PosToPix(0), rcLine.top, str, sizeof(str)-1);

                    if (blockExist)
                    {
                        m_paintAccessories->SelectTextFont(memSelDc);
                        TextOutA(memSelDc.m_hDC, m_Rulers[0].PosToPix(0), rcLine.top, str, sizeof(str)-1);
                    }
                }
            }

            if (rcBar.left != rcBar.right)
            {
                int width = rcBar.right - rcBar.left;
                if (blkMode == ebtStream && i < endFileLine
                && !GetCursorBeyondEOL())
                {
                    int lineLength = GetLineLength(i + m_Rulers[1].m_Topmost);

                    if (blk.start.line == blk.end.line
                    && rcBar.left >= m_Rulers[0].PosToPix(lineLength))
                    {
                        width = 0;
                    }
                    else
                    {
                        rcBar.left = min(rcBar.left, (long)m_Rulers[0].PosToPix(lineLength));
                        width = min(rcBar.right, (long)m_Rulers[0].PosToPix(lineLength + 1)) - rcBar.left;
                    }
                }
                if (width)
                    memDc.BitBlt(rcBar.left, 0,
                                 width, m_Rulers[1].m_CharSize,
                                 &memSelDc, rcBar.left, 0, SRCCOPY);
            }

            if (columnMarkers)
            {
                memDc.SelectObject(&m_paintAccessories->m_RulerPen);
                for (vector<int>::const_iterator it = columnMarkersSet.begin();
                    it != columnMarkersSet.end();
                    ++it)
                {
                    memDc.MoveTo(m_Rulers[0].PosToPix(*it), 0);
                    memDc.LineTo(m_Rulers[0].PosToPix(*it), m_Rulers[1].m_CharSize);
                }
            }

            if (indentGuide
            && firstWordPos > indentSpacing)
            {
                memDc.SelectObject(&m_paintAccessories->m_IndentGuidelinePen);
                for (int pos = indentSpacing; pos < firstWordPos /*+ m_Rulers[0].m_Topmost*/; pos += indentSpacing)
                {
                    memDc.MoveTo(m_Rulers[0].PosToPix(pos), -m_Rulers[1].InxToPix(i) % 2);
                    memDc.LineTo(m_Rulers[0].PosToPix(pos), m_Rulers[1].m_CharSize);
                }
            }

            if (locked)
            {
                /* Alpha blending -  for read only painting */
                static BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255-32, 0/*AC_SRC_ALPHA*/};

                dc.FillSolidRect(rcLine.left, m_Rulers[1].InxToPix(i),
                        rcLine.right - rcLine.left, m_Rulers[1].m_CharSize,
                        RGB(95,95,95) 
                    );

                dc.AlphaBlend(rcLine.left, m_Rulers[1].InxToPix(i),
                        rcLine.right - rcLine.left, m_Rulers[1].m_CharSize,
                        &memDc, rcLine.left, 0, rcLine.right - rcLine.left, m_Rulers[1].m_CharSize,
                        blend
                    );
            }
            else
                dc.BitBlt(rcLine.left, m_Rulers[1].InxToPix(i),
                        rcLine.right - rcLine.left, m_Rulers[1].m_CharSize,
                        &memDc, rcLine.left, 0, SRCCOPY);
        }

        memDc.SelectObject(pOrgBitmap);
        memDc.SelectObject(pOrgdMemDcPen);

        if (blockExist && pOrgSelBitmap)
            memSelDc.SelectObject(pOrgSelBitmap);
    }

    if (m_nMaxLineLength < maxLineLength)
    {
        m_nMaxLineLength = maxLineLength;
        AdjustHorzScroller();
    }
}

void COEditorView::OnSettingsChanged ()
{
    const Settings& settings = GetSettings();
    string language = settings.GetLanguage();
    const VisualAttributesSet& set = settings.GetVisualAttributesSet();

    m_syntaxGutter = settings.GetSyntaxGutter();

    m_paintAccessories = COEViewPaintAccessories::GetPaintAccessories(language);
    m_paintAccessories->OnSettingsChanged(this, set);

    m_highlighter = HighlighterFactory::CreateHighlighter(language);
    
    if (m_highlighter.get())
    {
        m_highlighter->InitStorageScanner(GetMultilineQuotesScanner());
        m_highlighter->Init(set);
    }

    const VisualAttribute& textAttr = set.FindByName("Text");
    const VisualAttribute& curLinetAttr = set.FindByName("Current Line");
    HighlightCurrentLine(textAttr.m_Background != curLinetAttr.m_Background ? true : false);

    bool recalcSize = false;

    if (m_Rulers[0].m_CharSize != m_paintAccessories->m_CharSize.cx
    || m_Rulers[1].m_CharSize != m_paintAccessories->m_CharSize.cy)
    {
        m_Rulers[0].m_CharSize = m_paintAccessories->m_CharSize.cx;
        m_Rulers[1].m_CharSize = m_paintAccessories->m_CharSize.cy;
        recalcSize = true;
    }

    int leftMargin = CalculateLeftMargin();

    if (m_Rulers[0].m_Indent != leftMargin)
    {
        m_Rulers[0].m_Indent = leftMargin;
        recalcSize = true;
    }

// ruler
    int rulerHight = settings.GetRuler() ? m_paintAccessories->m_RulerCharSize.cy + 4 : 0;
    if (m_Rulers[1].m_Indent != rulerHight)
    {
        m_Rulers[1].m_Indent = rulerHight;
        recalcSize = true;
    }

    if (recalcSize)
    {
        CRect rc;
        GetClientRect(rc);
        DoSize(SIZE_RESTORED, rc.Width(), rc.Height());
        ShowCaret(::GetFocus() == m_hWnd, true/*dont_destroy_caret*/);
        AdjustCaretPosition();
    }

    Invalidate(FALSE);
}
