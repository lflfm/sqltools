/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 2005 Aleksey Kochetov

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
#include "ConnectData.h"

    using namespace std;
    using namespace Common;

    static 
    void timet_to_filetime (time_t t, FILETIME& ft)
    {
        LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000LL;
        ft.dwLowDateTime  = (DWORD) ll;
        ft.dwHighDateTime = (DWORD) (ll >> 32);
    }

    static 
    void filetime_to_timet (FILETIME& ft,  time_t& t)
    {
        LONGLONG ll = ((LONGLONG)ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
        ll -= 116444736000000000LL;
        t = (time_t)(ll / 10000000);
    }

// read legacy
void ConnectData::ImportFromRegistry (const string& password)
{
    static const char* cszProfilesKeys[2] = { "Profiles", "ProfilesEx" };
    static const char* cszCurrProfileKey = "CurrProfile";
    static const char* cszUserKey        = "User";
    static const char* cszPasswordKey    = "Password";
    static const char* cszConnectKey     = "Connect";
    static const char* cszCounterKey     = "Counter";
    static const char* cszHostKey        = "Host";
    static const char* cszTcpPortKey     = "TcpPort";
    static const char* cszSIDKey         = "SID";  
    static const char* cszModeKey        = "Mode";  
    static const char* cszSafetyKey      = "Safety";  
    static const char* cszLastUsage      = "LastUsage";  
    static const char* cszServiceInsteadOfSidKey = "ServiceInsteadOfSid";
    static const char* cszDirectConnectionKey = "DirectConnection";  
    static const char* cszSortColumnKey       = "SortColumn";
    static const char* cszSortDirectionKey    = "SortDirection";

    std::vector<ConnectEntry> entries;

    for (int branch = 0; branch < 2; branch++)
    {
        HKEY hKey = AfxGetApp()->GetSectionKey(cszProfilesKeys[branch]);

        if (!hKey) break;

        char  buff[120];
        DWORD size = sizeof buff;
        FILETIME lastWriteTime;

        for (
            DWORD index = 0;
            RegEnumKeyEx(hKey, index, buff, &size, NULL, NULL, NULL, &lastWriteTime) == ERROR_SUCCESS;
            index++, size = sizeof buff
        )
        {
            ConnectEntry entry;
            entry.m_status = ConnectEntry::MODIFIED;

            CString subKeyName;
            subKeyName += cszProfilesKeys[branch];
            subKeyName += '\\';
            subKeyName += buff;
            HKEY hSubKey = AfxGetApp()->GetSectionKey(subKeyName);
    
            DWORD type;
            if (!branch)
            {
                entry.SetDirect(false);

                size = sizeof buff;
                if (RegQueryValueEx(hSubKey, cszConnectKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                {
                    buff[sizeof(buff)-1] = 0;
                    entry.SetAlias(buff);
                }
            }
            else
            {
                entry.SetDirect(true);

                size = sizeof buff;
                if (RegQueryValueEx(hSubKey, cszHostKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                {
                    buff[sizeof(buff)-1] = 0;
                    entry.SetHost(buff);
                }
                size = sizeof buff;
                if (RegQueryValueEx(hSubKey, cszTcpPortKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                {
                    buff[sizeof(buff)-1] = 0;
                    entry.SetPort(buff);
                }
                size = sizeof buff;
                if (RegQueryValueEx(hSubKey, cszSIDKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                {
                    buff[sizeof(buff)-1] = 0;
                    entry.SetSid(buff);
                }
                size = sizeof buff;
                if (RegQueryValueEx(hSubKey, cszServiceInsteadOfSidKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                {
                    buff[sizeof(buff)-1] = 0;
                    entry.SetUseService((DWORD*)buff ? true : false);
                }
            }
            size = sizeof buff;
            if (RegQueryValueEx(hSubKey, cszUserKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
            {
                buff[sizeof(buff)-1] = 0;
                entry.SetUser(buff);
            }

            if (m_SavePasswords)
            {
                size = sizeof buff;
                if (RegQueryValueEx(hSubKey, cszPasswordKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
                {
                    buff[sizeof(buff)-1] = 0;
                    entry.SetDecryptedPassword(buff);
                }
            }
            else
                entry.SetDecryptedPassword("");
    
            size = sizeof buff;
            if (RegQueryValueEx(hSubKey, cszCounterKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
            {
                itoa(*(DWORD*)buff, buff, 10);
                entry.SetUsageCounter(atoi(buff));
            }

            size = sizeof buff;
            if (RegQueryValueEx(hSubKey, cszModeKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
            {
                buff[sizeof(buff)-1] = 0;
                entry.SetMode(*(DWORD*)buff);
            }

            size = sizeof buff;
            if (RegQueryValueEx(hSubKey, cszSafetyKey, NULL, &type, (LPBYTE)buff, &size) == ERROR_SUCCESS)
            {
                buff[sizeof(buff)-1] = 0;
                entry.SetSafety(*(DWORD*)buff);
            }
            time_t t;
            filetime_to_timet(lastWriteTime, t);
            entry.SetLastUsage(t);

            RegCloseKey(hSubKey);

            entries.push_back(entry);
        }

        RegCloseKey(hKey);
    }

    swap(m_entries, entries);

    EncryptAll(password);
}
