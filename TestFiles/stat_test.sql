SELECT * FROM all_objects;

select sid from v$session where audsid = userenv('SESSIONID');
SELECT * FROM v$statname WHERE name = 'session uga memory';
SELECT * FROM v$sesstat WHERE sid = 150 AND statistic# = 20;
SELECT * FROM v$statname WHERE name = 'session pga memory';
SELECT * FROM v$sesstat WHERE sid = 150 AND statistic# = 25;
SELECT * FROM v$statname WHERE name = 'parse time elapsed';
SELECT * FROM v$sesstat WHERE sid = 150 AND statistic# = 329;

--clean grid after reconnect if stats is not available anymore
--last line is empty
--grid icorct painting for unused space
--cannot copy from stat view
--stats is not available for newly oped doc
--selection space in comment highlight all space in comments and strings