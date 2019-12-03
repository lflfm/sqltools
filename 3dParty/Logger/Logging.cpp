#include "stdafx.h"
#include "threading.h"
#include "Logger.h"
#include <fstream>

using namespace framework::Diagnostics;
using namespace framework::Threading;

int _tmain(int argc, _TCHAR* argv[])
{
	CLogger<CNoLock> logger(LogLevel::Info, _T("MyApp"));

	logger.AddOutputStream(std::wcout, false, LogLevel::Error);
	logger.AddOutputStream(new std::wofstream("c:\\temp\\myapp.log"), true, framework::Diagnostics::LogLevel::Info);
		
	WRITELOG(logger, framework::Diagnostics::LogLevel::Info, _T("Program starting"));
	WRITELOG(logger, framework::Diagnostics::LogLevel::Warn, _T("Something may have gone wrong"));
	WRITELOG(logger, framework::Diagnostics::LogLevel::Error, _T("Something did go wrong"));
	
	CLogger<CNoLock>* loggerPtr = &logger; // Just to demonstrate an easy way to use pointer to a logger object
	LOGINFOP(loggerPtr, _T("Program Ending"));

	return 0;
}

