C:\Apps\NSIS3\makensis.exe /DSQLTOOLSVER=18b%1 /DSQLTOOLS_BUILDDIR=Release SQLTools.nsi
C:\Apps\NSIS3\makensis.exe /DSQLTOOLSVER=18b%1 /DSQLTOOLS_BUILDDIR=Release /DNOADMIN=1 SQLTools.nsi

rm -r SQLTools_18b%1
mkdir SQLTools_18b%1
copy ..\SQLHelp\SqlQkRef.chm			SQLTools_18b%1
copy ..\_output_\Release\SQLTools.chm	SQLTools_18b%1
copy ..\_output_\Release\SQLTools.exe	SQLTools_18b%1
copy ..\Grep\grep.exe					SQLTools_18b%1
copy ..\SQLTools\ReadMe.txt			    SQLTools_18b%1
copy ..\SQLTools\License.txt			SQLTools_18b%1
copy ..\SQLTools\History.txt			SQLTools_18b%1

mkdir SQLTools_18b%1\Launcher
copy ..\sqlt-launcher\sqlt-launcher.exe .\SQLTools_18b%1\Launcher
copy ..\sqlt-launcher\sqlt-launcher.xml .\SQLTools_18b%1\Launcher

pkzip25 -dir -max -add SQLTools_18b%1 SQLTools_18b%1\*.*
rm -r SQLTools_18b%1