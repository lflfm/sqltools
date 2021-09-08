/* 
    SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004, 2017 Aleksey Kochetov

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

// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 2017-11-28 added new export formats and OnShiftOnly checkbox

#include "stdafx.h"
#include "SQLTools.h"
#include "PropGridOutputPage.h"
#include <COMMON/DlgDataExt.h>
#include <COMMON/FilenameTemplate.h>

using namespace Common;

// CPropGridOutputPage dialog

CPropGridOutputPage1::CPropGridOutputPage1(SQLToolsSettings& settings) 
    : CPropertyPage(CPropGridOutputPage1::IDD),
    m_settings(settings)
{
    m_bShowShowOnShiftOnly =
    m_bShowOnShiftOnly = FALSE;
    m_bEnableFilenameTemplate = TRUE;
    m_psp.dwFlags &= ~PSP_HASHELP;
}

BOOL CPropGridOutputPage1::OnInitDialog ()
{
    CPropertyPage::OnInitDialog();

    if (!m_edtTemplate.SubclassWindow(::GetDlgItem(m_hWnd, IDC_GOO_FILENAME_TEMPLATE)))
        ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_GOO_SUBSTITUTION), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);

    ::ShowWindow(::GetDlgItem(m_hWnd, IDC_GOO_SHOW_ON_SHIFT_ONLY), m_bShowShowOnShiftOnly ? SW_RESTORE : SW_HIDE);

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_FILENAME_TEMPLATE_LABEL), m_bEnableFilenameTemplate);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_FILENAME_TEMPLATE), m_bEnableFilenameTemplate);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_SUBSTITUTION), m_bEnableFilenameTemplate);

    setupItems();
    return TRUE;  
}

void CPropGridOutputPage1::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_GOO_SHOW_ON_SHIFT_ONLY, m_bShowOnShiftOnly);
    DDX_Radio(pDX, IDC_GOO_PLAIN_TEXT, m_settings.m_GridExpFormat);
    DDX_Check(pDX, IDC_GOO_WITH_HEADERS, m_settings.m_GridExpWithHeader);
    DDX_Text (pDX, IDC_GOO_FILENAME_TEMPLATE, m_settings.m_GridExpFilenameTemplate);
}

// CPropGridOutputPage message handlers
BEGIN_MESSAGE_MAP(CPropGridOutputPage1, CPropertyPage)
    ON_BN_CLICKED(IDC_GOO_HTML, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_PLAIN_TEXT, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_COMMA_DELIMITED, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_TAB_DELIMITED, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_XML_ELEM, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_XML_ATTR, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_PLSQL1, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_PLSQL2, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_PLSQL3, OnExpFormatChanged)
    ON_BN_CLICKED(IDC_GOO_PLSQL4, OnExpFormatChanged)

    ON_BN_CLICKED(IDC_GOO_SUBSTITUTION, OnBnClicked_Substitution)
    ON_COMMAND_RANGE(IDC_GOO_SUBSTITUTION + 1, 
        IDC_GOO_SUBSTITUTION + 1 + FilenameTemplateFields::max_size() + 1,
        OnSubstitution)
END_MESSAGE_MAP()

void CPropGridOutputPage1::setupItems ()
{
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_GOO_WITH_HEADERS), m_settings.m_GridExpFormat < 3 ); //text, CSV, tab delimited
}

void CPropGridOutputPage1::OnExpFormatChanged ()
{
    UpdateData();
    setupItems();
}

void CPropGridOutputPage1::OnBnClicked_Substitution ()
{
    try { EXCEPTION_FRAME;

        ASSERT(::GetDlgItem(m_hWnd, IDC_GOO_SUBSTITUTION));
        HWND hButton = ::GetDlgItem(m_hWnd, IDC_GOO_SUBSTITUTION);

        if (hButton
        && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED)
        {
            CMenu menu;
            menu.CreatePopupMenu();

            const FilenameTemplateFields& formatFields = FilenameTemplate::GetFormatFields();
            FilenameTemplateFields::const_iterator it = formatFields.begin();
            for (int i = 0; it != formatFields.end(); ++it, ++i)
            {
                ASSERT(::GetDlgItem(m_hWnd, IDC_GOO_SUBSTITUTION + i + 1) == NULL);
                if (i == 1) menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)0);
                menu.AppendMenu(MF_STRING|MF_ENABLED, IDC_GOO_SUBSTITUTION + i + 1, Common::wstr(it->menuLabel).c_str());
            }
            menu.InsertMenu(0, MF_BYPOSITION|MF_STRING|MF_ENABLED, IDC_GOO_SUBSTITUTION + FilenameTemplateFields::max_size() + 1, L"&&script\tThe parent script name");
            menu.ModifyMenu(IDC_GOO_SUBSTITUTION + 1, MF_BYCOMMAND, IDC_GOO_SUBSTITUTION + 1, L"&&n\tSave file counter");

            ::SendMessage(hButton, BM_SETCHECK, BST_CHECKED, 0L);

            CRect rect;
            ::GetWindowRect(hButton, &rect);

            TPMPARAMS param;
            param.cbSize = sizeof param;
            param.rcExclude.left   = 0;
            param.rcExclude.top    = rect.top;
            param.rcExclude.right  = USHRT_MAX;
            param.rcExclude.bottom = rect.bottom;

            if (::TrackPopupMenuEx(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                rect.left, rect.bottom + 1, *this, &param))
            {
                MSG msg;
                ::PeekMessage(&msg, hButton, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
            }

            ::PostMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0L);
        }
    }
    _OE_DEFAULT_HANDLER_;
}

void CPropGridOutputPage1::OnSubstitution (UINT id)
{
    try { EXCEPTION_FRAME;

        unsigned int index = id - (IDC_GOO_SUBSTITUTION + 1);

        ASSERT(index < FilenameTemplateFields::max_size() + 1);

        if (index < FilenameTemplateFields::max_size())
            m_edtTemplate.InsertAtCurPos(Common::wstr(FilenameTemplate::GetFormatFields().at(index).displayFormat).c_str(), -1);
        else if (index == FilenameTemplateFields::max_size())
            m_edtTemplate.InsertAtCurPos(L"&script", -1);
    }
    _OE_DEFAULT_HANDLER_;
}

CPropGridOutputPage2::CPropGridOutputPage2(SQLToolsSettings& settings) 
    : CPropertyPage(CPropGridOutputPage2::IDD),
    m_settings(settings)
{
    m_psp.dwFlags &= ~PSP_HASHELP;
}

    static
    void DDV_CheckXmlElem (CDataExchange* pDX, const std::string& value, const char* fieldName)
    {
        if (pDX->m_bSaveAndValidate)
        {
            if (value.empty())
            {
                AfxMessageBox(CString(fieldName) + " is required!", MB_OK|MB_ICONSTOP);
                pDX->Fail();
            }
            else
            {
                {
                    char ch = *value.begin();
                    if (!(isalpha(ch) || isalpha(ch == '_')))
                    {
                        AfxMessageBox(CString(fieldName) + " cannot start with " + ch + "!", MB_OK|MB_ICONSTOP);
                        pDX->Fail();
                    }
                }

                for (string::const_iterator it = value.begin(); it != value.end(); ++it)
                    if (!(isalnum(*it) || *it == '.' || *it == '-' || *it == '_'))
                    {
                        AfxMessageBox(CString(fieldName) + " can contain " + *it + "!", MB_OK|MB_ICONSTOP);
                        pDX->Fail();
                        break;
                    }

            }
        }
    }

void CPropGridOutputPage2::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_GOO2_COMMA_CHAR, m_settings.m_GridExpCommaChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpCommaChar, 1);
    DDX_Text(pDX, IDC_GOO2_QUOTE_CHAR, m_settings.m_GridExpQuoteChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpQuoteChar, 1);
    DDX_Text(pDX, IDC_GOO2_QUOTE_ESC_CHAR, m_settings.m_GridExpQuoteEscapeChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpQuoteEscapeChar, 1);
    DDX_Text(pDX, IDC_GOO2_PREFIX_CHAR, m_settings.m_GridExpPrefixChar);
    DDV_MaxChars(pDX, m_settings.m_GridExpPrefixChar, 1);

    DDX_Text (pDX, IDC_GOO2_ROOT_ELEMENT, m_settings.m_GridExpXmlRootElement);
    DDV_CheckXmlElem(pDX, m_settings.m_GridExpXmlRootElement, "\"Root element\"");

    DDX_Text (pDX, IDC_GOO2_RECORD_ELEMENT, m_settings.m_GridExpXmlRecordElement);
    DDV_CheckXmlElem(pDX, m_settings.m_GridExpXmlRecordElement, "\"Record element\"");

    DDX_Radio(pDX, IDC_GOO2_ELEM_CASE_NOCHANGE, m_settings.m_GridExpXmlFieldElementCase);
}

// CPropGridOutputPageCSV message handlers
BEGIN_MESSAGE_MAP(CPropGridOutputPage2, CPropertyPage)
END_MESSAGE_MAP()

