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
*/

#include "stdafx.h"
#include <string.h> 
#include <Shlwapi.h>
#include <string>
#include <fstream>
#include <sstream>

#include "ExceptionHelper.h"
#include <WinException/WinException.h>
#include "FileExplorerWnd.h"
#include "RecentFileWnd.h"
#include "WorkbookMDIFrame.h"
#include "GUICommandDictionary.h"
#include "AppUtilities.h"
#include "StrHelpers.h"
#include "ShellContextMenu.h"
#include "MyUtf.h"

using namespace std;

#include "TextEncodingDetect/text_encoding_detect.h"
using namespace AutoIt::Common;

    static 
    bool make_preview (LPCWSTR path, CString& preview, CString& encodingStr, const int nlines)
    {
        const size_t PREVIEW_MAX_LINE_LENGTH = 100;

        char buffer[4096]; 
        ifstream file(path, ios::binary);
        file.seekg(0, ios::end);
        size_t length = min<size_t>(sizeof(buffer), (size_t)file.tellg());
        file.seekg(0, ios::beg);
        file.read(buffer, length);

        if (length <= 0) return false;

        TextEncodingDetect textDetect;
        TextEncodingDetect::Encoding encoding = textDetect.DetectEncoding((unsigned char*)buffer, length);

        int codepage = -1;
        switch (encoding)
        {
            case TextEncodingDetect::Encoding::ANSI:       // 0-255
                encodingStr = "ANSI 8-bit";
                codepage = CP_ACP;
                break;
            case TextEncodingDetect::Encoding::ASCII:      // 0-127
                encodingStr = "ASCII 7-bit";
                codepage = CP_ACP;
                break;
            case TextEncodingDetect::Encoding::UTF8_BOM:   // UTF8 with BOM
                encodingStr = "UTF-8 with BOM";
                codepage = CP_UTF8;
                break;
            case TextEncodingDetect::Encoding::UTF8_NOBOM: // UTF8 without BOM
                encodingStr = "UTF-8 w/o BOM";
                codepage = CP_UTF8;
                break;
            case TextEncodingDetect::Encoding::UTF16_LE_BOM:// UTF16 LE with BOM
                encodingStr = "UTF-16 LE with BOM";
                codepage = 1203;
                break;
            case TextEncodingDetect::Encoding::UTF16_LE_NOBOM:// UTF16 LE without BOM
                encodingStr = "UTF-16 LE w/o BOM";
                codepage = 1203;
                break;
        };

        if  (codepage != -1)
        {
            preview.Empty();

            int wlen = 0;
            wchar_t wbuff[sizeof(buffer)];

            if (codepage != 1203)
            {
                int  boom_offset = (encoding == TextEncodingDetect::Encoding::UTF8_BOM) ? 3 : 0;

                memset(wbuff, 0, sizeof(wbuff)/sizeof(wbuff[0]));
                wlen = MultiByteToWideChar(
                    codepage, 0, 
                    buffer + boom_offset, length - boom_offset - 1, wbuff, sizeof(wbuff)/sizeof(wbuff[0])-1
                );
            } 
            else
            {
                int  boom_offset = (encoding == TextEncodingDetect::Encoding::UTF16_LE_BOM) ? 1 : 0; // in wchar_t
                wlen = min(length/sizeof(wbuff[0]) - boom_offset, sizeof(wbuff)/sizeof(wbuff[0])-1);
                wcsncpy(wbuff, (wchar_t*)buffer + boom_offset, wlen);
            }

            if (wlen > 0)
            {
            
                int i = 0;
                int position = 0;
                std::wstring line;
                bool cr;

                for (; i < nlines && Common::get_line(wbuff, wlen, position, line, cr); ++i)
                {
                    if (!preview.IsEmpty()) preview += '\n';

                    if (line.length() > 0 && *line.rbegin() == '\r')
                        line.resize(line.length()-1);

                    int displayLength = 0;
                    auto it = line.begin();
                    for (; it != line.end() && displayLength < PREVIEW_MAX_LINE_LENGTH; ++it)
                    {
                        switch (*it)
                        {
                        case '>': preview += L"&gt;"; break;
                        case '<': preview += L"&lt;"; break;
                        case '&': preview += L"&amp;"; break;
                        default:  preview += *it;
                        }
                        if (!Common::is_combining_implementation(*it))
                            ++displayLength;
                    }

                    if (it != line.end())
                        preview += L"...";
                }
                if (i == nlines && !preview.IsEmpty()) 
                    preview += L"\n...";

                return true;
            }
        }

        return false;
    }

    BOOL  make_tooltip_text (const CString& path, CString& tooltipText, BOOL previewInTooltips, int previewLines)
    {
        if (::PathFileExists(path))
        {
            CString fileSize, creationTime, lastModifiedTime;

            if (!Common::AppIsFolder(path, true/*nothrow*/))
            {
                BY_HANDLE_FILE_INFORMATION fileInformation;
                if (Common::AppGetFileAttrs(path, &fileInformation))
                {
                    if (!fileInformation.nFileSizeHigh)
                        Common::format_number(fileSize, fileInformation.nFileSizeLow, L" bytes", false);

                    Common::filetime_to_string(fileInformation.ftCreationTime,  creationTime);
                    Common::filetime_to_string(fileInformation.ftLastWriteTime, lastModifiedTime);
                }
            }

            tooltipText = L"<b>";
            tooltipText += PathFindFileName(path); 
            tooltipText += L"</b>";

            if (!Common::AppIsFolder(path, true/*nothrow*/))
            {
                if (!creationTime.IsEmpty())
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"Date created:\t";
                    tooltipText +=  creationTime;
                }
                if (!lastModifiedTime.IsEmpty()
                && lastModifiedTime != creationTime)
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"Date modified:\t";
                    tooltipText +=  lastModifiedTime;
                }
                if (!fileSize.IsEmpty())
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"File Size:\t\t";
                    tooltipText +=  fileSize;
                }
                if (previewInTooltips)
                {
                    CString preview, encoding;
                    if (make_preview(path, preview, encoding, previewLines))
                    {
                        if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                        tooltipText += L"Encoding:\t\t";
                        tooltipText += encoding;
                        if (!tooltipText.IsEmpty()) tooltipText += L"\r\n<hr size=\"1\"/>\r\n";
                        tooltipText += preview;
                    }
                }
            }
            else
            {
                CString folders, files;
                int nfolders = 0, nfiles = 0;
                ULONGLONG directorySize = 0;
                bool tooManyFolders = false, tooManyFiles = false;

                CFileFind find;
                CString   strTemp = path;

                if ( strTemp[strTemp.GetLength()-1] == '\\' )
                    strTemp += L"*.*";
                else
                    strTemp += L"\\*.*";
                        
                BOOL bFind = find.FindFile( strTemp );
                    
                while ( bFind )
                {
                    bFind = find.FindNextFile();

                    if ( find.IsDirectory() && !find.IsDots() )
                    {
                        if (folders.GetLength() < 60)
                        {
                            if (!folders.IsEmpty()) folders += L", ";
                            folders += find.GetFileName();
                        }
                        else
                            tooManyFolders = true;
                        ++nfolders;

                    }
                    if ( !find.IsDirectory() && !find.IsHidden() )
                    {
                        if (files.GetLength() < 60)
                        {
                            if (!files.IsEmpty()) files += L", ";
                            files += find.GetFileName();
                        }
                        else
                            tooManyFiles = true;
                        directorySize += find.GetLength();
                        ++nfiles;
                    }
                }
                if (!nfolders && !nfiles)
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"The folder is empty";
                }

                if (!folders.IsEmpty())
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"Folders: ";
                    tooltipText +=  folders;
                    if (tooManyFolders) tooltipText +=  L", ...";
                }
                if (!files.IsEmpty())
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"Files: ";
                    tooltipText +=  files;
                    if (tooManyFiles) tooltipText +=  L", ...";
                }
                if (directorySize != 0)
                {
                    if (!tooltipText.IsEmpty()) tooltipText += L"\r\n";
                    tooltipText += L"Size:\t";
                    if (!folders.IsEmpty()) tooltipText += L">= ";
                        
                    CString dirSizeStr;
                    Common::format_number(dirSizeStr, (unsigned int)directorySize, L" bytes", false);
                    tooltipText +=  dirSizeStr;
                }
            }

            return TRUE;
        }
        return FALSE;
    }


void FileExplorerWnd::NotifyDisplayTooltip (NMHDR * pNMHDR)
{
    CWaitCursor wait;
    NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

    if (m_explorerTree.m_hWnd == pNotify->hwndTool)
    {
        TVHITTESTINFO ti;
        ti.pt = *pNotify->pt;
        m_explorerTree.ScreenToClient(&ti.pt);

        m_explorerTree.HitTest(&ti);
        UINT nFlags = ti.flags;

        if (nFlags & TVHT_ONITEM)
        {
            CString path = m_explorerTree.GetFullPath(ti.hItem);
            if (!make_tooltip_text(path, pNotify->ti->sTooltip, m_PreviewInTooltips, m_PreviewLines))
                pNotify->ti->sTooltip = L"<b>No access or object does not exist anymore.<b>";

            //*pNotify->pt = CPoint(rcCtrl.left, rcCtrl.bottom);
            CRect rect, rcCtrl;
            m_explorerTree.GetItemRect(ti.hItem, rect, TRUE);
            m_explorerTree.GetWindowRect(&rcCtrl);
            CPoint pt = rcCtrl.TopLeft();
            rect.OffsetRect(pt);
            rect.OffsetRect(1, 1);
            pt = rect.TopLeft();
            pt.y = rect.bottom;
            *pNotify->pt = pt;
        }
    }
}

void RecentFileWnd::NotifyDisplayTooltip (NMHDR * pNMHDR)
{
    CWaitCursor wait;
    NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

    if (m_recentFilesListCtrl.m_hWnd == pNotify->hwndTool)
    {
        LVHITTESTINFO ti;
        ti.pt = *pNotify->pt;
        m_recentFilesListCtrl.ScreenToClient(&ti.pt);

        m_recentFilesListCtrl.HitTest(&ti);
        UINT nFlags = ti.flags;

        if (nFlags & TVHT_ONITEM)
        {
            CString path = m_recentFilesListCtrl.GetPath(m_recentFilesListCtrl.GetItemData(ti.iItem)).c_str();
            if (!make_tooltip_text(path, pNotify->ti->sTooltip, m_PreviewInTooltips, m_PreviewLines))
                pNotify->ti->sTooltip = L"<b>No access or object does not exist anymore.<b>";

            //*pNotify->pt = CPoint(rcCtrl.left, rcCtrl.bottom);
            CRect rect, rcCtrl;
            m_recentFilesListCtrl.GetItemRect(ti.iItem, rect, LVIR_LABEL);
            m_recentFilesListCtrl.GetWindowRect(&rcCtrl);
            CPoint pt = rcCtrl.TopLeft();
            rect.OffsetRect(pt);
            rect.OffsetRect(1, 1);
            pt = rect.TopLeft();
            pt.y = rect.bottom;
            *pNotify->pt = pt;
        }
    }
}
