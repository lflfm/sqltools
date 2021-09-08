/* 
    Copyright (C) 2005 Aleksey Kochetov

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
#include <time.h>
#include "resource.h"
#include "OENewFileDlg.h"
#include "OEDocument.h"
#include <Common/DlgDataExt.h>
#include "Common/MyUtf.h"

typedef COEMultiDocTemplate::FormatFields FormatFields;

COENewFileDlg::COENewFileDlg(COEMultiDocTemplate& docTemplate, CWnd* pParent /*=NULL*/)
    : CDialog(COENewFileDlg::IDD, pParent),
    m_docTemplate(docTemplate)
{
}

COENewFileDlg::~COENewFileDlg()
{
}

void COENewFileDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OENF_CLASS,    m_category);
    DDX_Text(pDX, IDC_OENF_TEMPLATE, m_template);
    DDX_Text(pDX, IDC_OENF_FILENAME, m_filename);
}


BEGIN_MESSAGE_MAP(COENewFileDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_OENF_CLASS, OnCbnSelchange_Category)
    ON_BN_CLICKED(IDC_OENF_SUBSTITUTION, OnBnClicked_Substitution)
    ON_EN_CHANGE(IDC_OENF_TEMPLATE, OnEnChange_Template)
    ON_COMMAND_RANGE(IDC_OENF_SUBSTITUTION + 1, 
        IDC_OENF_SUBSTITUTION + 1 + FormatFields::max_size(),
        OnSubstitution)
END_MESSAGE_MAP()


// COENewFileDlg message handlers

BOOL COENewFileDlg::OnInitDialog()
{
    try { EXCEPTION_FRAME;

        CDialog::OnInitDialog();

        if (!m_edtTemplate.SubclassWindow(::GetDlgItem(m_hWnd, IDC_OENF_TEMPLATE)))
            ::SetWindowPos(::GetDlgItem(m_hWnd, IDC_OENF_SUBSTITUTION), 0, 0, 0, 0, 0, SWP_HIDEWINDOW);

        const OpenEditor::SettingsManager& mgrl 
            = COEDocument::GetSettingsManager();

        std::wstring defClass = Common::wstr(mgrl.GetGlobalSettings()->GetDefaultClass());

        SendDlgItemMessage(IDC_OENF_CLASS, CB_RESETCONTENT);
        
        int count = mgrl.GetClassCount();

        for (int i = 0; i < count; ++i)
        {
            std::wstring name = Common::wstr(mgrl.GetClassByPos(i)->GetName());

            SendDlgItemMessage(IDC_OENF_CLASS, CB_ADDSTRING, 0, (LPARAM)name.c_str());

            if (name == defClass)
                SendDlgItemMessage(IDC_OENF_CLASS, CB_SETCURSEL, i);
        }
        
        OnCbnSelchange_Category();
    }
    _OE_DEFAULT_HANDLER_;

    return TRUE;
}

void COENewFileDlg::OnCbnSelchange_Category()
{
    try { EXCEPTION_FRAME;

        UpdateData();

        const OpenEditor::SettingsManager& mgrl = COEDocument::GetSettingsManager();

        OpenEditor::ClassSettingsPtr settings = mgrl.GetClassByPos(SendDlgItemMessage(IDC_OENF_CLASS, CB_GETCURSEL));

        m_template = Common::wstr(settings->GetFilenameTemplate()).c_str();

        m_filename = m_template;

        m_docTemplate.FormatTitle(m_template, m_filename);

        UpdateData(FALSE);
    }
    _OE_DEFAULT_HANDLER_;
}


void COENewFileDlg::OnBnClicked_Substitution ()
{
    try { EXCEPTION_FRAME;

        ASSERT(::GetDlgItem(m_hWnd, IDC_OENF_SUBSTITUTION));
        HWND hButton = ::GetDlgItem(m_hWnd, IDC_OENF_SUBSTITUTION);

        if (hButton
        && ::SendMessage(hButton, BM_GETCHECK, 0, 0L) == BST_UNCHECKED)
        {
            CMenu menu;
            menu.CreatePopupMenu();

            const FormatFields& formatFields = m_docTemplate.GetFormatFields();
            FormatFields::const_iterator it = formatFields.begin();
            for (int i = 0; it != formatFields.end(); ++it, ++i)
            {
                ASSERT(::GetDlgItem(m_hWnd, IDC_OENF_SUBSTITUTION + i + 1) == NULL);
                if (i == 1) menu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)0);
                menu.AppendMenu(MF_STRING|MF_ENABLED, IDC_OENF_SUBSTITUTION + i + 1, Common::wstr(it->menuLabel).c_str());
            }

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

void COENewFileDlg::OnSubstitution (UINT id)
{
    try { EXCEPTION_FRAME;

        unsigned int index = id - (IDC_OENF_SUBSTITUTION + 1);

        ASSERT(index < FormatFields::max_size());

        if (index < FormatFields::max_size())
            m_edtTemplate.InsertAtCurPos(Common::wstr(m_docTemplate.GetFormatFields().at(index).displayFormat).c_str(), -1);
    }
    _OE_DEFAULT_HANDLER_;
}

void COENewFileDlg::OnEnChange_Template ()
{
    try { EXCEPTION_FRAME;

        UpdateData();

        m_docTemplate.FormatTitle(m_template, m_filename);

        ::SetWindowText(::GetDlgItem(m_hWnd, IDC_OENF_FILENAME), m_filename);
    }
    _OE_DEFAULT_HANDLER_;
}
