#include "stdafx.h"

_NT_BEGIN

#include "log.h"

CLogFile CLogFile::s_logfile;

static const UCHAR s_sd[] = {
	0x01 ,0x00 ,0x14 ,0x90 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x14 ,0x00 ,0x00 ,0x00,
	0x00 ,0x00 ,0x00 ,0x00 ,0x02 ,0x00 ,0x1c ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x11 ,0x00 ,0x14 ,0x00,
	0x00 ,0x00 ,0x00 ,0x00 ,0x01 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x10 ,0x00 ,0x10 ,0x00 ,0x00,
};

NTSTATUS CLogFile::Init()
{
	STATIC_OBJECT_ATTRIBUTES_EX(oa, "\\systemroot\\temp\\CDABA566C6BB44b7BFF0BF8D47553C1F", 
		OBJ_CASE_INSENSITIVE, const_cast<UCHAR*>(s_sd), 0);

	HANDLE hFile;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status = NtCreateFile(&hFile, FILE_ADD_FILE, &oa, &iosb, 0, FILE_ATTRIBUTE_DIRECTORY,
		FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, FILE_DIRECTORY_FILE, 0, 0);

	if (0 <= status)
	{
		NtClose(hFile);
	}

	return status;
}

NTSTATUS CLogFile::Init(_In_ PTIME_FIELDS tf)
{
	WCHAR lpFileName[128];

	if (0 >= swprintf_s(lpFileName, _countof(lpFileName), 
		L"\\systemroot\\temp\\CDABA566C6BB44b7BFF0BF8D47553C1F\\%u-%02u-%02u.log", tf->Year, tf->Month, tf->Day))
	{
		return STATUS_INTERNAL_ERROR;
	}

	IO_STATUS_BLOCK iosb;
	UNICODE_STRING ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE, const_cast<UCHAR*>(s_sd) };
	RtlInitUnicodeString(&ObjectName, lpFileName);

	NTSTATUS status = NtCreateFile(&oa.RootDirectory, FILE_APPEND_DATA|SYNCHRONIZE, &oa, &iosb, 0, 0,
		FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, 0, 0);

	if (0 <= status)
	{
		_hFile = oa.RootDirectory;
	}

	return status;
}

void CLogFile::printf(_In_ PCSTR format, ...)
{
	union {
		FILETIME ft;
		LARGE_INTEGER time;
	};

	TIME_FIELDS tf;

	GetSystemTimeAsFileTime(&ft);
	RtlTimeToTimeFields(&time, &tf);

	va_list ap;
	va_start(ap, format);

	PSTR buf = 0;
	int len = 0;

	enum { tl = _countof("[hh:mm:ss 12345] ") - 1};

	while (0 < (len = _vsnprintf(buf, len, format, ap)))
	{
		if (buf)
		{
			if (tl - 1 == sprintf_s(buf -= tl, tl, "[%02u:%02u:%02u %5x]", tf.Hour, tf.Minute, tf.Second, GetCurrentProcessId()))
			{
				buf[tl - 1] = ' ';

				HANDLE hFile;

				if (_day != tf.Day)
				{
					AcquireSRWLockExclusive(&_SRWLock);	

					if (_day != tf.Day)
					{
						if (hFile = _hFile)
						{
							NtClose(hFile);
							_hFile = 0;
						}

						if (0 <= Init(&tf))
						{
							_day = tf.Day;
						}
					}

					ReleaseSRWLockExclusive(&_SRWLock);
				}

				AcquireSRWLockShared(&_SRWLock);

				if (hFile = _hFile) WriteFile(hFile, buf, tl + len, &ft.dwLowDateTime, 0);

				ReleaseSRWLockShared(&_SRWLock);
			}

			break;
		}

		buf = (PSTR)alloca(len + tl) + tl;
	}
}

CLogFile::~CLogFile()
{
	if (HANDLE hFile = _hFile)
	{
		NtClose(hFile);
	}
}

_NT_END