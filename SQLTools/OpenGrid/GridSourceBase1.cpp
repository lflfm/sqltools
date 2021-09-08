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

/***************************************************************************/
/*      Purpose: Text data source implementation for grid component        */
/*      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~           */
/*      (c) 1996-1999,2003 Aleksey Kochetov                                */
/***************************************************************************/

// 27.10.2003 bug fix, html export does not handle empty strings (with spaces) properly
// 07.11.2003 bug fix, wrong order of html tags for html export
// 16.12.2004 (Ken Clubok) Add CSV prefix option
// 22.12.2004 (Ken Clubok) Bug 1078884: XML Export does not handle column names with illegal characters
// 02.01.2004 (Ken Clubok) Bug 1092516: Copy column from datagrid gives extra delimiter
// 03.01.2005 (ak) Possible problem: a number column can contain ',' as a separator
// 2017-11-28 implemented range selection
// 2017-11-28 added 3 new export formats
// 2017-12-27 added INSERT format, modified XML format

#include "stdafx.h"
#include <sstream>
#include <COMMON\SimpleDragDataSource.h>         // for Drag & Drop
#include <COMMON\StrHelpers.h>
#include "GridSourceBase.h"
#include "CellEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace OG2
{

///////////////////////////////////////////////////////////////////////////////
/** @class Streamer
 * @brief generic class for exporting grid data.
 *
 * Subclasses include specifics for generating plaintext, CSV, tabbed, HTML, or XML output.
 */
    class Streamer
    {
    public:
        Streamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);
        virtual void Write (std::ostream& of) const;

    protected:
        virtual void writeHeader (std::ostream&) const   {};
        virtual void writeRow    (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;
        virtual void writeFooter (std::ostream&) const   {};

        const string  getColHeader (int col) const              { return m_source.GetColHeader(col); }
        void getCellStr (string& str, int row, int col) const   { m_source.GetCellStrForExport(str, row, col, m_subformat); }

        const GridStringSource& m_source;
        int m_subformat;
        int m_startColumn, m_colCount, m_startRow, m_rowCount;
        bool m_IsTableOrientation /**< True if rows in grid are database records; false if grid is transposed. */;
    };


/** @fn Streamer::Streamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : m_source(source)
 * @brief Constructor for Streamer.
 *
 * @arg source Grid from which we will export data.
 * @arg startRow Row to export.  If -1, then export all rows.
 * @arg startCol Column to export.  If -1, then export all columns.
 */
Streamer::Streamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : m_source(source), m_subformat(0)
{
    m_IsTableOrientation = m_source.IsTableOrientation();

    if (startCol != -1) {
        m_startColumn = startCol;
        m_colCount = colCount;
    } else {
        m_startColumn = 0;
        m_colCount = m_source.GetCount(edHorz);
    }

    if (startRow != -1) {
        m_startRow = startRow;
        m_rowCount = rowCount;
    } else {
        m_startRow = 0;
        m_rowCount = m_source.GetCount(edVert);
    }
}

/** void Streamer::Write (std::ostream& of) const
 * @brief Exports data from grid.
 *
 * @arg of Output stream.
 */
void Streamer::Write (std::ostream& of) const
{
    writeHeader(of);

    for (int row = m_startRow; row < m_startRow + m_rowCount; row++) 
        writeRow(of, row);

    writeFooter(of);
}

/** void Streamer::WriteRow (std::ostream& of, int row) const
 * @brief Writes data from one row of grid.
 *
 * @arg of Output stream.
 * @arg row Row to write.
 */
void Streamer::writeRow (std::ostream& of, int row) const 
{
    for (int col = m_startColumn; col < m_startColumn + m_colCount; col++) 
        writeCell(of, row, col);
}

/** void Streamer::WriteRow (std::ostream& of, int row) const
 * @brief Writes data from one cell of grid.
 *
 * @arg of Output stream.
 * @arg row Row of cell to write.
 * @arg col column of cell to write.
 */
void Streamer::writeCell (std::ostream& of, int row, int col) const
{
    string str;
    getCellStr(str, row, col);
    of << str;
}

///////////////////////////////////////////////////////////////////////////////
/** @class PlainTextStreamer
 * @brief Class for exporting grid data in plaintext format.
 */    
    class PlainTextStreamer : public Streamer
    {
    public:
        PlainTextStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);
    protected:
        virtual void writeHeader (std::ostream& of) const;
        virtual void writeRow    (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;
    private:
        void writeText (std::ostream& of, const string& str, int width, EColumnAlignment align) const;
        Common::nvector<size_t> m_maxColLen;
        size_t m_maxHeadersLen;
        bool m_OutputWithHeader, m_endingCR;
        char m_FieldDelimiterChar;
    };

PlainTextStreamer::PlainTextStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : Streamer (source, startRow, rowCount, startCol, colCount),
    m_maxColLen("PlainTextStreamer::m_maxColLen"),
    m_endingCR(colCount == -1 || colCount == m_source.GetCount(edHorz))
{ 
    if (startRow != -1)
        m_source.GetMaxColumnWidthByFirstLine(m_maxColLen, startRow, startRow + rowCount - 1); 
    else
        m_source.GetMaxColumnWidthByFirstLine(m_maxColLen, 0, std::numeric_limits<int>::max()); 

    m_maxHeadersLen = 0;
    m_OutputWithHeader = m_source.GetOutputWithHeader() && !(rowCount == 1 && colCount == 1); // don't want a header for a single value

    if (m_OutputWithHeader)
    {
        if (m_IsTableOrientation)
        {
            for (int i = 0; i < static_cast<int>(m_maxColLen.size()); i++)
                if (m_maxColLen[i] < m_source.GetColHeader(i).size())
                    m_maxColLen[i] = m_source.GetColHeader(i).size();
        }
        else
        {
            int ncount = m_source.GetColumnCount();
            for (int i = 0; i < ncount; i++)
            if (m_maxHeadersLen < m_source.GetColHeader(i).length())
                m_maxHeadersLen = m_source.GetColHeader(i).length();
        }
    }

    m_FieldDelimiterChar = ' ';
}

/** @fn void PlainTextStreamer::writeText (std::ostream& of, const string& str, int width, EColumnAlignment align) const
 * @brief Writes a string in plaintext format.
 *
 * @arg of Output stream.
 * @arg str String to write.
 * @arg width Width of column.  If the string is shorter, the field will be filled with spaces.
 * @arg align Specifies whether to right-justify or left-justify.
 */
void PlainTextStreamer::writeText (std::ostream& of, const string& str, int width, EColumnAlignment align) const
{
    if (align == ecaLeft)
        of << str;
    
    int nspaces = (width > static_cast<int>(str.length())) ? (width - str.length()) : 0;
    
    while (nspaces--) 
        of << m_FieldDelimiterChar;

    if (align == ecaRight)
        of << str;
}

/** @fn void PlainTextStreamer::writeHeader (std::ostream& of) const
 * @brief Writes the header row in plaintext format - only if grid has "normal" orientation, and header output option is on.
 *
 * @arg of Output stream.
 */
void PlainTextStreamer::writeHeader (std::ostream& of) const
{
    if (m_OutputWithHeader && m_IsTableOrientation)
    {
        for (int col = m_startColumn; col < m_startColumn + m_colCount; col++)
        {
            if (col != m_startColumn) of << m_FieldDelimiterChar;
            writeText(of, getColHeader(col), m_maxColLen[col], m_source.GetColAlignment(col));
        }
        of << '\n';
    }
}

/** @fn void PlainTextStreamer::writeRow (std::ostream& of, int row) const
 * @brief Writes a single row in plaintext format.
 *
 * @arg of Output stream.
 * @arg row Row to write.
 */
void PlainTextStreamer::writeRow (std::ostream& of, int row) const
{
    if (m_OutputWithHeader && !m_IsTableOrientation)
    {                                                         
        writeText(of, getColHeader(row), m_maxHeadersLen, ecaLeft);
        of << m_FieldDelimiterChar;
    }
    
    Streamer::writeRow(of, row);

    if (m_endingCR || ( row < m_startRow + m_rowCount - 1))
        of << '\n';
}

/** @fn void PlainTextStreamer::writeCell (std::ostream& of, int row, int col) const
 * @brief Writes a single cell in plaintext format.
 *
 * @arg of Output stream.
 * @arg row Row of cell to write.
 * @arg col Column of cell to write.
 */
void PlainTextStreamer::writeCell (std::ostream& of, int row, int col) const
{
    if (col != m_startColumn) 
        of << m_FieldDelimiterChar;

    string str;
    getCellStr(str, row, col);
    writeText(of, str, m_maxColLen[col], m_source.GetColAlignment(m_IsTableOrientation ?  col :  row));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
    class PlSqlStreamer1 : public Streamer
    {
    public:
        PlSqlStreamer1 (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
            : Streamer (source, startRow, rowCount, startCol, colCount)
        { 
            m_subformat = 1;
        }
    protected:
        virtual void writeCell   (std::ostream& of, int row, int col) const
        {
            __super::writeCell(of, row, col);
            if (col < m_startColumn + m_colCount -1)
                of << ',' << ' ';
            else
                of << endl;
        }
    };

    class PlSqlStreamer2 : public Streamer
    {
    public:
        PlSqlStreamer2 (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
            : Streamer (source, startRow, rowCount, startCol, colCount)
        { 
            m_subformat = 2;
        }
    protected:
        virtual void writeRow (std::ostream& of, int row) const
        {
            if (m_IsTableOrientation)
                __super::writeRow(of, row);
            else
            {
                if (m_colCount > 1)
                {
                    of << m_source.GetColHeader(row) << " IN (";
                    __super::writeRow(of, row);
                    of << ")" << endl;
                }
                else
                {
                    string value;
                    getCellStr(value, row, m_startColumn);
                    of << m_source.GetColHeader(row) << (value == "NULL" ? " IS " : " = ")
                      << value << endl;
                }
            }
        }

        virtual void writeCell (std::ostream& of, int row, int col) const
        {
            if (m_IsTableOrientation)
            {
                string value;
                getCellStr(value, row, col);

                of << m_source.GetColHeader(col) << (value == "NULL" ? " IS " : " = ") << value;

                if (col < m_startColumn + m_colCount -1)
                    of << " AND ";
                else
                    of << endl;
            }
            else
            {
                __super::writeCell(of, row, col);

                if (col < m_startColumn + m_colCount -1)
                    of << ',' << ' ';
            }
        }
    };

    class PlSqlStreamer3 : public Streamer
    {
    public:
        PlSqlStreamer3 (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
            : Streamer (source, startRow, rowCount, startCol, colCount)
        { 
            m_subformat = 3;
        }
    protected:
        void writeHeader (std::ostream& of) const
        {
            if (m_IsTableOrientation)
            {
                if (m_colCount > 1) of << '(';
                for (int col = m_startColumn; col < m_startColumn + m_colCount; col++) 
                {
                    if (col != m_startColumn) of << ',' << ' ';
                    of << getColHeader(col);
                }
                if (m_colCount > 1) of << ')' << endl;
                else of << ' ';
                of << "IN" << endl << '(' << endl;
            }
        }
        void writeFooter (std::ostream& of) const
        {
            if (m_IsTableOrientation)
            {
                of << ")";
            }
        }

        virtual void writeCell (std::ostream& of, int row, int col) const
        {
            __super::writeCell(of, row, col);

            if (col < m_startColumn + m_colCount -1)
                of << ',' << ' ';
        }

        virtual void writeRow (std::ostream& of, int row) const
        {
            if (m_IsTableOrientation)
            {
                of << "  ";
                if (m_colCount > 1) of << '(';
                __super::writeRow(of, row);
                if (m_colCount > 1) of << ')';
                if (row < m_startRow + m_rowCount - 1) of << ',';
                of << endl;
            }
            else
            {
                of << "AND ";
                if (m_colCount > 1)
                {
                    of << m_source.GetColHeader(row) << " IN (";
                    __super::writeRow(of, row);
                    of << ")" << endl;
                }
                else
                {
                    string value;
                    getCellStr(value, row, m_startColumn);
                    of << m_source.GetColHeader(row) << (value == "NULL" ? " IS " : " = ")
                      << value << endl;
                }
            }
        }
    };

    class PlSqlStreamer4 : public Streamer
    {
    public:
        PlSqlStreamer4 (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
            : Streamer (source, startRow, rowCount, startCol, colCount)
        { 
            m_subformat = 4;

            if (!m_IsTableOrientation)
            {
                MessageBeep((UINT)-1);
                AfxMessageBox(L"INSERTs can be genereated only from normally-oriented data grid!", MB_OK|MB_ICONSTOP);
                AfxThrowUserException();
            }
        }
    protected:
        void writeHeader (std::ostream& of) const
        {
            if (m_IsTableOrientation)
            {
                of << "define table = " << endl;
            }
        }
        void writeFooter (std::ostream& of) const
        {
            if (m_IsTableOrientation)
            {
                of << "COMMIT;" << endl;
            }
        }

        virtual void writeCell (std::ostream& of, int row, int col) const
        {
            __super::writeCell(of, row, col);

            if (col < m_startColumn + m_colCount -1)
                of << ',' << ' ';
        }

        virtual void writeRow (std::ostream& of, int row) const
        {
            if (m_IsTableOrientation)
            {
                of << "INSERT INTO &table (";
                for (int col = m_startColumn; col < m_startColumn + m_colCount; col++) 
                {
                    if (col != m_startColumn) of << ',' << ' ';
                    of << getColHeader(col);
                }
                of << ')' << endl << "  VALUES(";
                __super::writeRow(of, row);
                of << ");" << endl;
            }
        }
    };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/** @class TabTextStreamer
 * @brief Class for exporting grid data in tab-delimited format.
 */  
    class TabTextStreamer : public Streamer
    {
    public:
        TabTextStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);
    protected:
        virtual void writeHeader (std::ostream& of) const;
        virtual void writeRow    (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;
    private:
        void writeFooter (std::ostream&) const;
        bool m_OutputWithHeader;
        char m_FieldDelimiterChar; /**< Set to tab character by constructor. */
    };

/** @fn TabTextStreamer::TabTextStreamer (const GridStringSource& source, int startRow, int startCol)
 * @brief Constructor for TabTextStreamer.
 *
 * @arg source See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selRow See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selCol See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 */
TabTextStreamer::TabTextStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : Streamer (source, startRow, rowCount, startCol, colCount)
{ 
    m_OutputWithHeader = m_source.GetOutputWithHeader() && !(rowCount == 1 && colCount == 1); // don't want a header for a single value
    m_FieldDelimiterChar = '\t';
}

/** @fn void TabTextStreamer::writeHeader (std::ostream& of) const
 * @brief Writes the header row in tab-delimited format - only if grid has "normal" orientation, and header output option is on.
 *
 * @arg of Output stream.
 */
void TabTextStreamer::writeHeader (std::ostream& of) const
{
    if (m_OutputWithHeader && m_IsTableOrientation)
    {
        for (int col = m_startColumn; col < m_startColumn + m_colCount; col++) 
        {
            if (col != m_startColumn) of << m_FieldDelimiterChar;
            of << getColHeader(col);
        }
        of << '\n';
    }
}

/** @fn void TabTextStreamer::writeRow (std::ostream& of, int row) const
 * @brief Writes a single row in tab-delimited format.
 *
 * @arg of Output stream.
 * @arg row Row to write.
 */
void TabTextStreamer::writeRow (std::ostream& of, int row) const
{
    if (m_OutputWithHeader && !m_IsTableOrientation)
    {
        of << getColHeader(row) << m_FieldDelimiterChar;
    }
    Streamer::writeRow(of, row);
    of << '\n';
}

void TabTextStreamer::writeFooter (std::ostream& of) const
{
/*
    remove trailing \n to avoid an extra empty line on insert in excel
*/
    std::ostream::pos_type pos = of.tellp();
    if (pos > 0)
    {
        of.seekp((unsigned long)pos - 1);
        of << ends;
    }
}

/** @fn void TabTextStreamer::writeCell (std::ostream& of, int row, int col) const
 * @brief Writes a single cell in tab-delimited format.
 *
 * @arg of Output stream.
 * @arg row Row of cell to write.
 * @arg col Column of cell to write.
 */
void TabTextStreamer::writeCell (std::ostream& of, int row, int col) const
{
    if (col != m_startColumn) of << m_FieldDelimiterChar;
    Streamer::writeCell(of, row, col);
}

///////////////////////////////////////////////////////////////////////////////
/** @class CSVTextStreamer
 * @brief Class for exporting grid data in CSV format.
 */
    class CsvTextStreamer : public Streamer
    {
    public:
        CsvTextStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);
    protected:
        virtual void writeHeader (std::ostream& of) const;
        virtual void writeRow    (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;
    private:
        void writeText (std::ostream& of, const string& str, boolean isText) const;
        void writeChar (std::ostream& of, const char c) const;
        bool m_OutputWithHeader;
        char m_FieldDelimiterChar;
        char m_QuoteChar;
        char m_QuoteEscapeChar;
        char m_PrefixChar;
        string m_requiredQuotation;
    };

/** @fn CsvTextStreamer::CsvTextStreamer (const GridStringSource& source, int startRow, int startCol)
 * @brief Constructor for CsvTextStreamer.
 *
 * @arg source See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selRow See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selCol See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 */
CsvTextStreamer::CsvTextStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : Streamer(source, startRow, rowCount, startCol, colCount),
    m_requiredQuotation("\r\n")
{ 
    m_OutputWithHeader   = m_source.GetOutputWithHeader() && !(rowCount == 1 && colCount == 1); // don't want a header for a single value
    m_FieldDelimiterChar = m_source.GetFieldDelimiterChar();
    m_QuoteChar          = m_source.GetQuoteChar();
    m_QuoteEscapeChar    = m_source.GetQuoteEscapeChar();
    m_PrefixChar         = m_source.GetPrefixChar();
    m_requiredQuotation += m_QuoteChar;
    m_requiredQuotation += m_FieldDelimiterChar;
}

/** @fn void CsvTextStreamer::writeText (std::ostream& of, const string& str, boolean isText) const
 * @brief Writes a string in CSV format.
 *
 * @arg of Output stream.
 * @arg str String to write.
 * @arg isText Determines whether string will be treated as text field - Do we add prefix and quote characters.
 */
void CsvTextStreamer::writeText (std::ostream& of, const string& str, boolean isText) const
{
    // a number column can contain ',' as a separator
    bool isQuotationRequired = isText || (str.find_first_of(m_requiredQuotation) != std::string::npos);

    if (isQuotationRequired)
    {
        if (str.find_first_of(m_FieldDelimiterChar) == std::string::npos
        && str.find_first_of('\n') == std::string::npos) // added to show multiline fields in Excel properly 
            writeChar(of, m_PrefixChar);

        writeChar(of, m_QuoteChar);
    }

    std::string::const_iterator it = str.begin();
    for (; it != str.end(); ++it)
    {
        if (*it == m_QuoteChar) 
            writeChar(of, m_QuoteEscapeChar);

        of << *it;
    }

    if (isQuotationRequired)
        writeChar(of, m_QuoteChar);
}

/** @fn void CsvTextStreamer::writeChar (std::ostream& of, const char c) const
 * @brief Writes one character, if not null.
 *
 * This is used for single-character parameters, that the user could set to be empty.
 *
 * @arg of Output stream.
 * @arg str String to write.
 */
void CsvTextStreamer::writeChar (std::ostream& of, const char c) const
{
    if (c == '\n')
        of << "\r\n";
    else if (c)
        of << c;
}

/** @fn void CsvTextStreamer::writeHeader (std::ostream& of) const
 * @brief Writes the header row in CSV format - only if grid has "normal" orientation, and header output option is on.
 *
 * @arg of Output stream.
 */
void CsvTextStreamer::writeHeader (std::ostream& of) const
{
    if (m_OutputWithHeader && m_IsTableOrientation)
    {
        for (int col = m_startColumn; col < m_startColumn + m_colCount; col++) 
        {
            if (col != m_startColumn) writeChar(of, m_FieldDelimiterChar);
            writeText(of, getColHeader(col), true);
        }
        of << '\n';
    }
}

/** @fn void CsvTextStreamer::writeRow (std::ostream& of, int row) const
 * @brief Writes a single row in CSV format.
 *
 * @arg of Output stream.
 * @arg row Row to write.
 */
void CsvTextStreamer::writeRow (std::ostream& of, int row) const
{
    if (m_OutputWithHeader && !m_IsTableOrientation)
    {
        writeText(of, getColHeader(row), true);
        writeChar(of, m_FieldDelimiterChar);
    }
    Streamer::writeRow(of, row);
    of << '\n';
}

/** @fn void CsvTextStreamer::writeCell (std::ostream& of, int row, int col) const
 * @brief Writes a single cell in CSV format.
 *
 * @arg of Output stream.
 * @arg row Row of cell to write.
 * @arg col Column of cell to write.
 */
void CsvTextStreamer::writeCell (std::ostream& of, int row, int col) const
{
    if (col != m_startColumn) 
        writeChar(of, m_FieldDelimiterChar);

    // Put in prefix character, so Excel doesn't drop leading zeros - but only for text fields
    boolean isText = m_IsTableOrientation ? m_source.IsTextField(col) : m_source.IsTextField(row);

    string str;
    getCellStr(str, row, col);
    writeText(of, str, isText);
}

///////////////////////////////////////////////////////////////////////////////
/** @class XMLStreamer
 * @brief Generic class for exporting grid data in XML format.
 *
 * Neither subclass behaves great when grid is transposed, in that the column names are not included
 * in the output.
 */
    class XmlStreamer : public Streamer
    {
    public:
        XmlStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);

    protected:
        virtual void writeHeader (std::ostream& of) const;
        virtual void writeFooter (std::ostream& of) const;

        void writeXMLString (std::ostream& of, const string& str) const;
        void writeName (std::ostream& of, const string& str) const;

    private:
        boolean IsNameStart (char c) const;
        boolean IsNameChar (char c) const;
        string m_XmlRootElement;
    };

XmlStreamer::XmlStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
: Streamer (source, startRow, rowCount, startCol, colCount) 
{
    m_XmlRootElement = m_source.GetXmlRootElement();
}

/** @fn void XmlStreamer::writeXMLString (std::ostream& of, const string& str) const
 * @brief Writes a string, making appropriate substitutions for special characters in XML.
 *
 * @arg of Output stream.
 * @arg str String to write.
 */
void XmlStreamer::writeXMLString (std::ostream& of, const string& str) const
{
    if (str.find_first_of( "&'\"<>") != std::string::npos)
    {
        std::string::const_iterator it = str.begin();
        for (; it != str.end(); ++it)
            switch (*it) {
            case '&':  of << "&amp;";  break;
            case '\'': of << "&apos;"; break;
            case '"':  of << "&quot;"; break;
            case '>':  of << "&gt;";   break;
            case '<':  of << "&lt;";   break;
            default:   of << *it;      break;
            }
    }
    else
        of << str;
}

/** @fn void XmlStreamer::writeHeader (std::ostream& of) const
 * @brief Writes XML Header.
 *
 * Output is enclosed by <QUERY> tag.
 *
 * @arg of Output stream.
 */
void XmlStreamer::writeHeader (std::ostream& of) const
{
    of << "<?xml version=\"1.0\"?>\n<" << m_XmlRootElement << ">\n";
}

/** @fn void XmlStreamer::writeFooter (std::ostream& of) const
 * @brief Writes XML Footer.
 *
 * @arg of Output stream.
 */
void XmlStreamer::writeFooter (std::ostream& of) const
{
    of << "</" << m_XmlRootElement << ">\n"; 
}

/** @fn inline boolean XmlStreamer::IsNameStart (char c) const
 * @brief Checks whether a character is valid as the first character of an XML attribute/element name.
 *
 * @arg c Character to check.
 */
inline boolean XmlStreamer::IsNameStart (char c) const
{
   return (isalpha(c) || c == '_');
}

/** @fn inline boolean XmlStreamer::IsNameChar (char c) const
 * @brief Checks whether a character is valid in an XML attribute/element name.
 *
 * @arg c Character to check.
 */

inline boolean XmlStreamer::IsNameChar (char c) const 
{
   return (isalnum(c) || c == '.' || c == '-' || c == '_');
}

/** @fn void XmlStreamer::writeName (std::ostream& of, const string& str) const
 * @brief Writes string as XML attribute/element name.
 *
 * Invalid characters are replaced by underscores.
 *
 * @arg of Output stream.
 * @arg str String to write.
 */
void XmlStreamer::writeName (std::ostream& of, const string& str) const
{
   std::string::const_iterator it = str.begin();
   if (IsNameStart(*it))
       of << *it;
   else
       of << "_";
   
   // 2017-11-19 bug fix, dereferenced iterator
   for (++it; it != str.end(); ++it)
   {
      if (IsNameChar(*it))
          of << *it;
      else
          of << "_";
   }
}

///////////////////////////////////////////////////////////////////////////////
/** @class XMLElemStreamer
 * @brief Class for exporting grid data in XML format, with values as elements.
 */
    class XmlElemStreamer : public XmlStreamer
    {
    public:
        XmlElemStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);
    protected:
        virtual void writeRow    (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;
    private:
        string m_XmlRecordElement;
        int    m_XmlFieldElementCase;
        vector<string> m_headers;
        string m_transposedHeader;
    };

/** @fn XmlElemStreamer::XmlElemStreamer (const GridStringSource& source, int startRow, int startCol)
 * @brief Constructor for XmlElemStreamer.
 *
 * @arg source See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selRow See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selCol See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 */
XmlElemStreamer::XmlElemStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : XmlStreamer(source, startRow, rowCount, startCol, colCount)
{ 
    m_XmlRecordElement    = m_source.GetXmlRecordElement   ();
    m_XmlFieldElementCase = m_source.GetXmlFieldElementCase();

    if (m_IsTableOrientation)
        for (int i = startCol; i < startCol + colCount; ++i)
        {
            string buff = getColHeader(i);
            switch (m_XmlFieldElementCase)
            {
            case 1:
                Common::to_lower_str(buff.c_str(), buff);
                break;
            case 2:
                Common::to_upper_str(buff.c_str(), buff);
                break;
            }
             m_headers.push_back(buff);
        }
    else
    {
        string buff = "Column-";
        switch (m_XmlFieldElementCase)
        {
        case 1:
            Common::to_lower_str(buff.c_str(), buff);
            break;
        case 2:
            Common::to_upper_str(buff.c_str(), buff);
            break;
        }
        m_transposedHeader = buff;
    }
}

/** @fn void XmlElemStreamer::writeRow (std::ostream& of, int row) const
 * @brief Writes a row of data in XML format.
 *
 * Output is enclosed by <REC> tag, with row number in attribute N.
 *
 * @arg of Output stream.
 * @arg row Row to write.
*/
void XmlElemStreamer::writeRow (std::ostream& of, int row) const
{
    of << "\t<" << m_XmlRecordElement << ">\n";
    Streamer::writeRow(of, row);
    of << "\t</" << m_XmlRecordElement << ">\n";
}

/** @fn void XmlElemStreamer::writeCell (std::ostream& of, int row) const
 * @brief Writes a cell of data in XML format.
 *
 * Depending on whether "Column Name as Attribute" is selected, tag is either the column name itself, 
 * or <COLUMN> with the name in attribute NAME.
 *
 * @arg of Output stream.
 * @arg row Row of cell to write.
 * @arg col Column of cell to write.
*/
void XmlElemStreamer::writeCell (std::ostream& of, int row, int col) const
{
    string str;
    getCellStr(str, row, col);

    if (!str.empty())
    {
        if (m_IsTableOrientation)
        {
            of << "\t\t<";
            writeName(of, m_headers.at(col - m_startColumn));
            of << '>';
        }
        else
            of << "\t\t<" << m_transposedHeader << col << '>';
        
        writeXMLString(of, str);

        if (m_IsTableOrientation)
        {
            of << "</";
            writeName(of, m_headers.at(col - m_startColumn));
            of << ">\n";
        }
        else
            of << "</" << m_transposedHeader << col << ">\n";
    }
}

///////////////////////////////////////////////////////////////////////////////
/** @class XMLAttrStreamer
 * @brief Class for exporting grid data in XML format, with values as attributes.
 */
    class XmlAttrStreamer : public XmlStreamer
    {
    public:
        XmlAttrStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount);
    protected:
        virtual void writeRow    (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;
    private:
        bool m_ColumnNameAsAttr;
    };

/** @fn XmlAttrStreamer::XmlAttrStreamer (const GridStringSource& source, int startRow, int startCol)
 * @brief Constructor for XmlAttrStreamer.
 *
 * @arg source See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selRow See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 * @arg selCol See Streamer::Streamer (const GridStringSource& source, int selRow, int selCol).
 */
XmlAttrStreamer::XmlAttrStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
    : XmlStreamer(source, startRow, rowCount, startCol, colCount)
{ 
    m_ColumnNameAsAttr   = m_source.GetColumnNameAsAttr();
}

/** @fn void XmlAttrStreamer::writeRow (std::ostream& of, int row) const
 * @brief Writes a row of data in XML format.
 *
 * Output is attributes of <REC> tag, with row number in attribute N.
 *
 * @arg of Output stream.
 * @arg row Row to write.
*/
void XmlAttrStreamer::writeRow (std::ostream& of, int row) const
{
    of << "<REC N=\"" << row << "\"";;
    Streamer::writeRow(of, row);
    of << "/>\n";
}

/** @fn void XmlAttrStreamer::writeCell (std::ostream& of, int row) const
 * @brief Writes a cell of data in XML format.
 *
 * Column name is name of the attribute.
 *
 * @arg of Output stream.
 * @arg row Row of cell to write.
 * @arg col Column of cell to write.
*/
void XmlAttrStreamer::writeCell (std::ostream& of, int row, int col) const
{
    string str;
    getCellStr(str, row, col);

    if (!str.empty())
    {                                             
        if (m_IsTableOrientation)
        {
            of << " ";
            writeName(of, getColHeader(col));
            of << "=\"";
        }
        else
            of << " " << "COL" << col << "=\"";
        
        writeXMLString(of, str);

        of << '"';
    }
    
}

///////////////////////////////////////////////////////////////////////////////
/** @class HTMLStreamer
 * @brief Class for exporting grid data in HTML format.
 */
class HtmlStreamer : public Streamer
    {
    public:
        HtmlStreamer::HtmlStreamer (const GridStringSource& source, int startRow, int rowCount, int startCol, int colCount)
            : Streamer (source, startRow, rowCount, startCol, colCount) {}

    protected:
        virtual void writeHeader (std::ostream& of) const;
        virtual void writeFooter (std::ostream& of) const;
        virtual void writeRow (std::ostream& of, int row) const;
        virtual void writeCell   (std::ostream& of, int row, int col) const;

        void writeTH (std::ostream& of, const std::string& str) const;
        void writeTD (std::ostream& of, const std::string& str, EColumnAlignment align) const;
        void writeHtmlString (std::ostream& of, const string& str) const;
    };

void HtmlStreamer::writeHtmlString (std::ostream& of, const string& str) const
{
    bool empty = str.empty();

    if (!empty)
    {
        // 27.10.2003 bug fix, html export does not handle empty strings (with spaces) properly
        std::string::const_iterator it = str.begin();
        for (; it != str.end(); ++it)
            if (!isspace(*it))
                break;

        if (it == str.end())
            empty = true;
    }

    if (!empty)
    {
        if (str.find_first_of( "&\"<>\n") != std::string::npos)
        {
            std::string::const_iterator it = str.begin();
            for (; it != str.end(); ++it)
                switch (*it) {
                case '&':  of << "&amp;";  break;
                case '"':  of << "&quot;"; break;
                case '>':  of << "&gt;";   break;
                case '<':  of << "&lt;";   break;
                case '\n': of << "<br>\n"; break;
                default:   of << *it;      break;
                }
        }
        else
            of << str;
    }
    else
        of << "&nbsp;";
}

void HtmlStreamer::writeTH (std::ostream& of, const std::string& str) const
{
    of << "<TH>";
    writeHtmlString(of, str);
    of << "</TH>";
}

void HtmlStreamer::writeTD (std::ostream& of, const std::string& str, EColumnAlignment align) const
{
    of << (align == ecaRight ? "<TD ALIGN=\"right\">" : "<TD>");
    writeHtmlString(of, str);
    of << "</TD>";
}

void HtmlStreamer::writeHeader (std::ostream& of) const
{
    of << "<HTML>\n<BODY>\n<TABLE BORDER=\"1\" CELLPADDING=\"1\" CELLSPACING=\"0\">\n";
    if (m_IsTableOrientation)
    {
        of << "<TR>";
        for (int col = m_startColumn; col < m_startColumn + m_colCount; col++) 
            writeTH(of, getColHeader(col));
        of << "</TR>\n";
    }
}

void HtmlStreamer::writeFooter (std::ostream& of) const
{
    of << "</TABLE>\n</BODY>\n</HTML>\n"; // bug fix, wrong order of html tags for html export
}

void HtmlStreamer::writeRow (std::ostream& of, int row) const
{
    of << "<TR>";
    if (!m_IsTableOrientation)
        writeTH(of, getColHeader(row));
    Streamer::writeRow(of, row);
    of << "</TR>\n";
}

void HtmlStreamer::writeCell (std::ostream& of, int row, int col) const
{
    string str;
    getCellStr(str, row, col);
    EColumnAlignment align = m_source.GetColAlignment(m_IsTableOrientation ?  col :  row);
    writeTD(of, str, align);
}

/** @fn void GridStringSource::ExportText (std::ostream& of, int startRow, int rowCount, int startCol, int colCount) const
 * @brief Based on output format selected, chooses correct streamer to perform data export.
 *
 * @arg selRow Row to export.  If -1, then export all rows.
 * @arg selCol Column to export.  If -1, then export all columns.
 * @arg of Output stream.
 */
int GridStringSource::ExportText (std::ostream& of, int startRow, int rowCount, int startCol, int colCount, int format) const
{
    int actualFormat = format != etfDefault ? format : m_OutputFormat;

     switch (actualFormat)
     {
     case etfQuotaDelimited   : CsvTextStreamer  (*this, startRow, rowCount, startCol, colCount).Write(of); break;
     case etfTabDelimitedText : TabTextStreamer  (*this, startRow, rowCount, startCol, colCount).Write(of); break;
     case etfXmlElem          : XmlElemStreamer  (*this, startRow, rowCount, startCol, colCount).Write(of); break;         
     case etfXmlAttr          : XmlAttrStreamer  (*this, startRow, rowCount, startCol, colCount).Write(of); break;         
     case etfHtml             : HtmlStreamer     (*this, startRow, rowCount, startCol, colCount).Write(of); break;            
     case etfPLSQL1           : PlSqlStreamer1   (*this, startRow, rowCount, startCol, colCount).Write(of); break;            
     case etfPLSQL2           : PlSqlStreamer2   (*this, startRow, rowCount, startCol, colCount).Write(of); break;            
     case etfPLSQL3           : PlSqlStreamer3   (*this, startRow, rowCount, startCol, colCount).Write(of); break;            
     case etfPLSQL4           : PlSqlStreamer4   (*this, startRow, rowCount, startCol, colCount).Write(of); break;            
     default                  : actualFormat = etfPlainText; _ASSERTE(0);
     case etfPlainText        : PlainTextStreamer(*this, startRow, rowCount, startCol, colCount).Write(of); break;
     };

     return actualFormat;
}

}//namespace OG2

