SELECT * FROM dual;

--https://www.red-gate.com/simple-talk/sql/oracle/ansi-sql/
SELECT *
           /* multi-line comment, line #1
    multi-line comment, line #2
    multi-line comment, line #3 */
  FROM (SELECT * FROM a)
    JOIN b
      ON (a.key1 = b.key1 AND a.key2 = b.key2)
    INNER JOIN c
      ON (a.key1 = c.key1 AND a.key2 = c.key2)
    RIGHT JOIN d
      ON (a.key1 = d.key1 AND a.key2 = d.key2)
    LEFT JOIN e
      ON (a.key1 = e.key1 AND a.key2 = e.key2)
    JOIN f USING(key1, key2)
    NATURAL JOIN g
  WHERE 1=2
/

--get_something(1, 2, 3, 4, 5, 6, 7, 8, 9)
select case when 1 = 2 then (3 - 4) when 5 = 6 then case when 7 < 8 then 9 else 10 end else case when 11 < 12 then 13 else 14 end end col_1, case when 1 = 2 then 3 else 4 end col_2 from dual;

  select;

                select
									col col_alias,
									col	"col alias",
									col	as col_alias,
									col	as "col	alias",
									tab.col,
									func(col),
									tab.col	col_alias,
									func(col)	col_alias,
									tab.col	as col_alias,
									func(col)	as col_alias,
									(select	1	from dual) col_alias,
									(select	1, 2,	3, 4,	5, 6,	7, 8,	9	from dual) col_alias
								from dual;
/

select /*+parallel*/
    i.obj#,
    --ignore, we might want to keep this endt-comment on the previous line
    case
        when 1 < 2 then 'true'
        else 'false'
    end as always_true,
    i.samplesize,
    i.dataobj#,
    nvl(i.spare1, i.intcols),
    i.spare6,
    decode(
        i.pctthres$,
        null,
        null,
        mod(trunc(i.pctthres$ / 256), 256)
    ),
    ist.cachedblk,
    ist.cachehit,
    ist.logicalread "logicalread"
from ind$ i,
    ind_stats$ ist,
    (
        select enabled,
            min(cols) as unicols,
            min(to_number(bitand(defer, 1))) as deferrable#,
            min(to_number(bitand(defer, 4))) as valid#
        from cdef$
        where obj# = :1
            and enabled > 1
        group by enabled
    ) c
where i.obj# = c.enabled(+)
    and i.obj# = ist.obj#(+)
    and i.bo# = :1
order by i.obj#