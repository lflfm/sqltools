/* 
	SQLTools is a tool for Oracle database developers and DBAs.
    Copyright (C) 1997-2015 Aleksey Kochetov

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

#ifndef __METAOBJECTS_H__
#define __METAOBJECTS_H__
#pragma once
#pragma warning ( push )
#pragma warning ( disable : 4786 )

//
// Purpose: Class library for storing oracle db objects in meta-dictionary
// Note: Copy-constraction & assign-operation is not supported for all classes
//

// 04.08.2002 bug fix: debug privilege for 9i
// 17.09.2002 bug fix: cluster reengineering fails
// 06.04.2003 bug fix, some oracle 9i privilege are not supported
// 03.08.2003 improvement, domain index support
// 25.02.2007 improvement, partition support

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include "arg_shared.h"
#include <Common/Properties.h>
#include "MetaDict/MetaSettings.h"

namespace OraMetaDict 
{
    class Dictionary;
    class TextOutput;

    using std::string;
    using std::set;
    using std::map;
    using std::list;
    using std::vector;
    using arg::counted_ptr;

    /// Abstract base class for all db object classes ///////////////////////
    class DbObject 
    {
    public:
        DbObject (Dictionary& dictionary) 
            : m_Dictionary(dictionary) {};
        virtual ~DbObject () {};

        Dictionary& GetDictionary ()      const { return m_Dictionary; }
        const char* GetOwner  ()          const { return m_strOwner.c_str(); }
        const char* GetName   ()          const { return m_strName.c_str(); }
  
        virtual bool IsCode ()      const       { return false; }
        virtual bool IsLight ()     const       { return false; }
        virtual bool IsGrantable () const       { return false; }
        virtual bool IsGenerated () const       { return false; }

        virtual const char* GetDefExt ()  const { return "sql"; };
        virtual const char* GetTypeStr () const = 0;

        // return code offset for functions,procedures,packages & views
        virtual int  Write (TextOutput& out, const WriteSettings&) const = 0;
        virtual void WriteGrants (TextOutput& out, const WriteSettings&) const;

    protected:
        Dictionary& m_Dictionary;
    public:
        string m_strOwner,
               m_strName;

    private:
        // copy-constraction & assign-operation is not supported
        DbObject (const DbObject&);
        void operator = (const DbObject&);
    };


    /// USER object /////////////////////////////////////////////////////////
    class User : public DbObject 
    {
    public:
        string m_strDefTablespace;
    public:

        User (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        virtual const char* GetTypeStr () const { return "USER"; };
        virtual int Write (TextOutput&, const WriteSettings&) const { return 0; };

    private:
        // copy-constraction & assign-operation is not supported
        User (const User&);
        void operator = (const User&);
    };


     template <class T>
     class Variable
     {
        T m_value;
        int m_status;
     public:

        Variable () : m_status(0) {}

        void Set (const T& val)         { m_value = val; m_status = 0x01; }
        void SetDefault (const T& val)  { m_value = val; m_status = 0x01|0x02; }
        void SetIfNotNull (const Variable<T>& other) 
        { 
            if (!other.IsNull()) { 
                m_value = other.m_value; 
                m_status = other.m_status; 
            } 
        }
        
        void SetNull  ()                { m_status = 0; }
        void SetDefault ()              { m_status |= 0x02; }

        bool IsNull () const            { return !m_status; }
        bool IsDefault () const         { return 0x02 & m_status ? true : false; }

        bool IsPrintable (bool force, const Variable<T>* other) const
        {
            if (IsNull()) return false;
            if (force) return true;
            if (other && !other->IsNull() && m_value == other->m_value) return false;
            return true;
        }

        const T& GetValue () const { return m_value; }

        bool operator != (const Variable& other) const
        {
            return IsNull() && !other.IsNull()
                || !IsNull() && other.IsNull()
                || GetValue() != other.GetValue();
        }
    };

    /// Storage parameters for tablespaces, tables, indices & clusters //////
    class Storage
    {
    public:
        Storage ();

        void WriteTablespace (TextOutput& out) const;
        void WriteStorage (TextOutput& out, const Storage& defStorage, const WriteSettings&) const;
        void CopyNotNulls (const Storage&);

    public:
        Variable<string> m_strTablespaceName;
        Variable<int>    m_nInitialExtent, m_nNextExtent, m_nMinExtents, m_nMaxExtents, m_nPctIncrease;
        Variable<bool>   m_bLogging, m_bCompression;
        Variable<string> m_strCompressFor;
    };


    /// TABLESPACE object ///////////////////////////////////////////////////
    class Tablespace : public DbObject, public Storage 
    {
    public:
        Tablespace (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual const char* GetTypeStr () const { return "TABLESPACE"; };
        virtual int Write (TextOutput&, const WriteSettings&) const { return 0; };

    private:
        // copy-constraction & assign-operation is not supported
        Tablespace (const Tablespace&);
        void operator = (const Tablespace&);
    };


    /// Additional storage parameters for tables & indices //////////////////
    class TableStorage : public Storage 
    {
    public:
        TableStorage ();

        void WriteParallel (TextOutput& out, const TableStorage& defStorage, const WriteSettings& settings) const;
        void WriteStorage (TextOutput&, const TableStorage& defStorage, const WriteSettings&) const;
        void CopyNotNulls (const TableStorage&);

        static void BuildDefault (TableStorage&, const Dictionary& dictionary, const string& schema, const string& tablespace);

    public:
        Variable<int> m_nFreeLists, m_nFreeListGroups, m_nPctFree, m_nPctUsed, 
            m_nIniTrans, m_nMaxTrans, m_nDegree, m_nInstances;
        Variable<string> m_strBufferPool;
    };

    class IndexStorage : public TableStorage
    {
    public:
        IndexStorage (/**Dictionary& dictionary*/) { m_nCompressionPrefixLength = -1; }

    public:
        int m_nCompressionPrefixLength; // for indexes only
    };
    
    /// Table partitions subobject //////////////////////////////////////////
    enum PartitioningType { NONE, RANGE, LIST, HASH };

    class Partition : public TableStorage
    {
    public:
        typedef counted_ptr<Partition> PartitionPtr;
        typedef vector<PartitionPtr> PartitionContainer;

        PartitionContainer m_subpartitions;

        string m_strName;
        string m_strHighValue;

        Partition () {}

    private:
        // copy-constraction & assign-operation is not supported
        Partition (const Partition&);
        void operator = (const Partition&);
    };

    /// Base columns subobject (for tables & clusters) //////////////////////
    enum CharLengthSemantics { USE_BYTE, USE_CHAR };

    class Column 
    {
    public:
        string m_strColumnName,
               m_strDataType;
        int    m_nDataPrecision,
               m_nDataScale,
               m_nDataLength;
        bool   m_bNullable;

        CharLengthSemantics m_charLengthSemantics, m_defCharLengthSemantics;

        void GetSpecString (string& strBuff, const WriteSettings&) const;

        Column ();

    private:
        class SubtypeMap : public map<string, int> {
        public:
            SubtypeMap ();
        };
        static SubtypeMap m_mapSubtypes;

    private:
        // copy-constraction & assign-operation is not supported
        Column (const Column&);
        void operator = (const Column&);
    };


    /// Table columns subobject /////////////////////////////////////////////
    class TabColumn : public Column
    {
    public:
        string m_strDataDefault,
               m_strComments;
        bool m_bVirtual;
        bool m_bIdentity;
        string m_strSequenceName, m_strIndentityGenerationType;

        TabColumn () : m_bVirtual(false), m_bIdentity(false) {}

    private:
        // copy-constraction & assign-operation is not supported
        TabColumn (const TabColumn&);
        void operator = (const TabColumn&);
    };

    /// Index partitions subobject //////////////////////////////////////////
    class IndexPartition : public IndexStorage
    {
    public:
        typedef counted_ptr<IndexPartition> PartitionPtr;
        typedef vector<PartitionPtr> PartitionContainer;

        PartitionContainer m_subpartitions;

        string m_strName;
        string m_strHighValue;

        IndexPartition () {}

    private:
        // copy-constraction & assign-operation is not supported
        IndexPartition (const IndexPartition&);
        void operator = (const IndexPartition&);
    };

    /// INDEX object ////////////////////////////////////////////////////////
    enum IndexType { eitNormal, eitBitmap, eitCluster, eitFunctionBased, eitFunctionBasedBitmap, eitIOT_TOP, eitDomain };

    class Index : public DbObject, public IndexStorage  
    {
    public:
        Index (Dictionary& dictionary);

        virtual const char* GetTypeStr () const { return "INDEX"; };
        virtual bool IsGenerated ()       const { return m_bGenerated; }
        virtual int Write (TextOutput& out, const WriteSettings&) const;

        void WriteIndexParams (TextOutput&, const WriteSettings&, bool skip_parallel = false) const;
        void WriteCompress (TextOutput&, const WriteSettings&) const;
        void WriteIOTClause (TextOutput&, const WriteSettings&, bool overflow) const;

    public:
        string m_strTableOwner, 
               m_strTableName; // may be cluster

        string m_strDomainOwner, // 03.08.2003 improvement, domain index support
               m_strDomain,
               m_strDomainParams;

        IndexType m_Type;
        bool      m_bUniqueness;
        bool      m_bTemporary;
        bool      m_bGenerated;
        bool      m_bReverse;
        bool      m_bIndexCompression;

        map<int,string> m_Columns; // or expression for eitFunctionBased
        map<int,bool>   m_isExpression;

        TableStorage m_IOTOverflow_Storage; 
        int m_IOTOverflow_PCTThreshold;

        typedef vector<string>  PartKeyColumnContainer;
        typedef counted_ptr<IndexPartition> PartitionPtr;
        typedef vector<PartitionPtr> PartitionContainer;
        typedef counted_ptr<TableStorage> IOTOverflowStoragePtr;
        typedef vector<IOTOverflowStoragePtr> IOTOverflowStorageContainer;

        PartitioningType        m_PartitioningType;
        PartKeyColumnContainer  m_PartKeyColumns;
        PartitionContainer      m_Partitions;
        PartitioningType        m_subpartitioningType;
        string                  m_interval; // for range partitioning

        IOTOverflowStorageContainer m_IOTOverflowPartitions; 
        TableStorage m_IOTOverflowPartitionDefaultStrorage;

        bool m_bLocal;

    private:
        void writeDomainClause (TextOutput&, const WriteSettings&) const;
        void writeIndexPartitions (TextOutput&, const WriteSettings&) const;
        void writeSubpartitions (TextOutput& out, const WriteSettings& settings, const PartitionContainer& subpartitions) const;

        // copy-constraction & assign-operation is not supported
        Index (const Index&);
        void operator = (const Index&);
    };


    /// CONSTRAINT object ///////////////////////////////////////////////////
    class Constraint : public DbObject 
    {
    public:
        Constraint (Dictionary& dictionary);

        bool IsNotNullCheck () const;
        virtual bool IsGenerated () const { return m_bGenerated; }

        virtual const char* GetTypeStr () const { return "CONSTRAINT"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;
        void WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strTableName,
               m_strType,
               m_strROwner,
               m_strRConstraintName,
               m_strDeleteRule,
               m_strStatus,
               m_strSearchCondition;

        map<int,string> m_Columns;

        bool m_bDeferrable;
        bool m_bDeferred;
        bool m_bGenerated;
        bool m_bOnView;
        bool m_bRely;
        bool m_bNoValidate;

    private:
        // copy-constraction & assign-operation is not supported
        Constraint (const Constraint&);
        void operator = (const Constraint&);
    };


    /// TableBase is ancestor of TABLE, SNAPSHOT & SNAPSHOT LOG /////////////
    class TableBase : public DbObject, public TableStorage 
    {
    public:
        TableBase (Dictionary& dictionary);

        void WriteIndexes (TextOutput& out, const WriteSettings&) const;
        void WriteConstraints (TextOutput& out, const WriteSettings&, char chType) const;
        void WriteTriggers (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strClusterName,
               m_strComments;
        Variable<bool> m_bCache;

        set<string> m_setConstraints, 
                    m_setIndexes,
                    m_setTriggers;
        
        map<int,string> m_clusterColumns; // map table to cluster columns

    private:
        // copy-constraction & assign-operation is not supported
        TableBase (const TableBase&);
        void operator = (const TableBase&);
    };


    /// TABLE object ////////////////////////////////////////////////////////
    enum TableType              { ettNormal, ettTemporary, ettIOT, ettIOT_Overflow };
    enum TemporaryTableDuration { etttdUnknown, ettdTransaction, ettdSession };

    class Table : public TableBase
    {
    public:
        Table (Dictionary& dictionary);

        void WriteDefinition (TextOutput& out, const WriteSettings&) const;
        void WriteComments (TextOutput& out, const WriteSettings&) const;
        void WritePrimaryKeyForIOT (TextOutput& out, const WriteSettings& settings) const;
        const Constraint* FindPrimaryKey () const;

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "TABLE"; };
        virtual bool IsGenerated () const       { return m_TableType == ettIOT_Overflow; }
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        typedef counted_ptr<TabColumn> ColumnPtr;
        typedef map<int,ColumnPtr>  ColumnContainer;
        typedef vector<string>  PartKeyColumnContainer;
        typedef counted_ptr<Partition> PartitionPtr;
        typedef vector<PartitionPtr> PartitionContainer;

        void writeColumns (TextOutput&, const WriteSettings&) const;
        void writeTempClause (TextOutput&, const WriteSettings&) const;
        void writeIOTClause (TextOutput&, const WriteSettings&) const;
        void writeClusterClause (TextOutput&, const WriteSettings&) const;
        void writePartitions (TextOutput&, const WriteSettings&) const;
        void writeSubpartitions (TextOutput&, const WriteSettings&, const PartitionContainer&) const;

        TableType m_TableType;
        TemporaryTableDuration m_TemporaryTableDuration;

        ColumnContainer m_Columns;
        
        // should the following attributes be moved to TableBase?
        int m_IOTOverflow_includeColumn;
        
        PartitioningType         m_PartitioningType;
        PartKeyColumnContainer   m_PartKeyColumns;
        PartitionContainer       m_Partitions;
        PartitioningType         m_subpartitioningType;
        PartKeyColumnContainer   m_subpartKeyColumns;
        PartitionContainer       m_subpartTemplates;

        Variable<bool> m_bRowMovement;
        string m_interval; // for range partitioning
 
    private:
        // copy-constraction & assign-operation is not supported
        Table (const Table&);
        void operator = (const Table&);
    };


    /// TRIGGER object //////////////////////////////////////////////////////
    class Trigger : public DbObject 
    {
    public:
        Trigger (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual bool        IsCode ()     const { return true; };
        virtual const char* GetDefExt ()  const { return "trg"; };
        virtual const char* GetTypeStr () const { return "TRIGGER"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strTableOwner,
               m_strTableName, 
               m_strDescription,
               m_strWhenClause,
               m_strRefNames,
               m_strTriggerBody,
               m_strStatus,
			   m_strActionType,
			   m_strBaseObjectType;

    private:
        // copy-constraction & assign-operation is not supported
        Trigger (const Trigger&);
        void operator = (const Trigger&);
    };


    /// VIEW object /////////////////////////////////////////////////////////
    class View : public DbObject 
    {
    public:
        View (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        void WriteConstraints (TextOutput& out, const WriteSettings&, char chType) const;
        void WriteTriggers (TextOutput& out, const WriteSettings&) const;
        void WriteComments (TextOutput& out, const WriteSettings&) const;

        virtual bool IsCode () const            { return true; };
        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "VIEW"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strText,
               m_strComments;

        map<int,string>  m_Columns;
        set<string>      m_setTriggers;
        map<int, string> m_mapColComments;

        string      m_strViewConstraint;
        set<string> m_setConstraints;

    private:
        // copy-constraction & assign-operation is not supported
        View (const View&);
        void operator = (const View&);
    };


    /// SEQUENCE object /////////////////////////////////////////////////////
    class Sequence : public DbObject 
    {
    public:
        Sequence (Dictionary& dictionary);

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "SEQUENCE"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

        string GetIdentityOptions (const WriteSettings& settings) const;

    public:
        string m_strMinValue,
               m_strMaxValue,
               m_strIncrementBy,
               m_strCacheSize,
               m_strLastNumber;
        bool   m_bCycleFlag,
               m_bOrderFlag,
               m_bGenerated;

    private:
        // copy-constraction & assign-operation is not supported
        Sequence (const Sequence&);
        void operator = (const Sequence&);
    };


    /// Abstract base class for procedural db object classes ////////////////
    class PlsCode : public DbObject 
    {
    public:
        PlsCode (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual bool IsGrantable () const       { return true; }
        virtual bool IsCode () const            { return true; };
        virtual const char* GetDefExt () const  { return "sql"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strType,
               m_strText;

    private:
        // copy-constraction & assign-operation is not supported
        PlsCode (const PlsCode&);
        void operator = (const PlsCode&);
    };


    /// FUNCTION object /////////////////////////////////////////////////////
    class Function : public PlsCode 
    {
    public:
        Function (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "FUNCTION"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Function (const Function&);
        void operator = (const Function&);
    };


    /// PROCEDURE object ////////////////////////////////////////////////////
    class Procedure : public PlsCode 
    {
    public:
        Procedure (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "PROCEDURE"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Procedure (const Procedure&);
        void operator = (const Procedure&);
    };


    /// PACKAGE object //////////////////////////////////////////////////////
    class Package : public PlsCode 
    {
    public:
        Package (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "PACKAGE"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Package (const Package&);
        void operator = (const Package&);
    };


    /// PACKAGE BODY object /////////////////////////////////////////////////
    class PackageBody : public PlsCode 
    {
    public:
        PackageBody (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "PACKAGE BODY"; };
        virtual const char* GetDefExt ()  const { return "plb"; };

    private:
        // copy-constraction & assign-operation is not supported
        PackageBody (const PackageBody&);
        void operator = (const PackageBody&);
    };


    /// TYPE object /////////////////////////////////////////////////////////
    class Type : public PlsCode 
    {
    public:
        Type (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        void WriteAsIncopmlete (TextOutput& out, const WriteSettings&) const;

        virtual const char* GetTypeStr () const { return "TYPE"; };
        virtual const char* GetDefExt ()  const { return "pls"; };

    private:
        // copy-constraction & assign-operation is not supported
        Type (const Type&);
        void operator = (const Type&);
    };


    /// TYPE BODY object ////////////////////////////////////////////////////
    class TypeBody : public PlsCode 
    {
    public:
        TypeBody (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "TYPE BODY"; };
        virtual const char* GetDefExt ()  const { return "plb"; };

    private:
        // copy-constraction & assign-operation is not supported
        TypeBody (const TypeBody&);
        void operator = (const TypeBody&);
    };


    /// JAVA SOURCE object ////////////////////////////////////////////////////
    class JavaSource : public PlsCode 
    {
    public:
        JavaSource (Dictionary& dictionary) 
            : PlsCode(dictionary) {};

        virtual const char* GetTypeStr () const { return "JAVA SOURCE"; };
        virtual const char* GetDefExt ()  const { return "java"; };

        int Write (TextOutput& out, const WriteSettings& settings) const;

    private:
        // copy-constraction & assign-operation is not supported
        JavaSource (const JavaSource&);
        void operator = (const JavaSource&);
    };


    /// SYNONYM object //////////////////////////////////////////////////////
    class Synonym : public DbObject 
    {
    public:
        Synonym (Dictionary& dictionary) 
            : DbObject(dictionary) {};

        virtual bool        IsLight ()   const  { return true; };
        virtual const char* GetTypeStr () const { return "SYNONYM"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strRefOwner, 
               m_strRefName, 
               m_strRefDBLink;

    private:
        // copy-constraction & assign-operation is not supported
        Synonym (const Synonym&);
        void operator = (const Synonym&);
    };


    /// GRANT something (DB objects privileges only) ////////////////////////////
    struct Grant
    {
        string m_strOwner, 
               m_strName, 
               m_strGrantee;

        set<string> m_privileges;
        set<string> m_privilegesWithAdminOption;

        map<string,set<string> > m_colPrivileges;
        map<string,set<string> > m_colPrivilegesWithAdminOption;

        Grant () {}
        
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    private:
        // copy-constraction & assign-operation is not supported
        Grant (const Grant&);
        void operator = (const Grant&);
    };


    /// GrantContainer subject (USER or ROLE in DB) /////////////////////////
    //
    //  GrantContainer can be Grantor or Grantee
    //
    //  Object of Grantor type is not equal "grantor" in "all_tab_privs" view,
    //  it is "schema_name"
    //
    typedef counted_ptr<Grant>       GrantPtr;
    typedef map<string, GrantPtr>    GrantMap;
    typedef GrantMap::iterator       GrantMapIter;
    typedef GrantMap::const_iterator GrantMapConstIter;

    class GrantContainer : public DbObject
    {
    public:
        GrantContainer (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        // If it's grantor then the arguments are (grantee_name, object_name)
        // If it's grantee then the arguments are (grantor_name, object_name)
        Grant& CreateGrant (const char*, const char*);
        Grant& LookupGrant (const char*, const char* = 0);
        void   DestroyGrant (const char*, const char* = 0);
        
        void EnumGrants (void (*pfnDo)(const Grant&, void*), void* = 0, bool sort = false) const;

        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        GrantMap m_mapGrants;

    private:
        // copy-constraction & assign-operation is not supported
        GrantContainer (const GrantContainer&);
        void operator = (const GrantContainer&);
    };

    class Grantor : public GrantContainer
    {
    public:
        Grantor (Dictionary& dictionary) : GrantContainer(dictionary) {}
        virtual const char* GetTypeStr () const { return "GRANTOR"; };

        void WriteObjectGrants (const char* name, TextOutput&, const WriteSettings&) const;
    };

    class Grantee : public GrantContainer
    {
    public:
        Grantee (Dictionary& dictionary) : GrantContainer(dictionary) {}
        virtual const char* GetTypeStr () const { return "GRANTEE"; };
    };


    /// CLUSTER object //////////////////////////////////////////////////////
    class Cluster : public DbObject, public TableStorage 
    {
    public:
        Cluster (Dictionary& dictionary);

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "CLUSTER"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        bool   m_bIsHash;
        string m_strSize,
               m_strHashKeys,
               m_strHashExpression;
        bool   m_bCache;

        typedef counted_ptr<Column> ColumnPtr;
        typedef map<int,ColumnPtr> ColContainer;

        ColContainer m_Columns;
        set<string>  m_setIndexes;

    private:
        // copy-constraction & assign-operation is not supported
        Cluster (const Cluster&);
        void operator = (const Cluster&);
    };


    /// DATABASE LINK object ////////////////////////////////////////////////
    class DBLink : public DbObject 
    {
    public:
        string m_strUser,
               m_strPassword,
               m_strHost;
    public:

        DBLink (Dictionary& dictionary) 
        : DbObject(dictionary) {};

        virtual bool IsLight ()     const       { return true; }
        virtual const char* GetTypeStr () const { return "DATABASE LINK"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    private:
        // copy-constraction & assign-operation is not supported
        DBLink (const DBLink&);
        void operator = (const DBLink&);
    };


    /// SNAPSHOT LOG object /////////////////////////////////////////////////
    //
    // Does a snapshot have cache-option?
    //
    class SnapshotLog : public DbObject, public TableStorage 
    {
    public:
        SnapshotLog (Dictionary& dictionary)
        : DbObject(dictionary) {}

        virtual const char* GetTypeStr () const      { return "SNAPSHOT LOG";   }
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    private:
        // copy-constraction & assign-operation is not supported
        SnapshotLog (const SnapshotLog&);
        void operator = (const SnapshotLog&);
    };


    /// SNAPSHOT object /////////////////////////////////////////////////////
    //
    // Does a snapshot have cache-option, constraints, indexes, triggers?
    //
    class Snapshot : public DbObject, public TableStorage 
    {
    public:
        Snapshot (Dictionary& dictionary)
        : DbObject(dictionary) {}

        virtual bool IsGrantable () const       { return true; }
        virtual const char* GetTypeStr () const { return "SNAPSHOT"; };
        virtual int Write (TextOutput& out, const WriteSettings&) const;

    public:
        string m_strRefreshType,
               m_strStartWith,
               m_strNextTime,
               m_strQuery,
               m_strClusterName,
               m_strComments;
        /*
        bool        m_bCache;
        set<string> m_setConstraints, 
                    m_setIndexes,
                    m_setTriggers;
        */
        map<int,string> m_clusterColumns; // map table to cluster columns

    private:
        // copy-constraction & assign-operation is not supported
        Snapshot (const Snapshot&);
        void operator = (const Snapshot&);
    };


} // END namespace OraMetaDict 

#pragma warning ( pop )
#endif//__METAOBJECTS_H__
