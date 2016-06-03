#include "CmLog.h"
#include <sys/timeb.h>
#include <time.h>
#include <stdarg.h>
#include "CmFile.h"

DWORD CmLog::gConsoleWritenLen; //Just for avoiding debug warning when writeConsole is called
char CmLog::gLogBufferA[SAVE_BUF_LEN];
CmLog* CmLog::gLog = NULL;
//HANDLE CmLog::_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
int CmLog::_hConsole = 0;
string CmLog::_fileName;
bool CmLog::_LogPrefix = false;
string CmLog::_DataSN;

void CmLog::Set(CStr& name, bool append, CStr& dataSN)
{
	_fileName = name;
	if (!append) {
		FILE* file = fopen(_fileName.c_str(), "w");
		if (file)
			fclose(file);
		else
			; //LogError("Can't open log file: %s\n", _fileName.c_str());
	}
	_DataSN = dataSN;
}

void CmLog::Set(bool logPrefix)
{
	_LogPrefix = logPrefix;
}

// Some time information about current line of log
void CmLog::LogFilePrefix(FILE* file)
{
	if (!_LogPrefix)
		return;

	timeb tb;
	ftime(&tb);
	char gDataTime[64];
	strftime(gDataTime, sizeof(gDataTime), "%y/%m/%d %H:%M:%S", localtime(&tb.time));
	fprintf(file, "[%s:%03d] ", gDataTime, tb.millitm);
}

// Log information and show to the console
void CmLog::Log(CStr& msg)
{
	Log(msg.c_str());
}

void CmLog::Log(const char* msg)
{
	FILE* file = fopen(_fileName.c_str(), "a+");
	if (file != NULL) {
		LogFilePrefix(file);
		fprintf(file, "%s", msg);
		fclose(file);
	}

	if (_hConsole)
		printf("%s", msg);
}

void CmLog::LogLine(const char* fmt, ...)
{
	char buf[1 << 16];
	va_list args;
	va_start( args, fmt );
	vsnprintf(buf, 1<<16, fmt, args );
	Log(buf);
}


void CmLog::LogLine(WORD attribs, const char* fmt, ...)
{
//	if (_hConsole)
//		SetConsoleTextAttribute(_hConsole, attribs);

	char buf[1 << 16];
	va_list args;
	va_start( args, fmt );
	vsnprintf(buf, 1<<16, fmt, args );
	Log(buf);

//	if (_hConsole)
//		SetConsoleTextAttribute(_hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
}

void CmLog::LogError(const char* fmt, ...)
{
	char buf[1 << 16];
	strcpy(buf, "Error: ");
	va_list args;
	va_start( args, fmt );
	vsnprintf(buf + 7, 1<<16, fmt, args );
//	LogLine(FOREGROUND_RED | FOREGROUND_INTENSITY, buf);
	//exit(1);
}

void CmLog::LogWarning(const char* fmt, ...)
{
	char buf[1 << 16];
	strcpy(buf, "Warning: ");
	va_list args;
	va_start( args, fmt );
	vsnprintf(buf + 9, 1<<16, fmt, args );
//	LogLine(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, buf);
}

void CmLog::LogProgress(const char* fmt, ...)
{
	if (_hConsole == NULL)
		return;

	char buf[1 << 16];
	va_list args;
	va_start( args, fmt );
	vsnprintf(buf, 1<<16, fmt, args );
	printf(buf);
}

// Clear log file
void CmLog::LogClear(void)
{
	FILE* file = fopen(_fileName.c_str(), "w");
	CV_Assert(file != NULL);
	fclose(file);
}

FileStorage& CmLog::GetFS()
{
	static FileStorage fs(CmFile::GetFolder(_fileName) + CmFile::GetNameNE(_fileName) + ".xml", FileStorage::WRITE);
	return fs;
}

void CmLog::Demo()
{
	printf("%s:%d\n", __FILE__, __LINE__);
	CmLog::Set("Run.log", true);

	CmLog::Log("Test 1 2 3 4 5 6  7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31\n");
	CmLog::Log("Test 1 2 3 4 5 6  7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31\n");
	CmLog::LogLine("\t \t %s\n", "10");
//	CmLog::LogLine(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "%d  %s\n", 1, "Test 1 2 3 4 5 6  7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31");
	CmLog::LogError("%s %d %s\n",  __FILE__, __LINE__, __FILE__);
	CmLog::LogWarning("%s %d %s\n", __FILE__, __LINE__, __FILE__);
	for (int i = 0; i < 10000; i++)	{
		if (i % 2)
			CmLog::LogProgress("%s\r", __FILE__);
		else
			CmLog::LogProgress("%5d%74s\r", i, "");
	}
	CmLog::LogProgress("%79s\r", "");
	CmLog::LogProgress("OK\n");
	CmLog::Set(false);
	CmLog::LogLine("No prefix\n");
}
