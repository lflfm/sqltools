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
#include <commctrl.h>
#include <afxtaskdialog.h>
#include <shlwapi.h>
#include <Common/ConfigFilesLocator.h>
#include <Common/AppUtilities.h>
#include <Common/StrHelpers.h>

using namespace std;

namespace Common
{
    std::wstring                        ConfigFilesLocator::m_baseFolder;
    std::map<std::wstring, std::wstring> ConfigFilesLocator::m_filenameMap;

    static void identify_config_folder (
        LPCTSTR privateSubfolder, LPCTSTR sharedSubfolder, LPCTSTR keyFile, 
        wstring& baseFolder, wstring& privateFolder, wstring& sharedFolder
    )
    {
        TCHAR szBuffer[2 * MAX_PATH];
    
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
            CString path;
            AppGetPath(path);
            ::PathAppend(szBuffer, path);
            ::PathAppend(szBuffer, sharedSubfolder);
            sharedFolder = szBuffer;
            ::PathAppend(szBuffer, keyFile);
            if (::PathFileExists(szBuffer))
                baseFolder = sharedFolder;
        }
    }

bool ConfigFilesLocator::Initialize (LPCTSTR privateSubfolder, LPCTSTR sharedSubfolder, LPCTSTR keyFile)
{
    bool configFound = false;
    wstring baseFolder, privateFolder, sharedFolder;

    identify_config_folder (privateSubfolder, sharedSubfolder, keyFile, baseFolder, privateFolder, sharedFolder);
    
    if (baseFolder.empty())
    {
        if (!privateFolder.empty())
        {
            /*
            std::wstring message = L"The program configuration files were not found.";
            message +=  L"\n";
            message +=  L"\nPlease choose where to create the configuration files: ";
            message +=  L"\n";
            message +=  L"\n      press <Yes> to use the user profile folder (recommended)  ";
            message +=  L"\n            \"" + privateFolder + L"\", ";
            message +=  L"\n";
            message +=  L"\n      press <No> to use SQLTools program folder ";
            message +=  L"\n            \"" + sharedFolder + L"\".";

            switch (::MessageBox(NULL, message.c_str(), L"Setup", MB_ICONEXCLAMATION|MB_YESNOCANCEL))
            {
            case IDYES:
                baseFolder = privateFolder;
                break;
            case IDNO:
                baseFolder = sharedFolder;
                break;
            default:
                THROW_APP_EXCEPTION("User canceled operation!");
                //AfxThrowUserException();
            }
            */

            CTaskDialog taskDialog(
                L"",
                L"The program configuration files were not found.    "
                L"\n\nPlease choose where to create the configuration files: "
                , CString(AfxGetApp()->m_pszExeName) + L" Initial Setup", 
                TDCBF_CANCEL_BUTTON 
            );

            taskDialog.AddCommandControl(201, CString(L"User profile folder (recommended)") + L"\n\"" + privateFolder.c_str() + L"\"");
            taskDialog.AddCommandControl(202, CString(L"Program folder") + L"\n\"" + sharedFolder.c_str() + L"\"");

            switch (taskDialog.DoModal())
            {
            case 201:
                baseFolder = privateFolder;
                break;
            case 202:
                baseFolder = sharedFolder;
                break;
            default:
                THROW_APP_EXCEPTION("User canceled operation!");
            }
        }
        else // "...\Application Data" does not exist for some reason, only global configuration is possible 
        {
            baseFolder = privateFolder;
        }
        AppCreateFolderHierarchy(baseFolder.c_str());
    }
    else
        configFound = true;

    m_baseFolder = baseFolder;

    return !configFound; // true if new config created
}

void ConfigFilesLocator::ImportFromOldConfiguration (LPCTSTR oldPrivateSubfolder, LPCTSTR oldSharedSubfolder, LPCTSTR keyFile, LPCTSTR files)
{
    wstring oldBaseFolder, unused1, unused2;
    identify_config_folder(oldPrivateSubfolder, oldSharedSubfolder, keyFile, oldBaseFolder, unused1, unused2);

    if (!oldBaseFolder.empty())
    {
        if (::MessageBox(NULL, L"Would you like to import the settings from the previous version?", L"Setup", MB_ICONQUESTION|MB_YESNO) == IDYES)
        {
            int pos = 0;
            CString fileList(files);
            CString fileName = fileList.Tokenize(_T(","), pos);

            while (!fileName.IsEmpty())
            {
                CString oldPath, newPath;
                ::PathCombine(oldPath.GetBuffer(MAX_PATH), oldBaseFolder.c_str(), fileName);
                ::PathCombine(newPath.GetBuffer(MAX_PATH), m_baseFolder.c_str(), fileName);

                if (PathFileExists(oldPath))
                {
                    ifstream in(oldPath);
                    ofstream out(newPath);
                    string buffer;

                    while (std::getline(in, buffer))
                    {
                        int length = ::MultiByteToWideChar(CP_ACP, 0, buffer.c_str(), buffer.length(), 0, 0);
                        CString wbuffer;
                        ::MultiByteToWideChar(CP_ACP, 0, buffer.c_str(), buffer.length(), wbuffer.GetBuffer(length+1), length);
                        wbuffer.ReleaseBuffer(length);
                        out << Common::str(wbuffer) << endl;
                    }
                }

                fileName = fileList.Tokenize(_T(","), pos);
            }
        }
    }
}

void ConfigFilesLocator::RegisterExplicitly (LPCTSTR file, LPCTSTR path)
{
    wstring key;
    to_upper_str(file, key);
    m_filenameMap[key] = path;
}

bool ConfigFilesLocator::GetFilePath (LPCTSTR file, wstring& path) 
{
    wstring key;
    to_upper_str(file, key);

    auto it = m_filenameMap.find(key);
    
    if (it != m_filenameMap.end())
        path = it->second;
    else
    {
        if (m_baseFolder.empty())
            THROW_APP_EXCEPTION("ConfigFilesLocator is not initialized.");

        TCHAR szBuffer[2 * MAX_PATH];
        szBuffer[0] = 0;
        ::PathAppend(szBuffer, m_baseFolder.c_str());
        ::PathAppend(szBuffer, file);
        path = szBuffer;
    }

    return ::PathFileExists(path.c_str()) ? true : false;
}

};//namespace Common