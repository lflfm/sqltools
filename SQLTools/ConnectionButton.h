/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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

#pragma once
#include <afxtoolbarbutton.h>
#include "LabelWithWaitBar.h"


class ConnectionButton : public CMFCToolBarButton
{
	DECLARE_SERIAL(ConnectionButton)

public:
    ConnectionButton ();
    ConnectionButton (UINT uiId, int iImage, DWORD, int iWidth = 0) ;

    virtual ~ConnectionButton ();

    void OnChangeParentWnd (CWnd* pWndParent);
    virtual SIZE OnCalculateSize (CDC* pDC, const CSize& sizeDefault, BOOL bHorz);
    virtual void ConnectionButton::OnMove ();
    virtual void ConnectionButton::OnSize (int iSize);
    void AdjustRect ();

	virtual BOOL CanBeStretched() const { return TRUE; }
	virtual BOOL IsRibbonButton() const { return FALSE; }
    virtual void CopyFrom (const CMFCToolBarButton&);

    LabelWithWaitBar*  GetLabel () { return m_pLabel; }

private:
    int m_iWidth;
    LabelWithWaitBar* m_pLabel;
};

