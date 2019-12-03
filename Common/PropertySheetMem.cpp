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

#include "stdafx.h"
#include "Common/PropertySheetMem.h"

namespace Common {

    class CustomTreeCtrl : public CTreeCtrl {
    protected:
        virtual BOOL PreCreateWindow(CREATESTRUCT& cs) {
            //cs.style |= TVS_SHOWSELALWAYS|TVS_SINGLEEXPAND|TVS_DISABLEDRAGDROP;
            //cs.style &= ~(TVS_TRACKSELECT|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS);
            cs.style |= TVS_SHOWSELALWAYS|TVS_DISABLEDRAGDROP|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_FULLROWSELECT;
            cs.style &= ~(TVS_TRACKSELECT);
            cs.dwExStyle |= WS_EX_CLIENTEDGE;
            return CTreeCtrl::PreCreateWindow(cs);
        }
    };

CPropertySheetMem::CPropertySheetMem(LPCTSTR pszCaption, UINT& selectPage, CWnd* pParentWnd)
    : TreePropSheet::CTreePropSheet(pszCaption, pParentWnd, selectPage), m_selectPage(selectPage) 
{
	SetEmptyPageText("Please select a child item of '%s'.");
    SetTreeViewMode(/*bTreeViewMode =*/FALSE, /*bPageCaption =*/FALSE, /*bTreeImages =*/FALSE);
    SetAllExpand(TRUE);
}

CTreeCtrl* CPropertySheetMem::CreatePageTreeObject () 
{ 
    return new CustomTreeCtrl; 
}

// CPropertySheetMem
BEGIN_MESSAGE_MAP(CPropertySheetMem, TreePropSheet::CTreePropSheet)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

// CPropertySheetMem message handlers
void CPropertySheetMem::OnDestroy()
{
    m_selectPage = GetActiveIndex();
    CTreePropSheet::OnDestroy();
}

}; // namespace Common
