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

#pragma once
#include <arg_shared.h>
#include "OCI8/Statement.h"
#include "OCI8/Datatypes.h"

namespace OCI8
{
    using namespace std;
	using arg::counted_ptr;

///////////////////////////////////////////////////////////////////////////////
// BuffCursor - buffered cursor
///////////////////////////////////////////////////////////////////////////////

class BuffCursor : protected Statement
{
public:
    BuffCursor (Connect&, const char* sttm, int prefetch = 100, int strLimit = 4000);
    BuffCursor (Connect&, int prefetch = 100, int strLimit = 4000);
    ~BuffCursor ();

    using Statement::Close;
	
    using Statement::Prepare;
    void Execute ();
    bool Fetch ();

    void SetStringLimit (int);
    void SetDateFormat (const string&);
    void SetStringNull (const string&);
    const string& GetStringNull () const;
    void SetIntNull (int);
    void SetDoubleNull (double);

    int GetFieldCount() const;
    const string& GetFieldName (int) const;

    int    GetStringLength (int) const;
    void   GetString (int, char*, int) const;
    void   GetString (int, string&) const;

    void   GetTime (int, struct tm&, struct tm* null = 0) const;

    string ToString (int) const;
    string ToString (int, const string&) const;
    int    ToInt (int) const;
    int    ToInt (int, int) const;
    __int64 ToInt64 (int) const;
    __int64 ToInt64 (int, __int64) const;
    double ToDouble (int) const;
    double ToDouble (int, double) const;

    bool IsGood (int) const;
    bool IsNull (int) const;
    bool IsTruncated (int) const;
    bool IsStringField (int) const;
    bool IsNumberField (int) const;

    bool IsRecordGood () const;
    bool IsRecordTruncated () const;

    using Statement::Bind;

    void Bind (const char* name, const char* value, int len = -1);
    void Bind (const char* name, int value);
    void Bind (const char* name, double value);

    using Statement::GetConnect;
    using Statement::GetType;
    using Statement::GetRowCount;
    using Statement::GetSQLFunctionCode;
    using Statement::GetSQLFunctionDescription;
    using Statement::PossibleCompilationErrors;
    using Statement::GetParseErrorOffset;

protected:
    void Define();

    vector<string> m_fieldNames;
    vector<counted_ptr<HostArray> > m_fields;
    map<string, counted_ptr<Variable> > m_boundFields;
    ub4 m_curRecord, m_buffRecords, m_retRecords;
    bool m_endOfFetch, m_restore_lda;

    int m_strLimit;
    string m_dateFormat;
    string m_null;
    int    m_intNull;
    double m_doubleNull;

private:
    static string m_defaultDateFormat;
    static string m_defaultNull;
    static int    m_defaultIntNull;
    static double m_defaultDoubleNull;
private:
    // copy-constraction & assign-operation is not supported
    BuffCursor (const BuffCursor&);
    void operator = (const BuffCursor&);
};

///////////////////////////////////////////////////////////////////////////////

inline
void BuffCursor::SetStringLimit (int lim)
{
    m_strLimit = lim;
}

inline
void BuffCursor::SetDateFormat (const string& dateFormat)
{
    m_dateFormat = dateFormat;
}

inline
void BuffCursor::SetStringNull (const string& null)
{
    m_null = null;
}

inline
const string& BuffCursor::GetStringNull () const
{
    return m_null;
}

inline
void BuffCursor::SetIntNull (int null)
{
    m_intNull = null;
}

inline
void BuffCursor::SetDoubleNull (double null)
{
    m_doubleNull = null;
}

inline
int BuffCursor::GetFieldCount() const
{
    return m_fieldNames.size();
}

inline
const string& BuffCursor::GetFieldName (int col) const
{
    return m_fieldNames.at(col);
}

inline
void BuffCursor::GetString (int col, string& buffstr) const
{  
    m_fields.at(col)->GetString(m_curRecord, buffstr, m_null); 
}

inline
string BuffCursor::ToString (int col) const
{
    string buffstr;
    GetString(col, buffstr);
    return buffstr;
}

inline
string BuffCursor::ToString (int col, const string& null) const
{
    string buffstr;
    m_fields.at(col)->GetString(m_curRecord, buffstr, null);
    return buffstr;
}

inline
void BuffCursor::GetTime (int col, struct tm& time, struct tm* null) const
{
    m_fields.at(col)->GetTime(m_curRecord, time, null); 
}

inline
int BuffCursor::ToInt (int col) const
{  
    return m_fields.at(col)->ToInt(m_curRecord, m_intNull); 
}

inline
int BuffCursor::ToInt (int col, int null) const
{  
    return m_fields.at(col)->ToInt(m_curRecord, null); 
}

inline
__int64 BuffCursor::ToInt64 (int col) const
{  
    return m_fields.at(col)->ToInt64(m_curRecord, m_intNull); 
}

inline
__int64 BuffCursor::ToInt64 (int col, __int64 null) const
{  
    return m_fields.at(col)->ToInt64(m_curRecord, null); 
}

inline
double BuffCursor::ToDouble (int col) const
{  
    return m_fields.at(col)->ToDouble(m_curRecord, m_doubleNull); 
}

inline
double BuffCursor::ToDouble (int col, double null) const
{  
    return m_fields.at(col)->ToDouble(m_curRecord, null); 
}

inline
bool BuffCursor::IsGood (int col) const
{  
    return m_fields.at(col)->IsGood(m_curRecord); 
}

inline
bool BuffCursor::IsNull (int col) const
{  
    return m_fields.at(col)->IsNull(m_curRecord); 
}

inline
bool BuffCursor::IsTruncated (int col) const
{  
    return m_fields.at(col)->IsTruncated(m_curRecord); 
}

inline
bool BuffCursor::IsStringField (int col) const
{
    return typeid(*m_fields.at(col)) == typeid(StringArray); 
}

inline
bool BuffCursor::IsNumberField (int col) const
{
    return typeid(*m_fields.at(col)) == typeid(NumberArray); 
}

///////////////////////////////////////////////////////////////////////////////
};