/*
    Copyright (C) 2011 Aleksey Kochetov

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
#include "ShellContextMenu.h"

class CustomShellContextMenu : CShellContextMenu
{
public:
    CustomShellContextMenu (
        CWnd* pWnd, CPoint point, const CString& path, 
        CMenu* pUserMenu, bool fileProperties, bool tortoiseGit, bool tortoiseSvn
    );

    UINT ShowContextMenu ();

private:
    CWnd* m_pWnd; 
    CPoint m_point;
    CMenu* m_pUserMenu;
    bool m_fileProperties, m_tortoiseGit, m_tortoiseSvn;
    std::ostringstream m_traceStream;

    virtual HRESULT DoQueryContextMenu (LPCONTEXTMENU pContextMenu, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
};
