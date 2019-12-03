SELECT TABLE_NAME, /*COMPOSITE,*/ PARTITION_NAME, /*SUBPARTITION_COUNT,*/ HIGH_VALUE, HIGH_VALUE_LENGTH, PARTITION_POSITION, TABLESPACE_NAME, PCT_FREE, PCT_USED, INI_TRANS, MAX_TRANS, INITIAL_EXTENT, NEXT_EXTENT, MIN_EXTENT, MAX_EXTENT, PCT_INCREASE, FREELISTS, FREELIST_GROUPS, LOGGING, /*COMPRESSION,*/ NUM_ROWS, BLOCKS, EMPTY_BLOCKS, AVG_SPACE, CHAIN_CNT, AVG_ROW_LEN, SAMPLE_SIZE, LAST_ANALYZED, BUFFER_POOL/*, GLOBAL_STATS, USER_STATS*/ FROM USER_TAB_PARTITIONS          ;
SELECT INDEX_NAME, /*COMPOSITE,*/ PARTITION_NAME, /*SUBPARTITION_COUNT,*/ HIGH_VALUE, HIGH_VALUE_LENGTH, PARTITION_POSITION, STATUS, TABLESPACE_NAME, PCT_FREE, INI_TRANS, MAX_TRANS, INITIAL_EXTENT, NEXT_EXTENT, MIN_EXTENT, MAX_EXTENT, PCT_INCREASE, FREELISTS, FREELIST_GROUPS, LOGGING, /*COMPRESSION,*/ BLEVEL, LEAF_BLOCKS, DISTINCT_KEYS, AVG_LEAF_BLOCKS_PER_KEY, AVG_DATA_BLOCKS_PER_KEY, CLUSTERING_FACTOR, NUM_ROWS, SAMPLE_SIZE, LAST_ANALYZED, BUFFER_POOL/*, USER_STATS, PCT_DIRECT_ACCESS, GLOBAL_STATS, DOMIDX_OPSTATUS, PARAMETERS*/ FROM USER_IND_PARTITIONS          ;
--SELECT TABLE_NAME, COLUMN_NAME, LOB_NAME, PARTITION_NAME, LOB_PARTITION_NAME, LOB_INDPART_NAME, PARTITION_POSITION, COMPOSITE, CHUNK, PCTVERSION, CACHE, IN_ROW, TABLESPACE_NAME, INITIAL_EXTENT, NEXT_EXTENT, MIN_EXTENTS, MAX_EXTENTS, PCT_INCREASE, FREELISTS, FREELIST_GROUPS, LOGGING, BUFFER_POOL FROM USER_LOB_PARTITIONS          ;

SELECT INDEX_NAME, /*TABLE_NAME,*/ PARTITIONING_TYPE, /*SUBPARTITIONING_TYPE,*/ PARTITION_COUNT, /*DEF_SUBPARTITION_COUNT,*/ PARTITIONING_KEY_COUNT, /*SUBPARTITIONING_KEY_COUNT,*/ LOCALITY, ALIGNMENT, DEF_TABLESPACE_NAME, DEF_PCT_FREE, DEF_INI_TRANS, DEF_MAX_TRANS, DEF_INITIAL_EXTENT, DEF_NEXT_EXTENT, DEF_MIN_EXTENTS, DEF_MAX_EXTENTS, DEF_PCT_INCREASE, DEF_FREELISTS, DEF_FREELIST_GROUPS, DEF_LOGGING, DEF_BUFFER_POOL/*, DEF_PARAMETERS*/ FROM USER_PART_INDEXES            ;
SELECT NAME, OBJECT_TYPE, COLUMN_NAME, COLUMN_POSITION FROM USER_PART_KEY_COLUMNS        ;
--SELECT TABLE_NAME, COLUMN_NAME, LOB_NAME, LOB_INDEX_NAME, DEF_CHUNK, DEF_PCTVERSION, DEF_CACHE, DEF_IN_ROW, DEF_TABLESPACE_NAME, DEF_INITIAL_EXTENT, DEF_NEXT_EXTENT, DEF_MIN_EXTENTS, DEF_MAX_EXTENTS, DEF_PCT_INCREASE, DEF_FREELISTS, DEF_FREELIST_GROUPS, DEF_LOGGING, DEF_BUFFER_POOL FROM USER_PART_LOBS               ;
SELECT TABLE_NAME, PARTITIONING_TYPE, /*SUBPARTITIONING_TYPE,*/ PARTITION_COUNT, /*DEF_SUBPARTITION_COUNT,*/ PARTITIONING_KEY_COUNT, /*SUBPARTITIONING_KEY_COUNT,*/ /*STATUS,*/ DEF_TABLESPACE_NAME, DEF_PCT_FREE, DEF_PCT_USED, DEF_INI_TRANS, DEF_MAX_TRANS, DEF_INITIAL_EXTENT, DEF_NEXT_EXTENT, DEF_MIN_EXTENTS, DEF_MAX_EXTENTS, DEF_PCT_INCREASE, DEF_FREELISTS, DEF_FREELIST_GROUPS, DEF_LOGGING, /*DEF_COMPRESSION,*/ DEF_BUFFER_POOL FROM USER_PART_TABLES             ;

--SELECT TABLE_NAME, PARTITION_NAME, SUBPARTITION_NAME, /*HIGH_VALUE,*/ /*HIGH_VALUE_LENGTH,*/ SUBPARTITION_POSITION, TABLESPACE_NAME, PCT_FREE, PCT_USED, INI_TRANS, MAX_TRANS, INITIAL_EXTENT, NEXT_EXTENT, MIN_EXTENT, MAX_EXTENT, PCT_INCREASE, FREELISTS, FREELIST_GROUPS, LOGGING, /*COMPRESSION,*/ NUM_ROWS, BLOCKS, EMPTY_BLOCKS, AVG_SPACE, CHAIN_CNT, AVG_ROW_LEN, SAMPLE_SIZE, LAST_ANALYZED, BUFFER_POOL, GLOBAL_STATS, USER_STATS FROM USER_TAB_SUBPARTITIONS       ;
--SELECT INDEX_NAME, PARTITION_NAME, SUBPARTITION_NAME, /*HIGH_VALUE,*/ /*HIGH_VALUE_LENGTH,*/ SUBPARTITION_POSITION, STATUS, TABLESPACE_NAME, PCT_FREE, INI_TRANS, MAX_TRANS, INITIAL_EXTENT, NEXT_EXTENT, MIN_EXTENT, MAX_EXTENT, PCT_INCREASE, FREELISTS, FREELIST_GROUPS, LOGGING, /*COMPRESSION,*/ BLEVEL, LEAF_BLOCKS, DISTINCT_KEYS, AVG_LEAF_BLOCKS_PER_KEY, AVG_DATA_BLOCKS_PER_KEY, CLUSTERING_FACTOR, NUM_ROWS, SAMPLE_SIZE, LAST_ANALYZED, BUFFER_POOL, USER_STATS, GLOBAL_STATS FROM USER_IND_SUBPARTITIONS       ;
--SELECT TABLE_NAME, COLUMN_NAME, LOB_NAME, LOB_PARTITION_NAME, SUBPARTITION_NAME, LOB_SUBPARTITION_NAME, LOB_INDSUBPART_NAME, SUBPARTITION_POSITION, CHUNK, PCTVERSION, CACHE, IN_ROW, TABLESPACE_NAME, INITIAL_EXTENT, NEXT_EXTENT, MIN_EXTENTS, MAX_EXTENTS, PCT_INCREASE, FREELISTS, FREELIST_GROUPS, LOGGING, BUFFER_POOL FROM USER_LOB_SUBPARTITIONS       ;

--SELECT NAME, OBJECT_TYPE, COLUMN_NAME, COLUMN_POSITION FROM USER_SUBPART_KEY_COLUMNS     ;
--SELECT TABLE_NAME, SUBPARTITION_NAME, SUBPARTITION_POSITION, TABLESPACE_NAME, HIGH_BOUND FROM USER_SUBPARTITION_TEMPLATES  ;
