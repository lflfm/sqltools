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

/***************************************************************************/
/*      Purpose: Text data source implementation for grid component        */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~           */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/
// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 09.09.2011 bug fix, grid painting is extreamally slow 
//            if grid is loaded with very long strings (> 1mb) 
// 2017-11-28 implemented range selection
// 2017-11-28 added 3 new export formats

#include "stdafx.h"
#include <sstream>
#include <COMMON\SimpleDragDataSource.h>         // for Drag & Drop
#include "GridSourceBase.h"
#include "CellEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{
    using Common::nvector;

    ETextFormat GridStringSource::m_OutputFormat = etfPlainText;
    char GridStringSource::m_FieldDelimiterChar = ',';
    char GridStringSource::m_QuoteChar          = '"';
    char GridStringSource::m_QuoteEscapeChar    = '"';
	char GridStringSource::m_PrefixChar         = '=';
    bool GridStringSource::m_OutputWithHeader   = false;
    bool GridStringSource::m_ColumnNameAsAttr   = false;
    string GridStringSource::m_XmlRootElement     ("data");
    string GridStringSource::m_XmlRecordElement   ("record");
    int    GridStringSource::m_XmlFieldElementCase = 1;

GridSourceBase::GridSourceBase ()
: m_ColumnInfo("GridSourceBase::m_ColumnInfo")
{
    SetFixSize(eoVert, edHorz, 6);
    SetFixSize(eoVert, edVert, 1);
    SetFixSize(eoHorz, edHorz, 30);
    SetFixSize(eoHorz, edVert, 1);
    m_Editor = 0;
    m_ShowFixCol = m_ShowFixRow =
    m_ShowRowEnumeration = true;
    m_ShowTransparentFixCol = false;
    m_MaxShowLines = 1;
    m_ImageList = 0;
    m_bDeleteImageList = false;
    m_Orientation = eoVert;//eoHorz;
}

GridSourceBase::~GridSourceBase ()
{
    try { EXCEPTION_FRAME;

    if (m_bDeleteImageList)
        delete m_ImageList;
    }
    _DESTRUCTOR_HANDLER_;
}

// Cell management
int GridSourceBase::GetFirstFixCount (eDirection dir) const
{
    return (dir == edHorz && m_ShowFixCol || dir == edVert && m_ShowFixRow) ? 1 : 0;
}

int GridSourceBase::GetLastFixCount (eDirection) const
{
    return 0;
}

int GridSourceBase::GetSize (int pos, eDirection dir) const
{
    if (IsTableOrientation() && dir == edHorz)
        return GetColCharWidth(pos);
    else if (IsFormOrientation() && dir == edVert)
        return GetColCharHeight(pos);

    return 1;
}

int GridSourceBase::GetFirstFixSize (int pos, eDirection dir) const
{
    _ASSERTE(!pos);
    return GetFixSize(m_Orientation, dir);
}

int GridSourceBase::GetLastFixSize (int, eDirection) const
{
    _ASSERTE(0);
    return 1;
}

void GridSourceBase::PaintCell (DrawCellContexts& dcc) const
{
    CPen* oldPen;
    CRect _rc = dcc.m_Rect;
    _rc.InflateRect(dcc.m_CellMargin);
    int bkClrIndex = IsTableOrientation() ? dcc.m_Row & 1 : dcc.m_Col & 1;

    if (dcc.IsDataCell())
    {
        if (IsEmpty() || dcc.m_Select == esNone)
        {
            dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_BkBrush[bkClrIndex]);
            dcc.m_Dc->SetBkColor(dcc.m_BkColor[bkClrIndex]);
            dcc.m_Dc->SetTextColor(dcc.m_TextColor);
        }
        else if (dcc.m_Select == esRect)
        {
            dcc.m_Dc->SetBkColor(dcc.m_HightLightColor);
            dcc.m_Dc->SetTextColor(dcc.m_HightLightTextColor);
            dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_HightLightBrush);
        }
        else if (dcc.m_Select == esBorder)
        {
            oldPen = dcc.m_Dc->SelectObject(&dcc.m_HightLightPen);
            dcc.m_Dc->Rectangle(dcc.m_Rect);
            dcc.m_Dc->SelectObject(oldPen);
        }
        if (!IsEmpty())
        {
            //_ASSERTE(GetCount(edVert));
            if (GetCount(edVert))
            {
                std::wstring text;
                GetCellStr(text, dcc.m_Row, dcc.m_Col);
                if (text.length() <= 1024)
                {
                    ::DrawText(dcc.m_Dc->m_hDC, text.data(), text.length(), _rc,
                        GetColAlignment((IsTableOrientation() ? dcc.m_Col : dcc.m_Row)) == ecaLeft
                        ? DT_LEFT|DT_VCENTER|DT_EXPANDTABS|DT_NOPREFIX|DT_WORD_ELLIPSIS
                        : DT_RIGHT|DT_VCENTER|DT_EXPANDTABS|DT_NOPREFIX|DT_WORD_ELLIPSIS);             
                }
                // 09.09.2011 bug fix, grid painting is extreamally slow if grid is loaded with very long strings (> 1mb) 
                // 2016.06.05 bug fix, if multiline clob with very long first line is shown as single-lined 
                else
                {
                    std::wstring line, buffer;
                    std::wistringstream in(text);
                    
                    int nrows = dcc.m_Rect.Height() / dcc.m_CharSize[edVert];
                    int ncols = dcc.m_Rect.Width() / dcc.m_CharSize[edHorz] * 2;

                    for (int i = 0; getline(in, line) && i < nrows; ++i)
                    {
                        buffer += line.substr(0, ncols);
                        if ((int)line.length() > ncols) 
                            buffer += L"...";
                        buffer += '\n';
                    }
                    if (i)
                        buffer += L"...";
                    
                    ::DrawText(dcc.m_Dc->m_hDC, buffer.data(), buffer.length(), _rc,
                        GetColAlignment((IsTableOrientation() ? dcc.m_Col : dcc.m_Row)) == ecaLeft
                        ? DT_LEFT|DT_VCENTER|DT_EXPANDTABS|DT_NOPREFIX|DT_WORD_ELLIPSIS
                        : DT_RIGHT|DT_VCENTER|DT_EXPANDTABS|DT_NOPREFIX|DT_WORD_ELLIPSIS);             
                }
            }
        }
    } else {
        dcc.m_Dc->SetBkColor(dcc.m_FixBkColor);
        dcc.m_Dc->SetTextColor(dcc.m_FixTextColor);
        if (IsEmpty())
        {
            DrawDecFrame(dcc);
        }
        else
        {
            if (dcc.m_Type[IsTableOrientation() ? 0 : 1] == efNone && dcc.m_Type[IsTableOrientation() ? 1 : 0] == efFirst)
            {
                DrawDecFrame(dcc);
                const string& text = GetColHeader(IsTableOrientation() ? dcc.m_Col : dcc.m_Row);
                std::wstring wtext = Common::wstr(text);
                ::DrawTextW(dcc.m_Dc->m_hDC, wtext.data(), wtext.length(), _rc, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_WORD_ELLIPSIS);
            }
            else if (dcc.m_Type[IsTableOrientation() ? 1 : 0] == efNone && dcc.m_Type[IsTableOrientation() ? 0 : 1] == efFirst)
            {
                if (!m_ShowTransparentFixCol)
                    DrawDecFrame(dcc);
                else
                    dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_BkBrush[bkClrIndex]);
                if (m_ShowRowEnumeration)
                {
                    char buffer[18];
                    ::DrawTextA(dcc.m_Dc->m_hDC, itoa(IsTableOrientation() ? dcc.m_Row + 1 : dcc.m_Col + 1, buffer, 10), -1, _rc, DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_WORD_ELLIPSIS);
                }
                if (m_ImageList)
                {
                    int nIndex = GetImageIndex(dcc.m_Row);
                    if (nIndex != -1)
                        m_ImageList->Draw(dcc.m_Dc, nIndex, _rc.TopLeft(), ILD_TRANSPARENT);
                }
            }
            else
            {
                DrawDecFrame(dcc);
            }
        }
    }
}

void GridSourceBase::CalculateCell (DrawCellContexts& dcc, size_t maxTextLength) const
{
    dcc.m_Rect.SetRectEmpty();

    if (!IsEmpty())
    {
        if (dcc.IsDataCell())
        {
            _ASSERTE(GetCount(edVert));
            if (GetCount(edVert))
            {
                string text;
                GetCellStr(text, dcc.m_Row, dcc.m_Col);

                ::DrawTextA(dcc.m_Dc->m_hDC, text.data(), min(text.length(), maxTextLength), dcc.m_Rect, DT_CALCRECT|DT_EXPANDTABS|DT_NOPREFIX);

                dcc.m_Rect.InflateRect(-dcc.m_CellMargin.cx, -dcc.m_CellMargin.cy);
            }
        }
        else
        {
            if (dcc.m_Type[IsTableOrientation() ? 0 : 1] == efNone
            && dcc.m_Type[IsTableOrientation() ? 1 : 0] == efFirst)
            {
                const string& text = GetColHeader(IsTableOrientation() ? dcc.m_Col : dcc.m_Row);

                ::DrawTextA(dcc.m_Dc->m_hDC, text.data(), min(text.length(), maxTextLength), dcc.m_Rect, DT_CALCRECT|DT_NOPREFIX);

                dcc.m_Rect.InflateRect(-dcc.m_CellMargin.cx, -dcc.m_CellMargin.cy);
            }
        }
    }
}

void GridSourceBase::DrawDecFrame (DrawCellContexts& dcc) const
{
    //dcc.m_Dc->FillRect(dcc.m_Rect, (dcc.m_Select == esNone) ? &dcc.m_FixBrush : &dcc.m_HightLightBrush);
    dcc.m_Dc->FillRect(dcc.m_Rect, &dcc.m_FixBrush);

    CPen lgtPen(PS_SOLID|PS_JOIN_ROUND, 1,  (dcc.m_Select == esNone) ? dcc.m_LightColor : dcc.m_FixShdColor);
    CPen* oldPen = dcc.m_Dc->SelectObject(&lgtPen);

    //dcc.m_Dc->SetROP2(R2_COPYPEN);
    {
        POINT pts[3] = { dcc.m_Rect.left, dcc.m_Rect.bottom, dcc.m_Rect.left, dcc.m_Rect.top, dcc.m_Rect.right, dcc.m_Rect.top };
        dcc.m_Dc->Polyline(pts, 3);
    }

    CPen shdPen(PS_SOLID, 1, (dcc.m_Select == esNone) ? dcc.m_FixShdColor : dcc.m_LightColor);
    dcc.m_Dc->SelectObject(&shdPen);

    {
        POINT pts[3] = { dcc.m_Rect.left + 1, dcc.m_Rect.bottom - 1, dcc.m_Rect.right - 1, dcc.m_Rect.bottom - 1, dcc.m_Rect.right - 1, dcc.m_Rect.top };
        dcc.m_Dc->Polyline(pts, 3);
    }

    dcc.m_Dc->SelectObject(*oldPen);
}

CWnd* GridSourceBase::BeginEdit (int row, int col, CWnd* parent, const CRect& rect)  // get editor for grid window
{
    _ASSERTE(!m_Editor && !IsEmpty());

    string text;
    GetCellStr(text, row, col);

    if (!text.empty())
    {
        std::istringstream istr(text);
        std::ostringstream ostr;
        std::string buff;

        int nlines = 0;
        while (std::getline(istr, buff, '\n'))
        {
            if (nlines++)
                ostr << '\r' << '\n';
            ostr << buff;
        }
        ostr << std::ends;

        DWORD dwStyle = WS_VISIBLE|WS_CHILD|
                        ES_AUTOHSCROLL|ES_AUTOVSCROLL|
                        ES_NOHIDESEL|ES_READONLY;
        if (nlines > 1)
            dwStyle |= ES_MULTILINE|WS_VSCROLL|WS_HSCROLL;

        m_Editor = new CellEditor;
        m_Editor->Create(dwStyle, rect, parent, 999);
        std::wstring wstr = Common::wstr(ostr.str());
        m_Editor->SetWindowText(wstr.c_str());

        if (nlines <= 1)
            m_Editor->SetSel(0, wstr.length());
    }
    return m_Editor;
}

void GridSourceBase::PostEdit () // event for commit edit result &
{
    _ASSERTE(!IsEmpty());
    m_Editor->DestroyWindow();
    delete m_Editor;
    m_Editor = 0;
}

void GridSourceBase::CancelEdit () // end edit session
{
    _ASSERTE(!IsEmpty());
    m_Editor->DestroyWindow();
    delete m_Editor;
    m_Editor = 0;
}

    // TODO: convert to unicode
    static
    void CopyHTML (const char *html) {
        // Create temporary buffer for HTML header...
        char *buf = new char [400 + strlen(html)];
        if(!buf) return;

        // Get clipboard id for HTML format...
        static int cfid = 0;
        if(!cfid) cfid = RegisterClipboardFormat(L"HTML Format");

        // Create a template string for the HTML header...
        strcpy(buf,
            "Version:0.9\r\n"
            "StartHTML:00000000\r\n"
            "EndHTML:00000000\r\n"
            "StartFragment:00000000\r\n"
            "EndFragment:00000000\r\n"
            "<html><body>\r\n"
            "<!--StartFragment -->\r\n");

        // Append the HTML...
        strcat(buf, html);
        strcat(buf, "\r\n");
        // Finish up the HTML format...
        strcat(buf,
            "<!--EndFragment-->\r\n"
            "</body>\r\n"
            "</html>");

        // Now go back, calculate all the lengths, and write out the
        // necessary header information. Note, wsprintf() truncates the
        // string when you overwrite it so you follow up with code to replace
        // the 0 appended at the end with a '\r'...
        char *ptr = strstr(buf, "StartHTML");
        wsprintfA(ptr+10, "%08u", strstr(buf, "<html>") - buf);
        *(ptr+10+8) = '\r';

        ptr = strstr(buf, "EndHTML");
        wsprintfA(ptr+8, "%08u", strlen(buf));
        *(ptr+8+8) = '\r';

        ptr = strstr(buf, "StartFragment");
        wsprintfA(ptr+14, "%08u", strstr(buf, "<!--StartFrag") - buf);
        *(ptr+14+8) = '\r';

        ptr = strstr(buf, "EndFragment");
        wsprintfA(ptr+12, "%08u", strstr(buf, "<!--EndFrag") - buf);
        *(ptr+12+8) = '\r';

        // Now you have everything in place ready to put on the
        // clipboard.

        // Open the clipboard...
/*        if(OpenClipboard(0))*/ {
        
            // Empty what's in there...
/*            EmptyClipboard();*/
        
            // Allocate global memory for transfer...
            HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE |GMEM_DDESHARE, strlen(buf)+4);
        
            // Put your string in the global memory...
            char *ptr = (char *)GlobalLock(hText);
            strcpy(ptr, buf);
            GlobalUnlock(hText);
        
            ::SetClipboardData(cfid, hText);
        
/*            CloseClipboard();*/
            // Free memory...
            GlobalFree(hText);
        
        }

        // Clean up...
        delete [] buf;
    }

// clipboard operation support
// need Open & Close
void GridSourceBase::DoEditCopy (int row, int nrows, int col, int ncols, ECopyWhat what, int format)
{
    if (EmptyClipboard())
    {
        std::string buff;
        int actualFormat = CopyToString(buff, row, nrows, col, ncols, what, format);
        if (!buff.empty())
        {
            if (actualFormat == etfHtml)
                CopyHTML(buff.c_str());

            std::wstring wbuff = Common::wstr(buff);
            int size = (wbuff.length() + 1)  * sizeof(wchar_t);
            HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, size);
            char* dest = (char*)GlobalLock(hData);
            if (dest) memcpy(dest, wbuff.c_str(), size);
            SetClipboardData(CF_UNICODETEXT, hData);

            if (!(ncols == -1 || ncols == GetCount(edHorz)))
            {
                HGLOBAL hCodeSignature =  NULL;
                hCodeSignature = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, sizeof("{code}"));
                char* codeSignature = (char*)GlobalLock(hCodeSignature);
                if (codeSignature) memcpy(codeSignature, "{code}", sizeof("{code}"));
                SetClipboardData(CF_PRIVATEFIRST, hCodeSignature);
            }
        }
    }
}

void GridSourceBase::CopyColumnNames (std::string& buff, int from, int to) const
{
    int nCols = m_ColumnInfo.size();
    for (int i = from; i < nCols && i <= to; i++)
    {
        buff += GetColHeader(i);
        if (i != nCols - 1 && i != to)
            buff += ", ";
    }
}

int GridSourceBase::CopyToString (std::string& buff, int row, int nrows, int col, int ncols, ECopyWhat what, int format)
{
    int actualFormat = etfPlainText;

    if (!IsEmpty())
    {
        if (!IsTableOrientation())
        {
            switch (what)
            {
            case ecDragRowHeader:
            case ecRowHeader:
                what = ecColumnHeader;
                col = row;
                ncols = nrows;
                row = nrows = -1;
                break;
            case ecDragColumnHeader:
            case ecColumnHeader:
                what = ecRow; // getCellStr transposes itself so no need change columns and rows like for headers
                //what = ecRowHeader;
                //row = col;
                //nrows = ncols;
                //col = ncols = -1;
                break;
            }
        }

        switch (what)
        {
        case ecFieldNames:
        case ecDragTopLeftCorner:
            CopyColumnNames(buff, 0, std::numeric_limits<int>::max());
            break;
        case ecColumnHeader:
        case ecDragColumnHeader:
            CopyColumnNames(buff, col, col + ncols - 1);
            break;
        case ecRowHeader:
            {
                std::stringstream of;
                of << row;
                buff = of.str();
            }
            break;
        case ecDataCell:
        case ecColumn:
        case ecRow:
        case ecDragRowHeader:
        case ecEverything:
            switch (what)
            {
            case ecColumn:
                row = nrows = -1;
                break;
            case ecRow:
            case ecDragRowHeader:
                break;
            case ecEverything:
                row = nrows = -1;
                col = ncols = -1;
            }
            std::stringstream of;
            actualFormat = ExportText(of, row, nrows, col, ncols, format);
            buff = of.str();
            break;
        }
    }
    
    return actualFormat;
}

void GridSourceBase::DoEditPaste ()
{
    // Not implement
}

void GridSourceBase::DoEditCut ()
{
    // Not implement
}

    class GridStringOleSource : public COleDataSource
    {
        GridSourceBase* m_pSource;
        int m_row, m_nrows, m_col, m_ncols;
        ECopyWhat m_what;
    public:
        GridStringOleSource(GridSourceBase* pSource, int row, int nrows, int col, int ncols, ECopyWhat what)
            : m_pSource(pSource), m_row(row), m_nrows(nrows), m_col(col), m_ncols(ncols), m_what(what)
        {
            DelayRenderData(CF_TEXT);
            if (!(ncols == -1 || ncols == pSource->GetCount(edHorz)))
                DelayRenderData(CF_PRIVATEFIRST);
        }

        virtual BOOL OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
        {
            CWaitCursor wait;

            string text;

            switch (lpFormatEtc->cfFormat)
            {
            case CF_TEXT:
                {
                    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    m_pSource->CopyToString(text, m_row, m_nrows, m_col, m_ncols, m_what, 
                        !shift && !ctrl ? etfPlainText : (
                                shift && !ctrl ? etfPLSQL1 : (
                                        !shift && ctrl ? etfPLSQL2 : etfPLSQL3
                                    )
                            )
                    );
                }
                break;
            case CF_PRIVATEFIRST:
                text = "{code}";
                break;
            }

            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, text.size() + 1);

            if (!text.empty())
            {
                if (hGlobal)
                {
                    char* buff = (char*)GlobalLock(hGlobal);
                    if (buff)
                    {
                        memcpy(buff, text.c_str(), text.size() + 1);
                        GlobalUnlock(hGlobal);
                        *phGlobal = hGlobal;
                        return TRUE;
                    }
                    GlobalFree(hGlobal);
                }
            }
            return FALSE;
        }
    };

BOOL GridSourceBase::DoDragDrop (int row, int nrows, int col, int ncols, ECopyWhat what)
{
    if (!IsEmpty())
    {
        GridStringOleSource(this, row, nrows, col, ncols, what).DoDragDrop(DROPEFFECT_COPY|DROPEFFECT_MOVE|DROPEFFECT_LINK);
        return TRUE;
    }
    return FALSE;
}

DROPEFFECT GridSourceBase::DoDragOver (COleDataObject*, DWORD)
{
    // Not implement
    return DROPEFFECT_NONE;
}

BOOL GridSourceBase::DoDrop (COleDataObject*, DROPEFFECT)
{
    // Not implement
    return FALSE;
}

//const char* GridSourceBase::GetTooltipString (TooltipCellContexts& tcc) const
//{
//    tcc.m_CellRecall = false;
//    if (tcc.IsDataCell())
//    {
//        return GetCellStr(tcc.m_Row, tcc.m_Col);
//    }
//    else if (tcc.m_Type[0] == efNone && tcc.m_Type[1] == efFirst)
//    {
//        return GetColHeader(tcc.m_Col).c_str();
//    }
//    else if (tcc.m_Type[1] == efNone && tcc.m_Type[0] == efFirst)
//    {
//        char buffer[18];
//        return itoa(tcc.m_Row + 1, buffer, 10);
//    }
//    return "";
//}

void GridSourceBase::SetCols (int cols)
{
    m_ColumnInfo.clear();
    m_ColumnInfo.resize(cols, ColumnInfo());
    SetEmpty(false);
}

void GridSourceBase::SetImageList (CImageList* imageList, BOOL deleteImageList)
{
    if (m_bDeleteImageList)
        delete m_ImageList;

    m_ImageList = imageList;
    m_bDeleteImageList = deleteImageList;
}

////////////////////////////////////////////////////////////////////////

int GridStringSource::GetCount (eDirection dir) const
{
    return /*(IsEmpty()) ? 1 :*/ ((IsTableOrientation() ? dir == edHorz : dir == edVert) ? m_table.GetColumnCount() : m_table.GetRowCount());
}

void GridStringSource::SetCols (int cols)
{
    GridSourceBase::SetCols(cols);
    m_table.Format(cols);
}

void GridStringSource::Clear ()
{
    m_table.Clear();
    SetEmpty(true);
}

int GridStringSource::Append ()
{
    int line = m_table.Append();
    Notify_ChangedRowsQuantity(line, 1);
    return line;
}

int GridStringSource::Insert (int row)
{
    m_table.Insert(row);
    Notify_ChangedRowsQuantity(row, 1);
    return row;
}

void GridStringSource::Delete (int row)
{
    ASSERT(m_table.GetRowCount() > 0);
    if (m_table.GetRowCount() > 0)
    {
        m_table.Delete(row);
        Notify_ChangedRowsQuantity(row, -1);
    }
}

void GridStringSource::DeleteAll ()
{
    if (int count = m_table.GetRowCount())
    {
        m_table.DeleteAll();
	    Notify_ChangedRowsQuantity(0, -count);
    }
}

int GridStringSource::GetSize (int pos, eDirection dir) const
{
    if (IsTableOrientation())
    {
        if (dir == edVert)
        {
            if (GetMaxShowLines() > 1)
                return m_table.GetMaxLinesInRow(pos, GetMaxShowLines());
            return 1;
        }
    }
    else // IsFormOrientation()
    {
        if (dir == edVert)
        {
            if (GetMaxShowLines() > 1)
                return m_table.GetMaxLinesInColumn(pos, GetMaxShowLines());
            return 1;
        }
        return 100;
    }

    return GridSourceBase::GetSize(pos, dir);
}

void GridStringSource::GetMaxColumnWidthByFirstLine (nvector<size_t>& maxLengths, int from, int to) const
{
    if (IsTableOrientation())
        m_table.GetMaxColumnWidthByFirstLine(maxLengths, from, to);
    else
        m_table.GetMaxRowWidthByFirstLine(maxLengths, from, to);
}

void GridStringSource::GetCellStrForExport (string& str, int line, int col, int) const
{
    GetCellStr(str, line, col);
}

}//namespace OG2

