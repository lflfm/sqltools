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

#include "stdafx.h"
#include "COMMON/ExceptionHelper.h"
#include "MetaDict/MetaObjects.h"
#include "MetaDict/MetaDictionary.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

namespace OraMetaDict 
{

XNotFound::XNotFound (const char* name) 
: exception((string("Object \"") + name + "\" is not found in the dictionary.").c_str())
{
}


XAlreadyExists::XAlreadyExists (const char* name)
: exception((string("Object \"") + name + "\" already exists in the dictionary.").c_str())
{
}


const char* make_key (char szKey[cnDbKeySize], const char* szOwner, const char* szName)
{
    for (int i(0); i < cnDbNameSize && szOwner[i]; i++) 
        szKey[i] = szOwner[i];

    szKey[i++] = '.';

    for (int j(0); j < cnDbNameSize && szName[j]; i++, j++) 
        szKey[i] = szName[j];

    szKey[i++] = 0;

    return szKey;
}


/// Template helper functions ///////////////////////////////////////////////

template<class T, class M>
T& create_object (Dictionary& dict, M& _map, const char* key)
{
    counted_ptr<T> ptr(new T(dict));

    if (_map.find(key) == _map.end())
        _map.insert(M::value_type(key, ptr));
    else if (dict.GetRewriteSharedDuplicate()) {
        _map.erase(key);
        _map.insert(M::value_type(key, ptr));
    } else
        throw XAlreadyExists(key);

    return *(ptr.get());
}


template<class T, class M>
T& create_object (Dictionary& dict, M& _map, const char* key1, const char* key2)
{
    char key[cnDbKeySize];

    counted_ptr<T> ptr(new T(dict));

    if (_map.find(make_key(key, key1, key2)) == _map.end())
        _map.insert(M::value_type(key, ptr));
    else if (dict.GetRewriteSharedDuplicate()) {
        _map.erase(key);
        _map.insert(M::value_type(key, ptr));
    } else
        throw XAlreadyExists(key);

    return *(ptr.get());
}


template<class T, class M>
T& lookup_object (M& _map, const char* key)
{
    M::iterator it = _map.find(key);

    if (it == _map.end())
        throw XNotFound(key);

    return *((*it).second.get());
}


template<class T, class M>
const T& lookup_object (const M& _map, const char* key)
{
    M::const_iterator it = _map.find(key);

    if (it == _map.end())
        throw XNotFound(key);

    return *((*it).second.get());
}


template<class T, class M>
T& lookup_object (M& _map, const char* key1, const char* key2)
{
    char key[cnDbKeySize];

    M::iterator it = _map.find(key2 ? make_key(key, key1, key2) : key1);

    if (it == _map.end())
        throw XNotFound(key);

    return *((*it).second.get());
}


template<class T, class M>
const T& lookup_object (const M& _map, const char* key1, const char* key2)
{
    char key[cnDbKeySize];

    M::const_iterator it = _map.find(key2 ? make_key(key, key1, key2) : key1);

    if (it == _map.end())
        throw XNotFound(key);

    return *((*it).second.get());
}


template<class T, class M>
void destroy_object (M& _map, const char* key)
{
    if (_map.erase(key))
        throw XNotFound(key);
}


template<class T, class M>
void destroy_object (M& _map, const char* key1, const char* key2)
{
    char key[cnDbKeySize];

    if (_map.erase(key2 ? make_key(key, key1, key2) : key1))
        throw XNotFound(key);
}

template<class T, class M>
void enum_objects (M& _map, void (*pfnDo)(DbObject&, void*), void* param)
{
    M::iterator it(_map.begin()), end(_map.end());
    
    for (; it != end; it++)
        pfnDo(*((*it).second.get()), param);
}

/// Member-functions for creating ///////////////////////////////////////////

User& Dictionary::CreateUser (const char* key)
{   return create_object<User, UserMap>(*this, m_mapUsers, key);  }

Tablespace& Dictionary::CreateTablespace (const char* key)
{   return create_object<Tablespace, TablespaceMap>(*this, m_mapTablespaces, key); }

Cluster& Dictionary::CreateCluster (const char* key1, const char* key2)
{   return create_object<Cluster, ClusterMap>(*this, m_mapClusters, key1, key2); }

Table& Dictionary::CreateTable (const char* key1, const char* key2)
{   return create_object<Table, TableMap>(*this, m_mapTables, key1, key2); }

Index& Dictionary::CreateIndex (const char* key1, const char* key2)
{   return create_object<Index, IndexMap>(*this, m_mapIndexes, key1, key2); }

Constraint& Dictionary::CreateConstraint (const char* key1, const char* key2)
{   return create_object<Constraint, ConstraintMap>(*this, m_mapConstraints, key1, key2); }

Trigger& Dictionary::CreateTrigger (const char* key1, const char* key2)
{   return create_object<Trigger, TriggerMap>(*this, m_mapTriggers, key1, key2); }

View& Dictionary::CreateView (const char* key1, const char* key2)
{   return create_object<View, ViewMap>(*this, m_mapViews, key1, key2); }

Sequence& Dictionary::CreateSequence (const char* key1, const char* key2)
{   return create_object<Sequence, SequenceMap>(*this, m_mapSequences, key1, key2); }

Function& Dictionary::CreateFunction (const char* key1, const char* key2)
{   return create_object<Function, FunctionMap>(*this, m_mapFunctions, key1, key2); }

Procedure& Dictionary::CreateProcedure (const char* key1, const char* key2)
{   return create_object<Procedure, ProcedureMap>(*this, m_mapProcedures, key1, key2); }

Package& Dictionary::CreatePackage (const char* key1, const char* key2)
{   return create_object<Package, PackageMap>(*this, m_mapPackages, key1, key2); }

PackageBody& Dictionary::CreatePackageBody (const char* key1, const char* key2)
{   return create_object<PackageBody, PackageBodyMap>(*this, m_mapPackageBodies, key1, key2); }

Type& Dictionary::CreateType (const char* key1, const char* key2)
{   return create_object<Type, TypeMap>(*this, m_mapTypes, key1, key2); }

TypeBody& Dictionary::CreateTypeBody (const char* key1, const char* key2)
{   return create_object<TypeBody, TypeBodyMap>(*this, m_mapTypeBodies, key1, key2); }

JavaSource& Dictionary::CreateJavaSource (const char* key1, const char* key2)
{   return create_object<JavaSource, JavaSourceMap>(*this, m_mapJavaSources, key1, key2); }

Synonym& Dictionary::CreateSynonym (const char* key1, const char* key2)
{   return create_object<Synonym, SynonymMap>(*this, m_mapSynonyms, key1, key2); }

// pseudo-objects
Grantor& Dictionary::CreateGrantor (const char* key)
{   return create_object<Grantor, GrantorMap>(*this, m_mapGrantors, key);  }

Grantee& Dictionary::CreateGrantee (const char* key)
{   return create_object<Grantee, GranteeMap>(*this, m_mapGrantees, key);  }

// last add-on
DBLink& Dictionary::CreateDBLink (const char* key1, const char* key2)
{   return create_object<DBLink, DBLinkMap>(*this, m_mapDBLinks, key1, key2); }

SnapshotLog& Dictionary::CreateSnapshotLog (const char* key1, const char* key2)
{   return create_object<SnapshotLog, SnapshotLogMap>(*this, m_mapSnapshotLogs, key1, key2); }

Snapshot& Dictionary::CreateSnapshot (const char* key1, const char* key2)
{   return create_object<Snapshot, SnapshotMap>(*this, m_mapSnapshots, key1, key2); }

/// Member-functions for lookuping //////////////////////////////////////////

User& Dictionary::LookupUser (const char* key)
{   return lookup_object<User, UserMap>(m_mapUsers, key); }

Tablespace& Dictionary::LookupTablespace (const char* key)
{   return lookup_object<Tablespace, TablespaceMap>(m_mapTablespaces, key); }

Cluster& Dictionary::LookupCluster (const char* key1, const char* key2)
{   return lookup_object<Cluster, ClusterMap>(m_mapClusters, key1, key2); }

Table& Dictionary::LookupTable (const char* key1, const char* key2)
{   return lookup_object<Table, TableMap>(m_mapTables, key1, key2); }

Index& Dictionary::LookupIndex (const char* key1, const char* key2)
{   return lookup_object<Index, IndexMap>(m_mapIndexes, key1, key2); }

Constraint& Dictionary::LookupConstraint (const char* key1, const char* key2)
{   return lookup_object<Constraint, ConstraintMap>(m_mapConstraints, key1, key2); }

Trigger& Dictionary::LookupTrigger (const char* key1, const char* key2)
{   return lookup_object<Trigger, TriggerMap>(m_mapTriggers, key1, key2); }

View& Dictionary::LookupView (const char* key1, const char* key2)
{   return lookup_object<View, ViewMap>(m_mapViews, key1, key2); }

Sequence& Dictionary::LookupSequence (const char* key1, const char* key2)
{   return lookup_object<Sequence, SequenceMap>(m_mapSequences, key1, key2); }

Function& Dictionary::LookupFunction (const char* key1, const char* key2)
{   return lookup_object<Function, FunctionMap>(m_mapFunctions, key1, key2); }

Procedure& Dictionary::LookupProcedure (const char* key1, const char* key2)
{   return lookup_object<Procedure, ProcedureMap>(m_mapProcedures, key1, key2); }

Package& Dictionary::LookupPackage (const char* key1, const char* key2)
{   return lookup_object<Package, PackageMap>(m_mapPackages, key1, key2); }

PackageBody& Dictionary::LookupPackageBody (const char* key1, const char* key2)
{   return lookup_object<PackageBody, PackageBodyMap>(m_mapPackageBodies, key1, key2); }

Type& Dictionary::LookupType (const char* key1, const char* key2)
{   return lookup_object<Type, TypeMap>(m_mapTypes, key1, key2); }

TypeBody& Dictionary::LookupTypeBody (const char* key1, const char* key2)
{   return lookup_object<TypeBody, TypeBodyMap>(m_mapTypeBodies, key1, key2); }

JavaSource& Dictionary::LookupJavaSource (const char* key1, const char* key2)
{   return lookup_object<JavaSource, JavaSourceMap>(m_mapJavaSources, key1, key2); }

Synonym& Dictionary::LookupSynonym (const char* key1, const char* key2)
{   return lookup_object<Synonym, SynonymMap>(m_mapSynonyms, key1, key2); }

// pseudo-objects
Grantor& Dictionary::LookupGrantor (const char* key)
{   return lookup_object<Grantor, GrantorMap>(m_mapGrantors, key); }

Grantee& Dictionary::LookupGrantee (const char* key)
{   return lookup_object<Grantee, GranteeMap>(m_mapGrantees, key); }

// last add-on
DBLink& Dictionary::LookupDBLink (const char* key1, const char* key2)
{   return lookup_object<DBLink, DBLinkMap>(m_mapDBLinks, key1, key2); }

SnapshotLog& Dictionary::LookupSnapshotLog (const char* key1, const char* key2)
{   return lookup_object<SnapshotLog, SnapshotLogMap>(m_mapSnapshotLogs, key1, key2); }

Snapshot& Dictionary::LookupSnapshot (const char* key1, const char* key2)
{   return lookup_object<Snapshot, SnapshotMap>(m_mapSnapshots, key1, key2); }


DbObject& Dictionary::LookupObject (const char* szOwner, const char* szName, const char* szType)
{
    if (szType[0] && szType[1] 
    && szType[2] && szType[3]) {
        if (!stricmp(szType, "TABLE"))        return LookupTable(szOwner, szName);
        if (!stricmp(szType, "VIEW"))         return LookupView(szOwner, szName);
        if (!stricmp(szType, "TRIGGER"))      return LookupTrigger(szOwner, szName);
        if (!stricmp(szType, "PACKAGE"))      return LookupPackage(szOwner, szName);
        if (!stricmp(szType, "PACKAGE BODY")) return LookupPackageBody(szOwner, szName);
        if (!stricmp(szType, "FUNCTION"))     return LookupFunction(szOwner, szName);
        if (!stricmp(szType, "PROCEDURE"))    return LookupProcedure(szOwner, szName);
        if (!stricmp(szType, "SEQUENCE"))     return LookupSequence(szOwner, szName);
        if (!stricmp(szType, "SYNONYM"))      return LookupSynonym(szOwner, szName);
        if (!stricmp(szType, "GRANTOR"))      return LookupGrantor(szName);
        if (!stricmp(szType, "GRANTEE"))      return LookupGrantee(szName);
        if (!stricmp(szType, "CLUSTER"))      return LookupCluster(szOwner, szName);
        if (!stricmp(szType, "TABLESPACE"))   return LookupTablespace(szName);
        if (!stricmp(szType, "USER"))         return LookupUser(szName);
        if (!stricmp(szType, "SNAPSHOT"))     return LookupSnapshot(szOwner, szName);
        if (!stricmp(szType, "SNAPSHOT LOG")) return LookupSnapshotLog(szOwner, szName);
        if (!stricmp(szType, "DATABASE LINK"))return LookupDBLink(szOwner, szName);
        if (!stricmp(szType, "TYPE"))         return LookupType(szOwner, szName);
        if (!stricmp(szType, "TYPE BODY"))    return LookupTypeBody(szOwner, szName);
        if (!stricmp(szType, "JAVA SOURCE"))  return LookupJavaSource(szOwner, szName);
        if (!stricmp(szType, "INDEX"))        return LookupIndex(szOwner, szName);
        if (!stricmp(szType, "CONSTRAINT"))   return LookupConstraint(szOwner, szName);
    }
    _CHECK_AND_THROW_(0, "Invalid object type");
}

const User& Dictionary::LookupUser (const char* key) const
{   return lookup_object<User, UserMap>(m_mapUsers, key); }

const Tablespace& Dictionary::LookupTablespace (const char* key) const
{   return lookup_object<Tablespace, TablespaceMap>(m_mapTablespaces, key); }

const Cluster& Dictionary::LookupCluster (const char* key1, const char* key2) const
{   return lookup_object<Cluster, ClusterMap>(m_mapClusters, key1, key2); }

const Table& Dictionary::LookupTable (const char* key1, const char* key2) const
{   return lookup_object<Table, TableMap>(m_mapTables, key1, key2); }

const Index& Dictionary::LookupIndex (const char* key1, const char* key2) const
{   return lookup_object<Index, IndexMap>(m_mapIndexes, key1, key2); }

const Constraint& Dictionary::LookupConstraint (const char* key1, const char* key2) const
{   return lookup_object<Constraint, ConstraintMap>(m_mapConstraints, key1, key2); }

const Trigger& Dictionary::LookupTrigger (const char* key1, const char* key2) const
{   return lookup_object<Trigger, TriggerMap>(m_mapTriggers, key1, key2); }

const View& Dictionary::LookupView (const char* key1, const char* key2) const
{   return lookup_object<View, ViewMap>(m_mapViews, key1, key2); }

const Sequence& Dictionary::LookupSequence (const char* key1, const char* key2) const
{   return lookup_object<Sequence, SequenceMap>(m_mapSequences, key1, key2); }

const Function& Dictionary::LookupFunction (const char* key1, const char* key2) const
{   return lookup_object<Function, FunctionMap>(m_mapFunctions, key1, key2); }

const Procedure& Dictionary::LookupProcedure (const char* key1, const char* key2) const
{   return lookup_object<Procedure, ProcedureMap>(m_mapProcedures, key1, key2); }

const Package& Dictionary::LookupPackage (const char* key1, const char* key2) const
{   return lookup_object<Package, PackageMap>(m_mapPackages, key1, key2); }

const PackageBody& Dictionary::LookupPackageBody (const char* key1, const char* key2) const
{   return lookup_object<PackageBody, PackageBodyMap>(m_mapPackageBodies, key1, key2); }

const Type& Dictionary::LookupType (const char* key1, const char* key2) const
{   return lookup_object<Type, TypeMap>(m_mapTypes, key1, key2); }

const TypeBody& Dictionary::LookupTypeBody (const char* key1, const char* key2) const
{   return lookup_object<TypeBody, TypeBodyMap>(m_mapTypeBodies, key1, key2); }

const JavaSource& Dictionary::LookupJavaSource (const char* key1, const char* key2) const
{   return lookup_object<JavaSource, JavaSourceMap>(m_mapJavaSources, key1, key2); }

const Synonym& Dictionary::LookupSynonym (const char* key1, const char* key2) const
{   return lookup_object<Synonym, SynonymMap>(m_mapSynonyms, key1, key2); }

// pseudo-objects
const Grantor& Dictionary::LookupGrantor (const char* key) const
{   return lookup_object<Grantor, GrantorMap>(m_mapGrantors, key); }

const Grantee& Dictionary::LookupGrantee (const char* key) const
{   return lookup_object<Grantee, GranteeMap>(m_mapGrantees, key); }

// last add-on
const DBLink& Dictionary::LookupDBLink (const char* key1, const char* key2) const
{   return lookup_object<DBLink, DBLinkMap>(m_mapDBLinks, key1, key2); }

const SnapshotLog& Dictionary::LookupSnapshotLog (const char* key1, const char* key2) const
{   return lookup_object<SnapshotLog, SnapshotLogMap>(m_mapSnapshotLogs, key1, key2); }

const Snapshot& Dictionary::LookupSnapshot (const char* key1, const char* key2) const
{   return lookup_object<Snapshot, SnapshotMap>(m_mapSnapshots, key1, key2); }

const DbObject& Dictionary::LookupObject (const char* szOwner, const char* szName, const char* szType) const
{
    if (szType[0] && szType[1] 
    && szType[2] && szType[3]) {
        if (!stricmp(szType, "TABLE"))        return LookupTable(szOwner, szName);
        if (!stricmp(szType, "VIEW"))         return LookupView(szOwner, szName);
        if (!stricmp(szType, "TRIGGER"))      return LookupTrigger(szOwner, szName);
        if (!stricmp(szType, "PACKAGE"))      return LookupPackage(szOwner, szName);
        if (!stricmp(szType, "PACKAGE BODY")) return LookupPackageBody(szOwner, szName);
        if (!stricmp(szType, "FUNCTION"))     return LookupFunction(szOwner, szName);
        if (!stricmp(szType, "PROCEDURE"))    return LookupProcedure(szOwner, szName);
        if (!stricmp(szType, "SEQUENCE"))     return LookupSequence(szOwner, szName);
        if (!stricmp(szType, "SYNONYM"))      return LookupSynonym(szOwner, szName);
        if (!stricmp(szType, "GRANTOR"))      return LookupGrantor(szName);
        if (!stricmp(szType, "GRANTEE"))      return LookupGrantee(szName);
        if (!stricmp(szType, "CLUSTER"))      return LookupCluster(szOwner, szName);
        if (!stricmp(szType, "TABLESPACE"))   return LookupTablespace(szName);
        if (!stricmp(szType, "USER"))         return LookupUser(szName);
        if (!stricmp(szType, "SNAPSHOT"))     return LookupSnapshot(szOwner, szName);
        if (!stricmp(szType, "SNAPSHOT LOG")) return LookupSnapshotLog(szOwner, szName);
        if (!stricmp(szType, "DATABASE LINK"))return LookupDBLink(szOwner, szName);
        if (!stricmp(szType, "TYPE"))         return LookupType(szOwner, szName);
        if (!stricmp(szType, "TYPE BODY"))    return LookupTypeBody(szOwner, szName);
        if (!stricmp(szType, "JAVA_SOURCE"))  return LookupJavaSource(szOwner, szName);
        if (!stricmp(szType, "INDEX"))        return LookupIndex(szOwner, szName);     
        if (!stricmp(szType, "CONSTRAINT"))   return LookupConstraint(szOwner, szName);
    }
    _CHECK_AND_THROW_(0, "Invalid object type");
}

/// Member-functions for enumeration ////////////////////////////////////////

void Dictionary::EnumClusters (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Cluster, ClusterMap>(m_mapClusters, pfnDo, param); }

void Dictionary::EnumTables (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Table, TableMap>(m_mapTables, pfnDo, param); }

void Dictionary::EnumTablespaces (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Tablespace, TablespaceMap>(m_mapTablespaces, pfnDo, param); }

void Dictionary::EnumConstraints (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Constraint, ConstraintMap>(m_mapConstraints, pfnDo, param); }

void Dictionary::EnumIndexes (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Index, IndexMap>(m_mapIndexes, pfnDo, param); }

void Dictionary::EnumTriggers (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Trigger, TriggerMap>(m_mapTriggers, pfnDo, param); }

void Dictionary::EnumViews (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<View, ViewMap>(m_mapViews, pfnDo, param); }

void Dictionary::EnumSequences (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Sequence, SequenceMap>(m_mapSequences, pfnDo, param); }

void Dictionary::EnumFunctions (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Function, FunctionMap>(m_mapFunctions, pfnDo, param); }

void Dictionary::EnumProcedures (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Procedure, ProcedureMap>(m_mapProcedures, pfnDo, param); }

void Dictionary::EnumPackages (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Package, PackageMap>(m_mapPackages, pfnDo, param); }

void Dictionary::EnumPackageBodies (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<PackageBody, PackageBodyMap>(m_mapPackageBodies, pfnDo, param); }

void Dictionary::EnumTypes (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Type, TypeMap>(m_mapTypes, pfnDo, param); }

void Dictionary::EnumTypeBodies (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<TypeBody, TypeBodyMap>(m_mapTypeBodies, pfnDo, param); }

void Dictionary::EnumJavaSources (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<JavaSource, JavaSourceMap>(m_mapJavaSources, pfnDo, param); }

void Dictionary::EnumSynonyms (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Synonym, SynonymMap>(m_mapSynonyms, pfnDo, param); }

void Dictionary::EnumUsers (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<User, UserMap>(m_mapUsers, pfnDo, param); }

// pseudo-objects
void Dictionary::EnumGrantors (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Grantor, GrantorMap>(m_mapGrantors, pfnDo, param); }

void Dictionary::EnumGrantees (void (*pfnDo)(DbObject&, void*), void* param)
{  enum_objects<Grantee, GranteeMap>(m_mapGrantees, pfnDo, param); }

// last add-on
void Dictionary::EnumDBLinks (void (*pfnDo)(DbObject&, void*), void* param)
{   enum_objects<DBLink, DBLinkMap>(m_mapDBLinks, pfnDo, param); }

void Dictionary::EnumSnapshotLogs (void (*pfnDo)(DbObject&, void*), void* param)
{   enum_objects<SnapshotLog, SnapshotLogMap>(m_mapSnapshotLogs, pfnDo, param); }

void Dictionary::EnumSnapshots (void (*pfnDo)(DbObject&, void*), void* param)
{   enum_objects<Snapshot, SnapshotMap>(m_mapSnapshots, pfnDo, param); }

/// Member-functions for destroying /////////////////////////////////////////

void Dictionary::DestroyAll ()
{
    m_mapUsers.clear();
    m_mapTablespaces.clear();
    m_mapClusters.clear();
    m_mapTables.clear();
    m_mapIndexes.clear();
    m_mapConstraints.clear();
    m_mapTriggers.clear();
    m_mapViews.clear();
    m_mapSequences.clear();
    m_mapFunctions.clear();
    m_mapProcedures.clear();
    m_mapPackages.clear();
    m_mapPackageBodies.clear();
    m_mapTypes.clear();
    m_mapTypeBodies.clear();
    m_mapSynonyms.clear();
    // pseudo-objects
    m_mapGrantors.clear();
    m_mapGrantees.clear();
    // last add-on
    m_mapDBLinks.clear();
    m_mapSnapshotLogs.clear();
    m_mapSnapshots.clear();
}

void Dictionary::DestroyUser (const char* key)
{   destroy_object<User, UserMap>(m_mapUsers, key); }

void Dictionary::DestroyTablespace (const char* key)
{   destroy_object<Tablespace, TablespaceMap>(m_mapTablespaces, key); }

void Dictionary::DestroyCluster (const char* key1, const char* key2)
{   destroy_object<Cluster, ClusterMap>(m_mapClusters, key1, key2); }

void Dictionary::DestroyTable (const char* key1, const char* key2)
{   destroy_object<Table, TableMap>(m_mapTables, key1, key2); }

void Dictionary::DestroyIndex (const char* key1, const char* key2)
{   destroy_object<Index, IndexMap>(m_mapIndexes, key1, key2); }

void Dictionary::DestroyConstraint (const char* key1, const char* key2)
{   destroy_object<Constraint, ConstraintMap>(m_mapConstraints, key1, key2); }

void Dictionary::DestroyTrigger (const char* key1, const char* key2)
{   destroy_object<Trigger, TriggerMap>(m_mapTriggers, key1, key2); }

void Dictionary::DestroyView (const char* key1, const char* key2)
{   destroy_object<View, ViewMap>(m_mapViews, key1, key2); }

void Dictionary::DestroySequence (const char* key1, const char* key2)
{   destroy_object<Sequence, SequenceMap>(m_mapSequences, key1, key2); }

void Dictionary::DestroyFunction (const char* key1, const char* key2)
{   destroy_object<Function, FunctionMap>(m_mapFunctions, key1, key2); }

void Dictionary::DestroyProcedure (const char* key1, const char* key2)
{   destroy_object<Procedure, ProcedureMap>(m_mapProcedures, key1, key2); }

void Dictionary::DestroyPackage (const char* key1, const char* key2)
{   destroy_object<Package, PackageMap>(m_mapPackages, key1, key2); }

void Dictionary::DestroyPackageBody (const char* key1, const char* key2)
{   destroy_object<PackageBody, PackageBodyMap>(m_mapPackageBodies, key1, key2); }

void Dictionary::DestroyType (const char* key1, const char* key2)
{   destroy_object<Type, TypeMap>(m_mapTypes, key1, key2); }

void Dictionary::DestroyTypeBody (const char* key1, const char* key2)
{   destroy_object<TypeBody, TypeBodyMap>(m_mapTypeBodies, key1, key2); }

void Dictionary::DestroyJavaSource (const char* key1, const char* key2)
{   destroy_object<JavaSource, JavaSourceMap>(m_mapJavaSources, key1, key2); }

void Dictionary::DestroySynonym (const char* key1, const char* key2)
{   destroy_object<Synonym, SynonymMap>(m_mapSynonyms, key1, key2); }

// pseudo-objects
void Dictionary::DestroyGrantor (const char* key)
{   destroy_object<Grantor, GrantorMap>(m_mapGrantors, key); }

void Dictionary::DestroyGrantee (const char* key)
{   destroy_object<Grantee, GranteeMap>(m_mapGrantees, key); }

// last add-on
void Dictionary::DestroyDBLink (const char* key1, const char* key2)
{   destroy_object<DBLink, DBLinkMap>(m_mapDBLinks, key1, key2); }

void Dictionary::DestroySnapshotLog (const char* key1, const char* key2)
{   destroy_object<SnapshotLog, SnapshotLogMap>(m_mapSnapshotLogs, key1, key2); }

void Dictionary::DestroySnapshot (const char* key1, const char* key2)
{   destroy_object<Snapshot, SnapshotMap>(m_mapSnapshots, key1, key2); }


}// END namespace OraMetaDict
