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

#include "stdafx.h"
#include "ConnectData.h"
#include "TinyXml\TinyXml.h"
#include "COMMON\AppUtilities.h"
#include "COMMON\MyUtf.h"

    using namespace std;
    using namespace Common;

    CMN_IMPL_PROPERTY_BINDER(ConnectEntry);
    CMN_IMPL_PROPERTY(ConnectEntry, Tag,            L"");
    CMN_IMPL_PROPERTY(ConnectEntry, User,           L"");
    CMN_IMPL_PROPERTY(ConnectEntry, Password,       "");
    CMN_IMPL_PROPERTY(ConnectEntry, Alias,          L"");
    CMN_IMPL_PROPERTY(ConnectEntry, Host,           L"");
    CMN_IMPL_PROPERTY(ConnectEntry, Port,           L"1521");
    CMN_IMPL_PROPERTY(ConnectEntry, Sid,            L"");
    CMN_IMPL_PROPERTY(ConnectEntry, Direct,         false);
    CMN_IMPL_PROPERTY(ConnectEntry, UseService,     false);
    CMN_IMPL_PROPERTY(ConnectEntry, Mode,           0);
    CMN_IMPL_PROPERTY(ConnectEntry, Safety,         0);
    CMN_IMPL_PROPERTY(ConnectEntry, LastUsage,      0);
    CMN_IMPL_PROPERTY(ConnectEntry, UsageCounter,   0);
    CMN_IMPL_PROPERTY(ConnectEntry, Slow,           false);

    CMN_IMPL_PROPERTY_BINDER(ConnectData);
    CMN_IMPL_PROPERTY(ConnectData, SavePasswords,               true);
    CMN_IMPL_PROPERTY(ConnectData, ScriptOnLogin,               "");
    CMN_IMPL_PROPERTY(ConnectData, Probe,                       "probe");
    CMN_IMPL_PROPERTY(ConnectData, SortColumn,                  0);
    CMN_IMPL_PROPERTY(ConnectData, SortDirection,               1); // 1 and -1
    CMN_IMPL_PROPERTY(ConnectData, TagFilter,                   L"");
    CMN_IMPL_PROPERTY(ConnectData, TagFilterOperation,          0); // contains
    CMN_IMPL_PROPERTY(ConnectData, UserFilter,                  L"");
    CMN_IMPL_PROPERTY(ConnectData, UserFilterOperation,         0); // contains
    CMN_IMPL_PROPERTY(ConnectData, AliasFilter,                 L"");
    CMN_IMPL_PROPERTY(ConnectData, AliasFilterOperation,        0); // contains
    CMN_IMPL_PROPERTY(ConnectData, UsageCounterFilter,          L"");
    CMN_IMPL_PROPERTY(ConnectData, UsageCounterFilterOperation, 1); // starts with
    CMN_IMPL_PROPERTY(ConnectData, LastUsageFilter,             L"");
    CMN_IMPL_PROPERTY(ConnectData, LastUsageFilterOperation,    1); // starts with

    bool is_ieql (const wstring& s1, const wstring& s2)
    {
        return !wcsicmp(s1.c_str(), s2.c_str());
    }

bool ConnectEntry::IsSignatureEql (const ConnectEntry& left, const ConnectEntry& right)
{
    if (left.GetDirect() == right.GetDirect()) 
    {
        if (!left.GetDirect()) {
            if (is_ieql(left.GetUser(), right.GetUser()) 
            && is_ieql(left.GetAlias(), right.GetAlias()))
                return true;
        } else {
            if (is_ieql(left.GetUser(), right.GetUser()) 
            && is_ieql(left.GetHost(), right.GetHost())
            && is_ieql(left.GetPort(), right.GetPort())
            && is_ieql(left.GetSid(), right.GetSid()))
                return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
ConnectData::ConnectData ()
{
    CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
}

void ConnectData::Clear ()                             
{ 
    CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
    m_entries.clear(); 
}

int ConnectData::Find (const ConnectEntry& entry)
{
    std::vector<ConnectEntry>::iterator it = m_entries.begin();

    for (int inx = 0; it != m_entries.end(); ++it, ++inx)
        if (ConnectEntry::IsSignatureEql(*it, entry))
            return inx;

    return -1;
}

void ConnectData::GetConnectEntry (int row, ConnectEntry& entry, const wstring& password)
{
    entry = m_entries.at(row);
    string buff;
    Decrypt(Common::str(password), entry.m_Password, buff);
    entry.m_DecryptedPassword = Common::wstr(buff);
}

void ConnectData::AppendConnectEntry (const ConnectEntry& _entry, const wstring& password)
{
    m_entries.push_back(_entry);
    ConnectEntry& entry = m_entries.back();
    entry.SetLastUsage(time(0));
    entry.SetUsageCounter(1);
    
    if (m_SavePasswords)
        Encrypt(Common::str(password), Common::str(entry.m_DecryptedPassword), entry.m_Password);
    else
        entry.m_Password.clear();

    entry.m_status = ConnectEntry::MODIFIED;
}

void ConnectData::SetConnectEntry (int row, const ConnectEntry& _entry, const wstring& password)
{
    ConnectEntry& entry = m_entries.at(row);
    int usageCounter = entry.GetUsageCounter() + 1;
    entry = _entry;
    entry.SetLastUsage(time(0));
    entry.SetUsageCounter(usageCounter);
    
    if (m_SavePasswords)
        Encrypt(Common::str(password), Common::str(entry.m_DecryptedPassword), entry.m_Password);
    else
        entry.m_Password.clear();

    entry.m_status = ConnectEntry::MODIFIED;
}

void ConnectData::MarkedDeleted (int row)
{
    m_entries.at(row).m_status = ConnectEntry::DELETED;
}

void ConnectData::ClearPasswords ()
{
    std::vector<ConnectEntry>::iterator it = m_entries.begin();
    for (; it != m_entries.end(); ++it)
    {
        if (!it->m_Password.empty())
        {
            it->m_status = ConnectEntry::MODIFIED;
            it->m_Password.clear();
        }
    }
}

void ConnectData::ResetPasswordsAndScriptOnLogin (const std::wstring& password)
{
    m_Probe = PROBE;
    m_ScriptOnLogin.clear();
    m_DecScriptOnLogin.clear();

    std::vector<ConnectEntry>::iterator it = m_entries.begin();
    for (; it != m_entries.end(); ++it)
    {
        it->m_Password.clear();
        it->m_DecryptedPassword.clear();
    }

    EncryptAll(Common::str(password));
}

void ConnectData::ChangePassword (const std::wstring& oldPassword, const std::wstring& newPassword)
{
    DecryptAll(Common::str(oldPassword));
    EncryptAll(Common::str(newPassword));
}

bool ConnectData::TestPassword (const std::wstring& password) const
{
    try
    {
        string dummy;
        Decrypt(Common::str(password), m_Probe, dummy);
        return true;
    }
    catch (const InvalidMasterPasswordException&) {}

    return false;
}

void ConnectData::EncryptAll (const string& password)
{
    ASSERT(m_Probe == PROBE);

    Encrypt(password, m_Probe.c_str(), m_Probe);
    Encrypt(password, Common::str(m_DecScriptOnLogin), m_ScriptOnLogin);

    std::vector<ConnectEntry>::iterator it = m_entries.begin();

    for (; it != m_entries.end(); ++it)
    {
        it->m_status = ConnectEntry::MODIFIED;
        
        if (m_SavePasswords) 
            Encrypt(password, Common::str(it->m_DecryptedPassword), it->m_Password);
        else
            it->m_Password.clear();
    }
}

void ConnectData::DecryptAll (const string& password)
{
    ASSERT(m_Probe != PROBE);

    string buff;
    Decrypt(password, m_Probe.c_str(), m_Probe);
    Decrypt(password, m_ScriptOnLogin, buff);
    m_DecScriptOnLogin = Common::wstr(buff);

    ASSERT(m_Probe == PROBE);

    std::vector<ConnectEntry>::iterator it = m_entries.begin();

    for (; it != m_entries.end(); ++it)
    {
        it->m_status = ConnectEntry::MODIFIED;
        Decrypt(password, it->m_Password, buff);
        it->m_DecryptedPassword = Common::wstr(buff);
    }
}
