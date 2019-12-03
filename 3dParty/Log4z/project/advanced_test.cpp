
#include "../log4z.h"
#include <iostream>
#include <stdio.h>

#if WIN32
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <stdlib.h>
#endif
using namespace zsummer::log4z;

//cross platform function:
//sleep in linux or windows
void SleepMillisecond(unsigned int ms);
//create thread in linux or windows
bool CreateThread(void(*run)());


//enum multi logger
enum ENUM_LOGGER
{
	L_MAIN=0, //the main logger, It away exist.
	L_MYSQL,	//the user-defined logger.
	L_NET,		//the user-defined logger.
	L_MONITER,	//the user-defined logger.
};

//virtual the mysql module in a project.
void MysqlModuleTrace()
{
	while(1)
	{
		LOG_STREAM(L_MYSQL, LOG_DEBUG, "mysql trace some msg ...");
		LOG_STREAM(L_MYSQL, LOG_DEBUG, "mysql trace some msg ...");
		if (rand()%100 <2)
		{
			LOG_STREAM(L_MYSQL, LOG_FATAL, "mysql some time put the fatal msg ...");
		}
		if (rand()%100 <5)
		{
			LOG_STREAM(L_MYSQL, LOG_ERROR, "mysql some time put the error msg ...");
		}
		SleepMillisecond(rand()%3000);
	}
}
//virtual the Network module in a project.
void NetworkModuleTrace()
{
	while(1)
	{
		LOG_STREAM(L_NET, LOG_DEBUG, "network trace some msg ...");
		LOG_STREAM(L_NET, LOG_DEBUG, "network trace some msg ...");
		if (rand()%100 <2)
		{
			LOG_STREAM(L_NET, LOG_FATAL, "network some time put the fatal msg ...");
		}
		if (rand()%100 <5)
		{
			LOG_STREAM(L_NET, LOG_ERROR, "network some time put the error msg ...");
		}
		SleepMillisecond(rand()%3000);
	}
}
//virtual the Moniter module in a project.
void MoniterModuleTrace()
{
	while(1)
	{
		LOG_STREAM(L_MONITER, LOG_DEBUG, "network trace some msg ...");
		LOG_STREAM(L_MONITER, LOG_DEBUG, "network trace some msg ...");
		if (rand()%100 <2)
		{
			LOG_STREAM(L_MONITER, LOG_WARN, "network some time put the warning msg ...");
		}
		if (rand()%100 <5)
		{
			LOG_STREAM(L_MONITER, LOG_ALARM, "network some time put the alarm  msg ...");
		}
		SleepMillisecond(rand()%3000);
	}
}


int main(int argc, char *argv[])
{
	//add and configure logger
	ILog4zManager::GetInstance()->AddLogger(L_MAIN,"", "MAIN", LOG_DEBUG);
	ILog4zManager::GetInstance()->AddLogger(L_MYSQL,"", "MYSQL", LOG_DEBUG);
	ILog4zManager::GetInstance()->AddLogger(L_NET,"", "NET", LOG_DEBUG);
	ILog4zManager::GetInstance()->AddLogger(L_MONITER,"", "MONITER", LOG_DEBUG);

	//start log4z
	ILog4zManager::GetInstance()->Start();

	//create thread, it create the virtual module .
	CreateThread(&MysqlModuleTrace);
	CreateThread(&NetworkModuleTrace);
	CreateThread(&MoniterModuleTrace);

	//virtual the main logic in project.
	while(1)
	{

		LOGI("main thread trace msg ...");
		SleepMillisecond(rand()%3000);
	}


	printf("press anykey to exit.");
	getchar();
	return 0;
}


//
void SleepMillisecond(unsigned int ms)
{
#ifdef WIN32
	::Sleep(ms);
#else
	usleep(1000*ms);
#endif
}



//
#ifdef WIN32
 unsigned int WINAPI  ThreadProc(LPVOID lpParam)
{
	((void(*)())(lpParam))();
	_endthreadex(0);
	return 0;
}
#else
void * ThreadProc(void * pParam)
{
	((void(*)())(pParam))();
	return NULL;
}
#endif
bool CreateThread(void(*run)())
{
#ifdef WIN32
	unsigned long long ret = _beginthreadex(NULL, 0, ThreadProc,(void*) run, 0, NULL);

	if (ret == -1 || ret == 1  || ret == 0)
	{
		return false;
	}
#else
	pthread_t ptid = 0;
	int ret = pthread_create(&ptid, NULL, ThreadProc, (void*)run);
	if (ret != 0)
	{
		return false;
	}
#endif
	return true;
}