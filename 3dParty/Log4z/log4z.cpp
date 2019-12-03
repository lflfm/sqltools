
/*
 * Log4z License
 * -----------
 * 
 * Log4z is licensed under the terms of the MIT license reproduced below.
 * This means that Log4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2012 YaweiZhang <yawei_zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */

#include "log4z.h"

#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
//#include <locale>
#include <assert.h>
#include <time.h>
#include <string.h>


#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma warning(disable:4996)
#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#endif




_ZSUMMER_BEGIN
_ZSUMMER_LOG4Z_BEGIN





void SleepMillisecond(unsigned int ms)
{
#ifdef WIN32
	::Sleep(ms);
#else
	usleep(1000*ms);
#endif
}

unsigned int GetTimeMillisecond()
{
#ifdef WIN32
	return ::GetTickCount();
#else
	struct timeval tm;
	gettimeofday(&tm, NULL);
	return (tm.tv_sec * 1000 + (tm.tv_usec/1000));
#endif
}


bool TimeToTm(time_t t, tm * tt)
{
#ifdef WIN32
	if (localtime_s(tt, &t) == 0)
	{
		return true;
	}
	return false;
#else
	if (localtime_r(&t, tt) != NULL)
	{
		return true;
	}
	return false;
#endif
}

time_t TmToTime(tm * tt)
{
	return mktime(tt);
}

std::string TimeToString(time_t t)
{
	char m[128] = {0};
	tm tt;
	if (TimeToTm(t, &tt))
	{
		sprintf(m, "%d-%02d-%02d %02d:%02d:%02d", tt.tm_year+1900, tt.tm_mon+1, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);
	}
	else
	{
		sprintf(m, "0000-00-00 00:00:00");
	}
	return m;
}


void TrimString(std::string & str, int flag = 2)
{
	if (str.length() == 0)
	{
		return ;
	}
	if (flag == 0 || flag == 2)
	{
		std::string::size_type pos = str.find_first_not_of(' ');
		if (pos != std::string::npos)
		{
			str = str.substr(pos, std::string::npos);
		}
	}
	if (flag == 1 || flag == 2)
	{
		std::string::size_type pos = str.find_last_not_of(' ');
		if (pos != std::string::npos)
		{
			str = str.substr(0, pos+1);
		}
	}
}


void FixWinSep(std::string &path)
{
	if (path.length() == 0)
	{
		return;
	}
	for (std::string::iterator iter = path.begin(); iter != path.end(); ++iter)
	{
		if (*iter == '\\')
		{
			*iter = '/';
		}
	}
}

void FixPath(std::string &path)
{
	if (path.length() == 0)
	{
		return;
	}
	FixWinSep(path);
	if (path.at(path.length()-1) != '/')
	{
		path += "/";
	}
}
void TrimXmlContent(std::string &content)
{
	if (content.length() == 0)
	{
		return ;
	}
	std::string dest;
	std::string::size_type pos1 = 0;
	std::string::size_type pos2 = 0;
	do 
	{
		pos2 = content.find("<!--", pos1);
		if (pos2 == std::string::npos)
		{
			dest.append(content.substr(pos1, std::string::npos));
			break;
		}

		dest.append(content.substr(pos1, pos2 - pos1));
		pos1 = pos2;
		pos2 = content.find("-->",pos1+4);
		if (pos2 == std::string::npos)
		{
			break;
		}
		pos1 = pos2+3;
	} while (1);
	content = dest;
}

bool GetXmlParam(std::string content, std::string param, std::vector<std::string> & data)
{
	TrimXmlContent(content);
	data.clear();
	std::string preParam = "<";
	preParam += param;
	preParam += ">";
	std::string suffParam = "</";
	suffParam += param;
	suffParam += ">";

	std::string::size_type pos1 = 0;
	while(1)
	{
		pos1 = content.find(preParam, pos1);
		if (pos1 == std::string::npos)
		{
			break;
		}
		pos1 += preParam.length();
		std::string::size_type pos2 = content.find(suffParam, pos1);
		if (pos2 == std::string::npos)
		{
			break;
		}

		data.push_back(content.substr(pos1, pos2-pos1));
		TrimString(data.back());
	}
	return true;
}
bool GetXmlParam(std::string content, std::string param, std::string & data)
{
	data.clear();
	std::vector<std::string> vct;
	GetXmlParam(content, param, vct);
	if (vct.size() > 0)
	{
		data = vct.at(0);
		return true;
	}
	return false;
}
bool GetXmlParam(std::string content, std::string param, int & data)
{
	std::string str;
	if (!GetXmlParam(content, param, str))
	{
		return false;
	}
	if (str.length() == 0)
	{
		return false;
	}
	data = atoi(str.c_str());
	return true;
}
bool GetXmlParam(std::string content, std::string param, bool & data)
{
	std::string str;
	if (!GetXmlParam(content, param, str))
	{
		return false;
	}
	if (str.length() == 0)
	{
		return false;
	}
	int n = atoi(str.c_str());
	if (n == 0)
	{
		data = false;
	}
	else
	{
		data = true;
	}
	return true;
}


bool IsDirectory(std::string path)
{
#ifdef WIN32
	return PathIsDirectory(path.c_str()) ? true : false;
#else
	DIR * pdir = opendir(path.c_str());
	if (pdir == NULL)
	{
		return false;
	}
	else
	{
		closedir(pdir);
		pdir = NULL;
		return true;
	}
#endif
	return false;
}


bool CreateDir(std::string path)
{
#ifdef WIN32
	return CreateDirectory(path.c_str(), NULL) ? true : false;
#else
	return (mkdir(path.c_str(), S_IRWXU|S_IRWXG|S_IRWXO) == 0);
#endif
	return false;
} 


bool CreateRecursionDir(std::string path)
{
	if (path.length() == 0) return true;
	std::string sub;
	char lastchar=0;
	FixPath(path);

	std::string::size_type pos = path.find('/');
	while (pos != std::string::npos)
	{
		std::string cur = path.substr(0, pos-0);
		if (cur.length() > 0 && !IsDirectory(cur))
		{
			if (!CreateDir(cur))
			{
				return false;
			}
		}
		pos = path.find('/', pos+1);
	}

	return true;
}


std::string GetMainLoggerName()
{
	std::string name;
	name = "MainLog";
#ifdef WIN32
	
	char buf[260] = {0};
	if (GetModuleFileName(NULL, buf, 259) > 0)
	{
		name = buf;
	}
	std::string::size_type pos = name.rfind("\\");
	if (pos != std::string::npos)
	{
		name = name.substr(pos+1, std::string::npos);
	}
	pos = name.rfind(".");
	if (pos != std::string::npos)
	{
		name = name.substr(0, pos-0);
	}
#else
	pid_t id = getpid();
	char buf[260];
	sprintf(buf, "/proc/%d/cmdline", (int)id);
	std::fstream i;
	i.open(buf, std::ios::in);
	if (!i.is_open())
	{
		return name;
	}
	std::string line;
	std::getline(i, line);
	i.close();
	if (line.length() > 0)
	{
		name = line;
	}
	std::string::size_type pos = name.rfind("/");
	if (pos != std::string::npos)
	{
		name = name.substr(pos+1, std::string::npos);
	}
#endif
	return name;
}



#ifdef WIN32

const static WORD cs_sColor[LOG_FATAL+1] = {
	0,
	FOREGROUND_BLUE|FOREGROUND_GREEN,
	FOREGROUND_GREEN|FOREGROUND_RED,
	FOREGROUND_RED,
	FOREGROUND_GREEN,
	FOREGROUND_RED|FOREGROUND_BLUE};
#else

const static char cs_strColor[LOG_FATAL+1][50] = { 
	"\e[0m",
	"\e[34m\e[1m",//hight blue
	"\e[33m", //yellow
	"\e[31m", //red
	"\e[32m", //green
	"\e[35m"};
#endif

void ShowColorText(const char *text, int level = LOG_DEBUG)
{
	if (level < LOG_DEBUG || level > LOG_FATAL) goto showfail;
	if (level == LOG_DEBUG) goto showfail;
#ifndef WIN32
	printf("%s%s\e[0m", cs_strColor[level], text);
#else
	HANDLE hStd = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStd == INVALID_HANDLE_VALUE) goto showfail;

	CONSOLE_SCREEN_BUFFER_INFO oldInfo;
	if (!GetConsoleScreenBufferInfo(hStd, &oldInfo)) goto showfail;

	if (SetConsoleTextAttribute(hStd, cs_sColor[level]))
	{
		printf(text);
		SetConsoleTextAttribute(hStd, oldInfo.wAttributes);
	}
	else
	{
		goto showfail;
	}
#endif

	return;

showfail:
	printf(text);
}





class CLock
{
public:
	CLock();
	virtual ~CLock();

public:
	void Lock();
	void UnLock();


private:
#ifdef WIN32
	CRITICAL_SECTION m_crit;
#else
	pthread_mutex_t  m_crit;
#endif
};


CLock::CLock()
{
#ifdef WIN32
	InitializeCriticalSection(&m_crit);
#else
	//m_crit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_crit, &attr);
	pthread_mutexattr_destroy(&attr);
#endif
}


CLock::~CLock()
{
#ifdef WIN32
	DeleteCriticalSection(&m_crit);
#else
	pthread_mutex_destroy(&m_crit);
#endif
}


void CLock::Lock()
{
#ifdef WIN32
	EnterCriticalSection(&m_crit);
#else
	pthread_mutex_lock(&m_crit);
#endif
}


void CLock::UnLock()
{
#ifdef WIN32
	LeaveCriticalSection(&m_crit);
#else
	pthread_mutex_unlock(&m_crit);
#endif
}

class CAotoLock
{
public:
	explicit CAotoLock(CLock & lk):m_lock(lk)
	{
	}
	~CAotoLock()
	{
		m_lock.UnLock();
	}
	inline void Lock()
	{
		m_lock.Lock();
	}


private:
	CLock & m_lock;
};


class CThread
{
public:
	CThread();
	virtual ~CThread();

public:
	bool Start();
	virtual void Run() = 0;
	bool Wait();
	inline unsigned long long GetThreadID() {return m_hThreadID;};
private:
	unsigned long long m_hThreadID;
};


CThread::CThread()
{
	m_hThreadID = 0;
}
CThread::~CThread()
{

}
#ifdef WIN32

static unsigned int WINAPI  ThreadProc(LPVOID lpParam)
{
	CThread * p = (CThread *) lpParam;
	p->Run();
	_endthreadex(0);
	return 0;
}
#else

static void * ThreadProc(void * pParam)
{
	CThread * p = (CThread *) pParam;
	p->Run();
	return NULL;
}
#endif


bool CThread::Start()
{
#ifdef WIN32
	unsigned long long ret = _beginthreadex(NULL, 0, ThreadProc, (void *) this, 0, NULL);

	if (ret == -1 || ret == 1  || ret == 0)
	{
		return false;
	}
	m_hThreadID = ret;
#else
	pthread_t ptid = 0;
	int ret = pthread_create(&ptid, NULL, ThreadProc, (void*)this);
	if (ret != 0)
	{
		return false;
	}
	m_hThreadID = ptid;

#endif
	return true;
}



bool CThread::Wait()
{
#ifdef WIN32
	if (WaitForSingleObject((HANDLE)m_hThreadID, INFINITE) != WAIT_OBJECT_0)
	{
		return false;
	}
#else
	if (pthread_join((pthread_t)m_hThreadID, NULL) != 0)
	{
		return false;
	}
#endif
	return true;
}


static const char *const LOG_STRING[]=
{
	"LOG_DEBUG",
	"LOG_INFO",
	"LOG_WARN",
	"LOG_ERROR",
	"LOG_ALARM",
	"LOG_FATAL",
};


struct tagLog
{
	int _id;	//dest logger id
	int _level;	//log level
	time_t _time; //loged time
	char _log[LOG_BUF_SIZE]; //log buf
};

struct tagLogger 
{
	std::string _path; 
	std::string _preName;
	int  _level; //logger filter level
	bool _display; //display log, if false that record to file only
	bool  _enable; //enable logger
	std::fstream	_logger; //the file handle.
	tagLogger(){_enable = true; _display = false; _level = LOG_DEBUG;}
};


class CLogerManager : public CThread, public ILog4zManager
{
public:
	CLogerManager();
	~CLogerManager();
	std::string GetDefaultConfig();

	bool AddLoggerFromConfig(std::string cfg);
	bool AddLogger(int nLoggerID, std::string path, std::string preName, int nLevel,bool display);

	bool ChangeLoggerLevel(int nLoggerID, int nLevel);
	bool ChangeLoggerDisplay(int nLoggerID, bool enable);

	bool PushtLog(int id, int level, const char * log);

	bool Start();
	bool Stop();

protected:
	virtual void Run();
	bool PopLogger(tagLog *& log);
private:
	bool		m_bRuning;
	time_t		m_uptimes;
	tagLogger	m_loggers[LOGGER_MAX];
	std::list<tagLog *> m_logs;
	CLock	m_lock;
};




ILog4zManager * ILog4zManager::GetInstance()
{
	static CLogerManager m;
	return &m;
}


CLogerManager::CLogerManager()
{
	m_bRuning = false;
	m_uptimes = 0;
	for (unsigned int i=0; i<LOGGER_MAX; i++)
	{
		m_loggers[i]._enable = false;
		m_loggers[i]._level = LOG_DEBUG;
		m_loggers[i]._display = true;
	}
}


CLogerManager::~CLogerManager()
{
	Stop();
}


std::string CLogerManager::GetDefaultConfig()
{
	return ""
		"<!--at current version, configure can't support xml Comments.-->\n" 
		"<!--logger id must in the region [0,LOGGER_MAX) -->\n"
		"<logger>\n"
		"\t<loggerid>0</loggerid> <!--#id-->\n"
		"\t<path>./log/</path> <!--#path-->\n"
		"\t<prename>test</prename> <!--#prefix name-->\n"
		"\t<level>0</level> <!--#DEBUG WARN ERROR ALARM FATAL-->\n"
		"\t<display>1</display> <!--#display to screent-->\n"
		"</logger>\n";
}


static time_t GetNextUptime(time_t tt)
{
	tm t;
	TimeToTm(tt, &t);
	return tt - t.tm_hour*3600 - t.tm_min*60 - t.tm_sec + 24*3600; //next update time
}

static void FixLoggerConfig(tagLogger & logger, int loggerID)
{
	TrimString(logger._path);
	if (logger._path.length() == 0)
	{
		logger._path = "./log/";
	}
	else
	{
		FixPath(logger._path);
	}
	if (logger._preName.length() == 0)
	{
		logger._preName = GetMainLoggerName();
		if (loggerID != 0)
		{
			char buf[20];
			sprintf(buf, "%d", loggerID);
			logger._preName.append(buf);
		}
	}
}

static bool CreateLoggerHandle(tagLogger & logger, time_t now)
{
	if (logger._logger.is_open())
	{
		logger._logger.close();
	}
	
	tm t;
	TimeToTm(now, &t);
	char buf[100];
	sprintf(buf, "%04d_%02d", t.tm_year+1900, t.tm_mon+1);
	std::string path = logger._path + buf + "/";
	if (!IsDirectory(path))
	{
		CreateRecursionDir(path);
	}
	sprintf(buf, "%s_%04d_%02d_%02d.log", logger._preName.c_str(), t.tm_year+1900, t.tm_mon+1, t.tm_mday);
	path += buf;
	logger._logger.open(path.c_str(), std::ios::app|std::ios::out|std::ios::binary);
	if (!logger._logger.is_open())
	{
//		printf( "ERROR: Open log file  failed !  the path=%s\n" << path.c_str());
		logger._enable = false;
		return false;
	}
	return true;
}

bool CLogerManager::AddLoggerFromConfig(std::string cfg)
{
	std::string content;
	std::ifstream f;
	f.open(cfg.c_str(), std::ios_base::in);
	content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

	std::vector<std::string> vctLogger;
	GetXmlParam(content, "logger", vctLogger);
	for (unsigned int i=0; i<vctLogger.size(); i++)
	{
		int lid = 0;
		tagLogger l;
		
		GetXmlParam(vctLogger[i], "loggerid", lid);
		GetXmlParam(vctLogger[i], "path", l._path);
		GetXmlParam(vctLogger[i], "prename", l._preName);
		GetXmlParam(vctLogger[i], "level", l._level);
		GetXmlParam(vctLogger[i], "display", l._display);
		AddLogger(lid, l._path, l._preName, l._level, l._display);
	}
	return true;
}
bool CLogerManager::AddLogger(int nLoggerID, std::string path, std::string preName, int nLevel, bool display)
{
	if (nLoggerID < 0 || nLoggerID >= LOGGER_MAX)
	{
		return false;
	}
	m_loggers[nLoggerID]._path = path;
	m_loggers[nLoggerID]._preName = preName;
	m_loggers[nLoggerID]._level = nLevel;
	m_loggers[nLoggerID]._enable = true;
	m_loggers[nLoggerID]._display = display;
	FixLoggerConfig(m_loggers[nLoggerID], nLoggerID);
	return true;
}


bool CLogerManager::ChangeLoggerLevel(int nLoggerID, int nLevel)
{
	if (nLoggerID <0 || nLoggerID >= LOGGER_MAX || nLevel < LOG_DEBUG || nLevel >LOG_FATAL) return false;
	m_loggers[nLoggerID]._level = nLevel;
	return true;
}

bool CLogerManager::ChangeLoggerDisplay(int nLoggerID, bool enable)
{
	if (nLoggerID <0 || nLoggerID >= LOGGER_MAX) return false;
	m_loggers[nLoggerID]._display = enable;
	return true;
}


bool CLogerManager::Start()
{
	if (!m_loggers[0]._enable)
	{
		AddLogger(0, "", "", LOG_DEBUG, true);
	}
	time_t now = time(NULL);
	m_uptimes = GetNextUptime(now);
	m_bRuning = true;
	bool ret = CThread::Start();
	assert(ret);
	return ret;
}
bool CLogerManager::Stop()
{
	if (m_bRuning == true)
	{
		m_bRuning = false;
		Wait();
		return true;
	}
	return false;
}

bool CLogerManager::PushtLog(int id, int level, const char * log)
{
	if ((id >=0 && id < LOGGER_MAX) &&  level < m_loggers[id]._level)
	{
		return false;
	}

	tagLog * pLog = new tagLog;
	pLog->_id =id;
	pLog->_level = level;
	pLog->_time = time(NULL);
	if (strlen(log) >= LOG_BUF_SIZE)
	{
		memcpy(pLog->_log, log, LOG_BUF_SIZE);
		pLog->_log[LOG_BUF_SIZE-1] = '\0';
	}
	else
	{
		memcpy(pLog->_log, log, strlen(log)+1);
	}
	CAotoLock l(m_lock);
	l.Lock();
	m_logs.push_back(pLog);
	return true;
}

bool CLogerManager::PopLogger(tagLog *& log)
{
	CAotoLock l(m_lock);
	l.Lock();
	if (m_logs.empty())
	{
		return false;
	}
	log = m_logs.front();
	m_logs.pop_front();
	return true;
}

void CLogerManager::Run()
{

	tagLog * pLog = NULL;
	std::string text;
	text.reserve(2000);
	{
		std::string str = "-----------------  log4z thread started!   ----------------------------";
		LOGA(str);
		for (int i=0; i<LOGGER_MAX; i++)
		{
			if (m_loggers[i]._enable)
			{
				LOGA("AddLogger id=" <<i 
					<<" path=" <<m_loggers[i]._path
					<<" preName=" <<m_loggers[i]._preName
					<<" level=" << m_loggers[i]._level
					<<" display=" << m_loggers[i]._display);
			}
		}
	}

	int needFlush[LOGGER_MAX];
	while (true)
	{
		memset(needFlush, 0, sizeof(needFlush));
		while(PopLogger(pLog))
		{
			//fix logger id
			if (!m_loggers[pLog->_id]._enable && m_loggers[0]._enable)
			{
				pLog->_id = 0;
			}
			//filter
			if (!m_loggers[pLog->_id]._enable || pLog->_level <m_loggers[pLog->_id]._level  )
			{
				delete pLog;
				pLog = NULL;
				continue;
			}
			//update new file
			if ((pLog->_time >= m_uptimes) 
				|| ( pLog->_time < m_uptimes && m_uptimes - pLog->_time > 24*3600))
			{
				m_uptimes = GetNextUptime(pLog->_time);
				for (unsigned int i=0; i<LOGGER_MAX; i++)
				{
					if (m_loggers[i]._enable)
					{
						CreateLoggerHandle(m_loggers[i], pLog->_time);
					}
				}
			}
			//check file handle
			if (!m_loggers[pLog->_id]._logger.is_open() 
				|| !m_loggers[pLog->_id]._logger.good())
			{
				CreateLoggerHandle(m_loggers[pLog->_id], pLog->_time);
				if (!m_loggers[pLog->_id]._logger.is_open())
				{
					m_loggers[pLog->_id]._enable = false;
					delete pLog;
					pLog = NULL;
					continue;
				}
			}
			
			//record
			text = TimeToString(pLog->_time);
			text += " ";
			text += LOG_STRING[pLog->_level];
			text += " ";
			text += pLog->_log;
			text += "\r\n";
			m_loggers[pLog->_id]._logger.write(text.c_str(), (std::streamsize)text.length());
			needFlush[pLog->_id] ++;
			//print to screen
			if (m_loggers[pLog->_id]._display)
			{
				ShowColorText(text.c_str(), pLog->_level);
			}
			delete pLog;
			pLog = NULL;
		}

		//flush
		for (unsigned int i=0; i<LOGGER_MAX; i++)
		{
			if (m_loggers[i]._enable && needFlush[i] > 0)
			{
				m_loggers[i]._logger.flush();
			}
		}
		//stopped
		if (!m_bRuning)
		{
			break;
		}
		//delay. 
		SleepMillisecond(200);
	}

	for (unsigned int i=0; i<LOGGER_MAX; i++)
	{
		if (m_loggers[i]._enable)
		{
			m_loggers[i]._logger.close();
		}
	}
	m_bRuning = false;
}





_ZSUMMER_LOG4Z_END
_ZSUMMER_END

