C:\Apps\NSIS3\makensis.exe /DSQLTOOLSVER=20b%1 /DSQLTOOLS_BUILDDIR=Release SQLTools.nsi
C:\Apps\NSIS3\makensis.exe /DSQLTOOLSVER=20b%1 /DSQLTOOLS_BUILDDIR=Release /DNOADMIN=1 SQLTools.nsi

rm -r SQLTools_20b%1
mkdir SQLTools_20b%1
copy ..\SQLHelp\SqlQkRef.chm			SQLTools_20b%1
copy ..\_output_\Release\SQLTools.chm	SQLTools_20b%1\SQLToolsU.chm
copy ..\_output_\Release\SQLTools.exe	SQLTools_20b%1\SQLToolsU.exe
copy ..\SQLTools\ReadMe.txt			    SQLTools_20b%1
copy ..\SQLTools\License.txt			SQLTools_20b%1
copy ..\SQLTools\History.txt			SQLTools_20b%1

mkdir SQLTools_20b%1\Launcher
copy ..\sqlt-launcher\sqlt-launcher.exe .\SQLTools_20b%1\Launcher
copy ..\sqlt-launcher\sqlt-launcher.xml .\SQLTools_20b%1\Launcher

pkzip25 -dir -max -add SQLTools_20b%1 SQLTools_20b%1\*.*
rm -r SQLTools_20b%1