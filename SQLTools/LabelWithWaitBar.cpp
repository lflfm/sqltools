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

#include "stdafx.h"
#include "LabelWithWaitBar.h"
#include <gdiplus.h>
using namespace Gdiplus;


// LabelWithWaitBar

//IMPLEMENT_DYNAMIC(LabelWithWaitBar, CStatic)

LabelWithWaitBar::LabelWithWaitBar()
    : m_nProgressPos(0), 
    m_nGradientWidth(100), 
    m_nTimerId(777),
    m_nTimerElapse(200),
    m_animation(false),
    m_nTextPosition(DT_CENTER)
{
}

LabelWithWaitBar::~LabelWithWaitBar()
{
}


BEGIN_MESSAGE_MAP(LabelWithWaitBar, CStatic)
    ON_WM_TIMER()
END_MESSAGE_MAP()


// LabelWithWaitBar message handlers

void LabelWithWaitBar::SetText (LPCTSTR pText, COLORREF color) 
{
    m_text = pText;
    m_textColor = color;
    if (m_hWnd) Invalidate(TRUE);
}

void LabelWithWaitBar::StartAnimation (bool animation)
{
    if (m_animation != animation)
    {
        m_animation = animation;

        if (!animation)
        {
            KillTimer(m_nTimerId);
            Invalidate(FALSE);
        }
        else
        {
            m_nProgressPos = -m_nGradientWidth + m_nGradientWidth / 5;
            SetTimer(m_nTimerId, m_nTimerElapse, NULL);
            Invalidate(FALSE);
        }
    }
}

void LabelWithWaitBar::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if (lpDrawItemStruct)
    {
        DWORD bkColor = GetSysColor(COLOR_3DFACE);
        HBRUSH hbrush = CreateSolidBrush(bkColor);
        ::FillRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, hbrush);
        ::DeleteObject(hbrush);

        if (m_animation)
        {
            Graphics graphics(lpDrawItemStruct->hDC);

            BYTE red   = static_cast<BYTE>((bkColor & 0xFF0000) >> 16);
            BYTE green = static_cast<BYTE>((bkColor & 0x00FF00) >> 8);
            BYTE blue  = static_cast<BYTE>((bkColor & 0x0000FF));

            LinearGradientBrush linGrBrush(
               Point(0,   10),
               Point(m_nGradientWidth, 10),
               Color(255, red, green, blue), 
               Color(255, 0, 255, 0));    

            REAL relativeIntensities[] = {0.0f, 1.0f, 0.0f};
            REAL relativePositions[]   = {0.0f, 0.5f, 1.0f};

            linGrBrush.SetBlend(relativeIntensities, relativePositions, 3);
            graphics.TranslateTransform(static_cast<REAL>(m_nProgressPos), 0);
            graphics.FillRectangle(&linGrBrush, lpDrawItemStruct->rcItem.left/* + m_nProgressPos*/, lpDrawItemStruct->rcItem.top, 
                m_nGradientWidth/*rc.Width()*/, lpDrawItemStruct->rcItem.bottom - lpDrawItemStruct->rcItem.top);
        }
        ::SetTextColor(lpDrawItemStruct->hDC, m_textColor);

        bool center = (m_nTextPosition == DT_CENTER) ? true : false;

        if (center)
        {
            RECT rc = lpDrawItemStruct->rcItem;
            ::DrawText(lpDrawItemStruct->hDC, m_text, -1, &rc, DT_CALCRECT|DT_VCENTER|DT_SINGLELINE);
            if ((rc.right - rc.left) > (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left))
                center = false;
        }

        SetBkMode(lpDrawItemStruct->hDC, TRANSPARENT);
        ::DrawText(lpDrawItemStruct->hDC, m_text, -1, &lpDrawItemStruct->rcItem, center ? DT_CENTER|DT_VCENTER|DT_SINGLELINE : DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    }
}


void LabelWithWaitBar::OnTimer(UINT_PTR nIDEvent)
{
    CStatic::OnTimer(nIDEvent);

    if (nIDEvent == m_nTimerId)
    {
        CRect rc;
        GetClientRect(rc);
        m_nProgressPos += m_nGradientWidth / 10;
        if (m_nProgressPos > rc.Width())
            m_nProgressPos = -m_nGradientWidth + m_nGradientWidth / 10;

        Invalidate(FALSE);
    }
}
