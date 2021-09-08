/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2020 Aleksey Kochetov

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
// 16.12.2004 (Ken Clubok) Add IsStringField method, to support CSV prefix option
// 03.01.2005 (ak) Inverse logic: if a field is not number and datetime, it is a plain text
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 2017-11-28 added BackcastField - helper function for text export

#pragma once
#include <arg_shared.h>
#include "OCI8/Statement.h"
#include "OCI8/Datatypes.h"

namespace OCI8
{
    using namespace std;
	using arg::counted_ptr;

#define OCI8_DECLARE_PROPERTY(T,N) \
    protected: \
        T m_##N; \
    public: \
        const T& Get##N () const { return m_##N; }; \
        void  Set##N (const T& val) { m_##N = val; }

///////////////////////////////////////////////////////////////////////////////
// AutoCursor - auto cursor
///////////////////////////////////////////////////////////////////////////////

class AutoCursor : protected Statement
{
public:
    AutoCursor (Connect&, const char* sttm, int prefetch = 100, int strLimit = 4000, int blobHexRowLength = 32);
    AutoCursor (Connect&, int prefetch = 100, int strLimit = 4000, int blobHexRowLength = 32);

    ~AutoCursor ();

    void Attach (RefCursorVariable&);

    using Statement::IsOpen;
    using Statement::Close;
	
    using Statement::Prepare;
    void Execute ();
    bool Fetch ();
	void Bind (Variable*, const string&);

    int GetFieldCount() const;
    const string& GetFieldName (int) const;
    bool IsNumberField (int) const; 
    bool IsDatetimeField (int) const; 
    string BackcastField (int, const string&) const; 

    void GetString (int, string&) const;
    //bool IsGood (int) const;
    bool IsNull (int) const;
    //bool IsTruncated (int) const;

    using Statement::GetConnect;
    using Statement::GetType;
    using Statement::GetRowCount;
    using Statement::GetSQLFunctionCode;
    using Statement::GetSQLFunctionDescription;
    using Statement::PossibleCompilationErrors;
    using Statement::GetParseErrorOffset;
    using Statement::EnableAutocommit;
    using Statement::IsStateInitialized;
    using Statement::IsStateExecuted;
    using Statement::IsStateEndOfFetch;

    OCI8_DECLARE_PROPERTY(string, NumberFormat     );
    OCI8_DECLARE_PROPERTY(string, DateFormat       );
    OCI8_DECLARE_PROPERTY(string, TimeFormat       );
    OCI8_DECLARE_PROPERTY(string, TimestampFormat  );
    OCI8_DECLARE_PROPERTY(string, TimestampTzFormat);

    OCI8_DECLARE_PROPERTY(string, StringNull);
    OCI8_DECLARE_PROPERTY(int,    IntNull   );
    OCI8_DECLARE_PROPERTY(double, DoubleNull);
    
    OCI8_DECLARE_PROPERTY(int, StringLimit);
    OCI8_DECLARE_PROPERTY(int, BlobHexRowLength);
    OCI8_DECLARE_PROPERTY(bool, SkipLobs);

    OCI8_DECLARE_PROPERTY(double, ExecutionTime);

protected:
    void Define();
    vector<string> m_fieldNames;
    vector<counted_ptr<Variable> > m_fields;
    map<string, counted_ptr<Variable> > m_boundFields;
    bool m_endOfFetch, m_restore_lda;
private:
    // copy-constraction & assign-operation is not supported
    AutoCursor (const AutoCursor&);
    void operator = (const AutoCursor&);
};

///////////////////////////////////////////////////////////////////////////////

inline
int AutoCursor::GetFieldCount() const
{
    return m_fieldNames.size();
}

inline
const string& AutoCursor::GetFieldName (int col) const
{
    return m_fieldNames.at(col);
}

inline
bool AutoCursor::IsNumberField (int col) const
{
    return m_fields.at(col)->IsNumber();
}

inline
bool AutoCursor::IsDatetimeField (int col) const
{
    return m_fields.at(col)->IsDatetime();
}

inline
string AutoCursor::BackcastField (int col, const string& val) const
{
    return m_fields.at(col)->Backcast(val);
}

inline
void AutoCursor::GetString (int col, string& buffstr) const
{  
    m_fields.at(col)->GetString(buffstr, m_StringNull); 
}

//inline
//bool AutoCursor::IsGood (int col) const
//{  
//    return m_fields.at(col)->IsGood(); 
//}

inline
bool AutoCursor::IsNull (int col) const
{  
    return m_fields.at(col)->IsNull(); 
}

//inline
//bool AutoCursor::IsTruncated (int col) const
//{  
//    return m_fields.at(col)->IsTruncated(); 
//}

///////////////////////////////////////////////////////////////////////////////
};