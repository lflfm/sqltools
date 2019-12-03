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

/*
    04.10.2006 B#XXXXXXX - cannot get DDL for db link if its name is longer than 30 chars
*/

#ifndef __METADICTIONARY_H__
#define __METADICTIONARY_H__
#pragma once
#pragma warning ( push )
#pragma warning ( disable : 4786 )

//
// Purpose: Class library for storing oracle db objects in meta-dictionary
// Note: Copy-constraction & assign-operation is not supported for all classes
//

#include <string>
#include <vector>
#include <list>
#include <map>
#include "arg_shared.h"
#include "MetaDict\MetaObjects.h"


namespace OraMetaDict 
{
    class TextOutput;

    using std::string;
    using std::map;
    using std::list;
    using std::vector;
    using arg::counted_ptr;

    class XNotFound : public std::exception 
    {
    public:
        XNotFound (const char* objName);
    };

    class XAlreadyExists : public std::exception 
    {
    public:
        XAlreadyExists (const char* objName);
    };

    // typedefs for all using maps
    typedef counted_ptr<User>              UserPtr;
    typedef map<string, UserPtr>           UserMap;
    typedef UserMap::iterator              UserMapIter;
    typedef UserMap::const_iterator        UserMapConstIter;

    typedef counted_ptr<Tablespace>        TablespacePtr;
    typedef map<string, TablespacePtr>     TablespaceMap;
    typedef TablespaceMap::iterator        TablespaceMapIter;
    typedef TablespaceMap::const_iterator  TablespaceMapConstIter;

    typedef counted_ptr<Cluster>           ClusterPtr;
    typedef map<string, ClusterPtr>        ClusterMap;
    typedef ClusterMap::iterator           ClusterMapIter;
    typedef ClusterMap::const_iterator     ClusterMapConstIter;

    typedef counted_ptr<Table>             TablePtr;
    typedef map<string, TablePtr>          TableMap;
    typedef TableMap::iterator             TableMapIter;
    typedef TableMap::const_iterator       TableMapConstIter;

    typedef counted_ptr<Index>             IndexPtr;
    typedef map<string, IndexPtr>          IndexMap;
    typedef IndexMap::iterator             IndexMapIter;
    typedef IndexMap::const_iterator       IndexMapConstIter;

    typedef counted_ptr<Constraint>        ConstraintPtr;
    typedef map<string, ConstraintPtr>     ConstraintMap;
    typedef ConstraintMap::iterator        ConstraintMapIter;
    typedef ConstraintMap::const_iterator  ConstraintMapConstIter;

    typedef counted_ptr<Trigger>           TriggerPtr;
    typedef map<string, TriggerPtr>        TriggerMap;
    typedef TriggerMap::iterator           TriggerMapIter;
    typedef TriggerMap::const_iterator     TriggerMapConstIter;

    typedef counted_ptr<View> ViewPtr;
    typedef map<string, ViewPtr>           ViewMap;
    typedef ViewMap::iterator              ViewMapIter;
    typedef ViewMap::const_iterator        ViewMapConstIter;

    typedef counted_ptr<Sequence>          SequencePtr;
    typedef map<string, SequencePtr>       SequenceMap;
    typedef SequenceMap::iterator          SequenceMapIter;
    typedef SequenceMap::const_iterator    SequenceMapConstIter;

    typedef counted_ptr<Function>          FunctionPtr;
    typedef map<string, FunctionPtr>       FunctionMap;
    typedef FunctionMap::iterator          FunctionMapIter;
    typedef FunctionMap::const_iterator    FunctionMapConstIter;

    typedef counted_ptr<Procedure>         ProcedurePtr;
    typedef map<string, ProcedurePtr>      ProcedureMap;
    typedef ProcedureMap::iterator         ProcedureMapIter;
    typedef ProcedureMap::const_iterator   ProcedureMapConstIter;

    typedef counted_ptr<Package>           PackagePtr;
    typedef map<string, PackagePtr>        PackageMap;
    typedef PackageMap::iterator           PackageMapIter;
    typedef PackageMap::const_iterator     PackageMapConstIter;

    typedef counted_ptr<PackageBody>       PackageBodyPtr;
    typedef map<string, PackageBodyPtr>    PackageBodyMap;
    typedef PackageBodyMap::iterator       PackageBodyMapIter;
    typedef PackageBodyMap::const_iterator PackageBodyMapConstIter;

    typedef counted_ptr<Type>              TypePtr;
    typedef map<string, TypePtr>           TypeMap;
    typedef TypeMap::iterator              TypeMapIter;
    typedef TypeMap::const_iterator        TypeMapConstIter;

    typedef counted_ptr<TypeBody>          TypeBodyPtr;
    typedef map<string, TypeBodyPtr>       TypeBodyMap;
    typedef TypeBodyMap::iterator          TypeBodyMapIter;
    typedef TypeBodyMap::const_iterator    TypeBodyMapConstIter;

    typedef counted_ptr<JavaSource>        JavaSourcePtr;
    typedef map<string, JavaSourcePtr>     JavaSourceMap;
    typedef JavaSourceMap::iterator        JavaSourceMapIter;
    typedef JavaSourceMap::const_iterator  JavaSourceMapConstIter;

    typedef counted_ptr<Synonym>           SynonymPtr;
    typedef map<string, SynonymPtr>        SynonymMap;
    typedef SynonymMap::iterator           SynonymMapIter;
    typedef SynonymMap::const_iterator     SynonymMapConstIter;

    typedef counted_ptr<Grantor> GrantorPtr;
    typedef map<string, GrantorPtr>        GrantorMap;
    typedef GrantorMap::iterator           GrantorMapIter;
    typedef GrantorMap::const_iterator     GrantorMapConstIter;

    typedef counted_ptr<Grantee> GranteePtr;
    typedef map<string, GranteePtr>        GranteeMap;
    typedef GranteeMap::iterator           GranteeMapIter;
    typedef GranteeMap::const_iterator     GranteeMapConstIter;

    typedef counted_ptr<DBLink>            DBLinkPtr;
    typedef map<string, DBLinkPtr>         DBLinkMap;
    typedef DBLinkMap::iterator            DBLinkMapIter;
    typedef DBLinkMap::const_iterator      DBLinkMapConstIter;

    typedef counted_ptr<SnapshotLog>       SnapshotLogPtr;
    typedef map<string, SnapshotLogPtr>    SnapshotLogMap;
    typedef SnapshotLogMap::iterator       SnapshotLogMapIter;
    typedef SnapshotLogMap::const_iterator SnapshotLogMapConstIter;

    typedef counted_ptr<Snapshot>          SnapshotPtr;
    typedef map<string, SnapshotPtr>       SnapshotMap;
    typedef SnapshotMap::iterator          SnapshotMapIter;
    typedef SnapshotMap::const_iterator    SnapshotMapConstIter;


    class Dictionary 
    {
    public:
        Dictionary () { m_bRewriteSharedDuplicate = false; };

        // if property is true, then a second loading (USER, TABLESPACE) is valid
        bool GetRewriteSharedDuplicate () const   { return m_bRewriteSharedDuplicate; };
        void SetRewriteSharedDuplicate (bool val) { m_bRewriteSharedDuplicate = val; };

        User&        CreateUser       (const char*) ;
        Tablespace&  CreateTablespace (const char*);
        Cluster&     CreateCluster    (const char*, const char*);
        Table&       CreateTable      (const char*, const char*);
        Index&       CreateIndex      (const char*, const char*);
        Constraint&  CreateConstraint (const char*, const char*);
        Trigger&     CreateTrigger    (const char*, const char*);
        View&        CreateView       (const char*, const char*);
        Sequence&    CreateSequence   (const char*, const char*);
        Function&    CreateFunction   (const char*, const char*);
        Procedure&   CreateProcedure  (const char*, const char*);
        Package&     CreatePackage    (const char*, const char*);
        PackageBody& CreatePackageBody(const char*, const char*);
        Type&        CreateType       (const char*, const char*);
        TypeBody&    CreateTypeBody   (const char*, const char*);
        JavaSource&  CreateJavaSource (const char*, const char*);
        Synonym&     CreateSynonym    (const char*, const char*);
        // pseudo-objects
        Grantor&     CreateGrantor    (const char*);
        Grantee&     CreateGrantee    (const char*);
        // last add-on
        DBLink&      CreateDBLink     (const char*, const char*);
        SnapshotLog& CreateSnapshotLog(const char*, const char*);
        Snapshot&    CreateSnapshot   (const char*, const char*);


        DbObject&    LookupObject     (const char*, const char*, const char* szType);
        User&        LookupUser       (const char*);
        Tablespace&  LookupTablespace (const char*);
        Cluster&     LookupCluster    (const char*, const char* = 0);
        Table&       LookupTable      (const char*, const char* = 0);
        Index&       LookupIndex      (const char*, const char* = 0);
        Constraint&  LookupConstraint (const char*, const char* = 0);
        Trigger&     LookupTrigger    (const char*, const char* = 0);
        View&        LookupView       (const char*, const char* = 0);
        Sequence&    LookupSequence   (const char*, const char* = 0);
        Function&    LookupFunction   (const char*, const char* = 0);
        Procedure&   LookupProcedure  (const char*, const char* = 0);
        Package&     LookupPackage    (const char*, const char* = 0);
        PackageBody& LookupPackageBody(const char*, const char* = 0);
        Type&        LookupType       (const char*, const char* = 0);
        TypeBody&    LookupTypeBody   (const char*, const char* = 0);
        JavaSource&  LookupJavaSource (const char*, const char* = 0);
        Synonym&     LookupSynonym    (const char*, const char* = 0);
        // pseudo-objects
        Grantor&     LookupGrantor    (const char*);
        Grantee&     LookupGrantee    (const char*);
        // last add-on
        DBLink&      LookupDBLink     (const char*, const char* = 0);
        SnapshotLog& LookupSnapshotLog(const char*, const char* = 0);
        Snapshot&    LookupSnapshot   (const char*, const char* = 0);

        const DbObject&    LookupObject     (const char*, const char*, const char* szType) const;
        const User&        LookupUser       (const char*) const;
        const Tablespace&  LookupTablespace (const char*) const;
        const Cluster&     LookupCluster    (const char*, const char* = 0) const;
        const Table&       LookupTable      (const char*, const char* = 0) const;
        const Index&       LookupIndex      (const char*, const char* = 0) const;
        const Constraint&  LookupConstraint (const char*, const char* = 0) const;
        const Trigger&     LookupTrigger    (const char*, const char* = 0) const;
        const View&        LookupView       (const char*, const char* = 0) const;
        const Sequence&    LookupSequence   (const char*, const char* = 0) const;
        const Function&    LookupFunction   (const char*, const char* = 0) const;
        const Procedure&   LookupProcedure  (const char*, const char* = 0) const;
        const Package&     LookupPackage    (const char*, const char* = 0) const;
        const PackageBody& LookupPackageBody(const char*, const char* = 0) const;
        const Type&        LookupType       (const char*, const char* = 0) const;
        const TypeBody&    LookupTypeBody   (const char*, const char* = 0) const;
        const JavaSource&  LookupJavaSource (const char*, const char* = 0) const;
        const Synonym&     LookupSynonym    (const char*, const char* = 0) const;
        // pseudo-objects
        const Grantor&     LookupGrantor    (const char*) const;
        const Grantee&     LookupGrantee    (const char*) const;
        // last add-on
        const DBLink&      LookupDBLink     (const char*, const char* = 0) const;
        const SnapshotLog& LookupSnapshotLog(const char*, const char* = 0) const;
        const Snapshot&    LookupSnapshot   (const char*, const char* = 0) const;


        void EnumUsers         (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumTablespaces   (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumClusters      (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumTables        (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumConstraints   (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumIndexes       (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumTriggers      (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumViews         (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumSequences     (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumFunctions     (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumProcedures    (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumPackages      (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumPackageBodies (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumTypes         (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumTypeBodies    (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumJavaSources   (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumSynonyms      (void (*pfnDo)(DbObject&, void*), void* = 0);
        // pseudo-objects
        void EnumGrantors      (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumGrantees      (void (*pfnDo)(DbObject&, void*), void* = 0);
        // last add-on
        void EnumDBLinks       (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumSnapshotLogs  (void (*pfnDo)(DbObject&, void*), void* = 0);
        void EnumSnapshots     (void (*pfnDo)(DbObject&, void*), void* = 0);

        //   InitIterators (map.begin(), map.end());
        void InitIterators (UserMapConstIter&,        UserMapConstIter&) const;
        void InitIterators (TablespaceMapConstIter&,  TablespaceMapConstIter&) const;
        void InitIterators (ClusterMapConstIter&,     ClusterMapConstIter&) const;
        void InitIterators (TableMapConstIter&,       TableMapConstIter&) const;
        void InitIterators (ConstraintMapConstIter&,  ConstraintMapConstIter&) const;
        void InitIterators (IndexMapConstIter&,       IndexMapConstIter&) const;
        void InitIterators (TriggerMapConstIter&,     TriggerMapConstIter&) const;
        void InitIterators (ViewMapConstIter&,        ViewMapConstIter&) const;
        void InitIterators (SequenceMapConstIter&,    SequenceMapConstIter&) const;
        void InitIterators (FunctionMapConstIter&,    FunctionMapConstIter&) const;
        void InitIterators (ProcedureMapConstIter&,   ProcedureMapConstIter&) const;
        void InitIterators (PackageMapConstIter&,     PackageMapConstIter&) const;
        void InitIterators (PackageBodyMapConstIter&, PackageBodyMapConstIter&) const;
        void InitIterators (TypeMapConstIter&,        TypeMapConstIter&) const;
        void InitIterators (TypeBodyMapConstIter&,    TypeBodyMapConstIter&) const;
        void InitIterators (JavaSourceMapConstIter&,  JavaSourceMapConstIter&) const;
        void InitIterators (SynonymMapConstIter&,     SynonymMapConstIter&) const;
        void InitIterators (GrantorMapConstIter&,     GrantorMapConstIter&) const;
        void InitIterators (GranteeMapConstIter&,     GranteeMapConstIter&) const;
        // last add-on
        void InitIterators (DBLinkMapConstIter&,      DBLinkMapConstIter&) const;
        void InitIterators (SnapshotLogMapConstIter&, SnapshotLogMapConstIter&) const;
        void InitIterators (SnapshotMapConstIter&,    SnapshotMapConstIter&) const;

        void DestroyAll ();
        void DestroyUser       (const char*);
        void DestroyTablespace (const char*);
        void DestroyCluster    (const char*, const char* = 0);
        void DestroyTable      (const char*, const char* = 0);
        void DestroyIndex      (const char*, const char* = 0);
        void DestroyConstraint (const char*, const char* = 0);
        void DestroyTrigger    (const char*, const char* = 0);
        void DestroyView       (const char*, const char* = 0);
        void DestroySequence   (const char*, const char* = 0);
        void DestroyFunction   (const char*, const char* = 0);
        void DestroyProcedure  (const char*, const char* = 0);
        void DestroyPackage    (const char*, const char* = 0);
        void DestroyPackageBody(const char*, const char* = 0);
        void DestroyType       (const char*, const char* = 0);
        void DestroyTypeBody   (const char*, const char* = 0);
        void DestroyJavaSource (const char*, const char* = 0);
        void DestroySynonym    (const char*, const char* = 0);
        // pseudo-objects
        void DestroyGrantor    (const char*);
        void DestroyGrantee    (const char*);
        // last add-on
        void DestroyDBLink     (const char*, const char*);
        void DestroySnapshotLog(const char*, const char*);
        void DestroySnapshot   (const char*, const char*);

    private:
        bool m_bRewriteSharedDuplicate; // if property is true, then a second loading (USER, TABLESPACE) is valid

        UserMap        m_mapUsers;
        TablespaceMap  m_mapTablespaces;
        ClusterMap     m_mapClusters;
        TableMap       m_mapTables;
        IndexMap       m_mapIndexes;
        ConstraintMap  m_mapConstraints;
        TriggerMap     m_mapTriggers;
        ViewMap        m_mapViews;
        SequenceMap    m_mapSequences;
        FunctionMap    m_mapFunctions;
        ProcedureMap   m_mapProcedures;
        PackageMap     m_mapPackages;
        PackageBodyMap m_mapPackageBodies;
        TypeMap        m_mapTypes;
        TypeBodyMap    m_mapTypeBodies;
        JavaSourceMap  m_mapJavaSources;
        SynonymMap     m_mapSynonyms;
        // pseudo-objects
        GrantorMap     m_mapGrantors;
        GranteeMap     m_mapGrantees;
        // last add-on
        DBLinkMap      m_mapDBLinks;
        SnapshotLogMap m_mapSnapshotLogs;
        SnapshotMap    m_mapSnapshots;

    private:
        // copy-constraction & assign-operation is not supported
        Dictionary (const Dictionary&);
        void operator = (const Dictionary&);
    };


    /// Helper functions and constants //////////////////////////////////////////

    const int cnDbNameSize     = 128;                   // db object name size, changed to 128 because of db link name
    const int cnDbNameBuffSize = cnDbNameSize + 1;      // buffer for name (+ '\0')
    const int cnDbKeySize      = 2 * cnDbNameBuffSize;  // buffer for key (owner + '.' + name + '\0')

    const char* make_key (char szKey[cnDbKeySize], const char* szOwner, const char* szName);
    
    /// Inline function implementation //////////////////////////////////////////
    
    inline
    void Dictionary::InitIterators (ClusterMapConstIter& begin, ClusterMapConstIter& end) const
    {
        begin = m_mapClusters.begin();
        end   = m_mapClusters.end();
    }

    inline
    void Dictionary::InitIterators (TableMapConstIter& begin, TableMapConstIter& end) const
    {
        begin = m_mapTables.begin();
        end   = m_mapTables.end();
    }
    
    inline
    void Dictionary::InitIterators (ConstraintMapConstIter& begin,  ConstraintMapConstIter& end) const
    {
        begin = m_mapConstraints.begin();
        end   = m_mapConstraints.end();
    }
    
    inline
    void Dictionary::InitIterators (IndexMapConstIter& begin, IndexMapConstIter& end) const
    {
        begin = m_mapIndexes.begin();
        end   = m_mapIndexes.end();
    }

    inline
    void Dictionary::InitIterators (ViewMapConstIter& begin, ViewMapConstIter& end) const
    {
        begin = m_mapViews.begin();
        end   = m_mapViews.end();
    }

    inline
    void Dictionary::InitIterators (GrantorMapConstIter& begin, GrantorMapConstIter& end) const
    {
        begin = m_mapGrantors.begin();
        end   = m_mapGrantors.end();
    }
}  // END namespace OraMetaDict 

#pragma warning ( pop )
#endif//__METADICTIONARY_H__