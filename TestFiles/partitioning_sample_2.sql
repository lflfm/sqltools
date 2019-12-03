CONNECT b2rsg/b2rsg_123@tnyb2st5

SELECT Count(*) FROM CMS_CTPY;

--DROP TABLE test_cms_ctpy;

CREATE TABLE test_cms_ctpy
(
  business_date          DATE          NOT NULL,
  run_id                 NUMBER        NOT NULL,
  ctpy_id_c              VARCHAR2(6)   NOT NULL,
  ctpy_n                 VARCHAR2(100) NOT NULL,
  ctpy_lvl_c             NUMBER(7,0)   NOT NULL,
  ctpy_prnt_id_c         VARCHAR2(6)   NULL,
  ctpy_hi_prnt_id_c      VARCHAR2(6)   NULL,
  cntry_dmcle_id_c       VARCHAR2(2)   NULL,
  cntry_inc_id_c         VARCHAR2(2)   NULL,
  cms_indus_naic_c       VARCHAR2(8)   NULL,
  ib_indus_c             VARCHAR2(5)   NULL,
  adac_clnt_typ_c        VARCHAR2(4)   NULL,
  grd_cst_id             VARCHAR2(12)  NULL,
  cntry_rsk_id_c         VARCHAR2(2)   NULL,
  ctpy_cs_id_c           VARCHAR2(10)  NULL,
  user_cr_anlyt_id_c     VARCHAR2(8)   NULL,
  user_resp_anlyt_c      VARCHAR2(8)   NULL,
  process_add_id         VARCHAR2(15)  NOT NULL,
  stamp_add_dtime        DATE          NOT NULL,
  cms_ps_affil_c         VARCHAR2(6)   NULL,
  secrecy_waive_ind      VARCHAR2(1)   NULL,
  cms_ctpy_bank_typ_c    VARCHAR2(1)   NULL,
  cms_ctpy_actvy_typ_c   VARCHAR2(1)   NULL,
  cms_ctpy_catg_typ_c    VARCHAR2(4)   NULL,
  cms_ctpy_dmcle_typ_c   VARCHAR2(18)  NULL,
  cms_oecd_cntry_i       VARCHAR2(1)   NULL,
  cms_econ_sectr_c       VARCHAR2(2)   NULL,
  cms_ctpy_int_ext_typ_c VARCHAR2(1)   NULL
)
PARTITION BY RANGE (business_date, run_id)
(
  PARTITION p_31MAY2009_1    VALUES LESS THAN (TO_DATE('31-MAY-2009', 'DD-MON-YYYY'), 2),
  PARTITION p_31MAY2009_2    VALUES LESS THAN (TO_DATE('31-MAY-2009', 'DD-MON-YYYY'), 3),
  PARTITION p_31MAY2009_rest VALUES LESS THAN (TO_DATE('31-MAY-2009', 'DD-MON-YYYY'), MAXVALUE)
)
;

CREATE INDEX i1_test_cms_ctpy ON test_cms_ctpy(business_date, run_id, adac_clnt_typ_c) LOCAL;


ALTER TABLE test_cms_ctpy ADD CONSTRAINT pk_test_cms_ctpy
  PRIMARY KEY (business_date, run_id, ctpy_id_c) USING INDEX LOCAL
/

BEGIN
  INSERT /*+append*/ INTO test_cms_ctpy (
    business_date, run_id, ctpy_id_c, ctpy_n, ctpy_lvl_c, ctpy_prnt_id_c,
    ctpy_hi_prnt_id_c, cntry_dmcle_id_c, cntry_inc_id_c, cms_indus_naic_c,
    ib_indus_c, adac_clnt_typ_c, grd_cst_id, cntry_rsk_id_c, ctpy_cs_id_c,
    user_cr_anlyt_id_c, user_resp_anlyt_c, process_add_id, stamp_add_dtime,
    cms_ps_affil_c, secrecy_waive_ind, cms_ctpy_bank_typ_c, cms_ctpy_actvy_typ_c,
    cms_ctpy_catg_typ_c, cms_ctpy_dmcle_typ_c, cms_oecd_cntry_i, cms_econ_sectr_c,
    cms_ctpy_int_ext_typ_c
    )
  SELECT
    business_date, &RUN_ID, ctpy_id_c, ctpy_n, ctpy_lvl_c, ctpy_prnt_id_c,
    ctpy_hi_prnt_id_c, cntry_dmcle_id_c, cntry_inc_id_c, cms_indus_naic_c,
    ib_indus_c, adac_clnt_typ_c, grd_cst_id, cntry_rsk_id_c, ctpy_cs_id_c,
    user_cr_anlyt_id_c, user_resp_anlyt_c, process_add_id, stamp_add_dtime,
    cms_ps_affil_c, secrecy_waive_ind, cms_ctpy_bank_typ_c, cms_ctpy_actvy_typ_c,
    cms_ctpy_catg_typ_c, cms_ctpy_dmcle_typ_c, cms_oecd_cntry_i, cms_econ_sectr_c,
    cms_ctpy_int_ext_typ_c
  FROM cms_ctpy
    WHERE ROWNUM < 100000;

  COMMIT;
END;
/

ANALYZE TABLE test_cms_ctpy compute STATISTICS;

BEGIN
     dbms_stats.gather_table_stats(OWNNAME          => USER,
                                   TABNAME          => 'TEST_CMS_CTPY',
                                   ESTIMATE_PERCENT => 10,
                                   BLOCK_SAMPLE     => FALSE,
                                   DEGREE           => 4,
                                   CASCADE          => TRUE);
END;
/

BEGIN
FOR buff IN (
  SELECT /*+index(test_cms_ctpy i1_test_cms_ctpy)*/ * FROM test_cms_ctpy
    WHERE business_date =
      TO_DATE('31-MAY-2009')
        AND run_id = 1
        AND adac_clnt_typ_c = 1
) LOOP
  NULL;
END LOOP;
END;
/

