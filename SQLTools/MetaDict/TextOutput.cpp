/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2004 Aleksey Kochetov

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
#include "MetaDict\TextOutput.h"
#include <COMMON\ExceptionHelper.h>
#include "OpenEditor\OEHelpers.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/*
    13.03.2003 bug fix, file writing error on export schema
    06.04.2003 bug fix, DDL reengendering fails if reserved words used as object/column name
*/

namespace OraMetaDict 
{
    using OpenEditor::LanguagesCollection;

    static int count_lines (const char* str, int len = INT_MAX)
    {
        int count = 0;
        for (int i(0); str[i] && i < len; i++)
            if (str[i] == '\n' || str[i] == '\r') {
                if (str[i] == '\n' && str[i+1] == '\r')
                    i++;
                count++;
            }
        return count;
    }


/// TextOutput //////////////////////////////////////////////////////////////

    LanguageKeywordMapPtr TextOutput::m_LanguageKeywordMap;

TextOutput::TextOutput (bool bLowerDBName)
: m_nIndent(0), 
    m_bLowerDBName(bLowerDBName)
{
    ASSERT_EXCEPTION_FRAME;

    try
    {
        if (!m_LanguageKeywordMap.get())
            m_LanguageKeywordMap = LanguagesCollection::Find("PL/SQL")->GetLanguageKeywordMap();
    }
    catch (const std::logic_error&)
    {
        _RAISE(std::logic_error("Text output stream error. Cannot get PL/SQL language support."));
    }
}

void TextOutput::countLines (const char* szStr, int nLen)
{
    m_nLineCounter += count_lines(szStr, nLen);
}

void TextOutput::SetLike (const TextOutput& other)
{
	m_nIndent      = other.m_nIndent;
    m_bLowerDBName = other.m_bLowerDBName;
	m_strIndent    = other.m_strIndent;
}

int TextOutput::SetIndent (int nIndent) 
{ 
    _ASSERTE(nIndent > 0);
    int nOldIndent = m_strIndent.size(); 
    m_strIndent.resize(nIndent, ' ');
    return nOldIndent;
}

int TextOutput::MoveIndent (int nOffset) 
{ 
    int nOldIndent = m_strIndent.size();
    _ASSERTE((nOldIndent + nOffset) >= 0);
    int nIndent = nOldIndent + nOffset;
    m_strIndent.resize(nIndent, ' ');
    return nOldIndent;
}

void TextOutput::PutLine (const string& strBuffer)
{
    PutIndent();
    Put(strBuffer);
    NewLine();
}

void TextOutput::PutLine (const char* szBuffer)
{
    PutIndent();
    Put(szBuffer);
    NewLine();
}

void TextOutput::PutLine (const char* szBuffer, int nLen)
{
    _ASSERTE(nLen >= 0);

    PutIndent();
    Put(szBuffer, nLen);
    NewLine();
}

void TextOutput::Fill (char chr, int count)
{
    char szBuff[2] = { chr, 0 };
    for (int i(0); i < count; i++)
        Put(szBuff);
}

void TextOutput::PutOwnerAndName (const char* szOwner, const char* szName, bool bPutOwner)
{
    string strBuffer;

    if (bPutOwner && szOwner && strlen(szOwner) > 0) {
        SafeWriteDBName(szOwner);
        Put(".");
    }

    SafeWriteDBName(szName);
}


void TextOutput::WriteColumns (const vector<string>& columns, int nIndent, bool safeWriteDBName)
{
    MoveIndent(nIndent);
    std::vector<string>::const_iterator it(columns.begin()),
                                        end(columns.end());
    int nSize = columns.size();
    for (int i(0); it != end; it++, i++) {
        PutIndent(); 

        if (safeWriteDBName)
            SafeWriteDBName(*it);
        else
            Put(*it);

        if (i < nSize - 1) Put(",");
        NewLine();
    }
    MoveIndent(-nIndent);
}

void TextOutput::WriteColumns (const list<string>& columns, int nIndent, bool safeWriteDBName)
{
    MoveIndent(nIndent);
    std::list<string>::const_iterator it(columns.begin()),
                                      end(columns.end());
    int nSize = columns.size();
    for (int i(0); it != end; it++, i++) {
        PutIndent(); 

        if (safeWriteDBName)
            SafeWriteDBName(*it);
        else
            Put(*it);

        if (i < nSize - 1) Put(",");
        NewLine();
    }
    MoveIndent(-nIndent);
}

void TextOutput::WriteColumns (const map<int,string>& columns, int nIndent, const map<int,bool>& safeWriteDBName)
{
    MoveIndent(nIndent);
    std::map<int,string>::const_iterator it(columns.begin()),
                                         end(columns.end());
    int nSize = columns.size();
    for (int i(0); it != end; it++, i++) {
        PutIndent(); 

        if (safeWriteDBName.empty() 
        || safeWriteDBName.find(i) == safeWriteDBName.end()
        || safeWriteDBName.find(i)->second != false)
            SafeWriteDBName(it->second);
        else
            Put(it->second);

        if (i < nSize - 1) Put(",");
        NewLine();
    }
    MoveIndent(-nIndent);
}

void TextOutput::SafeWriteDBName (const char* szName)
{
    _ASSERTE(szName);
    bool regular = true;
    bool isPublic = false;
    
    if (szName[0] < 'A' || szName[0] > 'Z') {
        regular = false;
    } else {
        const char* pChr = szName;
        while (*pChr) {
            if (!((*pChr >= 'A' && *pChr <= 'Z') 
            || (*pChr >= '0' && *pChr <= '9') 
            || *pChr == '_' || *pChr == '$' || *pChr == '#')) {
                regular = false;
                break;
            }
            pChr++;
        }
    }

    // 06.04.2003 bug fix, DDL reengendering fails if reserved words used as object/column name
    if (regular) 
    {
        OpenEditor::LanguageKeywordMapConstIterator
            it = m_LanguageKeywordMap->find(szName);

        if (it != m_LanguageKeywordMap->end() && it->second.groupIndex == 0) // only for SQL Keywords
        {
            isPublic = !strcmp(szName, "PUBLIC") ? true : false;
            regular = false;
        }
    }

    if (regular || isPublic) 
    {
        if (m_bLowerDBName && !isPublic) 
        {
            string name = szName;
            for (string::iterator it = name.begin(); it != name.end(); ++it)
                *it = static_cast<char>(tolower(*it));
            Put(name);
        } 
        else
            Put(szName);
    } 
    else
    {
        Put("\""); Put(szName); Put("\"");
    }
}

void TextOutput::WriteSQLString  (const string& str)
{
    string buffer;
    buffer.reserve(str.length() + 10);
    buffer += '\'';

    if (str.find('\'') == string::npos)
        buffer += str;
    else
        for (string::const_iterator it = str.begin(); it != str.end(); ++it)
        {
            if (*it == '\'') buffer += *it;
            buffer += *it;
        }

    buffer += '\'';
    Put(buffer);
}

#ifdef _AFX
/// TextOutputInMFCFile /////////////////////////////////////////////////////

int TextOutputInMFCFile::GetPosition () const 
{ 
    return static_cast<int>(m_File.GetPosition()); 
}
/*
int TextOutputInMFCFile::SetPosition (int pos)
{ 
    return m_File.Seek(pos, CFile::begin); 
}

void TextOutputInMFCFile::SetLength (int pos)
{ 
    m_File.SetLength(pos); 
}
*/
void TextOutputInMFCFile::PutIndent ()
{
    m_File.Write(m_strIndent.c_str(), m_strIndent.size());
}

void TextOutputInMFCFile::NewLine ()
{
    if (m_bEnableLineCounter) m_nLineCounter++;
    m_File.Write("\n", 1);
}

void TextOutputInMFCFile::Put (const string& strBuffer)
{
    if (m_bEnableLineCounter) {
        countLines(strBuffer.c_str(), strBuffer.size());
    }   
    m_File.Write(strBuffer.c_str(), strBuffer.size());
}

void TextOutputInMFCFile::Put (const char* szBuffer)
{
    if (m_bEnableLineCounter) {
        countLines(szBuffer, INT_MAX);
    }
    m_File.Write(szBuffer, strlen(szBuffer));
}

void TextOutputInMFCFile::Put (const char* szBuffer, int nLen)
{
    if (m_bEnableLineCounter) {
        countLines(szBuffer, nLen);
    }
    m_File.Write(szBuffer, nLen);
}
#endif//_AFX

/// TextOutputInFILE ////////////////////////////////////////////////////////

void CHECK_OPER_ERROR (bool x)  { _CHECK_AND_THROW_(x, "File operation error"); }
void CHECK_WRITE_ERROR (bool x) { _CHECK_AND_THROW_(x, "File writing error"); }

int TextOutputInFILE::GetPosition () const 
{ 
    int retVal = ftell(&m_File);
    CHECK_OPER_ERROR(retVal != -1);
    return retVal;
}
/*
int TextOutputInFILE::SetPosition (int pos)
{ 
    return fseek(&m_File, pos, SEEK_SET); 
}

void TextOutputInFILE::SetLength (int pos)
{ 
    // This procedure isn't tested! It isn't called for TextOutputInFILE inplementation.
    //_ASSERTE(0);
    //CHECK_OPER_ERROR(!fseek(&m_File, pos, SEEK_SET));
	const fpos_t fpos = pos;
    CHECK_OPER_ERROR(!fsetpos(&m_File, &fpos));
    //CHECK_WRITE_ERROR(fputc(0, &m_File) != EOF);
}
*/
void TextOutputInFILE::PutIndent ()
{
    CHECK_WRITE_ERROR(fputs(m_strIndent.c_str(), &m_File) != EOF);
}

void TextOutputInFILE::NewLine ()
{
    if (m_bEnableLineCounter) m_nLineCounter++;
    CHECK_WRITE_ERROR(fputc('\n', &m_File) != EOF);
}

void TextOutputInFILE::Put (const string& strBuffer)
{
    if (m_bEnableLineCounter) {
        countLines(strBuffer.c_str(), strBuffer.size());
    }   
    CHECK_WRITE_ERROR(fwrite(strBuffer.c_str(), 1, strBuffer.size(), &m_File) >= strBuffer.size());
}

void TextOutputInFILE::Put (const char* szBuffer)
{
    if (m_bEnableLineCounter) {
        countLines(szBuffer, INT_MAX);
    }
    CHECK_WRITE_ERROR(fputs(szBuffer, &m_File) != EOF);
}

void TextOutputInFILE::Put (const char* szBuffer, int nLen)
{
    if (m_bEnableLineCounter) {
        countLines(szBuffer, nLen);
    }
    CHECK_WRITE_ERROR(static_cast<int>(fwrite(szBuffer, 1, nLen, &m_File)) >= nLen);
}


/// TextOutputInMEM /////////////////////////////////////////////////////////

TextOutputInMEM::TextOutputInMEM ()
: TextOutput(true),
  m_nSize(0),
  m_nOffset(0),
  m_Base(0)
{
}

TextOutputInMEM::TextOutputInMEM (bool bLowerDBName, int size)
: TextOutput(bLowerDBName),
  m_nSize(size),
  m_nOffset(0)
{
    m_Base = (char*)malloc(m_nSize);
}

TextOutputInMEM::~TextOutputInMEM ()
{
    try { EXCEPTION_FRAME;

        free(m_Base);
    }
    _DESTRUCTOR_HANDLER_;
}

void TextOutputInMEM::Erase ()
{
    m_nOffset = 0;
}

void TextOutputInMEM::Realloc (int size)
{
    if (m_nSize < size) {
        size += m_nSize / 2;
        void* ptr = ::realloc(m_Base, size);
        
#ifdef _AFX
        if (!ptr)
            AfxThrowMemoryException();
#else
        if (!ptr)
            throw std::bad_alloc();
#endif//_AFX

        m_Base = (char*)ptr;
        m_nSize = size;
    }
}

void TextOutputInMEM::operator += (TextOutputInMEM& other)
{
    if (m_nSize < (m_nOffset + other.m_nOffset))
        Realloc(m_nOffset + other.m_nOffset + 1);
    
    write (other.m_Base, other.m_nOffset);
}

void TextOutputInMEM::TrimRight (const char* charset)
{
    OpenEditor::DelimitersMap map(charset);
    map[0] = true;

    int orgOffset = m_nOffset;
    while (m_nOffset > 0 && map[m_Base[m_nOffset]])
    {
        --m_nOffset;
    }

    if (orgOffset > m_nOffset)
      m_Base[++m_nOffset] = 0;
}

int TextOutputInMEM::CountLines () const
{
    return count_lines(m_Base, m_nOffset);
}

int TextOutputInMEM::GetPosition () const 
{ 
    return m_nOffset; 
}
/*
int TextOutputInMEM::SetPosition (int pos)
{ 
    if (m_nOffset > pos)
        m_nOffset = pos;

    return m_nOffset; 
}

void TextOutputInMEM::SetLength (int pos)
{ 
    SetPosition(pos); 
}
*/
void TextOutputInMEM::PutIndent ()
{
    write(m_strIndent.c_str(), m_strIndent.size());
}

void TextOutputInMEM::NewLine ()
{
    if (m_bEnableLineCounter) m_nLineCounter++;
    write("\n", 1);
}

void TextOutputInMEM::Put (const string& strBuffer)
{
    if (m_bEnableLineCounter) {
        countLines(strBuffer.c_str(), strBuffer.size());
    }   
    write(strBuffer.c_str(), strBuffer.size());
}

void TextOutputInMEM::Put (const char* szBuffer)
{
    if (m_bEnableLineCounter) {
        countLines(szBuffer, INT_MAX);
    }
    write(szBuffer, strlen(szBuffer));
}

void TextOutputInMEM::Put (const char* szBuffer, int nLen)
{
    if (m_bEnableLineCounter) {
        countLines(szBuffer, nLen);
    }
    write(szBuffer, nLen);
}

void TextOutputInMEM::write (const char* text, int size)
{
    Realloc(m_nOffset + size + 1);
    memcpy(m_Base + m_nOffset, text, size);
    m_nOffset += size;
    m_Base[m_nOffset] = 0;
}

} // END namespace OraMetaDict 
