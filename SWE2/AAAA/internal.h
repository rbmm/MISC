#pragma once

enum { aRedirect = 0x782303DE };

struct ITask
{
	HWND _hwnd;
	HANDLE _hJob = 0;
	LONG _dwRefCount = 1;

	ITask(HWND hwnd) : _hwnd(hwnd) { }

	~ITask()
	{
		if (HANDLE hJob = _hJob) NtClose(hJob);
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
	}

	void AddRef()
	{
		InterlockedIncrementNoFence(&_dwRefCount);
	}

	void Release()
	{
		if (!InterlockedDecrement(&_dwRefCount)) delete this;
	}

	ULONG Close()
	{
		return BOOL_TO_ERROR(TerminateJobObject(_hJob, 0));
	}
	
	bool OnIoCompletion(OVERLAPPED* lpOverlapped, ULONG NumberOfBytesTransferred);

	HRESULT Create(_In_ HANDLE CompletionPort, _In_ PPROCESS_INFORMATION lpProcessInformation);
};

struct __declspec(novtable) IExecTask : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Start() = 0;

	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;

	virtual HRESULT STDMETHODCALLTYPE Exec(_In_opt_ PCWSTR lpApplicationName, 
		_In_opt_ PCWSTR lpCommandLine, 
		_In_opt_ PCWSTR lpCurrentDirectory,
		_Out_ PPROCESS_INFORMATION lpProcessInformation) = 0;

	virtual HRESULT STDMETHODCALLTYPE EmbedTask(_In_opt_ HWND hwnd, 
		_In_ PPROCESS_INFORMATION lpProcessInformation,
		_Out_ ITask** ppTask) = 0;

	virtual void STDMETHODCALLTYPE Cleanup(_In_ PPROCESS_INFORMATION lpProcessInformation) = 0; 
};

HRESULT CreateTaskMngr(_Out_ IExecTask** ppExec);

void CleanupWinEvents(HWINEVENTHOOK hWinEventHook);

HRESULT AddParentInfo(_In_ HWND hwnd, _In_ PPROCESS_INFORMATION lpProcessInformation);

VOID CALLBACK WinEventProc(
						   HWINEVENTHOOK /*hWinEventHook*/,
						   DWORD event,
						   HWND hwnd,
						   LONG idObject,
						   LONG idChild,
						   DWORD dwEventThread,
						   DWORD /*dwmsEventTime*/
						   );
