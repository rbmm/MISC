#pragma once

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

void CleanupWinEvents(HWINEVENTHOOK hWinEventHook);

HRESULT AddParentInfo(_In_ HWND hwnd, _In_ PPROCESS_INFORMATION lpProcessInformation, _In_ ITask* pTask);

VOID CALLBACK WinEventProc(
						   HWINEVENTHOOK /*hWinEventHook*/,
						   DWORD event,
						   HWND hwnd,
						   LONG idObject,
						   LONG idChild,
						   DWORD dwEventThread,
						   DWORD /*dwmsEventTime*/
						   );
