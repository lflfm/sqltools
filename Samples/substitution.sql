REM make sure that "substition variables" turned on
REM   - & button on the toolbar
REM   - Menu Script -> Enable Subtitution Variables

PROMPT ************************************************************
PROMPT SELECT * FROM all_objects WHERE owner = 'SYS';
PROMPT ************************************************************

define param = SYS
SELECT * FROM all_objects WHERE owner = '&param';


PROMPT ************************************************************
PROMPT SELECT * FROM all_objects WHERE owner = 'PACKAGE';
PROMPT ************************************************************

define part1 = all_
define part2 = objects
define param = PACKAGE
SELECT * FROM &part1&part2 WHERE object_type = '&param';


PROMPT ************************************************************
PROMPT SELECT * FROM sys.all_objects WHERE owner = 'PACKAGE BODY';
PROMPT ************************************************************

define owner = sys
define part1 = all_
define part2 = objects
define param = PACKAGE BODY
SELECT * FROM &owner..&part1&part2 WHERE object_type = '&param';
