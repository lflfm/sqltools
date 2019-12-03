/////////////////////////////////////////////////////////////////////////
//
// CSizingControlBarCF          Version 2.44
// 
// Created: Dec 21, 1998        Last Modified: March 31, 2002
//
// See the official site at www.datamekanix.com for documentation and
// the latest news.
//
/////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998-2002 by Cristi Posea. All rights reserved.
//
// This code is free for personal and commercial use, providing this 
// notice remains intact in the source files and all eventual changes are
// clearly marked with comments.
//
// You must obtain the author's consent before you can include this code
// in a software library.
//
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
// Send bug reports, bug fixes, enhancements, requests, flames, etc. to
// cristi@datamekanix.com or post them at the message board at the site.
/////////////////////////////////////////////////////////////////////////
//
// 14.11.2005 (ak) modern appearance

#include <stdafx.h>
#include "scbarcf.h"

/////////////////////////////////////////////////////////////////////////
// CSizingControlBarCF

IMPLEMENT_DYNAMIC(CSizingControlBarCF, baseCSizingControlBarCF);

int CALLBACK EnumFontFamProc(ENUMLOGFONT FAR *lpelf,
                             NEWTEXTMETRIC FAR *lpntm,
                             int FontType,
                             LPARAM lParam)
{
    UNUSED_ALWAYS(lpelf);
    UNUSED_ALWAYS(lpntm);
    UNUSED_ALWAYS(FontType);
    UNUSED_ALWAYS(lParam);

    return 0;
}
 
CSizingControlBarCF::CSizingControlBarCF()
{
    m_bActive = FALSE;

    CDC dc;
    dc.CreateCompatibleDC(NULL);

    m_sFontFace = (::EnumFontFamilies(dc.m_hDC,
        _T("Tahoma"), (FONTENUMPROC) EnumFontFamProc, 0) == 0) ?
        _T("Tahoma") : _T("Arial");

    dc.DeleteDC();

    m_cyGripper += 4; // (ak)
}

BEGIN_MESSAGE_MAP(CSizingControlBarCF, baseCSizingControlBarCF)
    //{{AFX_MSG_MAP(CSizingControlBarCF)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETTEXT, OnSetText)
END_MESSAGE_MAP()

void CSizingControlBarCF::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
    baseCSizingControlBarCF::OnUpdateCmdUI(pTarget, bDisableIfNoHndler);

    if (!HasGripper())
        return;

    BOOL bNeedPaint = FALSE;

    CWnd* pFocus = GetFocus();
    BOOL bActiveOld = m_bActive;

    m_bActive = (pFocus->GetSafeHwnd() && IsChild(pFocus));

    if (m_bActive != bActiveOld)
        bNeedPaint = TRUE;

    if (bNeedPaint)
        SendMessage(WM_NCPAINT);
}

// gradient defines (if not already defined)
#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28
#define SPI_GETGRADIENTCAPTIONS         0x1008
#endif

void CSizingControlBarCF::NcPaintGripper(CDC* pDC, CRect rcClient)
{
    if (!HasGripper())
        return;

    // compute the caption rectangle
    BOOL bHorz = IsHorzDocked();
    CRect rcGrip = rcClient;
    CRect rcBtn = m_biHide.GetRect();
    if (bHorz)
    {   // right side gripper
        rcGrip.left -= m_cyGripper + 1;
        rcGrip.right = rcGrip.left + 11 + 4; // (ak)
        //rcGrip.top = rcBtn.bottom + 3; // (ak)
    }
    else
    {   // gripper at top
        rcGrip.top -= m_cyGripper + 1;
        rcGrip.bottom = rcGrip.top + 11 + 4; // (ak)
        //rcGrip.right = rcBtn.left - 3; // (ak)
    }
    rcGrip.InflateRect(bHorz ? 1 : 0, bHorz ? 0 : 1);

    // draw the caption background
    //CBrush br;
    COLORREF clrCptn = m_bActive ?
        ::GetSysColor(COLOR_ACTIVECAPTION) :
        ::GetSysColor(COLOR_3DFACE/*COLOR_INACTIVECAPTION*/); // (ak)

	// (ak): don't want gradient painting
    //// query gradient info (usually TRUE for Win98/Win2k)
    //BOOL bGradient = FALSE;
    //if (m_bActive)
    //::SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &bGradient, 0);
    
    //if (!bGradient)
        pDC->FillSolidRect(&rcGrip, clrCptn); // solid color
    //else
    //{
    //    // gradient from left to right or from bottom to top
    //    // get second gradient color (the right end)
    //    COLORREF clrCptnRight = m_bActive ?
    //        ::GetSysColor(COLOR_GRADIENTACTIVECAPTION) :
    //        ::GetSysColor(COLOR_GRADIENTINACTIVECAPTION);

    //    // this will make 2^6 = 64 fountain steps
    //    int nShift = 6;
    //    int nSteps = 1 << nShift;

    //    for (int i = 0; i < nSteps; i++)
    //    {
    //        // do a little alpha blending
    //        int nR = (GetRValue(clrCptn) * (nSteps - i) +
    //                  GetRValue(clrCptnRight) * i) >> nShift;
    //        int nG = (GetGValue(clrCptn) * (nSteps - i) +
    //                  GetGValue(clrCptnRight) * i) >> nShift;
    //        int nB = (GetBValue(clrCptn) * (nSteps - i) +
    //                  GetBValue(clrCptnRight) * i) >> nShift;

    //        COLORREF cr = RGB(nR, nG, nB);

    //        // then paint with the resulting color
    //        CRect r2 = rcGrip;
    //        if (bHorz)
    //        {
    //            r2.bottom = rcGrip.bottom - 
    //                ((i * rcGrip.Height()) >> nShift);
    //            r2.top = rcGrip.bottom - 
    //                (((i + 1) * rcGrip.Height()) >> nShift);
    //            if (r2.Height() > 0)
    //                pDC->FillSolidRect(r2, cr);
    //        }
    //        else
    //        {
    //            r2.left = rcGrip.left + 
    //                ((i * rcGrip.Width()) >> nShift);
    //            r2.right = rcGrip.left + 
    //                (((i + 1) * rcGrip.Width()) >> nShift);
    //            if (r2.Width() > 0)
    //                pDC->FillSolidRect(r2, cr);
    //        }
    //    }
    //}
    if (!m_bActive)
    {
        CBrush brushBlue(clrCptn);
        CBrush* pOldBrush = pDC->SelectObject(&brushBlue);
        CPen penShadow;
        penShadow.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
        CPen* pOldPen = pDC->SelectObject(&penShadow);
        pDC->Rectangle(&rcGrip);
        pDC->SelectObject(pOldBrush);
        pDC->SelectObject(pOldPen);
    }


    // draw the caption text - first select a font
    CFont font;
    int ppi = pDC->GetDeviceCaps(LOGPIXELSX);
    int pointsize = MulDiv(85, 96, ppi); // 8.5 points at 96 ppi

    LOGFONT lf;
    BOOL bFont = font.CreatePointFont(pointsize, m_sFontFace);
    if (bFont)
    {
        // get the text color
        COLORREF clrCptnText = m_bActive ?
            ::GetSysColor(COLOR_CAPTIONTEXT) :
            ::GetSysColor(COLOR_BTNTEXT/*COLOR_INACTIVECAPTIONTEXT*/); // (ak)

        int nOldBkMode = pDC->SetBkMode(TRANSPARENT);
        COLORREF clrOldText = pDC->SetTextColor(clrCptnText);

        if (bHorz)
        {
            // rotate text 90 degrees CCW if horizontally docked
            font.GetLogFont(&lf);
            font.DeleteObject();
            lf.lfEscapement = 900;
            font.CreateFontIndirect(&lf);
        }
        
        CFont* pOldFont = pDC->SelectObject(&font);
        CString sTitle;
        GetWindowText(sTitle);

        CPoint ptOrg = bHorz ?
            CPoint(rcGrip.left - 1 + 1, rcGrip.bottom - 3 - 2) : // (ak)
            CPoint(rcGrip.left + 3 + 1, rcGrip.top - 1 + 2); // (ak)

        pDC->ExtTextOut(ptOrg.x, ptOrg.y,
            ETO_CLIPPED, rcGrip, sTitle, NULL);

        pDC->SelectObject(pOldFont);
        pDC->SetBkMode(nOldBkMode);
        pDC->SetTextColor(clrOldText);
    }

    // draw the button
    m_biHide.bActive = m_bActive; // (ak)
    m_biHide.Paint(pDC);
}

LRESULT CSizingControlBarCF::OnSetText(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = baseCSizingControlBarCF::OnSetText(wParam, lParam);

    SendMessage(WM_NCPAINT);

    return lResult;
}
