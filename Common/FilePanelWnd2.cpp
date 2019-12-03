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
#include <Shlwapi.h>
#include <string>
#include <fstream>
#include <sstream>
#include <COMMON/ExceptionHelper.h>
#include <WinException/WinException.h>
#include <COMMON/FilePanelWnd.h>
#include <COMMON/WorkbookMDIFrame.h>
#include "COMMON/GUICommandDictionary.h"
#include "COMMON/AppUtilities.h"
#include "COMMON\StrHelpers.h"
#include "ShellContextMenu.h"

using namespace std;

    //SHFileOperation

    // taken from GNU grep
    typedef enum {
        UNKNOWN, DOS_BINARY, DOS_TEXT, UNIX_TEXT
    } File_type;

    static 
    File_type guess_type (char *buf, register size_t buflen)
    {
        int crlf_seen = 0;
        register char *bp = buf;

        while (buflen--)
        {
            /* Treat a file as binary if it has a NUL character.  */
            if (!*bp)
                return DOS_BINARY;

            /* CR before LF means DOS text file (unless we later see
                binary characters).  */
            else if (*bp == '\r' && buflen && bp[1] == '\n')
                crlf_seen = 1;

            bp++;
        }

        return crlf_seen ? DOS_TEXT : UNIX_TEXT;
    }

    //string encoding;
    //if (detect_encoding(path, encoding))
    //{
    //    if (!tooltipText.empty()) tooltipText += "\r\nEncoding:\t\t";
    //    tooltipText += encoding;
    //}
    static 
    bool detect_encoding (const char* path, std::string& encoding)
    {
		char buffer[4096]; 
        ifstream file(path, ios::binary);
        file.seekg(0, ios::end);
        int length = min<int>(sizeof(buffer), (int)file.tellg());
        file.seekg(0, ios::beg);
        file.read(buffer, length);

        File_type file_type = guess_type(buffer, length);

        if (file_type == DOS_TEXT || file_type == UNIX_TEXT) 
        {
            encoding = "ASCII";
		    return true;
        }

		return false;
    }

    static 
    bool make_preview (const char* path, std::string& preview, const int nlines)
    {
        const int PREVIEW_MAX_LINE_LENGTH = 100;

		char buffer[4096]; 
        ifstream file(path, ios::binary);
        file.seekg(0, ios::end);
        int length = min<int>(sizeof(buffer), (int)file.tellg());
        file.seekg(0, ios::beg);
        file.read(buffer, length);

        if (!length) return false;

        File_type file_type = guess_type(buffer, length);

        if (file_type == DOS_TEXT || file_type == UNIX_TEXT)
        {
            preview.clear();

            string line;
            istringstream text(string(buffer, length));

            int i = 0;
            for (; i < nlines && getline(text, line); ++i)
            {
                if (!preview.empty()) preview += '\n';
                if (line.length() > PREVIEW_MAX_LINE_LENGTH) 
                {
                    line.resize(PREVIEW_MAX_LINE_LENGTH-3);
                    line += "...";
                }
                //preview += line;
                for (string::const_iterator it = line.begin(); it != line.end(); ++it)
                    switch (*it)
                    {
                    case '>': preview += "&gt;"; break;
                    case '<': preview += "&lt;"; break;
                    case '&': preview += "&amp;"; break;
                    default:  preview += *it;
                    }
            }
            if (i == nlines && !preview.empty()) 
                preview += "\n...";

		    return true;
        }

		return false;
    }


//void CFilePanelWnd::DisplayFileInfoTooltip (NMHDR * pNMHDR, LPCSTR path)
//{
//}
//
void CFilePanelWnd::OnExplorerTree_NotifyDisplayTooltip (NMHDR * pNMHDR)
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

            if (::PathFileExists(path))
            {
                CString fileSize, creationTime, lastModifiedTime;

                if (!Common::AppIsFolder(path, true/*nothrow*/))
                {
                    BY_HANDLE_FILE_INFORMATION fileInformation;
                    if (Common::AppGetFileAttrs(path, &fileInformation))
                    {
                        if (!fileInformation.nFileSizeHigh)
                            Common::format_number(fileSize, fileInformation.nFileSizeLow, " bytes", false);

                        Common::filetime_to_string(fileInformation.ftCreationTime,  creationTime);
                        Common::filetime_to_string(fileInformation.ftLastWriteTime, lastModifiedTime);
                    }
                }

                std::string tooltipText;
    		    
                CRect rect, rcCtrl;
                m_explorerTree.GetClientRect(&rcCtrl);
			    m_explorerTree.GetItemRect(ti.hItem, rect, TRUE);
                if (rect.left < 0 || rect.right > rcCtrl.Width()
                || rect.top < 0 || rect.bottom > rcCtrl.Height())
                {
                    tooltipText = "<b>";
                    tooltipText += m_explorerTree.GetItemText(ti.hItem); 
                    tooltipText += "</b>";
                }

                if (!Common::AppIsFolder(path, true/*nothrow*/))
                {
                    if (!creationTime.IsEmpty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Date created:\t";
                        tooltipText +=  creationTime;
                    }
                    if (!lastModifiedTime.IsEmpty()
                    && lastModifiedTime != creationTime)
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Date modified:\t";
                        tooltipText +=  lastModifiedTime;
                    }
                    if (!fileSize.IsEmpty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "File Size:\t\t";
                        tooltipText +=  fileSize;
                    }
                    if (m_PreviewInTooltips)
                    {
                        string preview;
                        if (make_preview(path, preview, m_PreviewLines))
                        {
                            if (!tooltipText.empty()) tooltipText += "\r\n<hr size=\"1\"/>\r\n";
                            tooltipText += preview;
                        }
                    }
                }
                else
                {
                    string folders, files;
                    int nfolders = 0, nfiles = 0;
                    ULONGLONG directorySize = 0;
                    bool tooManyFolders = false, tooManyFiles = false;

	                CFileFind find;
	                CString   strTemp = path;

	                if ( strTemp[strTemp.GetLength()-1] == '\\' )
		                strTemp += "*.*";
	                else
		                strTemp += "\\*.*";
                		
	                BOOL bFind = find.FindFile( strTemp );
                	
	                while ( bFind )
	                {
		                bFind = find.FindNextFile();

		                if ( find.IsDirectory() && !find.IsDots() )
		                {
                            if (folders.length() < 60)
                            {
                                if (!folders.empty()) folders += ", ";
                                folders += find.GetFileName();
                            }
                            else
                                tooManyFolders = true;
                            ++nfolders;

		                }
		                if ( !find.IsDirectory() && !find.IsHidden() )
                        {
                            if (files.length() < 60)
                            {
                                if (!files.empty()) files += ", ";
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
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "The folder is empty";
                    }

                    if (!folders.empty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Folders: ";
                        tooltipText +=  folders;
                        if (tooManyFolders) tooltipText +=  ", ...";
                    }
                    if (!files.empty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Files: ";
                        tooltipText +=  files;
                        if (tooManyFiles) tooltipText +=  ", ...";
                    }
                    if (directorySize != 0)
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Size:\t";
                        if (!folders.empty()) tooltipText += ">= ";
                        
                        CString fileSize;
                        Common::format_number(fileSize, (unsigned int)directorySize, " bytes", false);
                        tooltipText +=  fileSize;
                    }
                }

                pNotify->ti->sTooltip = tooltipText.c_str();
            }
            else
                pNotify->ti->sTooltip = "<b>No access or object does not exist anymore.<b>";

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

void CFilePanelWnd::OnRecentFilesListCtrl_NotifyDisplayTooltip (NMHDR * pNMHDR)
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

            if (::PathFileExists(path))
            {
                CString fileSize, creationTime, lastModifiedTime;

                if (!Common::AppIsFolder(path, true/*nothrow*/))
                {
                    BY_HANDLE_FILE_INFORMATION fileInformation;
                    if (Common::AppGetFileAttrs(path, &fileInformation))
                    {
                        if (!fileInformation.nFileSizeHigh)
                            Common::format_number(fileSize, fileInformation.nFileSizeLow, " bytes", false);

                        Common::filetime_to_string(fileInformation.ftCreationTime,  creationTime);
                        Common::filetime_to_string(fileInformation.ftLastWriteTime, lastModifiedTime);
                    }
                }

                std::string tooltipText;
    		    
                CRect rect, rcCtrl;
                m_recentFilesListCtrl.GetClientRect(&rcCtrl);
                m_recentFilesListCtrl.GetItemRect(ti.iItem, rect, LVIR_LABEL);
                if (rect.left < 0 || rect.right > rcCtrl.Width()
                || rect.top < 0 || rect.bottom > rcCtrl.Height())
                {
                    tooltipText = "<b>";
                    tooltipText += m_recentFilesListCtrl.GetItemText(ti.iItem, 0); 
                    tooltipText += "</b>";
                }

                if (!Common::AppIsFolder(path, true/*nothrow*/))
                {
                    if (!creationTime.IsEmpty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Date created:\t";
                        tooltipText +=  creationTime;
                    }
                    if (!lastModifiedTime.IsEmpty()
                    && lastModifiedTime != creationTime)
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Date modified:\t";
                        tooltipText +=  lastModifiedTime;
                    }
                    if (!fileSize.IsEmpty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "File Size:\t\t";
                        tooltipText +=  fileSize;
                    }
                    if (m_PreviewInTooltips)
                    {
                        string preview;
                        if (make_preview(path, preview, m_PreviewLines))
                        {
                            if (!tooltipText.empty()) tooltipText += "\r\n<hr size=\"1\"/>\r\n";
                            tooltipText += preview;
                        }
                    }
                }
                else
                {
                    string folders, files;
                    int nfolders = 0, nfiles = 0;
                    ULONGLONG directorySize = 0;
                    bool tooManyFolders = false, tooManyFiles = false;

	                CFileFind find;
	                CString   strTemp = path;

	                if ( strTemp[strTemp.GetLength()-1] == '\\' )
		                strTemp += "*.*";
	                else
		                strTemp += "\\*.*";
                		
	                BOOL bFind = find.FindFile( strTemp );
                	
	                while ( bFind )
	                {
		                bFind = find.FindNextFile();

		                if ( find.IsDirectory() && !find.IsDots() )
		                {
                            if (folders.length() < 60)
                            {
                                if (!folders.empty()) folders += ", ";
                                folders += find.GetFileName();
                            }
                            else
                                tooManyFolders = true;
                            ++nfolders;

		                }
		                if ( !find.IsDirectory() && !find.IsHidden() )
                        {
                            if (files.length() < 60)
                            {
                                if (!files.empty()) files += ", ";
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
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "The folder is empty";
                    }

                    if (!folders.empty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Folders: ";
                        tooltipText +=  folders;
                        if (tooManyFolders) tooltipText +=  ", ...";
                    }
                    if (!files.empty())
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Files: ";
                        tooltipText +=  files;
                        if (tooManyFiles) tooltipText +=  ", ...";
                    }
                    if (directorySize != 0)
                    {
                        if (!tooltipText.empty()) tooltipText += "\r\n";
                        tooltipText += "Size:\t";
                        if (!folders.empty()) tooltipText += ">= ";
                        
                        CString fileSize;
                        Common::format_number(fileSize, (unsigned int)directorySize, " bytes", false);
                        tooltipText +=  fileSize;
                    }
                }

                pNotify->ti->sTooltip = tooltipText.c_str();
            }
            else
                pNotify->ti->sTooltip = "<b>No access or object does not exist anymore.<b>";

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
