#include "stdafx.h"
#include "ErrorLoader.h"
#include <OCI8/BCursor.h>
#include "ServerBackgroundThread\TaskQueue.h"

#define BOOST_REGEX_NO_LIB
#define BOOST_REGEX_STATIC_LINK
#include <boost/regex.hpp>
using namespace boost;

namespace ErrorLoader
{

    const int cn_err_line = 0;
    const int cn_err_pos  = 1;
    const int cn_err_text = 2;

    LPSCSTR err_sttm =
        "SELECT line, position, text FROM sys.all_errors"
            " WHERE name = :name AND type = :type"
            " AND owner = :owner";

    LPSCSTR def_schema_err_sttm =
        "SELECT line, position, text FROM sys.all_errors"
            " WHERE name = :name AND type = :type"
            " AND owner = SYS_CONTEXT('USERENV', 'CURRENT_SCHEMA')";

    LPSCSTR legacy_def_schema_err_sttm =
        "SELECT line, position, text FROM user_errors"
            " WHERE name = :name AND type = :type";

    static void adjust_java_errors (vector<comp_error>& errors, const char* name, int line)
    {
        string line_pattern;
        line_pattern += '^';
        line_pattern += name; 
        line_pattern += ":([0-9]+):";
        regex_t line_regex;
        regcomp(&line_regex, line_pattern.c_str(), REG_EXTENDED);

        string pos_pattern("^[ ]*(\\^)[ ]*$");
        regex_t pos_regex;
        regcomp(&pos_regex, pos_pattern.c_str(), REG_EXTENDED);

        vector<comp_error>::iterator it = errors.begin();
        vector<comp_error>::iterator seg_line_it = errors.end();

        for (; it != errors.end(); ++it)
        {
            regmatch_t match[2];
            memset(match, 0, sizeof(match));
            match[0].rm_eo = it->text.size();    // to

            if (!regexec(&line_regex, it->text.c_str(), sizeof(match)/sizeof(match[0]), match, REG_STARTEND))
            {
                string n;
                if (match[1].rm_so != match[1].rm_eo)
                    n = it->text.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so); 
                if (!n.empty())
                {
                    seg_line_it = it;
                    seg_line_it->line = atoi(n.c_str());
                }
            }

            memset(match, 0, sizeof(match));
            match[0].rm_eo = it->text.size();    // to
            if (!regexec(&pos_regex, it->text.c_str(), sizeof(match)/sizeof(match[0]), match, REG_STARTEND))
            {
                if (match[1].rm_so != match[1].rm_eo)
                {
                    if (seg_line_it != errors.end())
                    {
                        seg_line_it->position = match[1].rm_so;
                        seg_line_it = errors.end();
                    }
                }
            }
        }

        for (it = errors.begin(); it != errors.end(); ++it)
        {
            if (it->line) 
            {
                it->line += line;
            }
            else
            {
                it->line = -1;
                it->position = -1;
                it->skip = true;
            }
        }
    }

    static void adjust_errors (vector<comp_error>& errors, int line)
    {
        vector<comp_error>::iterator it = errors.begin();
        for (; it != errors.end(); ++it)
        {
            it->line += line - 1;
            it->position -= 1;
        }
    }

int Load (DocumentProxy& proxy, OciConnect& connect, const char* owner, const char* name, const char* type, int line)
{
    bool def_schema_err = !owner || !owner[0] || !strlen(owner);

    OciCursor err_curs(connect,
        def_schema_err
            ? ((connect.GetVersion() < OCI8::esvServer81X) ? legacy_def_schema_err_sttm : def_schema_err_sttm)
            : err_sttm);

    if (!def_schema_err) err_curs.Bind(":owner", owner);
    err_curs.Bind(":name", name);
    err_curs.Bind(":type", type);

    err_curs.Execute();
    vector<comp_error> errors;
    while (err_curs.Fetch())
    {
        comp_error error;
        error.line = err_curs.ToInt(cn_err_line);
        error.position = err_curs.ToInt(cn_err_pos);
        err_curs.GetString(cn_err_text, error.text);
        errors.push_back(error);
    }

    if (!strcmp(type, "JAVA SOURCE"))
        adjust_java_errors(errors, name, line);
    else
        adjust_errors(errors, line);

    proxy.PutErrors(errors);

    return errors.size();
}


    using namespace ServerBackgroundThread;

	struct LoadErrorsTask : Task
    {
        DocumentProxy m_proxy;
        string m_owner, m_name, m_type;
        int m_line;

        LoadErrorsTask (DocumentProxy& proxy, const char* owner, const char* name, const char* type, int line)
            : Task("Load errors", 0),
            m_proxy(proxy), m_owner(owner), m_name(name), m_type(type), m_line(line)
        {
        }

        void DoInBackground (OciConnect& connect)
        {
            Load(m_proxy, connect, m_owner.c_str(), m_name.c_str(), m_type.c_str(), m_line);
        }

        void ReturnInForeground ()
        {
        }
    };

void SubmitLoadTask (DocumentProxy& proxy, const char* owner, const char* name, const char* type, int line)
{
    FrgdRequestQueue::Get().Push(TaskPtr(new LoadErrorsTask(proxy, owner, name, type, line)));
}

}//namespace ErrorLoader