#include "pch.h"

#pragma warning( disable : 4200 4706)

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

EXTERN_C
DECLSPEC_IMPORT
ULONG
__cdecl
DbgPrint(
		 _In_z_ _Printf_format_string_ PCSTR Format,
		 ...
		 );

EXTERN_C
DECLSPEC_IMPORT
ULONG
NTAPI
RtlNtStatusToDosError(
					  NTSTATUS Status
					  );

inline ULONG BOOL_TO_ERROR(BOOL f)
{
	return f ? NOERROR : GetLastError();
}

struct USE_LOCK
{
	HANDLE hRemoveEvent = 0;
	LONG dwLockCount = 1;
	BOOLEAN Removed = FALSE;

	~USE_LOCK()
	{
		if (hRemoveEvent)
		{
			CloseHandle(hRemoveEvent);
		}
	}

	[[nodiscard]] BOOLEAN Lock()
	{
		InterlockedIncrementNoFence(&dwLockCount);
		if (!Removed)
		{
			return TRUE;
		}

		Unlock();

		return FALSE;
	}

	void Unlock()
	{
		if (!InterlockedDecrementNoFence(&dwLockCount))
		{
			SetEvent(hRemoveEvent);
		}
	}

	void RemoveLockAndWait()
	{
		Removed = TRUE;

		if (InterlockedDecrementNoFence(&dwLockCount))
		{
			WaitForSingleObject(hRemoveEvent, INFINITE);
		}
	}

	ULONG Create()
	{
		return (hRemoveEvent = CreateEvent(0, TRUE, FALSE, 0)) ? NOERROR : GetLastError();
	}
};

class __declspec(novtable) CIoObj : public USE_LOCK
{
	friend class IOSB;

	HANDLE _hFile = 0;
	LONG _dwRefCount = 1;
protected:

	virtual ~CIoObj()
	{
		if (_hFile) CloseHandle(_hFile);
	}

	virtual void IoCompleted(ULONG op, PVOID ctx, ULONG dwErrorCode, ULONG dwNumberOfBytesTransfered) = 0;

public:

	[[nodiscard]] BOOLEAN Lock(HANDLE& hFile)
	{
		hFile = _hFile;
		return USE_LOCK::Lock();
	}

	void Close()
	{
		RemoveLockAndWait();
		CloseHandle(_hFile), _hFile = 0;
	}

	void AddRef()
	{
		InterlockedIncrementNoFence(&_dwRefCount);
	}

	void Release()
	{
		if (!InterlockedDecrement(&_dwRefCount))
		{
			delete this;
		}
	}

	ULONG Assign(HANDLE hFile);
};

class IOSB : public OVERLAPPED
{
	CIoObj* pObj;
	PVOID ctx;
	ULONG op;

	VOID WINAPI IoCompleted(ULONG dwErrorCode, ULONG dwNumberOfBytesTransfered)
	{
		pObj->IoCompleted(op, ctx, dwErrorCode, dwNumberOfBytesTransfered);
		pObj->Release();
		delete this;
	}

	static VOID WINAPI s_IoCompleted(ULONG status, ULONG dwNumberOfBytesTransfered, OVERLAPPED* lpOverlapped)
	{
		static_cast<IOSB*>(lpOverlapped)->IoCompleted(RtlNtStatusToDosError(status), dwNumberOfBytesTransfered);
	}

public:

	IOSB(CIoObj* pObj, PVOID ctx, ULONG op) : pObj(pObj), ctx(ctx), op(op)
	{
		RtlZeroMemory(static_cast<OVERLAPPED*>(this), sizeof(OVERLAPPED));
		pObj->AddRef();
	}

	void CheckIoCompleted(BOOL fOk)
	{
		CheckIoCompleted(fOk ? NOERROR : GetLastError());
	}

	void CheckIoCompleted(ULONG dwErrorCode)
	{
		switch (dwErrorCode)
		{
		case NOERROR:
		case ERROR_IO_PENDING:
			return;
		}

		IoCompleted(dwErrorCode, 0);
	}

	static ULONG BindIoCompletion(HANDLE hFile)
	{
		return BindIoCompletionCallback(hFile, s_IoCompleted, 0) ? NOERROR : GetLastError();
	}
};

ULONG CIoObj::Assign(HANDLE hFile)
{
	ULONG dwError = IOSB::BindIoCompletion(hFile);

	if (dwError == NOERROR)
	{
		dwError = USE_LOCK::Create();

		if (dwError == NOERROR)
		{
			_hFile = hFile;
		}
	}

	return NOERROR;
}

class CPipe : public CIoObj
{
	enum { opRead, opWrite };
	ULONG _bufSize;
	CHAR _buf[];

	void Read()
	{
		HANDLE hFile;
		if (Lock(hFile))
		{
			if (IOSB* irp = new IOSB(this, _buf, opRead))
			{
				irp->CheckIoCompleted(ReadFile(hFile, _buf, _bufSize, 0, irp));
			}
			Unlock();
		}
	}

	void Dump(PSTR buf, ULONG dwNumberOfBytesTransfered)
	{
		ULONG cchWideChar = 0, cch;
		PWSTR lpWideCharStr = 0;
		while (cchWideChar = MultiByteToWideChar(CP_OEMCP, 0, buf, dwNumberOfBytesTransfered, lpWideCharStr, cchWideChar))
		{
			if (lpWideCharStr)
			{
				do
				{
					cch = min(0x100, cchWideChar);

					DbgPrint("%.*S", cchWideChar, lpWideCharStr);

				} while (lpWideCharStr += cch, cchWideChar -= cch);
				break;
			}
			lpWideCharStr = (PWSTR)alloca(cchWideChar * sizeof(WCHAR));
		}
	}

	virtual void IoCompleted(ULONG op, PVOID ctx, ULONG dwErrorCode, ULONG dwNumberOfBytesTransfered)
	{
		if (dwErrorCode)
		{
			PWSTR buf;
			if (FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				0, dwErrorCode, 0, (PWSTR)&buf, 0, 0))
			{
				DbgPrint("IO(\r\n%x %S)\n", op, buf);
				LocalFree(buf);
			}
		}

		switch (op)
		{
		default:
			__debugbreak();
		case opWrite:
			delete[] ctx;
			return;
		case opRead:
			if (!dwErrorCode)
			{
				if (dwNumberOfBytesTransfered)
				{
					Dump((PSTR)ctx, dwNumberOfBytesTransfered);
				}
				Read();
			}
		}
	}

	CPipe(ULONG cb) : _bufSize(cb)
	{
	}

public:

	CPipe() {}

	void* operator new(size_t, PVOID buf)
	{
		return buf;
	}

	void* operator new(size_t s, ULONG cb)
	{
		return new(LocalAlloc(0, s + FIELD_OFFSET(CPipe, _buf[cb]) - sizeof(CPipe))) CPipe(cb);
	}

	void operator delete(void* pv)
	{
		LocalFree(pv);
	}

	ULONG RunCmdFrom(PCWSTR lpApplicationName, HANDLE hProcess);
	ULONG RunCmdFrom(ULONG dwProcessId);
	ULONG Create(HANDLE hProcess, LPHANDLE lpTargetHandle);

	void Cmd(PCSTR fmt, ...)
	{
		va_list arg;
		va_start(arg, fmt);

		PSTR buf = 0;
		int cch = 0;

		while (0 < (cch = _vsnprintf(buf, cch, fmt, arg)))
		{
			if (buf)
			{
				HANDLE hFile;
				if (Lock(hFile))
				{
					if (IOSB* irp = new IOSB(this, buf, opWrite))
					{
						irp->CheckIoCompleted(WriteFile(hFile, buf, cch, 0, irp));
					}
					Unlock();
				}

				break;
			}

			if (!(buf = new CHAR[cch]))
			{
				break;
			}
		}

		va_end(arg);
	}
};

ULONG CPipe::Create(HANDLE hProcess, LPHANDLE lpTargetHandle)
{
	static const WCHAR Name[] = L"\\\\?\\pipe\\MyUniqueName";

	HANDLE hPipe = CreateNamedPipeW(Name,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, 0);

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		ULONG dwError = Assign(hPipe);

		if (dwError == NOERROR)
		{
			hPipe = CreateFileW(Name, FILE_GENERIC_READ | FILE_GENERIC_WRITE,
				0, 0, OPEN_EXISTING, 0, 0);

			if (hPipe != INVALID_HANDLE_VALUE)
			{
				return BOOL_TO_ERROR(DuplicateHandle(GetCurrentProcess(), hPipe,
					hProcess, lpTargetHandle, 0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE));
			}

			return GetLastError();
		}

		CloseHandle(hPipe);
	}

	return GetLastError();
}

ULONG CPipe::RunCmdFrom(PCWSTR lpApplicationName, HANDLE hProcess)
{
	PROCESS_INFORMATION pi;
	STARTUPINFOEXW si = { { sizeof(si)} };

	SIZE_T s = 0;
__loop:
	switch (ULONG dwError = BOOL_TO_ERROR(InitializeProcThreadAttributeList(si.lpAttributeList, 2, 0, &s)))
	{
	case NOERROR:
		if (si.lpAttributeList)
		{
			if (NOERROR == (dwError = BOOL_TO_ERROR(UpdateProcThreadAttribute(si.lpAttributeList,
				0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hProcess, sizeof(HANDLE), 0, 0))) &&
				NOERROR == (dwError = BOOL_TO_ERROR(UpdateProcThreadAttribute(si.lpAttributeList,
				0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &si.StartupInfo.hStdError, sizeof(HANDLE), 0, 0))) &&
				NOERROR == (dwError = Create(hProcess, &si.StartupInfo.hStdError)))
			{
				si.StartupInfo.hStdInput = si.StartupInfo.hStdError;
				si.StartupInfo.hStdOutput = si.StartupInfo.hStdError;
				si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;

				dwError = BOOL_TO_ERROR(CreateProcessW(lpApplicationName, 0, 0, 0, TRUE,
					CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, 0, 0, &si.StartupInfo, &pi));

				DuplicateHandle(hProcess, si.StartupInfo.hStdError, 0, 0, 0, 0, DUPLICATE_CLOSE_SOURCE);

				if (dwError == NOERROR)
				{
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);

					Read();
				}
			}

			return dwError;
		}

		return ERROR_GEN_FAILURE;

	case ERROR_INSUFFICIENT_BUFFER:
		if (si.lpAttributeList)
		{
	default:
		return dwError;
		}
		si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)alloca(s);
		goto __loop;
	}
}

ULONG CPipe::RunCmdFrom(ULONG dwProcessId)
{
	WCHAR cmd[MAX_PATH];

	ULONG dwError = BOOL_TO_ERROR(GetEnvironmentVariableW(L"ComSpec", cmd, _countof(cmd)));

	if (dwError == NOERROR)
	{
		if (HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS, FALSE, dwProcessId))
		{
			dwError = RunCmdFrom(cmd, hProcess);

			CloseHandle(hProcess);

			return dwError;
		}
	}

	return GetLastError();
}

void Demo(ULONG dwProcessId)
{
	if (CPipe* pipe = new(0x1000) CPipe)
	{
		ULONG dwError = pipe->RunCmdFrom(dwProcessId);

		if (dwError == NOERROR)
		{
			static const PCSTR cmds[] = {
				"exit\r\n", "dir\r\n", "help\r\n", "ver\r\n"
			};

			ULONG i = _countof(cmds);

			do
			{
				PCSTR cmd = cmds[--i];
				pipe->Cmd(cmd);

				MessageBoxA(0, 0, cmd, MB_ICONINFORMATION);

			} while (i);

			pipe->Close();
		}

		pipe->Release();
	}
}