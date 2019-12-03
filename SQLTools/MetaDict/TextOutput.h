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

#ifndef __TEXTOUTPUT_H__
#define __TEXTOUTPUT_H__
#pragma once
#pragma warning ( push )
#pragma warning ( disable : 4786 )

#include <OpenEditor/OELanguage.h>

namespace OraMetaDict 
{
    using std::string;
    using std::vector;
    using std::list;
    using std::map;
    using OpenEditor::LanguageKeywordMapPtr;

    /// TextOutput base class for text streaming ////////////////////////////

    class TextOutput
    {
    protected:
        int  m_nIndent, 
             m_nLineCounter;
        bool m_bLowerDBName,
             m_bEnableLineCounter;
        string m_strIndent;

        static LanguageKeywordMapPtr m_LanguageKeywordMap;

        void countLines (const char*, int);

    public:
        TextOutput (bool bLowerDBName);

        void SetLike (const TextOutput& other);

        bool IsLowerDBName ()  const                { return m_bLowerDBName; }
        void SetLowerDBName (bool lower)            { m_bLowerDBName = lower; }
        int  GetLineCounter () const                { return m_nLineCounter; }
        void EnableLineCounter (bool bEnable);

        virtual int  GetPosition () const = 0;
/*
        virtual int  SetPosition (int) = 0;
        virtual void SetLength (int) = 0;
*/
        int  GetIndent () const;
        int  SetIndent (int nIndent);
        int  MoveIndent (int nOffset);

        virtual void PutIndent () = 0;
        virtual void NewLine () = 0;
        virtual void Put (const string& strBuffer) = 0;
        virtual void Put (const char* szBuffer) = 0;
        virtual void Put (const char* szBuffer, int nLen) = 0;
        
        void PutLine (const string& strBuffer);
        void PutLine (const char* szBuffer);
        void PutLine (const char* szBuffer, int nLen);
        void Fill (char chr, int count);
        void PutOwnerAndName (const char* szOwner, const char* szName, bool bPutOwner);
        void PutOwnerAndName (const string& strOwner, const string& strName, bool bPutOwner);
        void WriteColumns (const vector<string>&, int nIndent, bool safeWriteDBName = true);
        void WriteColumns (const list<string>&, int nIndent, bool safeWriteDBName = true);
        void WriteColumns (const map<int,string>&, int nIndent, const map<int,bool>& = map<int,bool>());
        void SafeWriteDBName (const char* szName);
        void SafeWriteDBName (const string& strName);
        void WriteSQLString  (const string& str);
    };

    
    /// TextOutputInMFCFile is a TextOutput implementation on MFC CFile /////
    
#ifdef _AFX
    class TextOutputInMFCFile : public TextOutput
    {
        CFile& m_File;
    public:
        TextOutputInMFCFile (CFile& file, bool bLowerDBName)
        : TextOutput(bLowerDBName), 
          m_File(file) 
        {
        }

        virtual int  GetPosition () const;
/*
        virtual int  SetPosition (int);
        virtual void SetLength (int);
*/
        virtual void PutIndent ();
        virtual void NewLine ();
        virtual void Put (const string& strBuffer);
        virtual void Put (const char* szBuffer);
        virtual void Put (const char* szBuffer, int nLen);
    };
#endif//_AFX


    /// TextOutputInFILE is a TextOutput implementation on FILE stream //////
    
    class TextOutputInFILE : public TextOutput
    {
        FILE& m_File;
    public:
        TextOutputInFILE (FILE& file, bool bLowerDBName)
        : TextOutput(bLowerDBName), 
          m_File(file) 
        {
            _ASSERT(&m_File);
        }

        virtual int  GetPosition () const;
/*
        virtual int  SetPosition (int);
        virtual void SetLength (int);
*/
        virtual void PutIndent ();
        virtual void NewLine ();
        virtual void Put (const string& strBuffer);
        virtual void Put (const char* szBuffer);
        virtual void Put (const char* szBuffer, int nLen);
    };


    /// TextOutputInMEM is a TextOutput implementation on memory block //////
    
    class TextOutputInMEM : public TextOutput
    {
    public:
        TextOutputInMEM ();
        TextOutputInMEM (bool bLowerDBName, int size = 16*1024);
        ~TextOutputInMEM ();

        void Erase ();
        void Realloc (int size);
        void operator += (TextOutputInMEM&);
        void TrimRight (const char* charset);

        int GetLength () const                                  { return m_nOffset; }
        const char* GetData () const                            { return m_Base; }
        operator const char* () const                           { return m_Base; }
        int CountLines () const;

        virtual int  GetPosition () const;
/*
        virtual int  SetPosition (int);
        virtual void SetLength (int);
*/
        virtual void PutIndent ();
        virtual void NewLine ();
        virtual void Put (const string& strBuffer);
        virtual void Put (const char* szBuffer);
        virtual void Put (const char* szBuffer, int nLen);

    private:
        void write (const char*, int size);

        char*  m_Base;
        int    m_nOffset, m_nSize;
    };

    /////////////////////////////////////////////////////////////////////////

    inline 
    void TextOutput::EnableLineCounter (bool bEnable) 
    { 
        m_bEnableLineCounter = bEnable; 
        m_nLineCounter = 0; 
    }

    inline
    int TextOutput::GetIndent () const 
    { 
        return m_strIndent.size(); 
    }

    inline
    void TextOutput::PutOwnerAndName (const string& strOwner, const string& strName, bool bPutOwner)
    {
        PutOwnerAndName(strOwner.c_str(), strName.c_str(), bPutOwner);
    }

    inline
    void TextOutput::SafeWriteDBName (const string& strName)
    {
        SafeWriteDBName(strName.c_str());
    }

} // END namespace OraMetaDict 

#pragma warning ( pop )
#endif//__TEXTOUTPUT_H__
