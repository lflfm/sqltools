/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

#ifndef _EXTRACTSCHEMAHELPER_H_
#define _EXTRACTSCHEMAHELPER_H_
#pragma once

#include "MetaDict\TextOutput.h"
#include "MetaDict\MetaDictionary.h"
#include "ExtractDDLSettings.h"

namespace ExtractSchemaHelpers
{

/// FileList is a produced files list ///////////////////////////////////////
    
class FileList 
{
public:
    enum EStage { esFist = 0, esSecond = 1 };

    FileList (const wchar_t*);
    ~FileList ();

    void Cleanup ();
    void Push (const std::wstring&, EStage);

private:
    void write ();
    
    FILE* m_pFile;
    std::list<std::wstring> m_lists[2];
};

    
/// FastFileOpen is a last opened file cash /////////////////////////////////

class FastFileOpen
{
public:
    FastFileOpen ();
    ~FastFileOpen ();

    bool  IsOpen () const;
    bool  Idem (const std::wstring& name);
    FILE* Open (const std::wstring& name, const wchar_t* mode);
    void  Close ();
    void  Cleanup ();
    FILE* GetStream () const;

private:
    FILE* m_pFile;
    std::wstring m_strFileName;
};

    inline
    FastFileOpen::FastFileOpen () { m_pFile = 0; }

    inline
    FastFileOpen::~FastFileOpen () { 
        try { EXCEPTION_FRAME;

            Close(); 
        }
        _DESTRUCTOR_HANDLER_;
    }

    inline
    bool FastFileOpen::IsOpen () const { return m_pFile ? true : false; }

    inline
    bool FastFileOpen::Idem (const std::wstring& name) { return (m_strFileName == name); }

    inline
    FILE* FastFileOpen::GetStream () const { return m_pFile; }

/// WriteContext is a common data for file writing iterations ///////////////

class WriteContext
{
public:
    static const std::wstring strNull;

    WriteContext (const std::wstring& strPath, 
                  const std::wstring& listFile, const ExtractDDLSettings&);

    void Cleanup ();

    const ExtractDDLSettings& GetDDLSettings() const { return m_DDLSettings; }
    
    void  SetSubDir (const std::wstring& = strNull);
    void  SetSuffix (const std::wstring& = strNull);
    void  SetDefExt (const std::wstring& = strNull);

     // don't close a file after use
    FILE* OpenFile (const std::wstring&, FileList::EStage = FileList::esFist, const wchar_t* mode = L"at");

    // Parameters override OpenFile parameters before EndSingleStream call.
    void BeginSingleStream (const std::wstring&, FileList::EStage = FileList::esFist, const wchar_t* mode = L"at");
    void EndSingleStream ();

    std::ostream& ErrorLog()            { return m_errorLog; }
    void ErrorLog (std::string& buffer) { buffer = m_errorLog.str(); }

private:
    const ExtractDDLSettings& m_DDLSettings;
    bool         m_bSingleStream;
    FileList     m_fileList;
    FastFileOpen m_fastFileOpen;
    std::wstring  m_strPath, m_strSubDir, 
                 m_strSuffix, m_strDefExt;

    std::ostringstream m_errorLog;
};

    inline
    WriteContext::WriteContext (const std::wstring& strPath, const std::wstring& listFile, const ExtractDDLSettings& settings)
    : m_strPath(strPath),
      m_bSingleStream(false),
      m_DDLSettings(settings),
      m_fileList((strPath + L"\\" + listFile).c_str()) {}

    inline void WriteContext::SetSubDir (const std::wstring& subDir) { m_strSubDir = subDir; }
    inline void WriteContext::SetDefExt (const std::wstring& defExt) { m_strDefExt = defExt; }
    inline void WriteContext::SetSuffix (const std::wstring& suffix) { m_strSuffix = suffix; }


/// dictionary iteration function ///////////////////////////////////////////

    void write_object               (OraMetaDict::DbObject& object, void* param);
    void write_table_definition     (OraMetaDict::DbObject& object, void* param);
    void write_table_indexes        (OraMetaDict::DbObject& object, void* param);
    void write_table_chk_constraint (OraMetaDict::DbObject& object, void* param);
    void write_table_unq_constraint (OraMetaDict::DbObject& object, void* param);
    void write_table_fk_constraint  (OraMetaDict::DbObject& object, void* param);
    void write_grantee              (OraMetaDict::DbObject& object, void* param);
    void write_grant                (const OraMetaDict::Grant& object, void* param);

    void write_incopmlete_type      (OraMetaDict::DbObject& object, void* param);

/////////////////////////////////////////////////////////////////////////////

    std::wstring GetFullPath   (ExtractDDLSettings&);
    std::wstring GetRootFolder (ExtractDDLSettings&);
    std::wstring GetSubdirName (ExtractDDLSettings&);

    void MakeFolders (std::wstring& path, ExtractDDLSettings& settings);
    void LoadSchema (OciConnect& connect, OraMetaDict::Dictionary& dict, HWND hStatusWnd, ExtractDDLSettings& settings);
    void OverrideTablespace (OraMetaDict::Dictionary& dict, ExtractDDLSettings& settings);
    void WriteSchema (
        OraMetaDict::Dictionary& dict, std::vector<const OraMetaDict::DbObject*>& views, 
        HWND hStatusWnd, std::wstring& path, string& error_log, ExtractDDLSettings& settings
    );
    void NextAction (OciConnect* connect, HWND hStatusWnd, char* text);
    void OptimizeViewCreationOrder (
        OciConnect& connect, OraMetaDict::Dictionary& dict, HWND hStatusWnd,
        std::vector<const OraMetaDict::DbObject*>& result,
        string& error_log, ExtractDDLSettings& settings
    );

/////////////////////////////////////////////////////////////////////////////

}; // namespace ExtractSchemaHelpers

#endif//_EXTRACTSCHEMAHELPER_H_
