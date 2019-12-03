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

// 16.03.2004 bug fix, command line parser fails on sysdba/sysoper
// 26.12.2004 (ak) old style streams replaced with new
// 03.01.2005 (ak) added static method CCommandLineParser::GetConnectionInfo
// 09.01.2005 (ak) bug fix: GetConnectionInfo does not handle "/@alias..."

#include "StdAfx.h"
#include "COMMON/AppUtilities.h"
#include "OpenEditor/OEDocument.h"
#include "CommandLineparser.h"

using std::string;

void CCommandLineParser::Clear ()
{
    m_error = false;
    m_errorPos = -1;

    m_connect = false;
    m_nolog = false;
    m_start = false;
    m_reuse = false;
    m_help  = false;

    m_errorMessage.clear();
    m_connectStr.clear();
    m_files.clear();
}

void CCommandLineParser::SetStartingDefaults ()
{
    if (!m_connect)
        m_connect = !m_nolog && m_files.empty();

    if (m_files.empty()
    && COEDocument::GetSettingsManager().GetGlobalSettings()->GetNewDocOnStartup())
        m_files.push_back("");
}

bool CCommandLineParser::check_option (const char* str, const char* option)
{
    for (int offset = 0; str[offset]; offset++)
        ;
    return !strnicmp(str, option, offset) ? true : false;
}

bool CCommandLineParser::check_option (const char* str, const char* option, string& arg)
{
    for (int offset = 0; str[offset] && str[offset] != '='; offset++)
        ;

    if (!strnicmp(str, option, offset))
    {
        if (str[offset] == '=')
            arg = str + offset + 1;

        return true;
    }
    return false;
}

bool CCommandLineParser::Parse ()
{
    for (int pos = 1; !m_error && pos < __argc; pos++)
    {
        const char* str = __argv[pos];

        switch (str[0])
        {
        case '/':
        case '-':
            switch (str[1])
            {
            case 'C':
            case 'c':
                m_connect = check_option(str+1, "connect", m_connectStr);
                m_error = !m_connect;
                break;
            case 'N':
            case 'n':
                m_nolog = check_option(str+1, "nolog");
                m_error = !m_nolog;
                break;

            case 'S':
            case 's':
                m_start = check_option(str+1, "start");
                m_error = !m_start;
                break;

            case 'R':
            case 'r':
                m_reuse = check_option(str+1, "reuse");
                m_error = !m_reuse;
                break;

            case 'H':
            case 'h':
                m_help = check_option(str+1, "help");
                m_error = !m_help;
                break;
            case '?':
                m_help = true;
                break;

            default:
                m_error = true;
            }

            if (m_error)
            {
                m_errorPos = pos;
                break;
            }
            break;

        default:
            m_files.push_back(str);
        }
    }

    if (m_error)
    {
        m_errorMessage = string("Invalid option \"") + __argv[m_errorPos] + "\".";
    }
    else
    {
        if (m_connect && m_nolog)
        {
            m_error = true;
            m_errorMessage = "You cannot use both \"NOLOG\" or \"CONNECT\" options at the same time.";
        }
        else if (m_start && m_reuse)
        {
            m_error = true;
            m_errorMessage = "You cannot use both \"START\" or \"REUSE\" options at the same time.";
        }
    }

    if (m_error)
    {
        AfxMessageBox((string("Command line error: \n\t") + m_errorMessage).c_str());
    }
    else if (m_help)
    {
        AfxMessageBox(
            "SQLTools command line options:\n"
            "\n"
            "sqltools [/h[elp]]\n"
            "\n"
                "\thelp\t\t- show this help\n"
            "\n"
            "sqltools [{/n[olog]}|{/c[onnect]=<connection>}] [{/s[tart]}|{/r[euse]}] <file_list>    \n"
            "\n"
                "\tnolog\t\t- don't try connect or show connection dialog\n"
                "\tconnect\t\t- connect using <connection> string\n"
                "\t<connection>\t- user/password@{tnsalias}|{host:port:sid}|{host:port/service}[@{sysdba}|{sysoper}]\n"
                "\tstart\t\t- start a new instance of the program\n"
                "\treuse\t\t- reuse already running instance of the program\n"
                "\t<file_list>\t- a space-separated list of files\n",
            MB_OK|MB_ICONINFORMATION
            );
    }

    return !(m_error || m_help);
}

void CCommandLineParser::Process ()
{
    if (!(m_error || m_help))
    {
        std::vector<string>::const_iterator it = m_files.begin();

        for (; it != m_files.end(); ++it)
            if (!it->empty())
                AfxGetApp()->OpenDocumentFile(it->c_str());
            else
                AfxGetApp()->OnCmdMsg(ID_FILE_NEW, 0, 0, 0);
    }
}

void CCommandLineParser::Pack (string& data) const
{
    std::ostringstream out;

    if (GetConnectOption())
    {

        if (!m_connectStr.empty())
            out << m_connectStr << std::endl;
        else
            out << "/\n";
    }
    else
        out << std::endl; // no connectin ingormation

    std::vector<string>::const_iterator it = m_files.begin();
    for (; it != m_files.end(); ++it)
    {
        string path;

        if (!it->empty())
            Common::AppGetFullPathName(*it, path);

        out << path << std::endl;
    }

    out << ends;

    data = out.str();
}

void CCommandLineParser::Unpack (const char* data, int len)
{
    Clear();

    std::istringstream in(data, len);

    std::getline(in, m_connectStr);

    if (!m_connectStr.empty())
    {
        m_connect = true;
        if (m_connectStr == "/")
            m_connectStr.clear();
    }

    string file;
    while (std::getline(in, file))
        m_files.push_back(file);
}

bool CCommandLineParser::GetConnectionInfo (
    const string& connectStr,
    string& user, string& password, string& alias,
    string& port, string& sid, string& mode, 
    string& service
)
{
    enum { eUser, ePassword, eAlias, ePort, eSid, eMode, eService } state = eUser;

    for (string::const_iterator it = connectStr.begin(); it != connectStr.end(); ++it)
    {
        switch (state)
        {
        case eUser:
            switch (*it)
            {
            case '/': state = ePassword; break;
            case '@': state = eAlias; break;
            default:  user += *it;
            }
            break;
        case ePassword:
            if (*it == '@') state = eAlias;
            else password += *it;
            break;
        case eAlias:
            if (*it == ':') state = ePort;
            else if (*it == '@') state = eMode;
            else alias += *it;
            break;
        case ePort:
            switch (*it)
            {
            case ':': state = eSid; break;
            case '/': state = eService; break;
            default: port += *it;
            }
            break;
        case eSid:
            if (*it == '@') state = eMode; // 16.03.2004 bug fix, command line parser fails on sysdba/sysoper
            else sid += *it;
            break;
        case eService:
            if (*it == '@') state = eMode;
            else service += *it;
            break;
        case eMode:
            mode += *it;
            break;
        }
    }

    return !user.empty() || !alias.empty();
}
