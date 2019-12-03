copy /-y InstallSQLTools_17b%1.exe \_www\sqltools\downloads
rem copy /-y InstallSQLTools_15b%1D.exe \_www\sqltools\downloads

mkdir \_www\sqltools\downloads\DebugInfo\17b%1

copy /-y ..\_output_\Release\SQLTools.exe \_www\sqltools\downloads\DebugInfo\17b%1
copy /-y ..\_output_\Release\SQLTools.map \_www\sqltools\downloads\DebugInfo\17b%1
copy /-y ..\_output_\Release\SQLTools.pdb \_www\sqltools\downloads\DebugInfo\17b%1

rem mkdir \_www\sqltools\downloads\DebugInfo\15b%1_optimization_is_disabled

rem copy /-y ..\_output_\Release-Beta\SQLTools.exe \_www\sqltools\downloads\DebugInfo\15b%1_optimization_is_disabled
rem copy /-y ..\_output_\Release-Beta\SQLTools.map \_www\sqltools\downloads\DebugInfo\15b%1_optimization_is_disabled
rem copy /-y ..\_output_\Release-Beta\SQLTools.pdb \_www\sqltools\downloads\DebugInfo\15b%1_optimization_is_disabled
