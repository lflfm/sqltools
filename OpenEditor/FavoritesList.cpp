/*
    Copyright (C) 2016 Aleksey Kochetov

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
#include <fstream>
#include <algorithm> 
#include "FavoritesList.h"
#include "Common/OsException.h"
#include "OpenEditor/OEDocument.h"
#include <tinyxml\tinyxml.h>
#include "COMMON/AppGlobal.h"
#include "COMMON/MyUtf.h"
#include "TiXmlUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    using namespace OpenEditor;

    static const int LOCK_TIMEOUT = 1000;

FavoritesList::FavoritesList ()
    : m_favorites((TiXmlDocument*)0),
    m_modified(false),
    m_fileAccessMutex(FALSE, L"GNU.OpenEditor.Favorites")
{
}

FavoritesList::~FavoritesList ()
{
}

void FavoritesList::Open (const std::wstring& path)
{
    m_path = path;

    CSingleLock lock(&m_fileAccessMutex);
    BOOL locked = lock.Lock(LOCK_TIMEOUT);
    if (!locked) 
        THROW_APP_EXCEPTION("Cannot get exclusive access to \"" + Common::str(m_path) + "\"!");

    if (::PathFileExists(m_path.c_str()))
    {
        m_favorites.reset(new TiXmlDocument);
        TiXmlUtils::LoadFile(*m_favorites, m_path.c_str());

        if (m_favorites->Error())
        {
            if (AfxMessageBox(CString(L"The file \"") + path.c_str() + L"\" is corruted!\r\nWould you like to backup it and cointinue?", MB_ICONEXCLAMATION|MB_OKCANCEL) == IDOK)
            {
                ::DeleteFile((path + L".bad.3").c_str());
                ::MoveFile((path + L".bad.2").c_str(), (path + L".bad.3").c_str());
                ::MoveFile((path + L".bad.1").c_str(), (path + L".bad.2").c_str());
                ::MoveFile( path             .c_str(), (path + L".bad.1").c_str());
                m_favorites.reset(0);
            }
            else
                THROW_APP_EXCEPTION("FavoritesList: xml parsing error!");
        }
    }
    
    if (!m_favorites.get()) // create empty one
    {
        m_favorites.reset(new TiXmlDocument);
        m_favorites->LinkEndChild(new TiXmlDeclaration("1.0","","yes"));
        TiXmlElement* rootEl = new TiXmlElement("Favorites");
        m_favorites->LinkEndChild(rootEl);
    }
    m_modified = false;

    CFileWatchClient::StartWatching(m_path.c_str());
}

void FavoritesList::Save ()
{
    if (m_modified && m_favorites.get())
    {
        CFileWatchClient::StopWatching();

        CSingleLock lock(&m_fileAccessMutex);
        BOOL locked = lock.Lock(LOCK_TIMEOUT);
        if (!locked) 
            THROW_APP_EXCEPTION("Cannot get exclusive access to \"" + Common::str(m_path) + "\"!");

        TiXmlUtils::SaveFile(*m_favorites, m_path.c_str());
        m_modified = false;

        // resume file watching
        CFileWatchClient::StartWatching(m_path.c_str());
    }
}

//TODO: make sure there is no file with the same name
void FavoritesList::AddWorkspaceQuickSave (const std::string& path)
{
    if (!m_favorites.get()) return;

    if (TiXmlElement* rootEl = m_favorites->FirstChildElement("Favorites"))
    {
        TiXmlElement* quickSavesEl = rootEl->FirstChildElement("QuickSaves");

        if (!quickSavesEl)
        {
            quickSavesEl = new TiXmlElement("QuickSaves");
            quickSavesEl->SetAttribute("hidden", 1);
            rootEl->LinkEndChild(quickSavesEl);
        }

        TiXmlElement* workspaceEl = new TiXmlElement("Workspace");
        workspaceEl->SetAttribute("path", path);

         if (TiXmlElement* firstWorkspaceEl = quickSavesEl->FirstChildElement("Workspace"))
         {
            quickSavesEl->InsertBeforeChild(firstWorkspaceEl, *workspaceEl);
            delete workspaceEl;
         }
         else
            quickSavesEl->LinkEndChild(workspaceEl);

         m_modified = true;
    }

    if (m_modified)
        Save();
}

void FavoritesList::GetWorkspaceQuickSaves (std::vector<std::string>& saves)
{
    if (!m_favorites.get()) return;

    if (const TiXmlElement* rootEl = m_favorites->FirstChildElement("Favorites"))
    {
        if (const TiXmlElement* quickSavesEl = rootEl->FirstChildElement("QuickSaves"))
        {
            if (const TiXmlElement* workspaceEl = quickSavesEl->FirstChildElement("Workspace"))
            {
                do 
                {
                    saves.push_back(workspaceEl->Attribute("path"));
                    workspaceEl = workspaceEl->NextSiblingElement("Workspace");
                }
                while (workspaceEl);
            }
        }
    }
}

void FavoritesList::RemoveWorkspaceQuickSave (const std::string& path)
{
    if (TiXmlElement* rootEl = m_favorites->FirstChildElement("Favorites"))
    {
        TiXmlElement* quickSavesEl = rootEl->FirstChildElement("QuickSaves");

        TiXmlElement* workspaceEl = quickSavesEl->FirstChildElement("Workspace");

        while (workspaceEl)
        {
            if (const char* str = workspaceEl->Attribute("path"))
            {
                if (str && str == path)
                {
                    quickSavesEl->RemoveChild(workspaceEl);
                    m_modified = true;
                    break;
                }
            }

            workspaceEl = workspaceEl->NextSiblingElement("Workspace");
        }
    }

    if (m_modified)
        Save();
}

void FavoritesList::UpdateWorkspaceQuickSaveMenu (CCmdUI* pCmdUI, UINT nMaxSize)
{
    if (pCmdUI->m_pMenu == NULL)
        return;

    vector<string> list;
    GetWorkspaceQuickSaves(list);

    if (list.empty())
    {
        // no MRU files
        if (!m_strOriginal.IsEmpty())
            pCmdUI->SetText(m_strOriginal);
        pCmdUI->Enable(FALSE);
        return;
    }

    if (m_strOriginal.IsEmpty() && pCmdUI->m_pMenu != NULL)
        pCmdUI->m_pMenu->GetMenuString(pCmdUI->m_nID, m_strOriginal, MF_BYCOMMAND);

    UINT iMRU;
    for (iMRU = 0; iMRU < nMaxSize; iMRU++)
        pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID + iMRU, MF_BYCOMMAND);

    vector<string>::const_iterator it = list.begin();
    for (iMRU = 0; iMRU < nMaxSize && it != list.end(); iMRU++, ++it)
    {
        // insert mnemonic + the file name
        TCHAR buf[10];
        int nItem = ((iMRU) % _AFX_MRU_MAX_COUNT) + 1;

        // number &1 thru &9, then 1&0, then 11 thru ...
        if (nItem > 10)
            _stprintf_s(buf, _countof(buf), _T("%d "), nItem);
        else if (nItem == 10)
            Checked::tcscpy_s(buf, _countof(buf), _T("1&0 "));
        else
            _stprintf_s(buf, _countof(buf), _T("&%d "), nItem);

        std::wstring path = Common::wstr(*it);
        LPCWSTR fname = ::PathFindFileName(path.c_str());
        pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++,
            MF_STRING | MF_BYPOSITION, pCmdUI->m_nID++,
            CString(buf) + fname);
    }

    // update end menu count
    pCmdUI->m_nIndex--; // point to last menu added
    pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();
    pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
}

void FavoritesList::OnWatchedFileChanged ()
{
    try { EXCEPTION_FRAME;

        Global::SetStatusText((L"Reloading file \"" + m_path + L"\" ...").c_str());
        CFileWatchClient::UpdateWatchInfo();
        Open(m_path);
        Global::SetStatusText((L"File \"" + m_path + L"\" reloaded.").c_str());
    }
    _OE_DEFAULT_HANDLER_
}
