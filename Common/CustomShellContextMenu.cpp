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

/*
  2017-12-05, bug fix, bitmap menu items are not properly filtered
*/
#include "stdafx.h"
#include "CustomShellContextMenu.h"

#define BOOST_REGEX_NO_LIB
#define BOOST_REGEX_STATIC_LINK
#include <boost/regex.hpp>
#include <WinException/WinException.h>

    using namespace boost;

    struct MenuFilter
    {
        regex_t     m_pattern;
        regmatch_t  match[10];

        MenuFilter (const char* pattern = "(^properties$)|(^tsvn.*$)|(^.*cvs.*$)")  
        //MenuFilter (const char* pattern = "open")  
        { 
            ::memset(this, 0, sizeof(*this)); 
            VERIFY(regcomp(&m_pattern, pattern, REG_EXTENDED|REG_ICASE) == REG_NOERROR);
        }

        ~MenuFilter () 
        { 
            try { regfree(&m_pattern); } 
            _DESTRUCTOR_HANDLER_; 
        }

        bool IsMatched (const char* str)
        {
            match[0].rm_so = 0;
            match[0].rm_eo = strlen(str);
            return regexec(&m_pattern, str, sizeof(match)/sizeof(match[0]), match, REG_STARTEND) == REG_NOERROR ? true : false;
        }

        void Walk (HMENU& hMenu, LPCONTEXTMENU pContextMenu)
        {
            MENUITEMINFO mii;
            memset(&mii, 0, sizeof mii);
            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

            int nitems = GetMenuItemCount(hMenu);

            for (int i = 0; i < nitems; i++)
            {
                char buffer[1024];
                mii.dwTypeData = buffer;
                mii.cch = sizeof buffer;

                if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
                {
                    if (mii.fType != MFT_SEPARATOR)
                    {
                        if (mii.hSubMenu != NULL)
                            Walk(mii.hSubMenu, pContextMenu);

                        char buffer[1024];
                        if (NOERROR == pContextMenu->GetCommandString(mii.wID-1, GCS_VERBA, 0, buffer, sizeof(buffer)))
                        {
                            TRACE2("\tCmd=%d Verb=%s\n", mii.wID-1, buffer);
                            if (!IsMatched(buffer))
                            {
                                ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND|MF_GRAYED);
                            }
                        }
                        else if (!mii.hSubMenu && (mii.wID-1 < 10000))
                        {
	                        TRACE("\tdisabled because of no verb\n");
                            ::EnableMenuItem(hMenu, i, MF_BYPOSITION|MF_GRAYED);
                        }
                    }
                }
            }
        }

        void Cleanup (HMENU& hMenu)
        {
            MENUITEMINFO mii;
            memset(&mii, 0, sizeof mii);
            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

            int nitems = GetMenuItemCount(hMenu);

            for (int i = nitems - 1; i >= 0; --i)
            {
                char buffer[1024];
                mii.dwTypeData = buffer;
                mii.cch = sizeof buffer;

                if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
                {
                    if (mii.hSubMenu != NULL)
                    {
                        Cleanup(mii.hSubMenu);

                        if (!GetMenuItemCount(mii.hSubMenu))
                            DeleteMenu(hMenu, i,  MF_BYPOSITION);
                    }
                    else if (mii.fType != MFT_SEPARATOR)
                    {
                        if (GetMenuState(hMenu, i, MF_BYPOSITION|MF_GRAYED) & MF_GRAYED)
                            DeleteMenu(hMenu, i, MF_BYPOSITION);
                    }
                }
            }

            nitems = GetMenuItemCount(hMenu);

            for (int i = nitems - 1; i >= 0; --i)
            {
                if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
                {
                    if (mii.fType == MFT_SEPARATOR)
                    {
                        if (!i)
                            DeleteMenu(hMenu, i, MF_BYPOSITION);

                        if (i > 0 && GetMenuItemInfo(hMenu, i - 1, TRUE, &mii))
                        {
                            if (mii.fType == MFT_SEPARATOR)
                                DeleteMenu(hMenu, i, MF_BYPOSITION);
                        }
                    }
                }
            }

        }
    };


CustomShellContextMenu::CustomShellContextMenu (
    CWnd* pWnd, CPoint point, const CString& path, 
    CMenu* pUserMenu, const CString& shellContexMenuFilter
)
: m_pWnd(pWnd), m_point(point), 
m_pUserMenu(pUserMenu), m_shellContexMenuFilter(shellContexMenuFilter)
{
	SetObjects(path);
}

UINT CustomShellContextMenu::ShowContextMenu ()
{
    return CShellContextMenu::ShowContextMenu(m_pWnd, m_point);
}

HRESULT CustomShellContextMenu::DoQueryContextMenu (LPCONTEXTMENU pContextMenu, HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT result = CShellContextMenu::DoQueryContextMenu(pContextMenu, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    MenuFilter filter(m_shellContexMenuFilter);
    filter.Walk(hMenu, pContextMenu);
    filter.Cleanup(hMenu);

    CMenu* pPopup = GetMenu();

    if (m_pUserMenu)
    {
        MENUITEMINFO itemInfo;
        memset(&itemInfo, 0, sizeof(itemInfo));
        itemInfo.cbSize = sizeof(itemInfo);
        itemInfo.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE | MIIM_STATE;
                                                                                                    
        UINT nitems = m_pUserMenu->GetMenuItemCount();                                               

        for (UINT i = 0; i < nitems; i++)
        {
            char buffer[1024];
            itemInfo.dwTypeData = buffer;
            itemInfo.cch = sizeof buffer;
            if (m_pUserMenu->GetMenuItemInfo(i, &itemInfo, TRUE))
                pPopup->InsertMenuItem(i, &itemInfo, TRUE);
        }

        itemInfo.wID = (UINT)-1;
        itemInfo.fType = MF_SEPARATOR;
        pPopup->InsertMenuItem(i, &itemInfo, TRUE);
    }

    return result;
}
