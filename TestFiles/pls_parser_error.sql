                    (SELECT * FROM v_bal_sheet_me
                     UNION ALL
--(
                        SELECT
                          bspl.cob_date,
                          dc.run_id,
                          dc.run_date,
                          dept_key,
                          org_key,
                          adac_acct_key,
                          gl_acct_key,
                          affiliate_org_key,
                          prod_type_key,
                          product_class_key,
                          ctpy_key,
                          ctpy_ref_date,
                          amount_type_key,
                          data_source,
                          currency_cd,
                          SUM(native_amt) AS native_amount,
                          SUM(usd_amt) AS usd_amount,
                          processing_location,
                          key_pos_53
                      FROM
                          f_bal_sheet_plug bspl,
                        (SELECT cob_date, run_id, run_date FROM dc_date_card) dc
                      WHERE
                          bspl.cob_date = dc.cob_date AND
                          bspl.run_id = dc.run_id
                          GROUP BY
                          bspl.cob_date,
                          dc.run_id,
                          dc.run_date,
                          dept_key,
                          org_key,
                          adac_acct_key,
                          gl_acct_key,
                          affiliate_org_key,
                          prod_type_key,
                          product_class_key,
                          ctpy_key,
                          ctpy_ref_date,
                          amount_type_key,
                          data_source,
                          currency_cd,
                          processing_location,
                        key_pos_53
--)
                   ) bal
