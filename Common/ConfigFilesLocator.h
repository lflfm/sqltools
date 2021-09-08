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

#pragma once

// TODO: use FileLock instead of CMutex for locking any config file
namespace Common
{

// all members are static - only one global configurator
class ConfigFilesLocator
{
public:
    // trys to locate a key config file and recognize a base folder
    // it can be either %APPDATA%\<privateFolder> for personal settings
    // or <program executable folder>\<sharedFolder> in the case of shared settings.
    // prompts to choose either personal or shared config to use if a key file does not exist 
    // it returns true if the call created a new configuration 
    static bool Initialize (LPCTSTR privateFolder, LPCTSTR sharedFolder, LPCTSTR keyFile);

    static void ImportFromOldConfiguration (LPCTSTR oldPrivateFolder, LPCTSTR oldSharedFolder, LPCTSTR keyFile, LPCTSTR files);

    // adds unconditional file location 
    static void RegisterExplicitly (LPCTSTR file, LPCTSTR path);

    // returns false if a file does not exist
    static bool GetFilePath (LPCTSTR file, std::wstring& path);

    // returns base folder BUT it might be not initialized yet!
    static const std::wstring& GetBaseFolder () { return m_baseFolder; };

private:
    static std::wstring m_baseFolder;
    static std::map<std::wstring, std::wstring> m_filenameMap;
};

};//namespace Common