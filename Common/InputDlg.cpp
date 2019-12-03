/* 
    Copyright (C) 2004 Aleksey Kochetov

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

// 03.01.2005 (ak) added m_password to hide input content
// 09.01.2005 (ak) created CPasswordDlg because an attrinute "password" can be changed dynamicly

#include "stdafx.h"
#include "InputDlg.h"
#include <COMMON/DlgDataExt.h>

namespace Common
{

void CInputDlgImpl::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_INP_PROMPT, m_prompt);
    DDX_Text(pDX, IDC_INP_VALUE, m_value);
}

BOOL CInputDlgImpl::OnInitDialog()
{
    if (!m_title.empty())
        SetWindowText(m_title.c_str());
     return CDialog::OnInitDialog();
}

};//Common