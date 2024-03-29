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

/*
    22.05.2002 - bug fix, a font size combo box does not represented a choice properly 
                 if font size is less then 10 
    22.06.2002 - bug fix, WinXP requires "" instead of 0 in CComboBox::AddString
*/

#include "stdafx.h"
#include "resource.h"
#include "ExceptionHelper.h"
#include "VisualAttributesPage.h"
#include "MyUtf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace std;
    using namespace Common;

/////////////////////////////////////////////////////////////////////////////
// helpers

    //DWORD g_colorArray[] =
    //    {
    //        RGB(255,255,255),   RGB(192,192,192),   RGB(128,128,128),   RGB(0,0,0),
    //        RGB(0,0,128),       RGB(0,0,160),       RGB(0,0,192),       RGB(0,0,255),
    //        RGB(0,128,0),       RGB(0,160,0),       RGB(0,192,0),       RGB(0,255,0),
    //        RGB(128,0,0),       RGB(160,0,0),       RGB(192,0,0),       RGB(255,0,0),
    //        RGB(128,0,128),     RGB(192,0,192),     RGB(255,0,192),     RGB(255,0,255),
    //        RGB(128,128,0),     RGB(192,192,0),     RGB(255,192,0),     
    //        RGB(255,255,192),   RGB(255,255,127),   RGB(255,255,0), 
    //        RGB(0,128,128),     RGB(0,192,192),     RGB(0,192,255),     RGB(0,255,255),
    //    };

    /*
    int CALLBACK EnumFontFamExProc(
      ENUMLOGFONTEX *lpelfe,    // logical-font data
      NEWTEXTMETRICEX *lpntme,  // physical-font data
      DWORD FontType,           // type of font
      LPARAM lParam             // application-defined data
    )
    {
        if ((3 & lpelfe->elfLogFont.lfPitchAndFamily) == FIXED_PITCH)
        {
            if (((CComboBox*)lParam)->FindStringExact(0, lpelfe->elfLogFont.lfFaceName) == -1)
                ((CComboBox*)lParam)->AddString(lpelfe->elfLogFont.lfFaceName);
        }
        return 1;
    }
    ::EnumFontFamiliesEx(CWindowDC(this), NULL, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)&m_FontName, 0);
    */

    BOOL APIENTRY enumFontsFunc (
            LPLOGFONT lpLogFont, LPTEXTMETRIC /*lpTextMetric*/, INT /*nFontType*/, LPVOID lpData
        )
    {
        if (((CComboBox*)lpData)->FindStringExact(0, lpLogFont->lfFaceName) == -1)
            ((CComboBox*)lpData)->AddString(lpLogFont->lfFaceName);
        return TRUE;
    }


    BOOL APIENTRY enumFixedFontsFunc (
            LPLOGFONT lpLogFont, LPTEXTMETRIC /*lpTextMetric*/, INT /*nFontType*/, LPVOID lpData
        )
    {
        if ((3 & lpLogFont->lfPitchAndFamily) == FIXED_PITCH
            && ((CComboBox*)lpData)->FindStringExact(0, lpLogFont->lfFaceName) == -1)
            ((CComboBox*)lpData)->AddString(lpLogFont->lfFaceName);
        return TRUE;
    }


    BOOL APIENTRY enumFontSizesFunc (
            LPLOGFONT /*lpLogFont*/, LPTEXTMETRIC lpTextMetric, INT nFontType, LPVOID lpData
        )
    {
        WCHAR buff[64*2];
        INT nPointSize;
        CComboBox& cb = *((CComboBox*)lpData);

        if (nFontType & RASTER_FONTTYPE)
        {
            nPointSize = VisualAttribute::PixelToPoint(lpTextMetric->tmHeight - lpTextMetric->tmInternalLeading);
            _itow(nPointSize, buff, 10);

            if (cb.FindStringExact(0, buff) == -1)
                cb.AddString(buff);
        }
// it looks strange and i can't remember why!!!
        else if (nFontType & TRUETYPE_FONTTYPE)
        {
            for (int i(8); i < 25; i++)
            {
                _itow(i, buff, 10);

                if (cb.FindStringExact(0, buff) == -1)
                    cb.AddString(buff);

                if (i >= 12) i++;
            }
        }
        return TRUE;
    }

/////////////////////////////////////////////////////////////////////////////
// CVisualAttributesPage property page

CVisualAttributesPage::CVisualAttributesPage()
: CPropertyPage(CVisualAttributesPage::IDD),
m_pFont(0)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
    //{{AFX_DATA_INIT(CVisualAttributesPage)
    //}}AFX_DATA_INIT
}

CVisualAttributesPage::CVisualAttributesPage(const vector<VisualAttributesSet*>& data, LPCSTR name)
: CPropertyPage(CVisualAttributesPage::IDD),
m_vasets(data),
m_pFont(0),
m_className(name)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
    //{{AFX_DATA_INIT(CVisualAttributesPage)
    //}}AFX_DATA_INIT
}

CVisualAttributesPage::~CVisualAttributesPage()
{
    try {
        delete m_pFont;
    }
    _DESTRUCTOR_HANDLER_;
}

void CVisualAttributesPage::Init (const vector<VisualAttributesSet*>& data)
{
    ASSERT(!m_hWnd);
    if (!m_hWnd) m_vasets = data;
}

void CVisualAttributesPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CVisualAttributesPage)
    DDX_Control(pDX, IDC_PFC_SAMPLE, m_Sample);
    DDX_Control(pDX, IDC_PFC_FONT_UNDERLINE, m_FontUnderline);
    DDX_Control(pDX, IDC_PFC_FONT_SIZE, m_FontSize);
    DDX_Control(pDX, IDC_PFC_FONT_ITALIC, m_FontItalic);
    DDX_Control(pDX, IDC_PFC_FONT_BOLD, m_FontBold);
    DDX_Control(pDX, IDC_PFC_FOREGROUND, m_Foreground);
    DDX_Control(pDX, IDC_PFC_BACKGROUND, m_Background);
    DDX_Control(pDX, IDC_PFC_CATEGORY, m_Categories);
    DDX_Control(pDX, IDC_PFC_FONT_NAME, m_FontName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVisualAttributesPage, CPropertyPage)
    //{{AFX_MSG_MAP(CVisualAttributesPage)
    ON_NOTIFY(TVN_SELCHANGED, IDC_PFC_CATEGORY, OnSelChangedOnCategory)
    ON_BN_CLICKED(IDC_PFC_FONT_BOLD, OnFontChanged)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_PFC_FONT_ITALIC, OnFontChanged)
    ON_CBN_SELCHANGE(IDC_PFC_FONT_NAME, OnFontChanged)
    ON_CBN_SELCHANGE(IDC_PFC_FONT_SIZE, OnFontChanged)
    ON_BN_CLICKED(IDC_PFC_FONT_UNDERLINE, OnFontChanged)
    ON_MESSAGE(CPN_SELCHANGE, OnColorChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVisualAttributesPage message handlers

LRESULT CVisualAttributesPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    try { EXCEPTION_FRAME;

        return CPropertyPage::WindowProc(message, wParam, lParam);
    }
    _COMMON_DEFAULT_HANDLER_;

    return 0;
}

BOOL CVisualAttributesPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    m_Foreground.SetDefaultText(_T(""));
    m_Background.SetDefaultText(_T(""));

    // fill a tree control
    //m_Categories.SetIndent(0);

    TVINSERTSTRUCT item;
    memset(&item, 0, sizeof(item));

    std::vector<VisualAttributesSet*>::iterator
        it = m_vasets.begin(), end = m_vasets.end();

    bool selectFirst = m_className.IsEmpty() ? true : false;
    int  selIndex = 0;

    for (int i = 0; it != end; ++it, i++)
    {
        VisualAttributesSet& vaset = **it;

        if (!selectFirst)
            selIndex = (vaset.GetName() == m_className) ? i : -1;

        if (selIndex == i) {
            item.item.mask    = TVIF_TEXT|TVIF_STATE;
            item.item.state = item.item.stateMask = TVIS_EXPANDED;
        } else {
            item.item.mask = TVIF_TEXT;
        }
        std::wstring buffer = wstr(vaset.GetName());
        item.item.pszText = const_cast<TCHAR*>(buffer.c_str());
        item.hParent      = m_Categories.InsertItem(&item);


        for (int k = 0, kcount = vaset.GetCount(); k < kcount; k++)
        {
            buffer = wstr(vaset.GetName(k));
            m_vasetsMap[&vaset[k]] = &vaset;
            item.item.mask    = TVIF_TEXT|TVIF_PARAM;
            item.item.pszText = const_cast<WCHAR*>(buffer.c_str());
            item.item.lParam  = reinterpret_cast<long>(&vaset[k]);
            HTREEITEM hItem = m_Categories.InsertItem(&item);
            if (selIndex == i && !k)
                m_Categories.SelectItem(hItem);
        }

        item.hParent = 0;
    }

    ShowAttribute();

    return TRUE;
}

HBRUSH CVisualAttributesPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);

    if (*pWnd == m_Sample)
    {
        static CBrush brush;
        brush.DeleteObject();


        VisualAttribute attr;
        if (GetCurrentAttributeMixedWithBase(attr))
        {
            brush.CreateSolidBrush(attr.m_Background);
            pDC->SetBkColor(attr.m_Background);
            pDC->SetTextColor(attr.m_Foreground);
            hbr = brush;
        }
    }
    return hbr;
}

const VisualAttribute* CVisualAttributesPage::GetBaseAttribute (const VisualAttribute* attr)
{
    map<const VisualAttribute*, VisualAttributesSet*>::const_iterator
        it = m_vasetsMap.find(attr);

    return (it != m_vasetsMap.end()) ? &((*(it->second))[0]) : 0;
}

VisualAttributesSet* CVisualAttributesPage::GetCurrentAttributesSet ()
{
    VisualAttribute* attr = 0;

    if (!attr)
    {
        HTREEITEM hItem = m_Categories.GetSelectedItem();

        for (int i = 0; hItem != NULL && i < 2; i++)
        {
            DWORD data = m_Categories.GetItemData(hItem);

            if (data)
            {
                attr = reinterpret_cast<VisualAttribute*>(data);
                break;
            }

            hItem = m_Categories.GetChildItem(hItem);
        }
    }

    map<const VisualAttribute*, VisualAttributesSet*>::iterator
        it = m_vasetsMap.find(attr);

    return (it != m_vasetsMap.end()) ? it->second : 0;
}

VisualAttribute* CVisualAttributesPage::GetCurrentAttribute ()
{
    HTREEITEM hItem = m_Categories.GetSelectedItem();

    if (hItem != NULL)
    {
        DWORD data = m_Categories.GetItemData(hItem);
        return reinterpret_cast<VisualAttribute*>(data);
    }

    return NULL;
}

bool CVisualAttributesPage::GetCurrentAttributeMixedWithBase (VisualAttribute& attr)
{
    VisualAttribute* curr = GetCurrentAttribute();
    if (curr)
    {
        curr->Construct(attr, *GetBaseAttribute(curr));
        return true;
    }
    return false;
}

void CVisualAttributesPage::ShowAttribute ()
{
    VisualAttribute* curr = GetCurrentAttribute();

    if (!curr)
    {
        m_FontName.     ResetContent();
        m_FontSize.     ResetContent();
        m_FontBold.     SetCheck(FALSE);
        m_FontItalic.   SetCheck(FALSE);
        m_FontUnderline.SetCheck(FALSE);

        m_FontName.     EnableWindow(FALSE);
        m_FontSize.     EnableWindow(FALSE);
        m_FontBold.     EnableWindow(FALSE);
        m_FontItalic.   EnableWindow(FALSE);
        m_FontUnderline.EnableWindow(FALSE);
        m_Foreground.   EnableWindow(FALSE);
        m_Background.   EnableWindow(FALSE);
        m_Sample.       EnableWindow(FALSE);
    }
    else
    {
        m_FontName.     EnableWindow(curr->m_Mask & vaoFontName);
        m_FontSize.     EnableWindow(curr->m_Mask & vaoFontSize);
        m_FontBold.     EnableWindow(curr->m_Mask & vaoFontBold);
        m_FontItalic.   EnableWindow(curr->m_Mask & vaoFontItalic);
        m_FontUnderline.EnableWindow(curr->m_Mask & vaoFontUnderline);

        m_Foreground.EnableWindow(curr->m_Mask & vaoForeground);
        m_Background.EnableWindow(curr->m_Mask & vaoBackground);

        m_Sample.EnableWindow(TRUE);

        VisualAttribute attr;
        if (GetCurrentAttributeMixedWithBase(attr))
            ShowFont(attr);

        m_Foreground.SetColor(curr->m_Foreground);
        m_Background.SetColor(curr->m_Background);
    }
}

void CVisualAttributesPage::ShowFont (const VisualAttribute& attr)
{
    m_FontName.ResetContent();
    if (attr.m_Mask & vaoFontFixedPitch)
        ::EnumFonts(CWindowDC(this), NULL, (FONTENUMPROC)enumFixedFontsFunc, (LPARAM)&m_FontName);
    else
        ::EnumFonts(CWindowDC(this), NULL, (FONTENUMPROC)enumFontsFunc, (LPARAM)&m_FontName);

    int index = m_FontName.FindStringExact(0, wstr(attr.m_FontName).c_str());
    m_FontName.SetCurSel((index !=  CB_ERR) ? index : -1);

    m_FontSize.ResetContent();
    EnumFonts(CWindowDC(this), wstr(attr.m_FontName).c_str(), (FONTENUMPROC)enumFontSizesFunc, (LPARAM)&m_FontSize);

    TCHAR buff[64*2];
    _itow(attr.m_FontSize, buff, 10);
    index = m_FontSize.FindStringExact(0, buff);
    m_FontSize.SetCurSel((index != CB_ERR) ? index : -1);

    m_FontBold.SetCheck(attr.m_FontBold);
    m_FontItalic.SetCheck(attr.m_FontItalic);
    m_FontUnderline.SetCheck(attr.m_FontUnderline);

    SetFont(attr);
}

void CVisualAttributesPage::SetFont (const VisualAttribute& attr)
{
    CFont* oldFont = m_pFont;
    m_pFont = attr.NewFont();
    m_Sample.SetFont(m_pFont);
    m_Sample.Invalidate();
    delete oldFont;
}

void CVisualAttributesPage::OnSelChangedOnCategory (NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    ShowAttribute();
    *pResult = 0;
}

void CVisualAttributesPage::OnFontChanged()
{
    VisualAttribute* curr = GetCurrentAttribute();

    if (curr)
    {
        CString buff;

        int inx = m_FontName.GetCurSel();

        if (inx != CB_ERR)
        {
            m_FontName.GetLBText(inx, buff);
            curr->m_FontName = str(buff);
        }

        inx = m_FontSize.GetCurSel();

        if (inx != CB_ERR)
        {
            m_FontSize.GetLBText(inx, buff);
            curr->m_FontSize = _wtoi(buff);
        }

        curr->m_FontBold      = m_FontBold.GetCheck() ? true : false;
        curr->m_FontItalic    = m_FontItalic.GetCheck() ? true : false;
        curr->m_FontUnderline = m_FontUnderline.GetCheck() ? true : false;

        ShowFont(*curr);

        SetModified(TRUE);
    }
}

LRESULT CVisualAttributesPage::OnColorChanged (WPARAM, LPARAM)
{
    VisualAttribute* curr = GetCurrentAttribute();

    if (curr)
    {
        curr->m_Foreground = m_Foreground.GetColor();
        curr->m_Background = m_Background.GetColor();

        m_Sample.Invalidate();

        SetModified(TRUE);
    }

    return TRUE;
}

