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
// 07.02.2005 (Ken Clubok) R1105003: Bind variables
// 13.03.2005 (ak) R1105003: Bind variables
// 2017-11-28 added BackcastField - helper function for text export

#pragma once
#include <oci.h>
#include "OCI8/Connect.h"

namespace OCI8
{
    class Statement;
    class AutoCursor;

///////////////////////////////////////////////////////////////////////////////
// OCI8::Variable
///////////////////////////////////////////////////////////////////////////////

/** @brief Data storage for select results and bind variables.
 */
class Variable
{
    friend class AutoCursor;
protected:
    Variable (ub2 type, sb4 size);
    Variable (dvoid* buffer, ub2 type, sb4 size);

    sb4    GetSize () const         { return m_size; }
    sb4    GetBufferSize () const   { return m_buffer_size; }
    ub2    GetReturnedSize () const { return m_ret_size; }

    virtual void BeforeFetch () {}

public:
    virtual ~Variable ();

    virtual void GetString (string&, const string& null = m_null) const /*= 0*/;

    //bool IsGood () const { return (m_indicator == -1 || m_indicator == 0) ? true : false; }
    virtual bool IsNull () const { return m_indicator == -1 ? true : false; }
    //virtual bool IsTruncated () const { return m_indicator == -2 || m_indicator > 0 ? true : false; }

    virtual bool IsNumber () const { return false; }
    virtual bool IsDatetime () const { return false; }
    virtual string Backcast (const string& val) { return val; }

    virtual void Define (Statement&, int pos);
    virtual void Bind   (Statement&, int pos);
    virtual void Bind   (Statement&, const char* name);

protected:
    ub2    m_type;
    sb2    m_indicator;
    sb4    m_size, m_buffer_size;
    ub2    m_ret_size;

    dvoid* m_buffer;
    bool   m_owner;
    DWORD m_threadId;

public:
    static const string m_null;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::RefCursorVariable
//      limited functionality, can be bound
///////////////////////////////////////////////////////////////////////////////

class RefCursorVariable : public Variable
{
public:
    RefCursorVariable (Connect&, int prefetch = 100);
    ~RefCursorVariable ();

    virtual void GetString (string&, const string& null = m_null) const;
    virtual void Bind (Statement&, int pos);
    virtual void Bind (Statement&, const char* name);

    bool IsStateInitialized ();
    bool IsStateExecuted ();
    bool IsStateEndOfFetch ();

protected:
    friend AutoCursor;

    Connect& m_connect;
    OCIStmt* m_sttmp;
    int m_prefetch;

    Connect&    GetConnect () const     { return m_connect; }
    OCIStmt*    GetOCIStmt () const     { return m_sttmp; }
    OCIError*   GetOCIError () const    { return m_connect.GetOCIError();  }

    OCIEnv*    GetOCIEnv ()     { return m_connect.GetOCIEnv(); }
    OCISvcCtx* GetOCISvcCtx ()  { return m_connect.GetOCISvcCtx(); }

    void CHECK_ALLOC (sword status) { m_connect.CHECK_ALLOC(status); }
    void CHECK (sword status)       { m_connect.CHECK(status); }
    void CHECK_INTERRUPT ()         { m_connect.CHECK_INTERRUPT(); }
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::XmlTypeVariable
///////////////////////////////////////////////////////////////////////////////

#ifdef XMLTYPE_SUPPORT
class XmlTypeVariable : public Variable
{
public:
    XmlTypeVariable (Connect&, int limit);
    ~XmlTypeVariable ();

    virtual void GetString (string&, const string& null = m_null) const;

    OCIInd*& GetIndicator () { return m_indicator; }
    void*& GetXmlDocNode () { return m_docnode; }

    virtual void Define (Statement& sttm, int pos);

protected:
    virtual void BeforeFetch ();

    Connect& m_connect;
    void* m_docnode;
    OCIInd* m_indicator;       
    int m_limit;
};
#endif//XMLTYPE_SUPPORT

///////////////////////////////////////////////////////////////////////////////
// OCI8::NativeOciVariable
///////////////////////////////////////////////////////////////////////////////

class NativeOciVariable : public Variable
{
protected:
    NativeOciVariable (Connect&, ub2 sqlt_type, ub2 oci_handle_type);
    ~NativeOciVariable ();

    virtual void Define (Statement&, int pos);

    Connect& m_connect;
    ub2 m_oci_handle_type;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::StringVar
///////////////////////////////////////////////////////////////////////////////

class StringVar : public Variable
{
public:
    StringVar (int size);
    StringVar (const string&);
    StringVar (const char*, int = -1);

    void Assign (const char*, int len = -1);
    void Assign (const string&);

    virtual void GetString (string&, const string& null = m_null) const;

    sb4 GetStringBufSize () const { return GetSize() - static_cast<sb4>(sizeof(int)); }

protected:
    //using Variable::Assign;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::NumStrVar
///////////////////////////////////////////////////////////////////////////////

class NumStrVar : public StringVar
{
public:
    NumStrVar () : StringVar(41) {}

    virtual bool IsNumber () const { return true; }
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::NumberVar
///////////////////////////////////////////////////////////////////////////////

class NumberVar : public Variable
{
public:
    NumberVar (Connect&);
    NumberVar (Connect&, const string& format);
    NumberVar (Connect&, int value);
    NumberVar (Connect&, double value);

	void Assign (int);
	void Assign (double);

    virtual void GetString (string&, const string& null = m_null) const;

    virtual int    ToInt (int = 0) const;
    virtual double ToDouble (double = 0) const;

    virtual bool IsNumber () const { return true; }

protected:
    Connect& m_connect;
    std::wstring m_numberFormat;
    OCINumber m_value;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::DateVar
// Oracle 7.3 Server and OCI 8.0.X supports only OCIDate 
///////////////////////////////////////////////////////////////////////////////

class DateVar : public Variable
{
public:
    DateVar (Connect&);
    DateVar (Connect&, const string& dateFormat);

    virtual void GetString (string&, const string& null = m_null) const;

    virtual bool IsDatetime () const { return true; }
    virtual string Backcast (const string& val) { return "To_Date('" + val + "', '" + Common::str(m_dateFormat) + "')"; }

protected:
    Connect& m_connect;
    std::wstring m_dateFormat;
    OCIDate m_value;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::TimestampVar
///////////////////////////////////////////////////////////////////////////////

class TimestampVar : public NativeOciVariable
{
public:
    enum ESubtype {
        DATE = SQLT_DATE,
        TIME = SQLT_TIME,
        TIMESTAMP = SQLT_TIMESTAMP,
        TIMESTAMP_TZ = SQLT_TIMESTAMP_TZ,
        TIMESTAMP_LTZ = SQLT_TIMESTAMP_LTZ,
    };

    TimestampVar (Connect&, ESubtype);
    TimestampVar (Connect&, ESubtype, const string& dateFormat);

    virtual void GetString (string&, const string& null = m_null) const;

    virtual bool IsDatetime () const { return true; }
    virtual string Backcast (const string& val) 
    { 
        return (
            (m_oci_handle_type == OCI_DTYPE_TIMESTAMP_TZ || m_oci_handle_type == OCI_DTYPE_TIMESTAMP_LTZ)
                ? "To_Timestamp_TZ('" : "To_Timestamp('"
            ) + val + "', '" + Common::str(m_dateFormat) + "')"; 
    }

protected:
    std::wstring m_dateFormat;
    static ub2 sqlt_dtype_map (ub2);
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::IntervalVar
///////////////////////////////////////////////////////////////////////////////

class IntervalVar : public NativeOciVariable
{
public:
    enum ESubtype {
        INTERVAL_YM = SQLT_INTERVAL_YM,
        INTERVAL_DS = SQLT_INTERVAL_DS,
    };

    IntervalVar (Connect&, ESubtype);

    virtual void GetString (string&, const string& null = m_null) const;

    virtual bool IsDatetime () const { return true; }
    virtual string Backcast (const string& val) { return "INTERVAL '" + val + (m_oci_handle_type == SQLT_INTERVAL_DS ? "' DAY TO SECOND" : "' YEAR TO MONTH"); }

protected:
    static ub2 sqlt_dtype_map (ub2);
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::DummyVar
///////////////////////////////////////////////////////////////////////////////

class DummyVar : public Variable
{
public:
    DummyVar (bool skipped = false);

    virtual void GetString (string&, const string& null = m_null) const;

public:
    bool m_skipped;
    static const string m_unsupportedStr;
    static const string m_skippedStr;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::LobVar
///////////////////////////////////////////////////////////////////////////////

enum ELobSubtype { elsCLob = SQLT_CLOB, elsBLob = SQLT_BLOB, elsBFile = SQLT_FILE };
enum ECharForm   { ecfImplicit = SQLCS_IMPLICIT, ecfNchar = SQLCS_NCHAR };

class LobVar : public NativeOciVariable
{
protected:
    int m_limit;
    ECharForm m_charForm;
    //bool m_temporary;
    //bool m_truncated;

    LobVar (Connect&, ELobSubtype, int limit, ECharForm = ecfImplicit);

    virtual void GetString (string&, const string& null = m_null) const;

    int  getLobLength () const;
    void getString (char* strbuff, int buffsize, ub2 csid) const;
    //void makeTemporary (ub1 type);
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::CLobVar
///////////////////////////////////////////////////////////////////////////////

class CLobVar : public LobVar
{
public:
    CLobVar (Connect& connect, int limit, ECharForm charForm = ecfImplicit) 
        : LobVar(connect, elsCLob, limit, charForm) {}
    //void MakeTemporary ();

    using LobVar::GetString;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::BLobVar
///////////////////////////////////////////////////////////////////////////////

class BLobVar : public LobVar
{
protected:
    int m_hexFormatLength;
    BLobVar (Connect&, ELobSubtype, int limit, int hexFormatLength);

public:
    BLobVar (Connect&, int limit, int hexFormatLength);

    virtual void GetString (string&, const string& null = m_null) const;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::BFileVar
///////////////////////////////////////////////////////////////////////////////

class BFileVar : public BLobVar
{
public:
    BFileVar (Connect&, int limit, int hexFormatLength);

    virtual void GetString (string&, const string& null = m_null) const;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::HostArray
///////////////////////////////////////////////////////////////////////////////

class HostArray
{
    friend class BuffCursor;
public:
    HostArray (ub2 type, sb4 size, sb4 count);
    virtual ~HostArray ();

    virtual void GetString (int, string&, const string& null = m_null) const = 0;
    virtual void GetTime   (int, struct tm&, struct tm* null = 0) const = 0;

    virtual int    ToInt (int, int = 0) const = 0;
    virtual __int64 ToInt64 (int, __int64 = 0) const = 0;
    virtual double ToDouble (int, double = 0) const = 0;

    bool IsGood (int inx) const;
    bool IsNull (int inx) const;
    bool IsTruncated (int inx) const;

	void Assign (int inx, const dvoid* buffer, ub2 size);
	void Assign (int inx, int);
	void Assign (int inx, long);

    void Define (Statement&, int pos);
    void Bind (Statement&, int pos);
    void Bind (Statement&, const char*);

protected:
    const void* At (int inx) const;
    const void* at (int inx) const      { return ((const char*)m_buffer) + m_elm_buff_size * inx; }

    friend Statement;
    ub2    m_type;
    sb2*   m_indicators;
    ub2*   m_sizes;
    sb4    m_elm_buff_size;
    sb4    m_count;
    dvoid* m_buffer;
    ub4    m_cur_count;
    DWORD  m_threadId;

public:
    static const string m_null;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::StringArray
///////////////////////////////////////////////////////////////////////////////

class StringArray : public HostArray
{
public:
    StringArray (int size, int count);

    virtual void GetString (int, string&, const string& null = m_null) const;
    virtual void GetTime   (int, struct tm&, struct tm* null = 0) const;

    virtual int    ToInt (int, int = 0) const;
    virtual __int64 ToInt64 (int, __int64 = 0) const;
    virtual double ToDouble (int, double = 0) const;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::NumberArray
///////////////////////////////////////////////////////////////////////////////

class NumberArray : public HostArray
{
public:
    NumberArray (int count);

    virtual void GetString (int, string&, const string& null = m_null) const;
    virtual void GetTime   (int, struct tm&, struct tm* null = 0) const;

    virtual int    ToInt (int, int = 0) const;
    virtual __int64 ToInt64 (int, __int64 = 0) const;
    virtual double ToDouble (int, double = 0) const;
};

///////////////////////////////////////////////////////////////////////////////
// OCI8::DateArray
///////////////////////////////////////////////////////////////////////////////

class DateArray : public HostArray
{
public:
    DateArray (int count, const string& = m_defaultDateFormat);

    virtual void GetString (int, string&, const string& null = m_null) const;
    virtual void GetTime   (int, struct tm&, struct tm* null = 0) const;

    virtual int    ToInt (int, int = 0) const;
    virtual __int64 ToInt64 (int, __int64 = 0) const;
    virtual double ToDouble (int, double = 0) const;

protected:
    int m_dataStrLength;
    string m_dateFormat;
public:
    static const string m_defaultDateFormat; // "%Y.%m.%d %H:%M:%S"
};

///////////////////////////////////////////////////////////////////////////////

inline
bool HostArray::IsGood (int inx) const
{
    return (m_indicators[inx] == -1 || m_indicators[inx] == 0) ? true : false;
}

inline
bool HostArray::IsNull (int inx) const
{
    return m_indicators[inx] == -1 ? true : false;
}

inline
bool HostArray::IsTruncated (int inx) const
{
    return m_indicators[inx] == -2 || m_indicators[inx] > 0 ? true : false;
}

///////////////////////////////////////////////////////////////////////////////
};