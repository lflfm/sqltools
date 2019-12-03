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

#ifndef __OEViewPaintAccessories_h__
#define __OEViewPaintAccessories_h__

#include <map>
#include <string>
#include <arg_shared.h>
#include "COMMON/VisualAttributes.h"

    using arg::counted_ptr;
    struct COEViewPaintAccessories;
    typedef counted_ptr<COEViewPaintAccessories> COEViewPaintAccessoriesPtr;
    
struct COEViewPaintAccessories
{
    CFont    m_Fonts[8];                // bold 0x01, italic 0x02, underline 0x04
    int      m_FontCharacterExtra[8];
    CSize    m_CharSize;
    unsigned m_TextFont;
    COLORREF m_CurrentLineBackground;
    COLORREF m_TextForeground;
    COLORREF m_TextBackground;
    COLORREF m_SelTextForeground;
    COLORREF m_SelTextBackground;
    CPen     m_BmkPen;
    CBrush   m_BmkBrush;
    COLORREF m_BmkBackground;
    CPen     m_RndBmkPen;
    CBrush   m_RndBmkBrush;
    COLORREF m_RndBmkForeground;
    COLORREF m_RndBmkBackground;
    COLORREF m_EOFForeground;
    CFont    m_RulerFont;
    CSize    m_RulerCharSize;

    COLORREF m_HighlightingBackground;
    COLORREF m_ErrorHighlightingBackground;

    static COLORREF m_COLOR_BTNHIGHLIGHT;
    static COLORREF m_COLOR_BTNFACE;
    static COLORREF m_COLOR_BTNTEXT;
    static COLORREF m_COLOR_BTNSHADOW;

    static CPen m_RulerPen;
    static CPen m_IndentGuidelinePen;
    static CPen m_BtnShadowPen;

    static CBrush m_BtnFaceBrush;

    static COEViewPaintAccessoriesPtr GetPaintAccessories (const std::string&);

    COEViewPaintAccessories ();

    CFont* SelectFont (CDC& dc, int font)
    {
        dc.SetTextCharacterExtra(m_FontCharacterExtra[font]);
        return dc.SelectObject(&m_Fonts[font]);
    }

    CFont* SelectTextFont (CDC& dc)
    {
        return SelectFont(dc, m_TextFont);
    }

    void OnSettingsChanged (CWnd*, const OpenEditor::VisualAttributesSet& set);

};

#endif//__OEViewPaintAccessories_h__
