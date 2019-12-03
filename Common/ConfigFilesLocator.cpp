/*
    Copyright (C) 2005 Aleksey Kochetov
    This file is a part of OpenEditor & SQLTools projects

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

#include <stdafx.h>
#include <shlwapi.h>
#include <Common/ConfigFilesLocator.h>
#include <Common/AppUtilities.h>
#include <Common/StrHelpers.h>

using namespace std;

namespace Common
{
    std::string                        ConfigFilesLocator::m_baseFolder;
    std::map<std::string, std::string> ConfigFilesLocator::m_filenameMap;

void ConfigFilesLocator::Initialize (const char* privateSubfolder, const char* sharedSubfolder, const char* keyFile)
{
    char szBuffer[2 * MAX_PATH];
    string baseFolder, privateFolder, sharedFolder;
    
    if (::SHGetSpecialFolderPath(AfxGetMainWnd() ? AfxGetMainWnd()->m_hWnd : NULL, szBuffer, CSIDL_APPDATA, TRUE))
    {
        ::PathAppend(szBuffer, privateSubfolder);
        privateFolder = szBuffer;
        ::PathAppend(szBuffer, keyFile);
        if (::PathFileExists(szBuffer))
            baseFolder = privateFolder;
    }
    else
        szBuffer[0] = 0;

    if (baseFolder.empty())
    {
        szBuffer[0] = 0;
        string path;
        AppGetPath(path);
        ::PathAppend(szBuffer, path.c_str());
        ::PathAppend(szBuffer, sharedSubfolder);
        sharedFolder = szBuffer;
        ::PathAppend(szBuffer, keyFile);
        if (::PathFileExists(szBuffer))
            baseFolder = sharedFolder;
    }

    if (baseFolder.empty())
    {
        if (!privateFolder.empty())
        {
            if (::MessageBox(NULL,
                    "The program configuration files are not found."
                    "\nPlease choose one of the following options: " 
                    "\n      press <Yes> for user private configuration (recommended),  "
                    "\n      press <No> for shared configuration.", 
                    "Setup",
                    MB_ICONEXCLAMATION|MB_YESNO
                    ) == IDYES
                )
                baseFolder = privateFolder;
            else
                baseFolder = sharedFolder;
        }
        else // "...\Application Data" does not exist for some reason, only global configuration is possible 
        {
            baseFolder = privateFolder;
        }
        AppCreateFolderHierarchy(baseFolder.c_str());
    }

    m_baseFolder = baseFolder;
}

void ConfigFilesLocator::RegisterExplicitly (const char* file, const char* path)
{
    string key;
    to_upper_str(file, key);
    m_filenameMap[key] = path;
}

bool ConfigFilesLocator::GetFilePath (const char* file, string& path) 
{
    string key;
    to_upper_str(file, key);

    map<string, string>::const_iterator it = m_filenameMap.find(key);
    
    if (it != m_filenameMap.end())
        path = it->second;
    else
    {
        if (m_baseFolder.empty())
            THROW_APP_EXCEPTION("ConfigFilesLocator is not initialized.");

        char szBuffer[2 * MAX_PATH];
        szBuffer[0] = 0;
        ::PathAppend(szBuffer, m_baseFolder.c_str());
        ::PathAppend(szBuffer, file);
        path = szBuffer;
    }

    return ::PathFileExists(path.c_str()) ? true : false;
}

};//namespace Common