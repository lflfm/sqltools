@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by SQLTools.HPJ. >"hlp\SQLTools.hm"
echo. >>"hlp\SQLTools.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\SQLTools.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\SQLTools.hm"

echo. >>"hlp\SQLTools.hm"
echo // Prompts (IDP_*) >>"hlp\SQLTools.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\SQLTools.hm"

echo. >>"hlp\SQLTools.hm"
echo // Resources (IDR_*) >>"hlp\SQLTools.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\SQLTools.hm"

echo. >>"hlp\SQLTools.hm"
echo // Dialogs (IDD_*) >>"hlp\SQLTools.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\SQLTools.hm"
makehm IDD_,HIDD_,0x20000 "\WORK\CPP\COMMON2\INCLUDE\COMMON\AboutDlgRes.h" >>"hlp\SQLTools.hm"

echo. >>"hlp\SQLTools.hm"
echo // Frame Controls (IDW_*) >>"hlp\SQLTools.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\SQLTools.hm"
REM -- Make help for Project SQLTools


echo Building Win32 Help files
rem start /wait "E:\Program Files\DevStudio\VC\bin\hcw" /C /E /M "hlp\SQLTools.hpj"
hcw /C /E /M "hlp\SQLTools.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\SQLTools.hlp" goto :Error
if not exist "hlp\SQLTools.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\SQLTools.hlp" Debug
if exist Debug\nul copy "hlp\SQLTools.cnt" Debug
if exist Release\nul copy "hlp\SQLTools.hlp" Release
if exist Release\nul copy "hlp\SQLTools.cnt" Release
echo.
goto :done

:Error
echo hlp\SQLTools.hpj(1) : error: Problem encountered creating help file

:done
echo.
