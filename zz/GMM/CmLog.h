#pragma once
#include "CmDefinition.h"

class CmLog
{
public:	
	// A demo function to show how to use this class
	static void Demo();

	static void Set(CStr& name, bool append, CStr& dataSN = string());
	static void Set(bool logPrefix); 

	// Log information and show to the console
	static void Log(CStr& msg);
	static void Log(const char* msg);
	static void LogLine(const char* format, ...);
	static void LogLine(WORD attribs, const char* format, ...);  //FOREGROUND_GREEN
	static void LogError(const char* format, ...);
	static void LogWarning(const char* format, ...);

	// Show information to the console but not log it
	static void LogProgress(const char* format, ...);

	// Get file storage
	static FileStorage& GetFS();

	// Clear log file
	static void LogClear(void);

	static const int SAVE_BUF_LEN = 1024;
private:
    //static HANDLE _hConsole;
    static int _hConsole;
	static string _fileName;
	static CmLog* gLog;
	static bool _LogPrefix;
	static string _DataSN; // data save name

	static DWORD gConsoleWritenLen; //Just for avoiding debug warning when writeConsole is called
	static char gLogBufferA[SAVE_BUF_LEN];

	// Some time information about current line of log
	static void LogFilePrefix(FILE* file);
};
