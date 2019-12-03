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
#include <sstream>
#include <ModulVer/ModulVer.h>
#include "osinfo.h"

namespace Common
{
    using namespace std;

    std::string getOsInfo ()
    {
        std::ostringstream osInfo;

        OSVERSIONINFOEX osvi;

        // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
        // If that fails, try using the OSVERSIONINFO structure.

        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        BOOL bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&osvi);
        if (!bOsVersionInfoEx)
        {
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            if (!GetVersionEx((OSVERSIONINFO*)&osvi))
                return "Not identified";
        }

        switch (osvi.dwPlatformId)
        {
        // Test for the Windows NT product family.
        case VER_PLATFORM_WIN32_NT:
            // Test for the specific product family.
            switch (osvi.dwMajorVersion)
            {
            case 6:
                switch (osvi.dwMinorVersion)
                {
                case 2: osInfo << ((osvi.wProductType != VER_NT_WORKSTATION) ? "Windows Server 2012" : "Windows 8");     break;
                case 1: osInfo << ((osvi.wProductType != VER_NT_WORKSTATION) ? "Windows Server 2008 R2" : "Windows 7");  break;
                case 0: osInfo << ((osvi.wProductType != VER_NT_WORKSTATION) ? "Windows Server 2008" : "Windows Vista"); break;
                }
                break;
            case 5:
                switch (osvi.dwMinorVersion)
                {
                case 2: osInfo << "MS Windows Server 2003 family"; break;
                case 1: osInfo << "MS Windows XP"; break;
                case 0: osInfo << "MS Windows 2000"; break;
                }
                break;
            case 4:
                osInfo << "MS Windows NT"; break;
            }

            // Test for specific product on Windows NT 4.0 SP6 and later.
            if (bOsVersionInfoEx)
            {
                // Test for the workstation type.
                if (osvi.wProductType == VER_NT_WORKSTATION)
                {
                    if (osvi.dwMajorVersion == 4)
                        osInfo << " Workstation 4.0";
                    else if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
                        osInfo << " Home Edition";
                    else
                        osInfo << " Professional";
                }

                // Test for the server type.
                else if (osvi.wProductType == VER_NT_SERVER)
                {
                    if (osvi.dwMajorVersion == 5)
                    {
                        switch (osvi.dwMinorVersion)
                        {
                        case 2:
                            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                osInfo << " Datacenter";
                            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                osInfo << " Enterprise";
                            else if (osvi.wSuiteMask == VER_SUITE_BLADE)
                                osInfo << " Web";
                            else
                                osInfo << " Standard";
                            break;
                        case 0:
                            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                osInfo << " Datacenter Server";
                            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                osInfo << " Advanced Server";
                            else
                                osInfo << " Server";
                            break;
                        }
                    }
                    else if (osvi.dwMajorVersion < 5) // Windows NT 4.0
                    {
                        if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                            osInfo << " Server 4.0 Enterprise";
                        else
                            osInfo << " Server 4.0";
                    }
                }
            }
            osInfo << ',';

            osInfo << ' ' << osvi.szCSDVersion;
            osInfo << " (Build " << (osvi.dwBuildNumber & 0xFFFF) << ")";
            break;

        // Test for the Windows 95 product family.
        case VER_PLATFORM_WIN32_WINDOWS:
            if (osvi.dwMajorVersion == 4)
            {
                switch (osvi.dwMinorVersion)
                {
                case 0:
                    osInfo << "MS Windows 95 ";
                    if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B')
                        osInfo << "OSR2 ";
                    break;
                case 10:
                    osInfo << "MS Windows 98 ";
                    if (osvi.szCSDVersion[1] == 'A')
                        osInfo << "SE ";
                    break;
                case 90:
                    osInfo << "MS Windows Millennium Edition";
                    break;
                }
            }
            break;

        case VER_PLATFORM_WIN32s:

            osInfo << "MS Win32s";
            break;
        }

        string result = osInfo.str();

        return !result.empty() ? result : string("Not identified");
    }

    std::string getShellInfo ()
    {
        return "";
    }

    std::string getDllVersion (const char* dllName)
    {
        CModuleVersion ver;

    	if (ver.GetFileVersionInfo(dllName))
        {
            std::ostringstream o;

            o << HIWORD(ver.dwFileVersionMS)
              << '.' << LOWORD(ver.dwFileVersionMS)
			  << '.' << HIWORD(ver.dwFileVersionLS)
              << '.' << LOWORD(ver.dwFileVersionLS);

            return o.str();
        }

        return "Not found";
    }

    std::string getDllVersionProperty (const char* dllName, const char* propName)
    {
        CModuleVersion ver;

    	if (ver.GetFileVersionInfo(dllName))
            return (LPCSTR)ver.GetValue(propName);

        return "Not found";
    }

};//namespace Common
