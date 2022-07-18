#pragma once

class CLogFile
{
private:
	HANDLE _hFile = 0;
	SRWLOCK _SRWLock = SRWLOCK_INIT;
	CSHORT _day = MAXSHORT;

	NTSTATUS Init(_In_ PTIME_FIELDS tf);
	// D:PNO_ACCESS_CONTROLS:(ML;;;;;LW)

public:
	static CLogFile s_logfile;

	~CLogFile();

	NTSTATUS Init();

	void printf(_In_ PCSTR format, ...);
};

#define DbgPrint CLogFile::s_logfile.printf

#define LOG(args)  CLogFile::s_logfile.args

#pragma message("!!! LOG >>>>>>")
