
#include "../log4z.h"
#include <iostream>
#include <stdio.h>
using namespace zsummer::log4z;

int main(int argc, char *argv[])
{

	//start log4z
	ILog4zManager::GetInstance()->Start();

	//log with the level.
	//LOGD: LOG WITH level LOG_DEBUG
	//LOGI: LOG WITH level LOG_INFO
	LOGD(" *** " << "hellow wolrd" <<" *** ");
	LOGI("loginfo");
	LOGW("log warning");
	LOGE("log err");
	LOGA("log alarm");
	LOGF("log fatal");

	//stop log4z, can ignore the step.
	//ILog4zManager::GetInstance()->Stop();
	printf("press anykey to exit.");
	getchar();
	return 0;
}

