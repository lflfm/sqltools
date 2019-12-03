/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015,2017 Aleksey Kochetov

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

// 24.08.99 changed through oracle default clause error
// 31.08.99 fix: a serious error in skip_back_space
// 18.05.00 fix: a failure in skip_db_name
// 20.11.01 fix: a pakage body text is corrupted if a pakage body header has more 
//               then one space between "package" and "body".
// 29.05.02 bug fix: using number intead of integer 
// 04.08.02 bug fix: debug privilege for 9i
// 17.09.02 bug fix: cluster reengineering fails
// 17.09.02 bug fix: better SQLPlus compatibility
// 06.04.2003 bug fix, some oracle 9i privilege are not supported
// 03.08.2003 improvement, domain index support
// 09.11.2003 bug fix, quote character in comments for table/view DDL
// 10.11.2003 bug fix, missing public keyword for public database links
// 16.11.2003 bug fix, missing public keyword for public synonyms
// 30.11.2004 bug fix, trigger reverse-engineering fails if in OF clause if column name contains "ON_"/"_ON"/"_ON_"
// 09.01.2005 bug fix, trigger reverse-engineering fails if description contains comments
// 12.04.2005 bug fix #1181324, (ak) trigger reverse-engineering fails if description contains comments (again)
// 13.06.2005 B#1078885 - Doubled GRANT when extracting Columns Privilages (tMk).
// 14.06.2005 B#1191426 - Invalid COMPRESS index generation (tMk).
// 19.01.2006 B#1404162 - Trigger with CALL - unsupported syntax (tMk)
// 08.10.2006 B#XXXXXXX - (ak) handling NULLs for PCTUSED, FREELISTS, and FREELIST GROUPS storage parameters
// 25.02.2007 improvement, partition support
// 2011.09.20 bug fix, IOT tables w/o overflow
// 2011.09.20 bug fix, default storage parameters for a partitioned index
// 2011.09.20 bug fix, swapped INITIALLY & DISABLE for FK
// 2011.10.17 bug fix, IOT table has extra PK definition if "No storage for PK,UK" is checked
// 2011.11.06 bug fix, Zero for INITIAL, MINEXTENTS, MAXEXTENTS in 11g database
// 2016.08.30 bug fix, added "ON DELETE SET NULL" for FK reverse-engineering
// 18.12.2016 DDL reverse engineering incrorrectly handles function-base indexes with quoted columns
// 2017-12-31 improvement, DDL reverse engineering handles 12c indentity columns

// DONE: buffer pool, table compress 
// DONE: iot pctthreshold
// DONE: no storage clause if m_IOTOverflow_includeColumn == 0
// TODO: review m_StorageSubstitutedClause
// TODO: lob storage
// TODO: introduce taget db version
//       generate DDL accordingly (for example 8i does not support table COMPRESS)

// TODO: improve error messages


/*
    TODO#2:
            +1) double PARALLEL
            +2) parameter should be printed before subpartitions
            3) align subpartition names
            4) COMPRESS on index level only! use COMPRESSION PREFIX_LENGTH from all_indexes
            5) supress paritions / subpartitions for local indexes
            7) def_logging can be NO, YES and NONE, process them accordingly
            8) interval for global indexes?
            9) Using index does not support PARALLEL
            10) tablespace for temp tables
            11) add option to supress NEXT for auto-managed tablespaces
*/

#include "stdafx.h"
#include <algorithm>
#include <sstream>
#include "COMMON/ExceptionHelper.h"
#include "COMMON/StrHelpers.h"
#include "MetaDict/TextOutput.h"
#include "MetaDict/MetaObjects.h"
#include "MetaDict/MetaDictionary.h"
#include "SQLWorksheet/PlsSqlParser.h"

#pragma warning ( disable : 4800 )


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

namespace OraMetaDict 
{
    using namespace std;
    using namespace Common;
    const int cnUnlimitedVal = 2147483645;

    int skip_space (const char* str)
    {
        for (int i(0); str && str[i] && isspace(str[i]); i++);
        return i;
    }
    int skip_back_space (const char* str, int len)
    {
        if (len) 
        {
            for (int i(len - 1); str && i >= 0; i--)
                if (!isspace(str[i])) 
                {
                    i++;
                    break;
                }
             return (i < 0) ? 0 : i;
        }
        return 0;
    }
    void get_word_SP (const char* str, const char*& wordPtr, int& wordLen)
    {
        _ASSERTE(str);
        wordPtr = 0;
        wordLen = 0;
        for (int i(0); str && str[i] && (isspace(str[i]) || ispunct(str[i])); i++);
        wordPtr = str + i;
        for (; str && str[i] && !isspace(str[i]) && !ispunct(str[i]); i++, wordLen++);
    }
    int skip_space_and_comments (const char* str)
    {
        for (int i(0); str && str[i]; i++) 
        {
            if (isspace(str[i])) 
            {
                continue;
            } 
            else if (str[i] == '-' && str[i+1] == '-') 
            {
                for (i += 2; str[i] && str[i] != '\n'; i++);
                continue;
            } 
            else if (str[i] == '/' && str[i+1] == '*') 
            {
                for (i += 2; str[i] && !(str[i] == '*' && str[i+1] == '/'); i++);
                i++;
                continue;
            } 
            else 
                break;
        }
        return i;
    }
    void get_word_ID (const char* str, const char*& wordPtr, int& wordLen)
    {
        _ASSERTE(str);
        wordPtr = 0;
        wordLen = 0;
        int i = skip_space_and_comments(str);
        wordPtr = str + i;
        if (str && str[i] != '\"') 
        {
            for (; str && str[i] && (isalnum(str[i]) || strchr("_$#",str[i])); i++, wordLen++);
        } 
        else 
        {
            for (i++, wordLen++; str && str[i] && str[i] != '\"'; i++, wordLen++); 
            if (str && str[i] == '\"')
                wordLen++;
        }
    }
    int skip_db_name (const char* str)
    {
        int len;
        const char* ptr;
        get_word_ID(str, ptr, len);
        ptr += len;
        int offset = skip_space_and_comments(ptr);

        if (ptr[offset] == '.') 
        {
            ptr += offset;
            ptr++;
            get_word_ID(ptr, ptr, len);
            ptr += len;
        }
//        ptr += skip_space_and_comments(ptr);

        return (ptr - str);
    }
    int count_lines (const char* str, int len = INT_MAX)
    {
        int count = 0;
        for (int i(0); str[i] && i < len; i++)
            if (str[i] == '\n') 
                count++;

        return count;
    }
    void write_text_block (TextOutput& out, const char* text, int len, bool trunc, bool strip)
    {
        _ASSERTE(!(!trunc && strip));
        
        len = skip_back_space(text, len);

        if (!trunc)
            out.PutLine(text, len);
        else 
        {
            istringstream i(text, len);
            string buffer;

            while (getline(i, buffer, '\n')) 
            {
                int size = skip_back_space(buffer.c_str(), buffer.size());
                if (!strip || size) 
                    out.PutLine(buffer.c_str(), size);
            }
        }
    }

    void write_substituted_clause (TextOutput& out, const string& text, const string& suffix)
    {
        string clause;
        Common::to_lower_str(text.c_str(), clause);
        if ((clause.size() + suffix.size()) > 30)
            clause.resize(30 - suffix.size());

        for (string::iterator it = clause.begin(); it != clause.end(); it++)
            if (!isalnum(*it)) *it = '_';

        out.PutIndent();
        out.Put("&");
        out.Put(clause);
        out.Put(suffix);
        out.NewLine();
    }

    static 
    bool isDefaultTablespace (const Dictionary& dictionary, const string& tablespace, const string& owner)
    {
        try 
        {
            if (tablespace == dictionary.LookupUser(owner.c_str()).m_strDefTablespace)
                return true;
        } 
        catch (const XNotFound&) {}

        return false;
    }

    /// DbObject ///////////////////////////////////////////////////////////////

    void DbObject::WriteGrants (TextOutput& out, const WriteSettings& settings) const
    {
        Grantor* pGrantor = 0;
        
        try 
        {
            pGrantor = &(m_Dictionary.LookupGrantor(m_strOwner.c_str()));
        } 
        catch (const XNotFound&) {}

        if (pGrantor)
            pGrantor->WriteObjectGrants(m_strName.c_str(), out, settings);
    }

    /// WriteSettings //////////////////////////////////////////////////////////

    CMN_IMPL_PROPERTY_BINDER(WriteSettings);
    // Common settings
    CMN_IMPL_PROPERTY(WriteSettings, Comments              , true);               
    CMN_IMPL_PROPERTY(WriteSettings, Grants                , true);                 
    CMN_IMPL_PROPERTY(WriteSettings, LowerNames            , true);             
    CMN_IMPL_PROPERTY(WriteSettings, ShemaName             , false);              
    CMN_IMPL_PROPERTY(WriteSettings, SQLPlusCompatibility  , true); 
    CMN_IMPL_PROPERTY(WriteSettings, GeneratePrompts       , true); 
    // Table Specific
    CMN_IMPL_PROPERTY(WriteSettings, CommentsAfterColumn   , false);    
    CMN_IMPL_PROPERTY(WriteSettings, CommentsPos           , 48);            
    CMN_IMPL_PROPERTY(WriteSettings, Constraints           , true);            
    CMN_IMPL_PROPERTY(WriteSettings, Indexes               , true);                
    CMN_IMPL_PROPERTY(WriteSettings, NoStorageForConstraint, false); 
    CMN_IMPL_PROPERTY(WriteSettings, StorageClause         , 1);          
    CMN_IMPL_PROPERTY(WriteSettings, AlwaysPutTablespace   , false);    
    CMN_IMPL_PROPERTY(WriteSettings, TableDefinition       , true);        
    CMN_IMPL_PROPERTY(WriteSettings, Triggers              , true);               
    // Othes
    CMN_IMPL_PROPERTY(WriteSettings, SequnceWithStart      , false);       
    CMN_IMPL_PROPERTY(WriteSettings, ViewWithTriggers      , true);       
    CMN_IMPL_PROPERTY(WriteSettings, ViewWithForce         , false);          
    // Hidden
    CMN_IMPL_PROPERTY(WriteSettings, EndOfShortStatement   , ";");    
    CMN_IMPL_PROPERTY(WriteSettings, StorageSubstitutedClause, false);
    CMN_IMPL_PROPERTY(WriteSettings, AlwaysWriteColumnLengthSematics, false);

    WriteSettings::WriteSettings ()
    {
        CMN_GET_THIS_PROPERTY_BINDER.initialize(*this);
    }

    /// Storage ////////////////////////////////////////////////////////////////

    Storage::Storage (/**Dictionary& dictionary*/) 
    /**: DbObject(dictionary)*/ 
    {
    }

    void Storage::WriteTablespace (TextOutput& out) const
    {
        if (!m_strTablespaceName.IsNull())
        {
            out.PutIndent();
            out.Put("TABLESPACE ");
            out.SafeWriteDBName(m_strTablespaceName.GetValue().c_str());
            out.NewLine();
        }
    }

#define IS_PRINTABLE(par, storage, settings) ((par).IsPrintable((settings).IsStorageAlways(), &((storage).par) ) )

    static void PRINT_LINE_INT(TextOutput& out, int par, const char* fmt)           
    { 
        char buff[128]; buff[sizeof(buff)-1] = 0; 
        _snprintf(buff, sizeof(buff)-1, fmt, par); 
        out.PutLine(buff); 
    }
    static void PRINT_INT(TextOutput& out, int par, const char* fmt)                
    { 
        char buff[128]; buff[sizeof(buff)-1] = 0; 
        _snprintf(buff, sizeof(buff)-1, fmt, par); 
        out.Put(buff);   
    }

    void Storage::WriteStorage (TextOutput& out, const Storage& defStorage, const WriteSettings& settings) const
    {
        if (settings.IsStorageEnabled()) 
        {
            if (IS_PRINTABLE(m_nInitialExtent, defStorage, settings)) PRINT_LINE_INT(out, m_nInitialExtent.GetValue()/1024,  "INITIAL %7d K");
            if (IS_PRINTABLE(m_nNextExtent,    defStorage, settings)) PRINT_LINE_INT(out, m_nNextExtent.GetValue()/1024,     "NEXT    %7d K");
            if (IS_PRINTABLE(m_nMinExtents,    defStorage, settings)) PRINT_LINE_INT(out, m_nMinExtents.GetValue(),          "MINEXTENTS  %3d");
            if (IS_PRINTABLE(m_nMaxExtents,    defStorage, settings)) 
            {
                if (m_nMaxExtents.GetValue() < cnUnlimitedVal) PRINT_LINE_INT(out, m_nMaxExtents.GetValue(), "MAXEXTENTS  %3d");
                else                                           out.PutLine("MAXEXTENTS    UNLIMITED"); 
            }
            if (IS_PRINTABLE(m_nPctIncrease,   defStorage, settings)) PRINT_LINE_INT(out, m_nPctIncrease.GetValue(),         "PCTINCREASE %3d");
        }
    }

    void Storage::CopyNotNulls (const Storage& other)
    {
        m_strTablespaceName.SetIfNotNull(other.m_strTablespaceName);
        m_nInitialExtent   .SetIfNotNull(other.m_nInitialExtent   );
        m_nNextExtent      .SetIfNotNull(other.m_nNextExtent      );
        m_nMinExtents      .SetIfNotNull(other.m_nMinExtents      );
        m_nMaxExtents      .SetIfNotNull(other.m_nMaxExtents      );
        m_nPctIncrease     .SetIfNotNull(other.m_nPctIncrease     );
        m_bLogging         .SetIfNotNull(other.m_bLogging         );
        m_bCompression     .SetIfNotNull(other.m_bCompression     );
        m_strCompressFor   .SetIfNotNull(other.m_strCompressFor   );
    }

    /// TableStorage ////////////////////////////////////////////////////////////////

    TableStorage::TableStorage ()
    {
    }

    void TableStorage::WriteParallel (TextOutput& out, const TableStorage& defStorage, const WriteSettings& settings) const
    {
        bool degree = IS_PRINTABLE(m_nDegree, defStorage, settings);
        bool instances = IS_PRINTABLE(m_nInstances, defStorage, settings);
        if (degree || instances)
        {
            
            out.PutIndent();
            out.Put("PARALLEL (");
            if (degree) 
                PRINT_INT(out, m_nDegree.GetValue(), "DEGREE %d");
                
            if (degree && instances) 
                out.Put(" "); 

            if (instances) 
                PRINT_INT(out, m_nInstances.GetValue(), "INSTANCES %d");

            out.Put(")");
            out.NewLine();
        }
    }

    // pass pDefStorage insted of dictionary & owner
    void TableStorage::WriteStorage (TextOutput& out, const TableStorage& defStorage, const WriteSettings& settings) const
    {
        if (settings.IsStorageEnabled()) 
        {
            char buff[128]; buff[sizeof(buff)-1] = 0;

            if (IS_PRINTABLE(m_strTablespaceName, defStorage, settings))
                WriteTablespace(out);

            if (IS_PRINTABLE(m_nPctFree, defStorage, settings))
                PRINT_LINE_INT(out, m_nPctFree.GetValue(), "PCTFREE  %3d");
            if (IS_PRINTABLE(m_nPctUsed, defStorage, settings))
                PRINT_LINE_INT(out, m_nPctUsed.GetValue(), "PCTUSED  %3d");
            if (IS_PRINTABLE(m_nIniTrans, defStorage, settings))
                PRINT_LINE_INT(out, m_nIniTrans.GetValue(), "INITRANS %3d");
            if (IS_PRINTABLE(m_nMaxTrans, defStorage, settings))
                PRINT_LINE_INT(out, m_nMaxTrans.GetValue(), "MAXTRANS %3d");

            {
                TextOutputInMEM out2(true, 2*1024);
                out2.SetLike(out);
                out2.MoveIndent(2);

                Storage::WriteStorage(out2, defStorage, settings);

                if (IS_PRINTABLE(m_nFreeLists, defStorage, settings))
                    PRINT_LINE_INT(out2, m_nFreeLists.GetValue(), "FREELISTS   %3d");

                if (IS_PRINTABLE(m_nFreeListGroups, defStorage, settings))
                    PRINT_LINE_INT(out2, m_nFreeListGroups.GetValue(), "FREELIST GROUPS %d");

                if (IS_PRINTABLE(m_strBufferPool, defStorage, settings))
                {
                    out2.PutIndent();
                    out2.Put("BUFFER_POOL   ");
                    out2.Put(m_strBufferPool.GetValue());
                    out2.NewLine();
                }

                out2.MoveIndent(-2);

                if (out2.GetLength())
                {
                    out.PutLine("STORAGE (");
                    out.Put(out2.GetData(), out2.GetLength());
                    out.PutLine(")");
                }
            }

            if (IS_PRINTABLE(m_bLogging, defStorage, settings))
                out.PutLine(m_bLogging.GetValue() ? "LOGGING" : "NOLOGGING"); 

            if (IS_PRINTABLE(m_bCompression, defStorage, settings)
            || IS_PRINTABLE(m_strCompressFor, defStorage, settings)
            )
            {
                out.PutIndent();
                out.Put(m_bCompression.GetValue() ? "COMPRESS" : "NOCOMPRESS");
                if (!m_strCompressFor.IsNull())
                {
                    out.Put(" ");
                    out.Put(m_strCompressFor.GetValue());
                }
                out.NewLine();
            }

            WriteParallel(out, defStorage, settings);

        }
        else 
            if (settings.m_AlwaysPutTablespace)
                WriteTablespace(out);
    }

    void TableStorage::CopyNotNulls (const TableStorage& other)
    {
        Storage::CopyNotNulls(other);

        m_nFreeLists     .SetIfNotNull(other.m_nFreeLists     );
        m_nFreeListGroups.SetIfNotNull(other.m_nFreeListGroups);
        m_nPctFree       .SetIfNotNull(other.m_nPctFree       );
        m_nPctUsed       .SetIfNotNull(other.m_nPctUsed       );
        m_nIniTrans      .SetIfNotNull(other.m_nIniTrans      );
        m_nMaxTrans      .SetIfNotNull(other.m_nMaxTrans      );
        m_nDegree        .SetIfNotNull(other.m_nDegree        );
        m_nInstances     .SetIfNotNull(other.m_nInstances     );
        m_strBufferPool  .SetIfNotNull(other.m_strBufferPool  );
    }

    void TableStorage::BuildDefault (TableStorage& storage, const Dictionary& dictionary, const string& schema, const string& tablespace)
    {
        storage = TableStorage();
        storage.m_nFreeLists     .SetDefault(1);
        storage.m_nFreeListGroups.SetDefault(1);
        storage.m_nPctFree       .SetDefault(10);
        storage.m_nPctUsed       .SetDefault(40);
        storage.m_nIniTrans      .SetDefault(1);
        storage.m_nMaxTrans      .SetDefault(255);
        storage.m_nDegree        .SetDefault(1);
        storage.m_nInstances     .SetDefault(1);
        storage.m_strBufferPool  .SetDefault("DEFAULT");

        const Tablespace* tbs = 0;
        const User* user = 0;

        try 
        {
            tbs  = &dictionary.LookupTablespace(tablespace.c_str());
        } 
        catch (const XNotFound&) {}
        try 
        {
            user = &dictionary.LookupUser(schema.c_str());
        } 
        catch (const XNotFound&) {}

        if (tbs)
            (Storage&)storage = *((Storage*)tbs);

        if (user /*&& user->m_strDefTablespace != tablespace*/)
            storage.m_strTablespaceName.Set(user->m_strDefTablespace);
    }

    /// Column //////////////////////////////////////////////////////////////

    /// helper class ///
    Column::SubtypeMap::SubtypeMap ()
    {
        insert(value_type("CHAR",      1));
        insert(value_type("NCHAR",     1));
        insert(value_type("RAW",       1));
        insert(value_type("VARCHAR2",  1));
        insert(value_type("NVARCHAR2", 1));
        insert(value_type("NUMBER",    2));
        insert(value_type("FLOAT",     2));
        insert(value_type("LONG",      0));
        insert(value_type("DATE",      0));
        insert(value_type("LONG RAW",  0));
        insert(value_type("ROWID",     0));
        insert(value_type("MLSLABEL",  0));
        insert(value_type("CLOB",      0));
        insert(value_type("NCLOB",     0));
        insert(value_type("BLOB",      0));
        insert(value_type("BFILE",     0));
    }

    Column::SubtypeMap Column::m_mapSubtypes;

    Column::Column ()
    {
        m_nDataPrecision =
        m_nDataScale     =
        m_nDataLength    = 0;
        m_bNullable      = false;
        m_charLengthSemantics = USE_BYTE;
        m_defCharLengthSemantics = USE_BYTE;
    }

    void Column::GetSpecString (string& strBuff, const WriteSettings& settings) const
    {
      char szBuff[20];
      strBuff = m_strDataType;
      switch (m_mapSubtypes[m_strDataType]) 
      {
      case 0: 
        break;
      case 1: 
        strBuff += '('; 
        strBuff += itoa(m_nDataLength, szBuff, 10); 
        if ((m_strDataType == "VARCHAR2" || m_strDataType == "CHAR")
            && (m_charLengthSemantics != m_defCharLengthSemantics || settings.m_AlwaysWriteColumnLengthSematics))
            strBuff += (m_charLengthSemantics == USE_CHAR) ? " CHAR" : " BYTE"; 
        strBuff += ')';
        break;
      case 2:
        // bug fix: using number intead of integer 
        if (m_nDataPrecision != -1 && m_nDataScale != -1) 
        {
          strBuff += '('; 
          strBuff += itoa(m_nDataPrecision, szBuff, 10); 
          strBuff += ',';
          strBuff += itoa(m_nDataScale, szBuff, 10); 
          strBuff += ')';
        } 
        else if (m_nDataPrecision != -1) 
        {
          strBuff += '('; 
          strBuff += itoa(m_nDataPrecision, szBuff, 10); 
          strBuff += ')';
        }
        else if (m_nDataScale != -1) 
        {
          strBuff = "INTEGER"; 
        }
        break;
      }
    }

    /// Index ////////////////////////////////////////////////////////////////
    Index::Index (Dictionary& dictionary)
    : DbObject(dictionary) 
    { 
        m_Type = eitNormal;
        m_bUniqueness = false; 
        m_bTemporary = false; 
        m_bGenerated = false;
        m_bReverse = false;
        m_PartitioningType = NONE;
        m_subpartitioningType = NONE;
        m_bLocal = false;
        m_IOTOverflow_PCTThreshold  = 0;
        m_bIndexCompression = false;
    };

    int Index::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE INDEX ");
            out.PutOwnerAndName(m_strOwner, m_strName, 
                                settings.m_ShemaName || m_strOwner != m_strTableOwner);
            out.NewLine();
        }

        out.PutIndent();
        out.Put("CREATE ");

        if (m_Type == eitBitmap || m_Type == eitFunctionBasedBitmap)
            out.Put("BITMAP ");
        else
            if (m_Type != eitCluster && m_bUniqueness) 
                out.Put("UNIQUE ");

        out.Put("INDEX ");
        out.PutOwnerAndName(m_strOwner, m_strName, 
                            settings.m_ShemaName || m_strOwner != m_strTableOwner);
        out.NewLine();

        out.MoveIndent(2);
        out.PutIndent();
        out.Put("ON ");

        if (m_Type != eitCluster)
        {
            out.PutOwnerAndName(m_strTableOwner, m_strTableName, settings.m_ShemaName);
            out.Put(" (");
            out.NewLine();

            std::map<int,bool> safeWriteDBName;
            std::map<int,string>::const_iterator it(m_Columns.begin()),
                                                 end(m_Columns.end());
            for (int i(0); it != end; it++, i++) {
                safeWriteDBName[i] = (m_isExpression.find(i) == m_isExpression.end());
            }

            out.WriteColumns(m_Columns, 2, safeWriteDBName);
            out.PutLine(")");
        }
        else
        {
            out.Put("CLUSTER ");
            out.PutOwnerAndName(m_strTableOwner, m_strTableName, settings.m_ShemaName);
            out.NewLine();
        }

        // 03.08.2003 improvement, domain index support
        if (m_Type == eitDomain)
            writeDomainClause(out, settings);
        else
            WriteIndexParams(out, settings);

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();

        return 0;
    }

    void Index::WriteIndexParams (TextOutput& out, const WriteSettings& settings, bool skip_parallel /*= false*/) const
    {
        if (m_bReverse)
            out.PutLine("REVERSE");

        if (!m_bTemporary)
        {
            TableStorage defStorage;
            BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
            defStorage.m_nIniTrans.SetDefault(2);
            
            if (skip_parallel) // parallel cannot be part create constraint
            {
                TableStorage storage(*this);
                storage.m_nDegree.SetNull();
                storage.m_nInstances.SetNull();
                storage.WriteStorage(out, defStorage, settings);
            }
            else
                WriteStorage(out, defStorage, settings);

            WriteCompress(out, settings);

            if (m_PartitioningType != NONE) 
                writeIndexPartitions(out, settings);
        }

        if (settings.m_StorageSubstitutedClause)
            write_substituted_clause(out, m_strName, "_st");
    }

    void Index::WriteCompress (TextOutput& out, const WriteSettings& settings) const
    {
        if (m_bIndexCompression)
        { 
            out.PutIndent();
            out.Put("COMPRESS"); 

            if (
                settings.IsStorageAlways()
                || (
                    !(m_bUniqueness && m_nCompressionPrefixLength == static_cast<int>(m_Columns.size()) - 1)
                    && !(!m_bUniqueness && m_nCompressionPrefixLength == static_cast<int>(m_Columns.size()))
                )
            )
            {
                char buff[128]; buff[sizeof(buff)-1] = 0;
                _snprintf(buff, sizeof(buff)-1, " %d", m_nCompressionPrefixLength); 
                out.Put(buff);
            }

            out.NewLine();
        }
    }

    void Index::writeDomainClause (TextOutput& out, const WriteSettings& settings) const
    {
        out.PutIndent();
        out.Put("INDEXTYPE IS ");
        out.PutOwnerAndName(m_strDomainOwner, m_strDomain, settings.m_ShemaName || m_strOwner != m_strDomainOwner);
        out.NewLine();

        if (!m_strDomainParams.empty())
        {
            out.MoveIndent(2);
            out.PutIndent();
            out.Put("PARAMETERS('");
            out.Put(m_strDomainParams);
            out.Put("')");
            out.NewLine();
            out.MoveIndent(-2);
        }
    }

    void Index::writeIndexPartitions (TextOutput& out, const WriteSettings& settings) const
    {
        if (m_PartitioningType != NONE)
        {
            if (m_bLocal && m_Type != eitIOT_TOP)
            {
                out.PutLine("LOCAL (");
            }
            else
            {
                out.PutIndent();
                if (m_Type != eitIOT_TOP) 
                    out.Put("GLOBAL ");

                switch (m_PartitioningType)
                {
                case RANGE:
                    out.Put("PARTITION BY RANGE ("); 
                    out.NewLine();
                    out.WriteColumns(m_PartKeyColumns, 2, true);
                    out.PutLine(")");
                    break;
                case HASH:
                    out.Put("PARTITION BY HASH (");
                    out.NewLine();
                    out.WriteColumns(m_PartKeyColumns, 2, true);
                    out.PutLine(")");
                    break;
                case LIST:
                    out.Put("PARTITION BY LIST (");
                    out.SafeWriteDBName(!m_PartKeyColumns.empty() ? *m_PartKeyColumns.begin() : string());
                    out.Put(")");
                    out.NewLine();
                    break;
                }

                out.PutLine("(");
            }
            out.MoveIndent(2);

            int nMaxNameLen = 0;
            for (PartitionContainer::const_iterator it = m_Partitions.begin(); it != m_Partitions.end(); ++it)
                nMaxNameLen = max(nMaxNameLen, static_cast<int>((*it)->m_strName.length()));

            int i = 0, count = m_Partitions.size();
            for (PartitionContainer::const_iterator it = m_Partitions.begin(); it != m_Partitions.end(); ++it, ++i)
            {
                out.PutIndent();
                out.Put("PARTITION ");
                out.SafeWriteDBName((*it)->m_strName);

                if ((!m_bLocal || m_Type == eitIOT_TOP)
                && m_PartitioningType != HASH)
                {
                    int j = (*it)->m_strName.size();
                    do out.Put(" "); while (++j < nMaxNameLen + 1);

                    out.Put(m_PartitioningType == RANGE ? "VALUES LESS THAN (" : "VALUES (");
                    out.Put((*it)->m_strHighValue);
                    out.Put(")");
                }

                {
                    TextOutputInMEM out2(true, 2*1024);
                    out2.SetLike(out);

                    TableStorage defStorage;
                    BuildDefault(defStorage, m_Dictionary, m_strOwner, (*it)->m_strTablespaceName.GetValue());
                    defStorage.CopyNotNulls(*this); // copy defaults defined on index level

                    out2.MoveIndent(2);
                    (*it)->WriteStorage(out2, defStorage, settings);
                    out2.MoveIndent(-2);

                    if (m_Type == eitIOT_TOP && !m_IOTOverflowPartitions.empty())
                    {
                        TextOutputInMEM out3(true, 2*1024);
                        out3.SetLike(out);

                        TableStorage defStorage;
                        BuildDefault(defStorage, m_Dictionary, m_strOwner, m_IOTOverflowPartitions.at(i)->m_strTablespaceName.GetValue());
                        defStorage.CopyNotNulls(m_IOTOverflowPartitionDefaultStrorage);

                        out3.MoveIndent(2);
                        m_IOTOverflowPartitions.at(i)->WriteStorage(out3, defStorage, settings);
                        out3.MoveIndent(-2);

                        if (out3.GetLength())
                        {
                            out2.PutLine("OVERFLOW");
                            out2.Put(out3.GetData(), out3.GetLength());
                        }
                    }

                    if (!(*it)->m_subpartitions.empty())
                        writeSubpartitions(out2, settings, (*it)->m_subpartitions);

                    out2.TrimRight(" \n\r");

                    if (out2.GetLength())
                    {
                        out.NewLine();
                        out.Put(out2.GetData(), out2.GetLength());
                    }

                    if ((i < count-1)) out.Put(",");
                    out.NewLine();
                }
            }

            out.MoveIndent(-2);
            out.PutLine(")");
        }
    }

    void Index::writeSubpartitions (TextOutput& out, const WriteSettings& settings, const PartitionContainer& subpartitions) const
    {
//TODO: how to handle defaul subpartitioning?
        out.MoveIndent(2);
        out.PutLine("(");
        out.MoveIndent(2);

        int nMaxNameLen = 0;
        for (PartitionContainer::const_iterator it = subpartitions.begin(); it != subpartitions.end(); ++it)
            nMaxNameLen = max(nMaxNameLen, static_cast<int>((*it)->m_strName.length()));
                
        int i = 0, count = subpartitions.size();
        for (PartitionContainer::const_iterator it = subpartitions.begin(); it != subpartitions.end(); ++it, ++i)
        {
            out.PutIndent();
            out.Put("SUBPARTITION ");
            out.SafeWriteDBName((*it)->m_strName);

            if (!m_bLocal && m_subpartitioningType != HASH)
            {
                int j = (*it)->m_strName.size();
                do out.Put(" "); while (++j < nMaxNameLen + 1);

                out.Put(m_subpartitioningType == RANGE ? "VALUES LESS THAN (" : "VALUES (");
                out.Put((*it)->m_strHighValue);
                out.Put(")");
            }

            if (!(*it)->m_strTablespaceName.IsNull()
                &&
                (
                    settings.IsStorageAlways() 
                    || settings.m_AlwaysPutTablespace
                    || !isDefaultTablespace(m_Dictionary, (*it)->m_strTablespaceName.GetValue(), m_strOwner)
                )
            )
            {
                out.Put(" TABLESPACE ");
                out.SafeWriteDBName((*it)->m_strTablespaceName.GetValue());
            }

            if ((i < count-1)) out.Put(",");
            out.NewLine();
        }
        out.MoveIndent(-2);
        out.PutIndent();
        out.Put(")");
        out.MoveIndent(-2);
    }

    void Index::WriteIOTClause (TextOutput& out, const WriteSettings& settings, bool overflow) const
    {
        if (m_PartitioningType == NONE)
        {
            TableStorage defStorage;
            BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
            defStorage.m_nIniTrans.SetDefault(2);

            out.MoveIndent(2);
            WriteStorage(out, defStorage, settings);
            out.MoveIndent(-2);

            if (overflow)
            {
                if (settings.IsStorageAlways()
                || m_IOTOverflow_PCTThreshold != 50)
                {
                    char buff[128]; buff[sizeof(buff)-1] = 0;
                    _snprintf(buff, sizeof(buff)-1, "PCTTHRESHOLD %3d", m_IOTOverflow_PCTThreshold); 
                    out.PutLine(buff); 
                }

                out.PutLine("OVERFLOW ");

                TableStorage defStorage;
                BuildDefault(defStorage, m_Dictionary, m_strOwner, m_IOTOverflow_Storage.m_strTablespaceName.GetValue());

                out.MoveIndent(2);
                m_IOTOverflow_Storage.WriteStorage(out, defStorage, settings);
                out.MoveIndent(-2);
            }
        }
        else // partitioned
        {
            TableStorage defStorage;
            BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
            defStorage.m_nIniTrans.SetDefault(2);

            out.MoveIndent(2);
            WriteStorage(out, defStorage, settings);
            WriteCompress(out, settings);
            out.MoveIndent(-2);

            if (overflow)
            {
                if (settings.IsStorageAlways()
                || m_IOTOverflow_PCTThreshold != 50)
                {
                    char buff[128]; buff[sizeof(buff)-1] = 0;
                    _snprintf(buff, sizeof(buff)-1, "PCTTHRESHOLD %3d", m_IOTOverflow_PCTThreshold); 
                    out.PutLine(buff); 
                }

                out.PutLine("OVERFLOW ");

                {
                    TableStorage defStorage;
                    BuildDefault(defStorage, m_Dictionary, m_strOwner, m_IOTOverflowPartitionDefaultStrorage.m_strTablespaceName.GetValue());
                    out.MoveIndent(2);
                    m_IOTOverflowPartitionDefaultStrorage.WriteStorage(out, defStorage, settings);
                    out.MoveIndent(-2);
                }
            }

            if (m_PartitioningType != NONE)
                writeIndexPartitions(out, settings);
        }
    }

    /// Constraint ////////////////////////////////////////////////////////////////

    Constraint::Constraint (Dictionary& dictionary) 
        : DbObject(dictionary) 
    { 
        m_bDeferrable = false; 
        m_bDeferred   = false; 
        m_bGenerated  = false; 
        m_bOnView     = false;
        m_bRely       = false;
        m_bNoValidate = false;
    };

    bool Constraint::IsNotNullCheck () const
    {
        if (m_strType[0] == 'C'
        && m_Columns.size() == 1) 
        {
            string strNotNullCheck;
            strNotNullCheck = m_Columns.begin()->second;
            strNotNullCheck += " IS NOT NULL";

            if (m_strSearchCondition == strNotNullCheck)
                return true;

            strNotNullCheck = "\"";
            strNotNullCheck += m_Columns.begin()->second;
            strNotNullCheck += "\" IS NOT NULL";

            if (m_strSearchCondition == strNotNullCheck)
                return true;
        }
        return false;
    }

    int Constraint::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put(!m_bOnView ? "PROMPT ALTER TABLE " : "PROMPT ALTER VIEW ");
            out.PutOwnerAndName(m_strOwner, m_strTableName, settings.m_ShemaName);
            out.Put(" ADD ");
            
            if (!IsGenerated()) 
            {
                out.Put("CONSTRAINT ");
                out.SafeWriteDBName(m_strName);
                out.Put(" ");
            }
            
            switch (m_strType[0]) 
            {
            case 'C': out.Put("CHECK");       break;
            case 'P': out.Put("PRIMARY KEY"); break;
            case 'U': out.Put("UNIQUE");      break;
            case 'R': out.Put("FOREIGN KEY"); break;
            }
            out.NewLine();
        }

        out.PutIndent();
        out.Put(!m_bOnView ? "ALTER TABLE " : "ALTER VIEW ");
        out.PutOwnerAndName(m_strOwner, m_strTableName, settings.m_ShemaName);
        out.NewLine();

        out.MoveIndent(2);
        out.PutIndent();
        out.Put("ADD ");
        if (!IsGenerated()) 
        {
            out.Put("CONSTRAINT ");
            out.SafeWriteDBName(m_strName);
            out.Put(" ");
        }
        switch (m_strType[0]) 
        {
        case 'C': out.Put("CHECK (");       break;
        case 'P': out.Put("PRIMARY KEY ("); break;
        case 'U': out.Put("UNIQUE (");      break;
        case 'R': out.Put("FOREIGN KEY ("); break;
        }
        out.NewLine();
        if (m_strType[0] == 'C') 
        {
            out.MoveIndent(2);
            out.PutLine(m_strSearchCondition);
            out.MoveIndent(-2);
        } else 
        {
            out.WriteColumns(m_Columns, 2);
        }

        if (m_strType[0] == 'R') 
        {
            Constraint& refKey = 
                m_Dictionary.LookupConstraint(m_strROwner.c_str(), 
                                              m_strRConstraintName.c_str());
            out.PutIndent(); 
            out.Put(") REFERENCES ");
            out.PutOwnerAndName(refKey.m_strOwner, refKey.m_strTableName, 
                                settings.m_ShemaName || m_strOwner != refKey.m_strOwner);
            out.Put(" (");
            out.NewLine();
            out.WriteColumns(refKey.m_Columns, 2);
            out.PutIndent(); 
            out.Put(")");
            if (m_strDeleteRule == "CASCADE")
                out.Put(" ON DELETE CASCADE");
            else if (m_strDeleteRule == "SET NULL") // 2016.08.30 bug fix, added "ON DELETE SET NULL" for FK reverse-engineering
                out.Put(" ON DELETE SET NULL");
            out.NewLine();
        } 
        else
        {
            out.PutLine(")");
        }

        if (m_bDeferrable)
        {
            out.PutIndent(); 
            
            if (m_bDeferred)
                out.Put("INITIALLY DEFERRED ");

            out.Put("DEFERRABLE");
            out.NewLine();
        }

        if (m_bRely)
            out.PutLine("RELY");

        if (m_strStatus == "DISABLED")
            out.PutLine("DISABLE");

        if (m_bNoValidate)
            out.PutLine("NOVALIDATE");

        if (!settings.m_NoStorageForConstraint
        && (m_strType[0] == 'P' || m_strType[0] == 'U')) 
        {
            try 
            {
                Index& index = m_Dictionary.LookupIndex(m_strOwner.c_str(), m_strName.c_str());

                if (!index.m_bTemporary)
                {
                    TextOutputInMEM out2(true, 2*1024);
                    out2.SetLike(out);

                    out2.MoveIndent(2);
                    index.WriteIndexParams(out2, settings, /*skip_parallel =*/ true);
                    out2.MoveIndent(-2);

                    if (out2.GetLength() > 0)
                    {
                        out.PutLine("USING INDEX");
                        out.Put(out2.GetData(), out2.GetLength());
                    }
                }
            } 
            catch (const XNotFound&) {}
        }

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();

        return 0;
    }
    
    void Constraint::WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings&) const
    {
        _CHECK_AND_THROW_(m_strType[0] == 'P', "IOT error:\nit is not primary key!");

        out.PutIndent();

        if (!IsGenerated()) 
        {
            out.Put("CONSTRAINT ");
            out.SafeWriteDBName(m_strName);
            out.Put(" ");
        }
        
        out.Put("PRIMARY KEY (");
        out.NewLine();
        out.WriteColumns(m_Columns, 2);
        out.PutLine(")");
    }

    /// TableBase is ancestor of TABLE, SNAPSHOT & SNAPSHOT LOG /////////////

    TableBase::TableBase (Dictionary& dictionary) 
    : DbObject(dictionary)
    {
    }
    
    void TableBase::WriteIndexes (TextOutput& out, const WriteSettings& settings) const
    {
        set<string>::const_iterator it(m_setIndexes.begin()),
                                    end(m_setIndexes.end());
        for (it; it != end; it++) 
        {
            Index& index = m_Dictionary.LookupIndex(it->c_str());
            
            if (index.m_Type != eitIOT_TOP)
                try 
                {
                    m_Dictionary.LookupConstraint(it->c_str());
                        
                    if (settings.m_NoStorageForConstraint)    // all indexes for constraints
                        index.Write(out, settings);
                } 
                catch (const XNotFound&) 
                {
                    index.Write(out, settings);                // or index without constraints
                }
        }
    }

        inline int least_constaint (const Constraint* c1, const Constraint* c2)
        {
            if (c1->m_strType[0] != c2->m_strType[0])
                return (c1->m_strType[0] < c2->m_strType[0]);

            bool first_is_null = c1->IsGenerated();
            bool second_is_null = c2->IsGenerated();

            if (!first_is_null || !second_is_null)
                return (first_is_null ? string() : c1->m_strName) 
                     < (second_is_null ? string() : c2->m_strName);

            if (c1->m_strType[0] == 'C')
                return c1->m_strSearchCondition < c2->m_strSearchCondition;

            return c1->m_Columns < c2->m_Columns;
        }

        typedef vector<const Constraint*> ConstraintVector;
        typedef ConstraintVector::const_iterator ConstraintVectorIter;

    void TableBase::WriteConstraints (TextOutput& out, const WriteSettings& settings, char chType) const
    {
        set<string>::const_iterator it(m_setConstraints.begin()),
                                    end(m_setConstraints.end());
        ConstraintVector vector;

        for (it; it != end; it++) 
        {
            Constraint& constraint = m_Dictionary.LookupConstraint(m_strOwner.c_str(), 
                                                                   it->c_str());
            if (chType == constraint.m_strType[0]
            && !constraint.IsNotNullCheck())
              vector.push_back(&constraint);
        }
        stable_sort(vector.begin(), vector.end(), least_constaint);

        ConstraintVectorIter vecIt(vector.begin()), vecEndIt(vector.end());

        for (; vecIt != vecEndIt; vecIt++) 
            if (*vecIt)
                (*vecIt)->Write(out, settings);
    }

    void TableBase::WriteTriggers (TextOutput& out, const WriteSettings& settings) const
    {
        set<string>::const_iterator it(m_setTriggers.begin()),
                                    end(m_setTriggers.end());
        for (it; it != end; it++)
            m_Dictionary.LookupTrigger(it->c_str()).Write(out, settings);
    }

    /// Table ////////////////////////////////////////////////////////////////

    Table::Table (Dictionary& dictionary) 
    : TableBase(dictionary)
    {
        m_TableType = ettNormal;
        m_TemporaryTableDuration = etttdUnknown;
        m_PartitioningType = NONE;
        m_subpartitioningType = NONE;
        m_IOTOverflow_includeColumn = 0;
    }

    void Table::WriteDefinition (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE TABLE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
        }

        out.PutIndent();
        out.Put("CREATE");
        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        if (m_TableType == ettTemporary)
            out.Put("GLOBAL TEMPORARY ");

        out.Put("TABLE ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        out.Put(" (");
        if (!settings.m_SQLPlusCompatibility
        && settings.m_CommentsAfterColumn
        && !m_strComments.empty()) 
        {
            out.Put(" -- ");
            out.Put(m_strComments);
        }
        out.NewLine();

        out.MoveIndent(2);

        writeColumns(out, settings);

        if (m_TableType == ettIOT)
            WritePrimaryKeyForIOT(out, settings);

        out.MoveIndent(-2);
        out.PutLine(")");

        out.MoveIndent(2);

        if (settings.m_StorageSubstitutedClause)
        {
            if (m_TableType == ettTemporary)
                writeTempClause(out, settings);
            else
                write_substituted_clause(out, m_strName, "_st");
        }
        else
        {
            if (m_TableType == ettTemporary)
                writeTempClause(out, settings);
            else if (!m_strClusterName.empty())
                writeClusterClause(out, settings);
            else if (m_TableType == ettIOT)
                writeIOTClause(out, settings); 
            else 
            {
                TableStorage defStorage;
                BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
                WriteStorage(out, defStorage, settings);

                if (m_PartitioningType != NONE)
                    writePartitions(out, settings);
            }

            if (!m_bCache.IsNull() && m_bCache.GetValue()) 
                out.PutLine("CACHE");
        }

        out.MoveIndent(-2);

        out.PutLine("/");
        out.NewLine();
    }

    void Table::writeColumns (TextOutput& out, const WriteSettings& settings) const
    {
        int 
            nMaxNameLen(0),     // max column name length
            nMaxTypeLen(0),     // max type-specification length
            nMaxOtherLen(0);    // max default-specification and null-specification length

        list<string> listTypeSpecs;
        int nSize = m_Columns.size();

        ColumnContainer::const_iterator
            it(m_Columns.begin()), end(m_Columns.end());

        for (int i(0); it != end; it++, i++) 
        {
            const TabColumn& column = *it->second.get();

            string strType;
            column.GetSpecString(strType, settings);
            listTypeSpecs.push_back(strType);

            nMaxNameLen = max(nMaxNameLen, static_cast<int>(column.m_strColumnName.size()));
            nMaxTypeLen = max(nMaxTypeLen, static_cast<int>(strType.size()));

            size_t nLength = 0;
            if (column.m_bIdentity)
            {
                Sequence& seq = m_Dictionary.LookupSequence(m_strOwner.c_str(), column.m_strSequenceName.c_str());
                string identityClause =  " GENERATED "  + column.m_strIndentityGenerationType + " AS IDENTITY";
                string identityOptions = seq.GetIdentityOptions(settings);
                if (!identityOptions.empty())
                    identityClause += " (" + identityOptions + ")";
                nLength += identityClause.length();
            }
            else if (!column.m_strDataDefault.empty())
            {
                nLength += ((column.m_bVirtual) ? sizeof (" AS ()") : sizeof (" DEFAULT ")) - 1
                        + column.m_strDataDefault.size();
            }

            if (!column.m_bIdentity)
            {
                nLength += ((column.m_bNullable) ? sizeof (" NULL") : sizeof (" NOT NULL")) - 1;
            }

            if (i < (nSize - 1)) 
                nLength++;      // add place for ',' 

            nMaxOtherLen = max(nMaxOtherLen, static_cast<int>(nLength));
        }

        list<string>::const_iterator 
            itTypes(listTypeSpecs.begin());

        it = m_Columns.begin();

        for (i = 0; it != end; it++, itTypes++, i++) 
        {
            const TabColumn& column = *it->second.get();

            out.PutIndent();
            int nPos = out.GetPosition();
            out.SafeWriteDBName(column.m_strColumnName);
            
            int j = column.m_strColumnName.size();
            do out.Put(" "); while (++j < nMaxNameLen + 1);

            out.Put(*itTypes);
            j = (*itTypes).size();
            if (column.m_bIdentity)
            {
                while (j++ < nMaxTypeLen) out.Put(" "); 

                Sequence& seq = m_Dictionary.LookupSequence(m_strOwner.c_str(), column.m_strSequenceName.c_str());
                string identityClause =  " GENERATED "  + column.m_strIndentityGenerationType + " AS IDENTITY";
                string identityOptions = seq.GetIdentityOptions(settings);
                if (!identityOptions.empty())
                    identityClause += " (" + identityOptions + ")";
                out.Put(identityClause);
            }
            else if (column.m_bVirtual || !column.m_strDataDefault.empty()) 
            {
                while (j++ < nMaxTypeLen) out.Put(" "); 
                out.Put(column.m_bVirtual ? " AS (" : " DEFAULT ");
                out.Put(column.m_strDataDefault);
                if(column.m_bVirtual) out.Put(")");
            }

            if (!column.m_bIdentity)
            {
                while (j++ < nMaxTypeLen) out.Put(" "); 
                out.Put(column.m_bNullable ? " NULL" : " NOT NULL");
            }

            if (i < (nSize - 1) || m_TableType == ettIOT) 
                out.Put(",");

            if (settings.m_CommentsAfterColumn
            && !column.m_strComments.empty()) 
            {
                int j = out.GetPosition() - nPos;
                int nMaxLen = nMaxNameLen + nMaxTypeLen + nMaxOtherLen + 2;
                nMaxLen = max(nMaxLen, settings.m_CommentsPos);

                while (j++ < nMaxLen)
                    out.Put(" ");

                out.Put("-- ");
                out.Put(column.m_strComments);
            }

            out.NewLine();
        }
    }

    void Table::writeTempClause (TextOutput& out, const WriteSettings&) const
    {
        out.PutIndent();

        if (m_TemporaryTableDuration == ettdTransaction)
            out.Put("ON COMMIT DELETE ROWS");
        else if (m_TemporaryTableDuration == ettdSession)
            out.Put("ON COMMIT PRESERVE ROWS");
        else
            _CHECK_AND_THROW_(0, "Temporary table error:\ninvalid duration!");

        out.NewLine();
    }

    void Table::writeIOTClause (TextOutput& out, const WriteSettings& settings) const
    {
        const Constraint* pk = FindPrimaryKey();
        _CHECK_AND_THROW_(pk, "IOT error:\nPK not found!");
        const Index& index = m_Dictionary.LookupIndex(pk->m_strOwner.c_str(), pk->m_strName.c_str());

        out.PutIndent();
        out.Put("ORGANIZATION INDEX");

        if (m_IOTOverflow_includeColumn > 0)
        {

            out.Put(" INCLUDING ");
            ColumnContainer::const_iterator 
                it = m_Columns.find(m_IOTOverflow_includeColumn - 1);

            _CHECK_AND_THROW_(it != m_Columns.end(), "IOT error:\ninvalide include column!");

            out.SafeWriteDBName(it->second->m_strColumnName);
        }
        out.NewLine();
        
        index.WriteIOTClause(out, settings, m_IOTOverflow_includeColumn > 0);
    }

    void Table::writeClusterClause (TextOutput& out, const WriteSettings&) const
    {
        out.PutIndent();
        out.Put("CLUSTER ");
        out.SafeWriteDBName(m_strClusterName);
        out.Put(" (");
        out.NewLine();

        out.WriteColumns(m_clusterColumns, 2);

        out.PutLine(")");
    }

    void Table::writePartitions (TextOutput& out, const WriteSettings& settings) const
    {
        switch (m_PartitioningType)
        {
        case RANGE:
        case HASH:
        case LIST:
            {
                switch (m_PartitioningType)
                {
                case RANGE:
                    out.PutLine("PARTITION BY RANGE (");
                    out.WriteColumns(m_PartKeyColumns, 2, true);
                    out.PutLine(")");
                    break;
                case HASH:
                    out.PutLine("PARTITION BY HASH (");
                    out.WriteColumns(m_PartKeyColumns, 2, true);
                    out.PutLine(")");
                    break;
                case LIST:
                    out.PutIndent();
                    out.Put("PARTITION BY LIST (");
                    out.SafeWriteDBName(!m_PartKeyColumns.empty() ? *m_PartKeyColumns.begin() : string());
                    out.Put(")");
                    out.NewLine();
                    break;
                }

                if (!m_interval.empty())
                {
                    out.PutIndent();
                    out.Put("INTERVAL (");
                    out.Put(m_interval);
                    out.Put(")");
                    out.NewLine();
                }

                switch (m_subpartitioningType)
                {
                case RANGE:
                    out.PutLine("SUBPARTITION BY RANGE (");
                    out.WriteColumns(m_subpartKeyColumns, 2, true);
                    out.PutLine(")");
                    break;
                case HASH:
                    out.PutLine("SUBPARTITION BY HASH (");
                    out.WriteColumns(m_subpartKeyColumns, 2, true);
                    out.PutLine(")");
                    break;
                case LIST:
                    out.PutIndent();
                    out.Put("SUBPARTITION BY LIST (");
                    out.SafeWriteDBName(!m_subpartKeyColumns.empty() ? *m_subpartKeyColumns.begin() : string());
                    out.Put(")");
                    out.NewLine();
                    break;
                }

                if (!m_subpartTemplates.empty())
                {
                    out.PutLine("SUBPARTITION TEMPLATE");
                    out.PutLine("(");
                    out.MoveIndent(2);

                    int nMaxNameLen = 0;
                    for (PartitionContainer::const_iterator it = m_subpartTemplates.begin(); it != m_subpartTemplates.end(); ++it)
                        nMaxNameLen = max(nMaxNameLen, static_cast<int>((*it)->m_strName.length()));
                
                    int i = 0, count = m_subpartTemplates.size();
                    for (PartitionContainer::const_iterator it = m_subpartTemplates.begin(); it != m_subpartTemplates.end(); ++it, ++i)
                    {
                        out.PutIndent();
                        out.Put("SUBPARTITION ");
                        out.SafeWriteDBName((*it)->m_strName);

                        if (m_subpartitioningType != HASH)
                        {
                            int j = (*it)->m_strName.size();
                            do out.Put(" "); while (++j < nMaxNameLen + 1);

                            out.Put(m_subpartitioningType == RANGE ? "VALUES LESS THAN (" : "VALUES (");
                            out.Put((*it)->m_strHighValue);
                            out.Put(")");
                        }

                        if (!(*it)->m_strTablespaceName.IsNull())
                        {
                            out.Put(" TABLESPACE ");
                            out.SafeWriteDBName((*it)->m_strTablespaceName.GetValue().c_str());
                        }

                        if ((i < count-1)) out.Put(",");
                        out.NewLine();
                    }

                    out.MoveIndent(-2);
                    out.PutLine(")");
                }

                out.PutLine("(");
                out.MoveIndent(2);

                int nMaxNameLen = 0;
                for (PartitionContainer::const_iterator it = m_Partitions.begin(); it != m_Partitions.end(); ++it)
                    nMaxNameLen = max(nMaxNameLen, static_cast<int>((*it)->m_strName.length()));
                
                int i = 0, count = m_Partitions.size();
                for (PartitionContainer::const_iterator it = m_Partitions.begin(); it != m_Partitions.end(); ++it, ++i)
                {
                    out.PutIndent();
                    out.Put("PARTITION ");
                    out.SafeWriteDBName((*it)->m_strName);

                    if (m_PartitioningType != HASH)
                    {
                        int j = (*it)->m_strName.size();
                        do out.Put(" "); while (++j < nMaxNameLen + 1);

                        out.Put(m_PartitioningType == RANGE ? "VALUES LESS THAN (" : "VALUES (");
                        out.Put((*it)->m_strHighValue);
                        out.Put(")");
                    }

                    switch (m_PartitioningType)
                    {
                    case RANGE:
                    case LIST:
                        {
                            TextOutputInMEM out2(true, 2*1024);
                            out2.SetLike(out);
                            out2.MoveIndent(2);
                            
                            TableStorage defStorage;
                            BuildDefault(defStorage, m_Dictionary, m_strOwner, (*it)->m_strTablespaceName.GetValue());
                            defStorage.CopyNotNulls(*this); // copy defaults defined on table level

                            (*it)->WriteStorage(out2, defStorage, settings);
                            out2.TrimRight(" \n\r");

                            if (out2.GetLength())
                            {
                                out.NewLine();
                                out.Put(out2.GetData(), out2.GetLength());
                            }
                        }
                        break;
                    case HASH: // only tablespace is supported for hash partition
                        {
                            if (!(*it)->m_strTablespaceName.IsNull()
                                &&
                                (
                                    settings.IsStorageAlways() 
                                    || settings.m_AlwaysPutTablespace
                                    || !isDefaultTablespace(m_Dictionary, (*it)->m_strTablespaceName.GetValue(), m_strOwner)
                                )
                            )
                            {
                                out.Put(" TABLESPACE ");
                                out.SafeWriteDBName((*it)->m_strTablespaceName.GetValue().c_str());
                            }
                        }
                        break;
                    }

                    if (!(*it)->m_subpartitions.empty())
                        Table::writeSubpartitions(out, settings, (*it)->m_subpartitions);

                    if ((i < count-1)) out.Put(",");
                    out.NewLine();
                }

                out.MoveIndent(-2);
                out.PutLine(")");
                
                if (!m_bRowMovement.IsNull() && m_bRowMovement.GetValue())
                    out.PutLine("ENABLE ROW MOVEMENT");
            }
            break;
        default:
            _CHECK_AND_THROW_(0, "Table writting error:\nunsupported partition type!");
        }    
    }

    void Table::writeSubpartitions (TextOutput& out, const WriteSettings& settings, const PartitionContainer& subpartitions) const
    {
        bool is_default = !settings.IsStorageAlways() && !settings.m_AlwaysPutTablespace;

        if (is_default)
        {
            if (subpartitions.size() != m_subpartTemplates.size()) 
                is_default = false;

            PartitionContainer::const_iterator it1 = m_subpartTemplates.begin();
            PartitionContainer::const_iterator it2 = subpartitions.begin();

            for (; it1 != m_subpartTemplates.end() && it2 != subpartitions.end(); ++it1, ++it2)
            {
                if ((*it1)->m_strHighValue != (*it2)->m_strHighValue)
                {
                    is_default = false;
                    break;
                }
                if (!(*it1)->m_strTablespaceName.IsNull() && (*it1)->m_strTablespaceName != (*it2)->m_strTablespaceName
                || (*it1)->m_strTablespaceName.IsNull() && m_strTablespaceName != (*it2)->m_strTablespaceName
                )
                {
                    is_default = false;
                    break;
                }
            }
        }
        
        if (is_default)
            return;

        out.NewLine();
        out.MoveIndent(2);
        out.PutLine("(");
        out.MoveIndent(2);

        int nMaxNameLen = 0;
        for (PartitionContainer::const_iterator it = subpartitions.begin(); it != subpartitions.end(); ++it)
            nMaxNameLen = max(nMaxNameLen, static_cast<int>((*it)->m_strName.length()));
                
        int i = 0, count = subpartitions.size();
        for (PartitionContainer::const_iterator it = subpartitions.begin(); it != subpartitions.end(); ++it, ++i)
        {
            out.PutIndent();
            out.Put("SUBPARTITION ");
            out.SafeWriteDBName((*it)->m_strName);

            if (m_subpartitioningType != HASH)
            {
                int j = (*it)->m_strName.size();
                do out.Put(" "); while (++j < nMaxNameLen + 1);

                out.Put(m_subpartitioningType == RANGE ? "VALUES LESS THAN (" : "VALUES (");
                out.Put((*it)->m_strHighValue);
                out.Put(")");
            }

            if (!(*it)->m_strTablespaceName.IsNull()
                &&
                (
                    settings.IsStorageAlways() 
                    || settings.m_AlwaysPutTablespace
                    || !isDefaultTablespace(m_Dictionary, (*it)->m_strTablespaceName.GetValue(), m_strOwner)
                )
            )
            {
                out.Put(" TABLESPACE ");
                out.SafeWriteDBName((*it)->m_strTablespaceName.GetValue());
            }

            if ((i < count-1)) out.Put(",");
            out.NewLine();
        }
        out.MoveIndent(-2);
        out.PutIndent();
        out.Put(")");
        out.MoveIndent(-2);
    }

    void Table::WriteComments (TextOutput& out, const WriteSettings& settings) const
    {
        if (!m_strComments.empty()) 
        {
            out.PutIndent();
            out.Put("COMMENT ON TABLE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.Put(" IS ");
            out.WriteSQLString(m_strComments);
            out.Put(settings.m_EndOfShortStatement);
            out.NewLine();
            out.NewLine();
        }

        ColumnContainer::const_iterator
            it(m_Columns.begin()), end(m_Columns.end());

        for (; it != end; it++) 
        {
            const TabColumn& column = *it->second.get();
            if (!column.m_strComments.empty()) 
            {
                out.PutIndent();
                out.Put("COMMENT ON COLUMN ");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                out.Put(".");
                out.SafeWriteDBName(column.m_strColumnName);
                out.Put(" IS ");
                out.WriteSQLString(column.m_strComments);
                out.Put(settings.m_EndOfShortStatement);
                out.NewLine();
            }
        }
        if (m_Columns.size())
            out.NewLine();
    }

    int Table::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_TableDefinition) 
            WriteDefinition(out, settings);
  
        if (settings.m_Indexes) 
            WriteIndexes(out, settings);

        if (settings.m_Constraints) 
        {
            WriteConstraints(out, settings, 'C');
            
            if (m_TableType != ettIOT)
                WriteConstraints(out, settings, 'P');

            WriteConstraints(out, settings, 'U');
            WriteConstraints(out, settings, 'R');
        }

        if (settings.m_Triggers) 
            WriteTriggers(out, settings);
        
        if (settings.m_Comments) 
            WriteComments(out, settings);
/*
        if (settings.m_bGrants)
            WriteGrants(out, settings);
*/
        return 0;
    }

    const Constraint* Table::FindPrimaryKey () const
    {
        set<string>::const_iterator it(m_setConstraints.begin()),
                                    end(m_setConstraints.end());
        for (it; it != end; it++) 
        {
            Constraint& constraint 
                = m_Dictionary.LookupConstraint(m_strOwner.c_str(), it->c_str());
            
            if (constraint.m_strType[0] == 'P')
            {
                return &constraint;
            }
        }

        return 0;
    }

    void Table::WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings& settings) const
    {
        const Constraint* pk = FindPrimaryKey();
        
        if (pk)
            pk->WritePrimaryKeyForIOT(out, settings);
    }

    /// Trigger ////////////////////////////////////////////////////////////////
    // 30.11.2004 bug fix, trigger reverse-engineering fails in OF clause if column name contains "ON_"/"_ON"/"_ON_"
    int Trigger::Write (TextOutput& out, const WriteSettings& settings) const
    {
		int nHeaderLines = 0;

		if (m_strBaseObjectType == "TABLE" || m_strBaseObjectType == "VIEW")
		{
			// TODO: move this class into a separated file
			class TriggerDescriptionAnalyzer : public SyntaxAnalyser
			{
			public:
				TriggerDescriptionAnalyzer () 
					: m_state(READING_TRIGGER_NAME) {}

				virtual void Clear () {}

				virtual void PutToken (const Token& tk)     
				{ 
					// 12.04.2005 bug fix #1181324, (ak) trigger reverse-engineering fails if description contains comments (again)
					if (tk == etEOL || tk == etEOF || tk == etCOMMENT) return;

					switch (m_state)
					{
					case READING_TRIGGER_NAME:
						switch (tk)
						{
						case etBEFORE:
						case etAFTER:
						case etINSTEAD:
						case etFOR:
							m_state = LOOKING_FOR_ON;
							break;
						default:
							m_triggerNameTokens.push_back(tk);
						}
						break;
					case LOOKING_FOR_ON:
						if (tk == etON)
							m_state = READING_TABLE_NAME;
						break;
					case READING_TABLE_NAME:
						switch (m_tableNameTokens.size())
						{
						case 0:
							m_tableNameTokens.push_back(tk);
							break;
						case 1:
							if (tk == etDOT)
								m_tableNameTokens.push_back(tk); 
							else
								m_state = SKIPPING;
							break;
						case 2:
							m_tableNameTokens.push_back(tk); 
							m_state = SKIPPING;
							break;
						}
						break;
					}
				}

				const std::vector<Token> GetTriggerNameTokens () const  { return m_triggerNameTokens; }
				const std::vector<Token> GetTableNameTokens () const    { return m_tableNameTokens; }

			private:
				enum { READING_TRIGGER_NAME, LOOKING_FOR_ON, READING_TABLE_NAME, SKIPPING } m_state;
				std::vector<Token> m_triggerNameTokens; // it might be from 1 to 3 tokens - "schema"."name"
				std::vector<Token> m_tableNameTokens;   // it might be from 1 to 3 tokens - "schema"."name"
			} 
			analyser;

			TokenMapPtr tokenMap(new TokenMap);
			tokenMap->insert(std::map<string, int>::value_type("'"      , etQUOTE              ));
			tokenMap->insert(std::map<string, int>::value_type("\""     , etDOUBLE_QUOTE       ));      
			tokenMap->insert(std::map<string, int>::value_type("("      , etLEFT_ROUND_BRACKET ));
			tokenMap->insert(std::map<string, int>::value_type(")"      , etRIGHT_ROUND_BRACKET));
			tokenMap->insert(std::map<string, int>::value_type("-"      , etMINUS              ));   
			tokenMap->insert(std::map<string, int>::value_type("/"      , etSLASH              ));   
			tokenMap->insert(std::map<string, int>::value_type("*"      , etSTAR               ));   
			tokenMap->insert(std::map<string, int>::value_type("ON"     , etON                 ));
			tokenMap->insert(std::map<string, int>::value_type("BEFORE" , etBEFORE             ));
			tokenMap->insert(std::map<string, int>::value_type("AFTER"  , etAFTER              ));
			tokenMap->insert(std::map<string, int>::value_type("INSTEAD", etINSTEAD            ));
			tokenMap->insert(std::map<string, int>::value_type("."      , etDOT                ));
			tokenMap->insert(std::map<string, int>::value_type("FOR"    , etFOR                )); // for compound

			PlsSqlParser parser(&analyser, tokenMap);
			{
				string buffer;
				istringstream in(m_strDescription.c_str());

				for (int line = 0; getline(in, buffer); line++)
					parser.PutLine(line, buffer.c_str(), buffer.length());

				parser.PutEOF(line);
			}

			const std::vector<Token>& triggerNameTokens = analyser.GetTriggerNameTokens();
			_CHECK_AND_THROW_(triggerNameTokens.size() > 0,// && triggerNameTokens.size() <= 3, 
				"Cannot parse trigger description: cannot extract trigger name!");

			const std::vector<Token>& tableNameTokens  = analyser.GetTableNameTokens();
			_CHECK_AND_THROW_(tableNameTokens.size() > 0,// && tableNameTokens.size() <= 3, 
				"Cannot parse trigger description: cannot extract table name!");

			if (settings.m_GeneratePrompts)
			{
				out.PutIndent();
				out.Put("PROMPT CREATE OR REPLACE TRIGGER ");
				out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName || m_strOwner != m_strTableOwner);
				out.NewLine();
				nHeaderLines++;
			}

			out.PutIndent();
			out.Put("CREATE OR REPLACE TRIGGER ");
			out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName || m_strOwner != m_strTableOwner);
			{
				string buffer;
				istringstream in(
					sub_str(m_strDescription, triggerNameTokens.rbegin()->line, triggerNameTokens.rbegin()->endCol(),
							tableNameTokens.begin()->line, tableNameTokens.begin()->begCol())
					);

				for (int line = 0; getline(in, buffer); line++) 
				{
					if (line) 
					{
						out.NewLine();
						nHeaderLines++;
					}
					out.Put(buffer);
				}
			}
			out.PutOwnerAndName(m_strTableOwner, m_strTableName, settings.m_ShemaName);
			{
				string buffer;
				istringstream in(
					sub_str(m_strDescription, tableNameTokens.rbegin()->line, tableNameTokens.rbegin()->endCol())
					);

				while (getline(in, buffer)) 
				{
					out.PutLine(buffer);
					nHeaderLines++;
				}
			}


			if (!m_strWhenClause.empty()) 
			{
				out.Put("WHEN (");
				out.Put(m_strWhenClause);
				out.PutLine(")");
				nHeaderLines += count_lines(m_strWhenClause.c_str()) + 1;
			}
	        
			if (m_strActionType == "CALL") 
			{
				out.Put("CALL ");
			}

			write_text_block(out, m_strTriggerBody.c_str(), m_strTriggerBody.size(),
							settings.m_SQLPlusCompatibility, false);

			out.PutLine("/");
			out.NewLine();

		}
		else // DDL, Schema and DATABASE triggers
		{
			if (settings.m_GeneratePrompts)
			{
				out.PutIndent();
				out.Put("PROMPT CREATE OR REPLACE TRIGGER ");
				out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName || m_strOwner != m_strTableOwner);
				out.NewLine();
				nHeaderLines++;
			}

			out.PutIndent();
			out.Put("CREATE OR REPLACE TRIGGER ");
			{
				string buffer;
				istringstream in(m_strDescription);
				while (getline(in, buffer)) 
				{
					out.PutLine(buffer);
					nHeaderLines++;
				}
			}

			if (!m_strWhenClause.empty()) 
			{
				out.Put("WHEN (");
				out.Put(m_strWhenClause);
				out.PutLine(")");
				nHeaderLines += count_lines(m_strWhenClause.c_str()) + 1;
			}
	        
			if (m_strActionType == "CALL") 
			{
				out.Put("CALL ");
			}

			write_text_block(out, m_strTriggerBody.c_str(), m_strTriggerBody.size(),
							settings.m_SQLPlusCompatibility, false);

			out.PutLine("/");
			out.NewLine();
		}

		return nHeaderLines;
    }

    /// View ////////////////////////////////////////////////////////////////

    void View::WriteConstraints (TextOutput& out, const WriteSettings& settings, char chType) const
    {
        set<string>::const_iterator it(m_setConstraints.begin()),
                                    end(m_setConstraints.end());
        ConstraintVector vector;

        for (it; it != end; it++) 
        {
            Constraint& constraint = m_Dictionary.LookupConstraint(m_strOwner.c_str(), 
                                                                   it->c_str());
            if (chType == constraint.m_strType[0]
            && !constraint.IsNotNullCheck())
              vector.push_back(&constraint);
        }
        stable_sort(vector.begin(), vector.end(), least_constaint);

        ConstraintVectorIter vecIt(vector.begin()), vecEndIt(vector.end());

        for (; vecIt != vecEndIt; vecIt++) 
            if (*vecIt)
                (*vecIt)->Write(out, settings);
    }

    void View::WriteTriggers (TextOutput& out, const WriteSettings& settings) const
    {
        set<string>::const_iterator it(m_setTriggers.begin()),
                                    end(m_setTriggers.end());

        for (it; it != end; it++)
            m_Dictionary.LookupTrigger(it->c_str()).Write(out, settings);
    }

    void View::WriteComments (TextOutput& out, const WriteSettings& settings) const
    {
        if (!m_strComments.empty()) 
        {
            out.PutIndent();
            out.Put("COMMENT ON TABLE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.Put(" IS ");
            out.WriteSQLString(m_strComments);
            out.Put(settings.m_EndOfShortStatement);
            out.NewLine();
            out.NewLine();
        }

        map<int,string>::const_iterator it(m_Columns.begin()),
                                        end(m_Columns.end());

        for (int i(0); it != end; it++, i++) 
        {
            map<int,string>::const_iterator comIt = m_mapColComments.find(i);

            if (comIt != m_mapColComments.end()) 
            {
                out.PutIndent();
                out.Put("COMMENT ON COLUMN ");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                out.Put(".");
                out.SafeWriteDBName(it->second);
                out.Put(" IS ");
                out.WriteSQLString((*comIt).second);
                out.Put(settings.m_EndOfShortStatement);
                out.NewLine();
            }
        }
        if (m_mapColComments.size())
            out.NewLine();
    }

    int View::Write (TextOutput& out, const WriteSettings& settings) const
    {
        int nHeaderLines = 0;

        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE OR REPLACE VIEW ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
            nHeaderLines++;
        }

        if (!settings.m_ViewWithTriggers
        && m_setTriggers.size()) 
        {
            out.PutLine("PROMPT This view has instead triggers");
            nHeaderLines++;
        }
        
        out.PutIndent();
        out.Put("CREATE OR REPLACE");
        if (settings.m_ViewWithForce) out.Put(" FORCE");
        
        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            nHeaderLines++;
            out.NewLine();
            out.PutIndent();
        }
        
        out.Put("VIEW ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        out.Put(" (");

        if (settings.m_CommentsAfterColumn
        && !m_strComments.empty()) 
        {
            out.Put(" -- ");
            out.Put(m_strComments);
        }
        out.NewLine();
        nHeaderLines++;

        int nMaxColLength = 0;
        int nSize = m_Columns.size();

        map<int,string>::const_iterator it(m_Columns.begin()),
                                        end(m_Columns.end());
        for (; it != end; it++)
            nMaxColLength = max(nMaxColLength, static_cast<int>(it->second.size()));

        nMaxColLength += 2;
        nMaxColLength = max(nMaxColLength, settings.m_CommentsPos);

        out.MoveIndent(2);
        it = m_Columns.begin();
        for (int i = 0; it != end; it++, i++) 
        {
            out.PutIndent();
            int nPos = out.GetPosition();
            out.SafeWriteDBName(it->second);

            if (i < (nSize - 1)) out.Put(",");

            if (settings.m_CommentsAfterColumn) 
            {
                map<int,string>::const_iterator comIt = m_mapColComments.find(i);

                if (comIt != m_mapColComments.end()) 
                {
                    int j = out.GetPosition() - nPos;
                    while (j++ < nMaxColLength)
                        out.Put(" ");
                    out.Put("-- ");
                    out.Put((*comIt).second);
                }
            }
            out.NewLine();
            nHeaderLines++;
        }

        out.MoveIndent(-2);
        out.PutLine(") AS");
        nHeaderLines++;

        write_text_block(out, m_strText.c_str(), m_strText.size(),
                         settings.m_SQLPlusCompatibility, settings.m_SQLPlusCompatibility);
/*        
        if (!settings.m_SQLPlusCompatibility)
            out.PutLine(m_strText.c_str(), skip_back_space(m_strText.c_str(), m_strText.size()));
        else {
            istrstream i(m_strText.c_str(), m_strText.size());
            string buffer;

            while (getline(i, buffer, '\n')) 
            {
                int len = skip_back_space(buffer.c_str(), buffer.size());
                if (len) out.PutLine(buffer.c_str(), len);
            }
        }
*/
        if (!m_strViewConstraint.empty())
        {
            out.MoveIndent(1);
            out.PutIndent();
            out.Put("CONSTRAINT ");
            out.SafeWriteDBName(m_strName);
            out.NewLine();
            out.MoveIndent(-1);
        }

        out.PutLine("/");
        out.NewLine();

        if (settings.m_Constraints) 
        {
            WriteConstraints(out, settings, 'C');
            WriteConstraints(out, settings, 'P');
            WriteConstraints(out, settings, 'U');
            WriteConstraints(out, settings, 'R');
        }

        if (settings.m_ViewWithTriggers) 
            WriteTriggers(out, settings);
        
        if (settings.m_Comments)         
            WriteComments(out, settings);
/*        
        if (settings.m_bGrants)
            WriteGrants(out, settings);
*/
        return nHeaderLines;
    }

    /// Sequence ////////////////////////////////////////////////////////////////

    Sequence::Sequence (Dictionary& dictionary) 
    : DbObject(dictionary) 
    {
        m_bCycleFlag =
        m_bOrderFlag = false;
    }

    int Sequence::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE SEQUENCE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
        }

        out.PutIndent();
        out.Put("CREATE");
        
        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("SEQUENCE ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        out.NewLine();

        out.MoveIndent(2);

        out.PutIndent();
        out.Put("MINVALUE ");
        out.Put(m_strMinValue);
        out.NewLine();

        out.PutIndent();
        out.Put("MAXVALUE ");
        out.Put(m_strMaxValue);
        out.NewLine();

        out.PutIndent();
        out.Put("INCREMENT BY ");
        out.Put(m_strIncrementBy);
        out.NewLine();

        if (settings.m_SequnceWithStart) 
        {
            out.PutIndent();
            out.Put("START WITH ");
            out.Put(m_strLastNumber);
            out.NewLine();
        }

        out.PutIndent();
        out.Put(m_bCycleFlag ? "CYCLE" : "NOCYCLE");
        out.NewLine();

        out.PutIndent();
        out.Put(m_bOrderFlag ? "ORDER" : "NOORDER");
        out.NewLine();

        out.PutIndent();
        if (m_strCacheSize.compare("0")) 
        {
            out.Put("CACHE ");
            out.Put(m_strCacheSize);
        } else {
            out.Put("NOCACHE");
        }
        out.NewLine();

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();
/*
        if (settings.m_bGrants)
            WriteGrants(out, settings);
*/
        return 0;
    }

    string Sequence::GetIdentityOptions (const WriteSettings& settings) const
    {
        string buff;

        if (m_strMinValue != "1")
            buff += "MINVALUE " + m_strMinValue;

        if (m_strMaxValue != "9999999999999999999999999999")
        {
            if (!buff.empty()) buff += ' ';
            buff += "MAXVALUE " + m_strMaxValue;
        }

        if (m_strIncrementBy != "1")
        {
            if (!buff.empty()) buff += ' ';
            buff += "INCREMENT BY " + m_strIncrementBy;
        }

        if (settings.m_SequnceWithStart) 
        {
            if (m_strLastNumber != "1")
            {
                if (!buff.empty()) buff += ' ';
                buff += "START WITH " + m_strLastNumber;
            }
        }

        if (m_bCycleFlag)
        {
            if (!buff.empty()) buff += ' ';
            buff += "CYCLE";
        }

        if (m_bOrderFlag)
        {
            if (!buff.empty()) buff += ' ';
            buff += "ORDER";
        }

        if (m_strCacheSize == "0") 
        {
            if (!buff.empty()) buff += ' ';
            buff += "NOCACHE";
        }
        else if (m_strCacheSize != "20") 
        {
            if (!buff.empty()) buff += ' ';
            buff += "CACHE " + m_strCacheSize;
        }

        return buff;
    }


    /// PlsCode ////////////////////////////////////////////////////////////////

    int PlsCode::Write (TextOutput& out, const WriteSettings& settings) const
    {
        int nHeaderLines = 0;
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE OR REPLACE ");
            out.Put(m_strType);
            out.Put(" ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
            nHeaderLines++;
        }

        out.PutIndent();
        out.Put("CREATE OR REPLACE");

        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
            nHeaderLines++;
        }

        int nLen;
        const char* pszText = m_strText.c_str();
        pszText += skip_space_and_comments(pszText);
        get_word_SP(pszText, pszText, nLen);

        if (strnicmp(m_strType.c_str(), pszText, m_strType.size()))
        {
            int nSecond;
            const char* pszSecond;
            get_word_SP(pszText + nLen, pszSecond, nSecond);

            if (!(m_strType == "PACKAGE BODY" || m_strType == "TYPE BODY")
            || !(!strnicmp("PACKAGE", pszText, sizeof("PACKAGE") - 1) 
                 || !strnicmp("TYPE", pszText, sizeof("TYPE") - 1))
            || strnicmp("BODY", pszSecond, sizeof("BODY") - 1))
            {
                _CHECK_AND_THROW_(0, "Loading Error: a body header is invalid!");
            }

            nLen = pszSecond + nSecond - pszText;
            out.Put(pszText, nLen);
            pszText += nLen;
        }
        else
        {
            out.Put(pszText, m_strType.size());
            pszText += m_strType.size();
        }

        out.Put(" ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
/*
        out.Put(" ");
*/
        pszText += skip_db_name(pszText);
/*
        out.PutLine(pszText, skip_back_space(pszText, strlen(pszText)));
*/
        write_text_block(out, pszText, strlen(pszText),
                         settings.m_SQLPlusCompatibility, false);

        out.PutLine("/");
        out.NewLine();
/*
        if (settings.m_bGrants && stricmp(GetTypeStr(), "PACKAGE BODY"))
            WriteGrants(out, settings);
*/
        return nHeaderLines;
    }

    /// Type ///////////////////////////////////////////////////////////////////

    void Type::WriteAsIncopmlete (TextOutput& out, const WriteSettings& settings) const
    {
        bool parseError = false;

        const char* pszBuff = m_strText.c_str();
        pszBuff += skip_space_and_comments(pszBuff);
        parseError |= !strnicmp(pszBuff, "TYPE", sizeof("TYPE")-1) ? false : true;
        pszBuff += sizeof("TYPE")-1;
        parseError |= isspace(*pszBuff) ? false : true;
        pszBuff += skip_space_and_comments(pszBuff);
        pszBuff += skip_db_name(pszBuff);
        parseError |= isspace(*pszBuff) ? false : true;
        pszBuff += skip_space_and_comments(pszBuff);
        parseError |= 
            (!strnicmp(pszBuff, "AS", sizeof("AS")-1) || !strnicmp(pszBuff, "IS", sizeof("IS")-1)) ? false : true;
        pszBuff += sizeof("AS")-1;
        parseError |= isspace(*pszBuff) ? false : true;
        pszBuff += skip_space_and_comments(pszBuff);

        _CHECK_AND_THROW_(!parseError, "Type parsing error: invalid header!");

        if (!(!strnicmp(pszBuff, "TABLE", sizeof(pszBuff)-1) && isspace(pszBuff[sizeof("TABLE")-1])))
        {
            out.PutIndent();
            out.Put("CREATE OR REPLACE TYPE ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.Put(";");
            out.NewLine();
            out.PutLine("/");
        }
    }

    /// PlsCode ////////////////////////////////////////////////////////////////

    int JavaSource::Write (TextOutput& out, const WriteSettings& settings) const
    {
        int nHeaderLines = 0;
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE OR REPLACE JAVA SOURCE ");
            out.Put(m_strType);
            out.Put(" ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
            nHeaderLines++;
        }

        out.PutIndent();
        out.Put("CREATE OR REPLACE AND COMPILE");

        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
            nHeaderLines++;
        }

        out.Put("JAVA SOURCE NAMED ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        out.Put(" AS ");
        out.NewLine();

        write_text_block(out, m_strText.c_str(), m_strText.length(),
                         settings.m_SQLPlusCompatibility, false);

        out.PutLine("/");
        out.NewLine();

        return nHeaderLines;
    }

    /// Synonym ////////////////////////////////////////////////////////////////

    int Synonym::Write (TextOutput& out, const WriteSettings& settings) const
    {
        // 16.11.2003 bug fix, missing public keyword for public synonyms
        if (m_strOwner == "PUBLIC")
        {
            if (settings.m_GeneratePrompts)
            {
                out.PutIndent();
                out.Put("PROMPT CREATE PUBLIC SYNONYM ");
                out.SafeWriteDBName(m_strName);
                out.NewLine();
            }

            out.PutIndent();
            out.Put("CREATE PUBLIC SYNONYM ");
            out.SafeWriteDBName(m_strName);
        }
        else
        {
            if (settings.m_GeneratePrompts)
            {
                out.PutIndent();
                out.Put("PROMPT CREATE SYNONYM ");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                out.NewLine();
            }

            out.PutIndent();
            out.Put("CREATE SYNONYM ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        }

        out.Put(" FOR ");
        out.PutOwnerAndName(m_strRefOwner, m_strRefName, settings.m_ShemaName || m_strOwner != m_strRefOwner);
        if (!m_strRefDBLink.empty()) 
        {
            out.Put("@");
            out.SafeWriteDBName(m_strRefDBLink);
        }
        out.Put(settings.m_EndOfShortStatement);
        out.NewLine();

        return 0;
    }

    /// Grant ///////////////////////////////////////////////////////////////

    // 06.04.2003 bug fix, some oracle 9i privilege are not supported
    int Grant::Write (TextOutput& out, const WriteSettings& settings) const
    {
        // Write GRANTs with global privileges. Example:
        // GRANT UPDATE ON table TO user;
        for (int i(0); i < 2; i++) 
        {
            const set<string>& privileges = !i ? m_privileges : m_privilegesWithAdminOption;
            set<string>::const_iterator it(privileges.begin()), end(privileges.end());

            if (it != end) 
            {
                TextOutputInMEM out2(true, 512);
                out2.SetLike(out);

                bool commaInList = false;

                for (; it != end; ++it)
                {
                    if (commaInList) out2.Put(",");
                    commaInList = true;
                    out2.Put(*it);
                }

                if (out2.GetLength() > 0)
                {
                    out.Put("GRANT ");

                    out.Put(out2.GetData(), out2.GetLength());

                    out.Put(" ON ");
                    out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                    out.Put(" TO ");
                    out.SafeWriteDBName(m_strGrantee);

                    if(i) out.Put(" WITH GRANT OPTION");

                    out.Put(settings.m_EndOfShortStatement);
                    out.NewLine();
                }
            }
            
            if (privileges.find("ENQUEUE") != end)
            {
                out.Put("exec Dbms_Aqadm.Grant_Queue_Privilege(\'enqueue', \'");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                
                out.Put("\',\'");
                out.SafeWriteDBName(m_strGrantee);
                out.Put("\',\'");

                if(!i) out.Put("\', FALSE);");
                else   out.Put("\', TRUE);");
            }

            if (privileges.find("DEQUEUE") != end)
            {
                out.Put("exec Dbms_Aqadm.Grant_Queue_Privilege(\'dequeue', \'");
                out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                out.Put("\',\'");
                out.SafeWriteDBName(m_strGrantee);

                if(!i) out.Put("\', FALSE);");
                else   out.Put("\', TRUE);");
                
                out.NewLine();
            }
        }

        // Write GRANTs with column privileges. Example:
        // GRANT UPDATE(colx) ON table TO user;
        for (i = 0; i < 2; i++) 
        {
            const std::map<string,set<string> >& colPrivileges = (!i) ? m_colPrivileges : m_colPrivilegesWithAdminOption;
            std::map<string,set<string> >::const_iterator it(colPrivileges.begin()), end(colPrivileges.end());

            if (it != end) 
            {
                TextOutputInMEM out2(true, 512);
                out2.SetLike(out);

                bool commaInList = false;

                for (; it != end; ++it)
                {
                    if (commaInList) out2.Put(", ");
                    commaInList = true;
                    out2.Put(it->first);
                    out2.Put("("); 

                    bool commaInColList = false;
                    std::set<string>::const_iterator colIt(it->second.begin()); 
                    for (; colIt != it->second.end(); ++colIt)
                    {
                        if (commaInColList) out2.Put(", ");
                        commaInColList = true;
                        out2.SafeWriteDBName(*colIt); 
                    }

                    out2.Put(")");
                }

                if (out2.GetLength() > 0)
                {
                    out.Put("GRANT ");

                    out.Put(out2.GetData(), out2.GetLength());

                    out.Put(" ON ");
                    out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
                    out.Put(" TO ");
                    out.SafeWriteDBName(m_strGrantee);

                    if(i) out.Put(" WITH GRANT OPTION");

                    out.Put(settings.m_EndOfShortStatement);
                    out.NewLine();
                }
            }
        }

        return 0;
    }

    /// GrantContainer //////////////////////////////////////////////////////
    // If it's grantor then the arguments are (grantee_name, object_name)
    // If it's grantee then the arguments are (grantor_name, object_name)

    Grant& GrantContainer::CreateGrant (const char* grantxx, const char* object)
    {   
        char key[cnDbKeySize];
    
        GrantPtr ptr(new Grant);
    
        if (m_mapGrants.find(make_key(key, grantxx, object)) == m_mapGrants.end())
            m_mapGrants.insert(GrantMap::value_type(key, ptr));
        else
            throw XAlreadyExists(key);

        return *(ptr.get());
    }
    
    
    Grant& GrantContainer::LookupGrant (const char* grantxx, const char* object)
    {   
        char key[cnDbKeySize];
    
        GrantMapConstIter it = m_mapGrants.find(make_key(key, grantxx, object));
    
        if (it == m_mapGrants.end())
            throw XNotFound(key);

        return *(it->second.get());
    }


    void GrantContainer::DestroyGrant (const char* grantxx, const char* object)
    {   
        char key[cnDbKeySize];

        if (m_mapGrants.erase(make_key(key, grantxx, object)))
            throw XNotFound(key);
    }


        inline int least (const Grant* g1, const Grant* g2)
        {
            return (g1->m_strGrantee < g2->m_strGrantee)
                   && (g1->m_strOwner < g2->m_strOwner) 
                   && (g1->m_strName < g2->m_strName);
        }

        typedef vector<const Grant*>  GrantVector;
        typedef GrantVector::iterator GrantVectorIter;

    int GrantContainer::Write (TextOutput& out, const WriteSettings& settings) const
    {
        GrantVector vecObjGrants;

        GrantMapConstIter mapIt(m_mapGrants.begin()), 
                          mapEndIt(m_mapGrants.end());

        for (; mapIt != mapEndIt; mapIt++) 
            vecObjGrants.push_back(((*mapIt).second.get()));

        stable_sort(vecObjGrants.begin(), vecObjGrants.end(), least);

        GrantVectorIter vecIt(vecObjGrants.begin()),
                        vecEndIt(vecObjGrants.end());

        for (; vecIt != vecEndIt; vecIt++) 
            if (*vecIt)
                (*vecIt)->Write(out, settings);

        return 0;
    }
    
    void GrantContainer::EnumGrants (void (*pfnDo)(const Grant&, void*), void* param, bool sort) const
    {
        GrantMapConstIter mapIt(m_mapGrants.begin()), 
                          mapEndIt(m_mapGrants.end());

        if (!sort)
            for (; mapIt != mapEndIt; mapIt++) 
                pfnDo(*(*mapIt).second.get(), param);
        else {
            GrantVector vecObjGrants;

            for (; mapIt != mapEndIt; mapIt++) 
                vecObjGrants.push_back(((*mapIt).second.get()));

            stable_sort(vecObjGrants.begin(), vecObjGrants.end(), least);

            GrantVectorIter vecIt(vecObjGrants.begin()),
                            vecEndIt(vecObjGrants.end());

            for (; vecIt != vecEndIt; vecIt++) 
                if (*vecIt)
                    pfnDo(**vecIt, param);
        }
    }
    
        struct _WriteGrantContext
        {
            const char*          m_name;
            TextOutput&          m_out;
            const WriteSettings& m_settings;

            _WriteGrantContext (const char* name, TextOutput& out, const WriteSettings& settings)
            : m_name(name), m_out(out), m_settings(settings)
            {
            }
        };

        void write_object_grant (const Grant& grant, void* param)
        {
            _WriteGrantContext& context = *reinterpret_cast<_WriteGrantContext*>(param);
            if (grant.m_strName == context.m_name)
                grant.Write(context.m_out, context.m_settings);
        }

    void Grantor::WriteObjectGrants (const char* name, TextOutput& out, const WriteSettings& settings) const
    {
        // use EnumGrants for sorting
        _WriteGrantContext cxt(name, out, settings);
        EnumGrants(write_object_grant, &cxt, true);
        // ! out.NewLine();
    }

    /// Cluster /////////////////////////////////////////////////////////////

    Cluster::Cluster (Dictionary& dictionary) 
    : DbObject(dictionary) 
    {
        m_bIsHash =
        m_bCache  = false;
    }

    int Cluster::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE CLUSTER ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
        }

        out.PutIndent();
        out.Put("CREATE");

        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("CLUSTER ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        out.Put(" (");
        out.NewLine();

        out.MoveIndent(2);

        ColContainer::const_iterator 
            it(m_Columns.begin()), 
            end(m_Columns.end());

        string type;

        for (int i = m_Columns.size() - 1; it != end; it++, i--) 
        {
            const Column& column = *it->second.get();

            out.PutIndent();
            out.SafeWriteDBName(column.m_strColumnName);
            out.Put(" ");
            column.GetSpecString(type, settings);
            out.Put(type);
            //out.Put(column.m_bNullable ? " NULL" : " NOT NULL");
            if (i) out.Put(",");
            out.NewLine();
        }
        out.MoveIndent(-2);
        out.PutLine(")");
        out.MoveIndent(2);
        
        if (!m_strSize.empty()) 
        {
            out.PutIndent();
            out.Put("SIZE ");
            out.Put(m_strSize);
            out.NewLine();
        }
        
        if (m_bIsHash) 
        {
            out.PutIndent();
            out.Put("HASHKEYS ");
            out.Put(m_strHashKeys);
            out.NewLine();

            if (!m_strHashExpression.empty()) 
            {
                out.PutIndent();
                out.Put("HASH IS ");
                out.Put(m_strHashExpression);
                out.NewLine();
            }
        }

        TableStorage defStorage;
        BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
        defStorage.m_nIniTrans.SetDefault(2);

        WriteStorage(out, defStorage, settings);

        if (m_bCache) out.PutLine("CACHE");

        out.MoveIndent(-2);
        out.PutLine("/");
        out.NewLine();

        // write indexes
        {
            set<string>::const_iterator it(m_setIndexes.begin()),
                                        end(m_setIndexes.end());
            for (it; it != end; it++)
                m_Dictionary.LookupIndex(it->c_str()).Write(out, settings);
        }

        return 0;
    }


    /// DATABASE LINK object ////////////////////////////////////////////////
    
    int DBLink::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            if (m_strOwner == "PUBLIC")
                out.Put("PROMPT CREATE PUBLIC DATABASE LINK ");
            else
                out.Put("PROMPT CREATE DATABASE LINK ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
        }

        out.PutIndent();

        // 10.11.2003 bug fix, missing public keyword for public database links
        if (m_strOwner == "PUBLIC")
            out.Put("CREATE PUBLIC DATABASE LINK ");
        else
            out.Put("CREATE DATABASE LINK ");

        out.SafeWriteDBName(m_strName); // don't have owner

        if (!m_strUser.empty() || !m_strHost.empty()) 
        {
            out.NewLine();
            out.MoveIndent(2);
            out.PutIndent();
            if (!m_strUser.empty())
            {
                out.Put("CONNECT TO ");
                out.SafeWriteDBName(m_strUser);
                out.Put(" IDENTIFIED BY ");
                if (!m_strPassword.empty())
                    out.Put( m_strPassword);
                else
                    out.Put("\'NOT ACCESSIBLE\'");
            }
            if (!m_strHost.empty())
            {
                out.Put(" USING \'"); 
                out.Put(m_strHost);    
                out.Put("\'"); 
                out.MoveIndent(-2);
            }
        }
        out.Put(settings.m_EndOfShortStatement);
        out.NewLine();
        out.NewLine();

        return 0;
    }

    /// SNAPSHOT LOG object /////////////////////////////////////////////////

    int SnapshotLog::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE SNAPSHOT LOG ON ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
        }

        out.PutIndent();
        out.Put("CREATE");
        
        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("SNAPSHOT LOG ON ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
        out.NewLine();

        out.MoveIndent(2);

        TableStorage defStorage;
        BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
        WriteStorage(out, defStorage, settings);

        if (settings.m_StorageSubstitutedClause)
            write_substituted_clause(out, m_strName, "_st");

        out.MoveIndent(-2);

        out.PutLine("/");
        out.NewLine();

        return 0;
    }

    /// SNAPSHOT object /////////////////////////////////////////////////////

    int Snapshot::Write (TextOutput& out, const WriteSettings& settings) const
    {
        if (settings.m_GeneratePrompts)
        {
            out.PutIndent();
            out.Put("PROMPT CREATE SNAPSHOT ");
            out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);
            out.NewLine();
        }

        out.PutIndent();
        out.Put("CREATE");
        
        if (settings.m_SQLPlusCompatibility) 
        {
            out.Put(" ");
        }
        else 
        {
            out.NewLine();
            out.PutIndent();
        }

        out.Put("SNAPSHOT ");
        out.PutOwnerAndName(m_strOwner, m_strName, settings.m_ShemaName);

        if (settings.m_CommentsAfterColumn
        && !m_strComments.empty()) 
        {
            out.Put(" -- ");
            out.Put(m_strComments);
        }
        out.NewLine();


        out.MoveIndent(2);

        if (m_strClusterName.empty())
        {
            TableStorage defStorage;
            BuildDefault(defStorage, m_Dictionary, m_strOwner, m_strTablespaceName.GetValue());
            WriteStorage(out, defStorage, settings);

            if (settings.m_StorageSubstitutedClause)
                write_substituted_clause(out, m_strName, "_st");
        }
        else 
        {
            out.PutIndent();
            out.Put("CLUSTER ");
            out.SafeWriteDBName(m_strClusterName);
            out.Put(" (");
            out.NewLine();

            out.WriteColumns(m_clusterColumns, 2);

            out.PutLine(")");
        }

        out.MoveIndent(-2);

        //TODO: implement USING INDEX for snapshots

        out.PutLine("REFRESH");
        
        out.MoveIndent(2);
        out.PutIndent();
        out.Put(m_strRefreshType);

        if (!m_strStartWith.empty())
        {
            out.Put(" START WITH ");
            out.Put(m_strStartWith);
        }

        if (!m_strNextTime.empty())
        {
            out.Put(" NEXT ");
            out.Put(m_strNextTime);
        }

        out.MoveIndent(-2);


        out.PutLine("AS");
        write_text_block(out, m_strQuery.c_str(), m_strQuery.size(),
                         settings.m_SQLPlusCompatibility, settings.m_SQLPlusCompatibility);

        out.PutLine("/");
        out.NewLine();

        //TODO: implement WriteComments for snapshots

        return 0;
    }

}// END namespace OraMetaDict
