 Platform Requirements
~~~~~~~~~~~~~~~~~~~~~~~
Operating Systems:
        - Windows 2000, XP, 7 (should work under Vista or 10 but not tested)
        - 5M of disk space
Oracle Software:
        - Client 8i/9i/10g/11g/12c
        - Server 8i/9i/10g/11g/12c

 Miscellaneous Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using with Oracle Instant Client (provided by Dennis McRitchie):

After extrating Instant Client on your hard drive you will need to 
set the TNS_ADMIN environment variable to point to the location of 
the various *.ora  files  (e.g., sqlnet.ora, ldap.ora, tnsnames.ora). 
Oracle suggests putting these in the ORACLE_HOME directory, and sqlplus 
can find them there, but without TNS_ADMIN, SQLTools apparently looks 
in the wrong place.

So what's needed is:
ORACLE_HOME=<instant_client_path>
PATH=%ORACLE_HOME%;<rest_of_path>
TNS_ADMIN=%ORACLE_HOME%

If you get “OCI8: Cannot allocate OCI handle” during the program startup
look at http://sqltools.net/cgi-bin/yabb2/YaBB.pl?board=sqlt_faq for solutions.

In the case “OCI.dll was not found, can’t open SQLTools” 
check http://sqltools.net/cgi-bin/yabb2/YaBB.pl?num=1159230811

See History.txt for information about improvements, known and fixed bugs.

You may use SQLTools 1.3.0.X for connection to Server 7.3.X/8.0.X.

Run the installer with the /S flag for silent installation.