        SELECT
            ROWNUM AS ROW_NUM,
            CASE WHEN 'N' = 'Y' THEN '="' || ADAC_UNIT_ID || '"' ELSE ADAC_UNIT_ID END AS ADAC_UNIT_ID,
            ADAC_UNIT_NAME,
            CASE WHEN 'N' = 'Y' THEN '="' || PS_LGL_ENTITY_ID || '"' ELSE PS_LGL_ENTITY_ID END AS PS_LGL_ENTITY_ID,
            PS_LGL_ENTITY_NAME,
            CASE WHEN 'N' = 'Y' THEN '="' || ADAC_ACCT_NUM || '"' ELSE ADAC_ACCT_NUM END AS ADAC_ACCT_NUM,
            ADAC_ACCT_NAME,
            CASE WHEN 'N' = 'Y' THEN '="' || AFFILIATE_ID || '"' ELSE AFFILIATE_ID END AS AFFILIATE_ID,
            AFFILIATE_NAME,
            DATA_SOURCE,
            USD_AMT,
            GROSS_EXP_B1,
            GROSS_EXP_B2,
            GROSS_EXP_ELIM_B1,
            GROSS_EXP_ELIM_B2,
            EAD_B1,
            EAD_B2,
            CAD_NET_B1,
            CAD_NET_B2
        FROM (
            -- Strategic Data Start
            SELECT
                ADAC_UNIT_ID,
                ADAC_UNIT_NAME,
                PS_LGL_ENTITY_ID,
                PS_LGL_ENTITY_NAME,
                ADAC_ACCT_NUM,
                ADAC_ACCT_NAME,
                AFFILIATE_ID,
                AFFILIATE_NAME,
                'S' AS DATA_SOURCE,
                MAX(USD_AMT) AS USD_AMT,
                SUM(GROSS_EXP_B1) AS GROSS_EXP_B1,
                SUM(GROSS_EXP_B2) AS GROSS_EXP_B2,
                SUM(GROSS_EXP_ELIM_B1) AS GROSS_EXP_ELIM_B1,
                SUM(GROSS_EXP_ELIM_B2) AS GROSS_EXP_ELIM_B2,
                SUM(EAD_B1) AS EAD_B1,
                SUM(EAD_B2) EAD_B2,
                SUM(CAD_NET_B1) AS CAD_NET_B1,
                SUM(CAD_NET_B2) AS CAD_NET_B2
            FROM (
                SELECT
                    DFO_LE.ADAC_UNIT_ID,
                    DFO_LE.ADAC_UNIT_NAME,
                    DFO_LE.PS_LGL_ENTITY_ID,
                    DFO_LE.PS_LGL_ENTITY_NAME,
                    DAA.ADAC_ACCT_NUM,
                    DAA.ADAC_ACCT_NAME,
                    DFO_AF.PS_LGL_ENTITY_ID AS AFFILIATE_ID,
                    DFO_AF.PS_LGL_ENTITY_NAME AS AFFILIATE_NAME,
                    SUM(FBCM.USD_AMT) AS USD_AMT,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL1' THEN SUM(FBCM.GROSS_EXPOSURE_AMOUNT) ELSE 0 END AS GROSS_EXP_B1,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL2' THEN SUM(FBCM.GROSS_EXPOSURE_AMOUNT) ELSE 0 END AS GROSS_EXP_B2,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL1' THEN SUM(FBCM.GROSS_EXPOSURE_ELIMINATED_AMT) ELSE 0 END AS GROSS_EXP_ELIM_B1,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL2' THEN SUM(FBCM.GROSS_EXPOSURE_ELIMINATED_AMT) ELSE 0 END AS GROSS_EXP_ELIM_B2,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL1' THEN SUM(FBCM.EXPOSURE_AT_DEFAULT) ELSE 0 END AS EAD_B1,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL2' THEN SUM(FBCM.EXPOSURE_AT_DEFAULT) ELSE 0 END AS EAD_B2,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL1' THEN SUM(FBCM.CAD_NET) ELSE 0 END AS CAD_NET_B1,
                    CASE WHEN FBCM.REGULATORY_APPROACH_CD = 'BASEL2' THEN SUM(FBCM.CAD_NET) ELSE 0 END AS CAD_NET_B2
                FROM
                    ( SELECT * FROM F_BAL_CAPITAL_ME FBCM
                        WHERE COB_DATE = '31-Oct-2007' AND RUN_ID = 10 AND REG_AUTH_ID = 2000) FBCM,
                    D_FIN_ORG DFO_LE, D_FIN_ORG DFO_AF, D_ADAC_ACCT DAA, D_PRODUCT_TYPE DPT
                WHERE
                    ( -- Start Join Conditions --
                        FBCM.ORG_KEY = DFO_LE.ORG_KEY
                        AND FBCM.AFFILIATE_ORG_KEY = DFO_AF.ORG_KEY
                        AND FBCM.ADAC_ACCT_KEY = DAA.ADAC_ACCT_KEY
                        AND FBCM.PROD_TYPE_KEY = DPT.PROD_TYPE_KEY
                    ) -- End Join Conditions --
                    AND
                    ( -- Start Filter Conditions --
                        (null IS NULL OR DPT.PROD_TYPE_0_DESC = null)
                        AND ('0933' IS NULL OR DFO_LE.ADAC_UNIT_ID = '0933')
                        AND ('602' IS NULL OR DFO_LE.PS_LGL_ENTITY_ID = '602')
                        AND (null IS NULL OR (InStr(', ' || null || ',', ', ' || DAA.ADAC_ACCT_NUM || ',') > 0))
                        AND (null IS NULL OR DFO_AF.PS_LGL_ENTITY_ID = null)
                    ) -- End Filter Conditions --
				GROUP BY
					DFO_LE.ADAC_UNIT_ID,
					DFO_LE.ADAC_UNIT_NAME,
					DFO_LE.PS_LGL_ENTITY_ID,
					DFO_LE.PS_LGL_ENTITY_NAME,
					DAA.ADAC_ACCT_NUM,
					DAA.ADAC_ACCT_NAME,
					DFO_AF.PS_LGL_ENTITY_ID,
					DFO_AF.PS_LGL_ENTITY_NAME,
					FBCM.REGULATORY_APPROACH_CD
                )
            GROUP BY
                ADAC_UNIT_ID,
                ADAC_UNIT_NAME,
                PS_LGL_ENTITY_ID,
                PS_LGL_ENTITY_NAME,
                ADAC_ACCT_NUM,
                ADAC_ACCT_NAME,
                AFFILIATE_ID,
                AFFILIATE_NAME
            -- Strategic Data End
            UNION ALL
            -- Tactical Data Start
            SELECT
                ADAC_UNIT_ID,
                ADAC_UNIT_NAME,
                PS_LGL_ENTITY_ID,
                PS_LGL_ENTITY_NAME,
                ADAC_ACCT_NUM,
                ADAC_ACCT_NAME,
                FBSA.AFFILIATE_ID,
                AFFILIATE_NAME,
                'T' AS DATA_SOURCE,
                MAX(USD_AMT),
                NVL(SUM(GROSS_EXP_B1), 0),
                NVL(SUM(GROSS_EXP_B2), 0),
                NVL(SUM(GROSS_EXP_ELIM_B1), 0),
                NVL(SUM(GROSS_EXP_ELIM_B2), 0),
                NVL(SUM(EAD_B1), 0),
                NVL(SUM(EAD_B2), 0),
                NVL(SUM(CAD_NET_B1), 0),
                NVL(SUM(CAD_NET_B2), 0)
            FROM
                (   SELECT -- Start FBSA
                        DFO_LE.ORG_KEY,
                        DFO_LE.ADAC_UNIT_ID,
                        DFO_LE.ADAC_UNIT_NAME,
                        DFO_LE.PS_LGL_ENTITY_ID,
                        DFO_LE.PS_LGL_ENTITY_NAME,
                        DAA.ADAC_ACCT_KEY,
                        DAA.ADAC_ACCT_NUM,
                        DAA.ADAC_ACCT_NAME,
                        DFO_AF.ORG_KEY AS AFFILIATE_ORG_KEY,
                        DFO_AF.PS_LGL_ENTITY_ID AS AFFILIATE_ID,
                        DFO_AF.PS_LGL_ENTITY_NAME AS AFFILIATE_NAME,
                        FBSA_TAB.REFERENCE_ID,
                        SUM(FBSA_TAB.USD_AMOUNT) AS USD_AMT
                    FROM
                        DC_DATE_CARD DDC, F_BAL_SHEET_ADJ FBSA_TAB,
                        D_FIN_ORG DFO_LE, D_ADAC_ACCT DAA, D_FIN_ORG DFO_AF,
                        D_PRODUCT_TYPE DPT
                    WHERE
                        ( -- COB/RUN Criteria
                                DDC.COB_DATE = '31-Oct-2007'
                                AND DDC.RUN_ID = 10
                        )
                        AND
                        ( -- Join F_BAL_SHEET_ADJ
                                FBSA_TAB.COB_DATE = DDC.COB_DATE
                                AND FBSA_TAB.RUN_ID <= DDC.RUN_ID
                        )
                        AND
                        ( -- Start Join Conditions --
                                FBSA_TAB.ORG_KEY = DFO_LE.ORG_KEY
                                AND FBSA_TAB.ADAC_ACCT_KEY = DAA.ADAC_ACCT_KEY
                                AND FBSA_TAB.AFFILIATE_ORG_KEY = DFO_AF.ORG_KEY
                                AND FBSA_TAB.PROD_TYPE_KEY = DPT.PROD_TYPE_KEY
                        ) -- End Join Conditions --
                        AND
                        ( -- Start Filter Conditions --
                            (null IS NULL OR DPT.PROD_TYPE_0_DESC = null)
                            AND ('0933' IS NULL OR DFO_LE.ADAC_UNIT_ID = '0933')
                            AND ('602' IS NULL OR DFO_LE.PS_LGL_ENTITY_ID = '602')
                            AND (null IS NULL OR (InStr(', ' || null || ',', ', ' || DAA.ADAC_ACCT_NUM || ',') > 0))
                            AND (null IS NULL OR DFO_AF.PS_LGL_ENTITY_ID = null)
                        ) -- End Filter Conditions --
                    GROUP BY
                        DFO_LE.ORG_KEY,
                        DFO_LE.ADAC_UNIT_ID,
                        DFO_LE.ADAC_UNIT_NAME,
                        DFO_LE.PS_LGL_ENTITY_ID,
                        DFO_LE.PS_LGL_ENTITY_NAME,
                        DAA.ADAC_ACCT_KEY,
                        DAA.ADAC_ACCT_NUM,
                        DAA.ADAC_ACCT_NAME,
                        DFO_AF.ORG_KEY,
                        DFO_AF.PS_LGL_ENTITY_ID,
                        DFO_AF.PS_LGL_ENTITY_NAME,
                        FBSA_TAB.REFERENCE_ID
                ) FBSA,
                (   SELECT  -- Start MISC
                        DFO_LE.ORG_KEY,
                        DAA.ADAC_ACCT_KEY,
                        DFO_AF.ORG_KEY AS AFFILIATE_ORG_KEY,
                        DMD.BAL_ADJ_REFERENCE_ID,
                        CASE WHEN FDMM.REG_APP_ID = 1
                            THEN SUM(FDMM.GROSS_EXPOSURE) ELSE 0 END AS GROSS_EXP_B1,
                        CASE WHEN FDMM.REG_APP_ID = 3 OR FDMM.REG_APP_ID = 4
                            THEN SUM(FDMM.GROSS_EXPOSURE) ELSE 0 END AS GROSS_EXP_B2,
                        0 AS GROSS_EXP_ELIM_B1,
                        0 AS GROSS_EXP_ELIM_B2,
                        CASE WHEN FDMM.REG_APP_ID = 1
                            THEN SUM(FDMM.EXPOSURE_AT_DEFAULT) ELSE 0 END AS EAD_B1,
                        CASE WHEN FDMM.REG_APP_ID = 3 OR FDMM.REG_APP_ID = 4
                            THEN SUM(FDMM.EXPOSURE_AT_DEFAULT) ELSE 0 END AS EAD_B2,
                        CASE WHEN FDMM.REG_APP_ID = 1
                            THEN SUM(FDMM.CAD_NET) ELSE 0 END AS CAD_NET_B1,
                        CASE WHEN FDMM.REG_APP_ID = 3 OR FDMM.REG_APP_ID = 4
                            THEN SUM(FDMM.CAD_NET) ELSE 0 END AS CAD_NET_B2
                    FROM
                        DC_DATE_CARD DDC, F_DEPT_MISC_ME FDMM, D_MISC_DETAIL DMD,
                        D_FIN_ORG DFO_LE, D_ADAC_ACCT DAA, D_FIN_ORG DFO_AF,
                        D_PRODUCT_TYPE DPT
                    WHERE
                        ( -- COB/RUN Criteria
                            DDC.COB_DATE = '31-Oct-2007'
                            AND DDC.RUN_ID = 10
                        )
                        AND
                        ( -- REG_AUTH Criteria
                            FDMM.REG_AUTH_ID = 2000
                        )
                        AND
                        ( -- Join F_DEPT_MISC_ME
                            FDMM.COB_DATE = DDC.COB_DATE
                            AND FDMM.RUN_ID <= DDC.RUN_ID
                        )
                        AND
                        ( -- Join D_MISC_DETAIL
                            FDMM.COB_DATE = DMD.COB_DATE
                            AND FDMM.RUN_ID = DMD.RUN_ID
                            AND FDMM.FEED_ID = DMD.FEED_ID
                            AND DMD.BAL_ADJ_REFERENCE_ID IS NOT NULL
                        )
                        AND
                        ( -- Start Join Conditions --
                            FDMM.ORG_KEY = DFO_LE.ORG_KEY
                            AND DMD.EXP_ADAC_ACCT_NUM = DAA.ADAC_ACCT_NUM AND DAA.REF_DATE = '31-Oct-2007'
                            AND FDMM.AFFILIATE_ID = DFO_AF.PS_LGL_ENTITY_ID AND DFO_AF.REF_DATE = '31-Oct-2007'
                            AND FDMM.PROD_TYPE_KEY = DPT.PROD_TYPE_KEY
                        ) -- End Join Conditions --
                        AND
                        ( -- Start Filter Conditions --
                            (null IS NULL OR DPT.PROD_TYPE_0_DESC = null)
                            AND ('0933' IS NULL OR DFO_LE.ADAC_UNIT_ID = '0933')
                            AND ('602' IS NULL OR DFO_LE.PS_LGL_ENTITY_ID = '602')
                            AND (null IS NULL OR (InStr(', ' || null || ',', ', ' || DAA.ADAC_ACCT_NUM || ',') > 0))
                            AND (null IS NULL OR DFO_AF.PS_LGL_ENTITY_ID = null)
                        ) -- End Filter Conditions --
                    GROUP BY
                        DFO_LE.ORG_KEY,
                        DAA.ADAC_ACCT_KEY,
                        DFO_AF.ORG_KEY,
                        DMD.BAL_ADJ_REFERENCE_ID,
                        FDMM.REG_APP_ID
                ) MISC
            WHERE
                ( -- Join FBSA, MISC
                  -- Outer Join: Ensure all records in F_BAL_SHEET_ADJ are included
                    FBSA.ORG_KEY = MISC.ORG_KEY (+)
                    AND FBSA.ADAC_ACCT_KEY = MISC.ADAC_ACCT_KEY (+)
                    AND FBSA.AFFILIATE_ORG_KEY = MISC.AFFILIATE_ORG_KEY (+)
                    AND FBSA.REFERENCE_ID  = MISC.BAL_ADJ_REFERENCE_ID (+)
                )
            GROUP BY
                FBSA.ADAC_UNIT_ID,
                FBSA.ADAC_UNIT_NAME,
                FBSA.PS_LGL_ENTITY_ID,
                FBSA.PS_LGL_ENTITY_NAME,
                FBSA.ADAC_ACCT_NUM,
                FBSA.ADAC_ACCT_NAME,
                FBSA.AFFILIATE_ID,
                FBSA.AFFILIATE_NAME
            -- Tactical Data End
        )
        ORDER BY
            ADAC_UNIT_ID, ADAC_UNIT_NAME, PS_LGL_ENTITY_ID,
            PS_LGL_ENTITY_NAME, ADAC_ACCT_NUM, ADAC_ACCT_NAME,
            AFFILIATE_ID, AFFILIATE_NAME, DATA_SOURCE;

