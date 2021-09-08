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

#pragma once
#include <string>
#include <afxmt.h>
#include "Common\ExceptionHelper.h"
#include <OpenEditor/OESettings.h>

class TiXmlElement;
class TiXmlDocument;

#define CD_SIMPLE_PROPERTY(T,N) \
    T m_##N; \
    const T& Get##N () const { return m_##N; }; \
    void  Set##N (const T& val) { m_##N = val; } 

#define CD_STREAMABLE_PROPERTY(T,N) \
    CMN_DECL_PUBLIC_PROPERTY(T,N) \
    public: \
        const T& Get##N () const { return m_##N; }; \
        void  Set##N (const T& val) { m_##N = val; } 

struct ConnectEntry 
{
    CMN_DECL_PROPERTY_BINDER(ConnectEntry);
    CD_STREAMABLE_PROPERTY(std::wstring, Tag);
    CD_STREAMABLE_PROPERTY(std::wstring, User);
    CD_STREAMABLE_PROPERTY(std::string,  Password);
    CD_STREAMABLE_PROPERTY(std::wstring, Alias);
    CD_STREAMABLE_PROPERTY(std::wstring, Host);
    CD_STREAMABLE_PROPERTY(std::wstring, Port);
    CD_STREAMABLE_PROPERTY(std::wstring, Sid);
    CD_STREAMABLE_PROPERTY(bool,         Direct);
    CD_STREAMABLE_PROPERTY(bool,         UseService);
    CD_STREAMABLE_PROPERTY(int,          Mode);
    CD_STREAMABLE_PROPERTY(int,          Safety); // changed to ReadOnly
    CD_STREAMABLE_PROPERTY(time_t,       LastUsage);
    CD_STREAMABLE_PROPERTY(int,          UsageCounter);
    CD_STREAMABLE_PROPERTY(bool,         Slow);
    CD_SIMPLE_PROPERTY(std::wstring,     DecryptedPassword);

    enum Status { UNDEFINED, UPTODATE, MODIFIED, DELETED }; 
    mutable Status m_status;

    ConnectEntry () : m_status(UNDEFINED)
    {
        CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
    }

    static bool IsSignatureEql (const ConnectEntry& left, const ConnectEntry& right);
};

class ConnectData : private noncopyable
{
    friend class ConnectDataXmlStreamer;
    std::vector<ConnectEntry> m_entries;
public:
    typedef std::vector<ConnectEntry> Entries;
    typedef Entries::const_iterator EntriesConstIterator;

    CMN_DECL_PROPERTY_BINDER(ConnectData);
    
    CD_STREAMABLE_PROPERTY(bool,         SavePasswords);    // it will nor work if master password is empty
    CD_STREAMABLE_PROPERTY(std::string,  ScriptOnLogin);    // should be encrypted
    CD_STREAMABLE_PROPERTY(std::string,  Probe);            // use this value for master password verification
    CD_SIMPLE_PROPERTY(std::wstring,     DecScriptOnLogin); 
    
    // minor setting - read on startup only
    CD_STREAMABLE_PROPERTY(int,          SortColumn);
    CD_STREAMABLE_PROPERTY(int,          SortDirection); // 1 and -1
    CD_STREAMABLE_PROPERTY(std::wstring, TagFilter);
    CD_STREAMABLE_PROPERTY(int,          TagFilterOperation);
    CD_STREAMABLE_PROPERTY(std::wstring, UserFilter);
    CD_STREAMABLE_PROPERTY(int,          UserFilterOperation);
    CD_STREAMABLE_PROPERTY(std::wstring, AliasFilter);
    CD_STREAMABLE_PROPERTY(int,          AliasFilterOperation);
    CD_STREAMABLE_PROPERTY(std::wstring, UsageCounterFilter);
    CD_STREAMABLE_PROPERTY(int,          UsageCounterFilterOperation);
    CD_STREAMABLE_PROPERTY(std::wstring, LastUsageFilter);
    CD_STREAMABLE_PROPERTY(int,          LastUsageFilterOperation);

public:

    ConnectData ();
    void Clear ();

    const Entries& GetConnectEntries () const { return m_entries; }

    int  Find (const ConnectEntry&); 
    void GetConnectEntry (int row, ConnectEntry&, const std::wstring& password); 
    void AppendConnectEntry (const ConnectEntry&, const std::wstring& password); 
    void SetConnectEntry (int row, const ConnectEntry&, const std::wstring& password); 
    void MarkedDeleted (int row);

    //void ImportFromRegistry (const string& password);
    void ClearPasswords ();
    void ResetPasswordsAndScriptOnLogin (const std::wstring& password);
    void ChangePassword (const std::wstring& oldPassword, const std::wstring& newPassword);
    bool TestPassword (const std::wstring& password) const;

private:
    static const char* PROBE;

    void EncryptAll (const string& password);
    void DecryptAll (const string& password);

    static void checkAndThrowEncryptionCode (int err);
    static void Encrypt (const string& password, const string& in, string& out);
    static void Decrypt (const string& password, const string& in, string& out);
};

struct EncryptionException : Common::AppException {
    EncryptionException (const char* desc) : Common::AppException(desc) {}
};
struct UnknownEncryptionException : EncryptionException {
    UnknownEncryptionException () : EncryptionException("Unknown encryption error.") {}
};
struct EncryptedDataCorrupt : EncryptionException {
    EncryptedDataCorrupt () : EncryptionException("Encrypted data corrupt.") {}
};
struct PasswordTooLongException : EncryptionException {
    PasswordTooLongException () : EncryptionException("Password is too long.") {}
};
struct InvalidEncryptionModeException : EncryptionException {
    InvalidEncryptionModeException () : EncryptionException("Invalid encrypyon mode.") {}
};
struct InvalidMasterPasswordException : EncryptionException {
    InvalidMasterPasswordException () : EncryptionException("Master password is invalid.") {}
};

