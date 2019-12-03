/*
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2014 Aleksey Kochetov

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
#include <sstream>
#include <iomanip>
#include "resource.h"
#include "ExplainPlanSource.h"
#include "COMMON\StrHelpers.h"

using namespace OG2;

    static struct 
    {
        const char* name;
        int width;
        EColumnAlignment alignment;
    }
    g_explain_plan_columns[] =
    {
        { "operation",      24, ecaLeft     },
        { "object_name",    12, ecaLeft     },
        { "object_alias",   12, ecaLeft     },
        { "rows",            4, ecaRight    },
        { "bytes",           4, ecaRight    },
        { "cost",            4, ecaRight    },
        { "time",            4, ecaRight    },
        { "part_start",      6, ecaRight    },
        { "part_stop",       6, ecaRight    },
        { "object_node",     8, ecaLeft     },
        { "other_tag",      12, ecaLeft     },
        { "predicates",      4, ecaLeft     },
    };

    CImageList ExplainPlanSource::m_imageList;

ExplainPlanSource::ExplainPlanSource ()
    : m_table("ExplainPlanSource::m_table"),
    m_lastRow(-1), m_lastIndex(-1)
{
    if (!m_imageList.m_hImageList)
        m_imageList.Create(IDB_PANE_ICONS, 16, 64, RGB(0, 255, 0));

    SetImageList(&m_imageList, FALSE);

    SetShowRowEnumeration(false);
    SetShowTransparentFixCol(true);
    SetFixSize(eoVert, edHorz, 2);

    int ncount = sizeof(g_explain_plan_columns)/sizeof(g_explain_plan_columns[0]);
    SetCols(ncount);
    for (int i = 0; i < ncount; ++i)
    {
        SetColHeader    (i,  g_explain_plan_columns[i].name);
        SetColCharWidth (i,  g_explain_plan_columns[i].width);
        SetColAlignment (i,  g_explain_plan_columns[i].alignment);
    }
}


ExplainPlanSource::~ExplainPlanSource ()
{
}

int ExplainPlanSource::RowToInx (int row) const
{
    if (row == m_lastRow)
        return m_lastIndex;

    Common::nvector<plan_table_record>::const_iterator it = m_table.begin();

    for (int i = 0, inx = 0; it != m_table.end(); ++it, ++inx)
    {
        if (it->display_visible)
        {
            if (row == i) 
            {
                m_lastRow = row;
                m_lastIndex = inx;
                return inx;
            }
            ++i;
        }
    }

    return -1;
}

void ExplainPlanSource::CalcRowNumbering ()
{
    m_lastRow = m_lastIndex = -1;

    Common::nvector<plan_table_record>::iterator it = m_table.begin();

    for (; it != m_table.end(); ++it)
        if (!it->display_visible) 
            it->display_visible = true;

    int mode = 0;
    int depth = 0;

    // 1) scan for a collapsed node
    // 2) scan for na ode with the same depth, making all nodes between "invisble"

    for (it = m_table.begin(); it != m_table.end(); ++it)
    {
        if (mode == 0)
        {
            if (!it->display_expanded)
            {
                depth = (int)it->depth;
                mode = 1;
            }
        }
        else
        {
            if (it->depth > depth)
                it->display_visible = false;
            else
                mode = 0;
        }
    }

//#ifdef _DEBUG
//    TRACE("ExplainPlanSource::CalcRowNumbering >>>\n");
//    it = m_table.begin();
//    for (; it != m_table.end(); ++it)
//        TRACE("Data = %d %s %d\n", (int)it->id, it->operation.c_str(), it->display_visible ? 1 : 0);
//
//    int ncount = GetCount(edVert);
//    for (int i = 0; i < ncount; ++i)
//        TRACE("Map = %d %d\n", i, RowToInx(i));
//
//    TRACE("ExplainPlanSource::CalcRowNumbering <<<\n");
//#endif
}

const plan_table_record& ExplainPlanSource::GetRow (int row) const
{
    return m_table.at(RowToInx(row));
}

    inline
    std::pair<std::string, std::string> make_pair (const char* name, const string& value)
    {
        return std::pair<std::string, std::string>(name, value);
    }

    inline
    std::pair<std::string, std::string> make_pair (const char* name, __int64 value)
    {
        std::string buffer;

        if (value != LLONG_MAX)
        {
            char buff[80];
            if (!_i64toa_s(value, buff, sizeof(buff), 10))
                buffer = buff;
        }

        return std::pair<std::string, std::string>(name, buffer);
    }

#define ADD_PROPERTY(prop) \
    { \
        std::pair<std::string, std::string> pair = make_pair(#prop, rec.prop); \
        if (!pair.second.empty()) \
            properties.push_back(pair); \
    }
        

void ExplainPlanSource::GetProperties (int row, std::vector<std::pair<string, string> >& properties) const
{
    const plan_table_record& rec = GetRow(row);

    if (!row)
    {
    ADD_PROPERTY(statement_id     ); 
    ADD_PROPERTY(plan_id          ); 
    ADD_PROPERTY(timestamp        ); 
    }
    ADD_PROPERTY(remarks          ); 
    ADD_PROPERTY(operation        ); 
    ADD_PROPERTY(options          ); 
    ADD_PROPERTY(object_node      ); 
    ADD_PROPERTY(object_owner     ); 
    ADD_PROPERTY(object_name      ); 
    ADD_PROPERTY(object_alias     ); 
    ADD_PROPERTY(object_instance  ); 
    ADD_PROPERTY(object_type      ); 
    ADD_PROPERTY(optimizer        ); 
    ADD_PROPERTY(search_columns   ); 
    ADD_PROPERTY(id               ); 
    ADD_PROPERTY(parent_id        ); 
    ADD_PROPERTY(depth            ); 
    ADD_PROPERTY(position         ); 
    ADD_PROPERTY(cost             ); 
    ADD_PROPERTY(cardinality      ); 
    ADD_PROPERTY(bytes            ); 
    ADD_PROPERTY(other_tag        ); 
    ADD_PROPERTY(partition_start  ); 
    ADD_PROPERTY(partition_stop   ); 
    ADD_PROPERTY(partition_id     ); 
    ADD_PROPERTY(other            ); 
    ADD_PROPERTY(other_xml        ); 
    ADD_PROPERTY(distribution     ); 
    ADD_PROPERTY(cpu_cost         ); 
    ADD_PROPERTY(io_cost          ); 
    ADD_PROPERTY(temp_space       ); 
    ADD_PROPERTY(access_predicates); 
    ADD_PROPERTY(filter_predicates); 
    ADD_PROPERTY(projection       ); 
    ADD_PROPERTY(time             ); 
    ADD_PROPERTY(qblock_name      ); 

}

void ExplainPlanSource::Append (const plan_table_record& rec) 
{ 
    if (!m_table.empty())
    {
        plan_table_record& last = *m_table.rbegin();
        last.display_children = (last.depth == rec.depth - 1) ? true : false;
    }
    m_table.push_back(rec); 
    m_table.rbegin()->display_children = false;
}

void ExplainPlanSource::Toggle (int row)
{
    int inx = RowToInx(row);

    if (m_table.at(inx).display_children)
    {
        int rows = ExplainPlanSource::GetCount(edVert);
        m_table.at(inx).display_expanded = !m_table.at(inx).display_expanded;

        CalcRowNumbering();

        Notify_ChangedRowsQuantity(row, -(rows - ExplainPlanSource::GetCount(edVert)));
    }
}

void ExplainPlanSource::Clear ()
{
    m_table.clear();
    SetEmpty(true);
}

int ExplainPlanSource::GetCount (OG2::eDirection dir) const
{
    if (dir == edVert) 
    {
        int ncount = 0;
        Common::nvector<plan_table_record>::const_iterator it = m_table.begin();

        for (; it != m_table.end(); ++it)
            if (it->display_visible) 
                ncount++;

        return ncount;
    } 
    
    return GetColumnCount();
}

int ExplainPlanSource::GetImageIndex (int row) const          
{ 
    int inx = RowToInx(row);
    return m_table.at(inx).display_children ? (m_table.at(inx).display_expanded ? 1 : 0) : -1; 
}

void ExplainPlanSource::SetImageIndex (int /*row*/, int /*image*/)     
{ 
}

    static
    void print_with_indent (string& buffer, const string& val, __int64 indent)
    {
        buffer = string((int)indent * 2, ' ');
        buffer += val;
    }

    static
    void print_number (string& buffer, __int64 val)
    {
        if (val != LLONG_MAX)
        {
            const char* suffix = 0;
            const char* suffixes[] = { " K", " M", " G", " T" };
       
            for (int i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i)
                if (val >= 1024)
                {
                    suffix = suffixes[i];
                    val /= 1024;
                }
                else
                    break;

            char buff[80];
            if (!_i64toa_s(val, buff, sizeof(buff), 10))
            {
                buffer = buff;
                if (suffix)
                    buffer += suffix;
            }
        }
        else
            buffer.clear();
    }

    static
    void print_time (string& buffer, __int64 seconds)
    {
        if (seconds != LLONG_MAX)
        {
            using namespace std;
            ostringstream out;

            int n_hours = int(seconds)/3600;
            int n_minutes = (int(seconds) - n_hours*3600)/60;
            int n_seconds = int(seconds) - n_hours*3600 - n_minutes*60;

            if (n_hours > 0)
                out << setw(2) << setfill('0') << n_hours
                    << ':' << setw(2) << setfill('0') << n_minutes
                    << ':' << setw(2) << setfill('0') << n_seconds;
            else if (n_minutes > 0)
                out << setw(2) << setfill('0') << n_minutes
                    << ':' << setw(2) << setfill('0') << n_seconds;
            else
                out << "00"
                    << ':' << setw(2) << setfill('0') << n_seconds;

            buffer = out.str();
        }
        else
            buffer.clear();
    }


void ExplainPlanSource::GetCellStr (string& str, int _row, int col) const     
{
    int row = RowToInx(_row);

    if (row < (int)m_table.size() && col < GetColumnCount())
    {
        switch(col)
        {
        case 0 :
            print_with_indent(str, m_table.at(row).operation, m_table.at(row).depth);
            if (!m_table.at(row).options.empty())
            {
                string buffer;
                Common::to_lower_str(m_table.at(row).options.c_str(), buffer);
                str += ' '; str += '['; str += buffer; str += ']';
            }
            return;
        case 1 : str = m_table.at(row).object_name;
            return;
        case 2 : str = m_table.at(row).object_alias;
            return;
        case 3 : print_number(str, m_table.at(row).cardinality); 
            return;
        case 4 : print_number(str, m_table.at(row).bytes);       
            return;
        case 5 : print_number(str, m_table.at(row).cost);
            return;
        case 6 : print_time(str, m_table.at(row).time);
            return;
        case 7 : str = m_table.at(row).partition_start;
            return;
        case 8 : str = m_table.at(row).partition_stop;
            return;
        case 9 : str = m_table.at(row).object_node;
            return;
        case 10: Common::to_lower_str(m_table.at(row).other_tag.c_str(), str); 
            return;
        case 11: str = !m_table.at(row).access_predicates.empty() 
                ? m_table.at(row).access_predicates : m_table.at(row).filter_predicates;
            return;
        }
    }

    str.clear();
}

int ExplainPlanSource::ExportText (std::ostream& out, int /*row = -1*/, int /*nrows = -1*/, int /*col = -1*/, int /*ncols = -1*/, int /*format = -1*/) const
{
    int nlines   = GetCount(edVert);
    int ncolumns = GetCount(edHorz);

    string headers;
    vector<string> lines(nlines), last(nlines);

    for (int col = 0; col != ncolumns; ++col)
    {
        int width = GetColHeader(col).size();

        for (int line = 0; line != nlines; ++line)
        {
            string buff;
            GetCellStr(buff, line, col);
            last.at(line) = buff;
            if ((int)buff.size() > width) 
                width = buff.size();
        }

        if (GetColAlignment(col) == ecaLeft)
        {
            string header = GetColHeader(col);
            header.resize(width + 1, ' ');
            headers += header;

            for (int line = 0; line != nlines; ++line)
                last.at(line).resize(width + 1, ' ');
        }
        else
        {
            string header = GetColHeader(col);
            header = string(width - header.size(), ' ') + header + ' ';
            headers += header;

            for (int line = 0; line != nlines; ++line)
                last.at(line) = string(width - last.at(line).size(), ' ') + last.at(line) + ' ';
        }

        for (int line = 0; line != nlines; ++line)
            lines.at(line).append(last.at(line));
    }

    out << headers << endl;
    for (int line = 0; line != nlines; ++line)
        out << lines.at(line) << endl;

    return etfPlainText;
}


/*
1) * disable row rezising
2) *! autofit for columns
3) * colaplse / expand on dbl click (icons)
4) -- icons for oper type
5) * export data in text
6) save and open in xml files
7) popup detail window and other_xml 
8) remove table name from alias
9) ctrl+left & ctrl+right to toggle
*/

/*
add font / color settings
#include <afxpropertygridctrl.h>
*/