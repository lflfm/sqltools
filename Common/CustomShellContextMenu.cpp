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
  2017-12-05, switched from boost to std::regex
  2017-12-05, bug fix, bitmap menu items are not properly filtered
*/

#include "stdafx.h"
#include "CustomShellContextMenu.h"

#include <regex>
#include <WinException/WinException.h>
#include "MyUtf.h"
#include "Common/ErrorDlg.h"

    using namespace boost;

    struct MenuFilter
    {
        std::regex  m_pattern;
        std::cmatch m_match;
        std::ostringstream& m_traceStream;

        //"(^properties$)|(^tsvn.*$)|(^.*cvs.*$)")  
        MenuFilter (LPCTSTR filter, std::ostringstream& traceStream)  
            : m_pattern(Common::str(filter), std::regex_constants::icase|std::regex_constants::extended),
            m_traceStream(traceStream)
        { 
        }

        ~MenuFilter () 
        { 
        }

        bool IsMatched (const char* str)
        {
            if (std::regex_search(str, m_match, m_pattern))
            {
                //string result = m_match.str(0);
                return true;
            }
            return false;
        }

        bool IsThatGitSubmenu (HMENU& hMenu, LPCONTEXTMENU pContextMenu)
        {
            std::regex  pattern("(Diff laterDiff later)|(\\bAdd\\b)|(Blame&Blame)|(AboutA&bout)", std::regex_constants::icase|std::regex_constants::ECMAScript);
            std::cmatch match;
            int match_counter = 0;

            MENUITEMINFO mii;
            memset(&mii, 0, sizeof mii);
            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

            int nitems = GetMenuItemCount(hMenu);

            for (int i = 0; i < nitems; i++)
            {
                TCHAR buffer[1024];
                mii.dwTypeData = buffer;
                mii.cch = sizeof buffer/sizeof(TCHAR);

                if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
                {
                    if (mii.fType != MFT_SEPARATOR)
                    {
                        char command[1024];
                        if (NOERROR == pContextMenu->GetCommandString(mii.wID-1, GCS_VERBA, 0, command, sizeof(command)))
                        {
                            TRACE2("\t\tcounter=%d Verb=%s\n", match_counter, Common::wstr(command).c_str());
                            if (std::regex_search(command, match, pattern))
                            {
                                match_counter++;
                            }
                        }
                    }
                }
            }

            return match_counter >= 3;
        }

        void Walk (HMENU& hMenu, LPCONTEXTMENU pContextMenu, bool tortoiseGit)
        {
            m_traceStream << "MenuFilter::Walk in " << hMenu << endl;

            MENUITEMINFO mii;
            memset(&mii, 0, sizeof mii);
            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

            m_traceStream << "MenuFilter::Walk calling GetMenuItemCount " << endl;
            int nitems = GetMenuItemCount(hMenu);
            m_traceStream << "MenuFilter::Walk GetMenuItemCount returned " << nitems << endl;

            for (int i = 0; i < nitems; i++)
            {
                m_traceStream << "MenuFilter::Walk looping, i = " << i << endl;

                TCHAR buffer[1024];
                mii.dwTypeData = buffer;
                mii.cch = sizeof buffer/sizeof(TCHAR);


                m_traceStream << "MenuFilter::Walk calling GetMenuItemInfo" << endl;
                if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
                {
                    if (mii.fType != MFT_SEPARATOR)
                    {
                        if (mii.hSubMenu != NULL
                        && tortoiseGit && IsThatGitSubmenu(mii.hSubMenu, pContextMenu))
                            continue;
                        
                        if (mii.hSubMenu != NULL)
                        {
                            m_traceStream << "MenuFilter::Walk recursive call for submenu... " << endl;
                            Walk(mii.hSubMenu, pContextMenu, tortoiseGit);
                        }

                        m_traceStream << "MenuFilter::Walk checking the item verb... " << endl;
                        char command[1024];
                        if (NOERROR == pContextMenu->GetCommandString(mii.wID-1, GCS_VERBA, 0, command, sizeof(command)))
                        {
                            m_traceStream << "MenuFilter::Walk the verb = \"" << command << "\", cmd = " << mii.wID-1 << endl;
                            TRACE2("\tCmd=%d Verb=%s\n", mii.wID-1, Common::wstr(command).c_str());
                            if (!IsMatched(command))
                            {
                                m_traceStream << "MenuFilter::Walk the verb is matched, enabling the menu item" << endl;
                                ::EnableMenuItem(hMenu, mii.wID, MF_BYCOMMAND|MF_GRAYED);
                            }
                        }
                        else if (!mii.hSubMenu && (mii.wID-1 < 10000))
                        {
                            m_traceStream << "MenuFilter::Walk desabling the menu item becausethere is no verb" << endl;
	                        TRACE("\tdisabled because of no verb\n");
                            ::EnableMenuItem(hMenu, i, MF_BYPOSITION|MF_GRAYED);
                        }
                    }
                }
            }

            m_traceStream << "MenuFilter::Walk out " << hMenu << endl;
        }

        void Cleanup (HMENU& hMenu)
        {
            m_traceStream << "MenuFilter::Cleanup in " << hMenu << endl;

            MENUITEMINFO mii;
            memset(&mii, 0, sizeof mii);
            mii.cbSize = sizeof(mii);
            mii.fMask  = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_TYPE;

            m_traceStream << "MenuFilter::Cleanup calling GetMenuItemCount" << endl;
            int nitems = GetMenuItemCount(hMenu);
            m_traceStream << "MenuFilter::Cleanup GetMenuItemCount returned " << nitems << endl;

            for (int i = nitems - 1; i >= 0; --i)
            {
                m_traceStream << "MenuFilter::Cleanup looping, i = " << i << endl;

                TCHAR buffer[1024];
                mii.dwTypeData = buffer;
                mii.cch = sizeof buffer;

                nitems = GetMenuItemCount(hMenu);
                m_traceStream << "MenuFilter::Cleanup paranoic GetMenuItemCount returned " << nitems << endl;
                m_traceStream << "MenuFilter::Cleanup calling GetMenuItemInfo" << endl;

                if (i < nitems // paranoic check
                && GetMenuItemInfo(hMenu, i, TRUE, &mii))
                {
                    if (mii.hSubMenu != NULL)
                    {
                        m_traceStream << "MenuFilter::Cleanup recursive call for submenu... " << endl;
                        Cleanup(mii.hSubMenu);

                        if (!GetMenuItemCount(mii.hSubMenu))
                        {
                            m_traceStream << "MenuFilter::Cleanup the submenu is empty, deleting it " << endl;
                            DeleteMenu(hMenu, i,  MF_BYPOSITION);
                        }
                    }
                    else if (mii.fType != MFT_SEPARATOR)
                    {
                        if (GetMenuState(hMenu, i, MF_BYPOSITION|MF_GRAYED) & MF_GRAYED)
                        {
                            m_traceStream << "MenuFilter::Cleanup the item is gray, deleting it " << endl;
                            DeleteMenu(hMenu, i, MF_BYPOSITION);
                        }
                    }
                }
            }

            int skipCleanupSeparaters = AfxGetApp()->GetProfileInt(L"CustomShellContextMenu", L"SkipCleanupSeparaters", 0);

            if (!skipCleanupSeparaters)
            try
            {
                m_traceStream << "MenuFilter::Cleanup calling GetMenuItemCount for the second loop for removing duplicate separators" << endl;
                nitems = GetMenuItemCount(hMenu);
                m_traceStream << "MenuFilter::Cleanup GetMenuItemCount returned " << nitems << endl;

                for (int i = nitems - 1; i >= 0; --i)
                {
                    m_traceStream << "MenuFilter::Cleanup looping, i = " << i << endl;

                    if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
                    {
                        m_traceStream << "MenuFilter::Cleanup calling GetMenuItemInfo for i " << i << endl;
                        if (mii.fType == MFT_SEPARATOR)
                        {
                            if (!i)
                            {
                                m_traceStream << "MenuFilter::Cleanup delete the separaor at i = 0" << endl;
                                DeleteMenu(hMenu, i, MF_BYPOSITION);
                            }

                            m_traceStream << "MenuFilter::Cleanup calling GetMenuItemInfo for i - 1 " << (i - 1) << endl;
                            if (i > 0 && GetMenuItemInfo(hMenu, i - 1, TRUE, &mii))
                            {
                                if (mii.fType == MFT_SEPARATOR)
                                {
                                    m_traceStream << "MenuFilter::Cleanup delete the separaor at i = " << i << endl;
                                    DeleteMenu(hMenu, i, MF_BYPOSITION);
                                }
                            }
                        }
                    }
                }
            }
            catch (...)
            {
                AfxGetApp()->WriteProfileInt(L"CustomShellContextMenu", L"SkipCleanupSeparaters", 1);
            }

            m_traceStream << "MenuFilter::Cleanup out " << hMenu << endl;
        }
    };


CustomShellContextMenu::CustomShellContextMenu (
    CWnd* pWnd, CPoint point, const CString& path, 
    CMenu* pUserMenu, bool fileProperties, bool tortoiseGit, bool tortoiseSvn
)
    : m_pWnd(pWnd), m_point(point), 
    m_pUserMenu(pUserMenu),
    m_fileProperties(fileProperties),
    m_tortoiseGit   (tortoiseGit   ),
    m_tortoiseSvn   (tortoiseSvn   )
{
	SetObjects(path);
}

UINT CustomShellContextMenu::ShowContextMenu ()
{
    return CShellContextMenu::ShowContextMenu(m_pWnd, m_point);
}

HRESULT CustomShellContextMenu::DoQueryContextMenu (LPCONTEXTMENU pContextMenu, HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    try
    {
        m_traceStream << "CustomShellContextMenu::DoQueryContextMenu in" << endl;

        m_traceStream << "calling CShellContextMenu::DoQueryContextMenu..." << endl;
        HRESULT result = CShellContextMenu::DoQueryContextMenu(pContextMenu, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

        std::wstring filterStr;
        if (m_fileProperties)
            filterStr = L"(^properties$)";
        if (m_tortoiseGit)
        {
            if (!filterStr.empty())
                filterStr += L"|";
            filterStr += L"git";
        }
        if (m_tortoiseSvn)
        {
            if (!filterStr.empty())
                filterStr += L"|";
            filterStr += L"(^tsvn.*$)";
        }

        MenuFilter filter(filterStr.c_str(), m_traceStream);
        filter.Walk(hMenu, pContextMenu, m_tortoiseGit);
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
                TCHAR buffer[1024];
                itemInfo.dwTypeData = buffer;
                itemInfo.cch = sizeof buffer;
                if (m_pUserMenu->GetMenuItemInfo(i, &itemInfo, TRUE))
                    pPopup->InsertMenuItem(i, &itemInfo, TRUE);
            }

            itemInfo.wID = (UINT)-1;
            itemInfo.fType = MF_SEPARATOR;
            pPopup->InsertMenuItem(i, &itemInfo, TRUE);
        }

        m_traceStream << "CustomShellContextMenu::DoQueryContextMenu out" << endl;


        return result;
    }
    catch (...)
    {
       CErrorDlg::m_extraInfo = m_traceStream.str();
       throw;
    }
}
