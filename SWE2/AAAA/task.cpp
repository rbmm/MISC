#include "stdafx.h"

_NT_BEGIN

#include "internal.h"

struct IOPort : public IExecTask
{
	HWINEVENTHOOK _hWinEventHook = 0;
	HANDLE _CompletionPort = 0;
	LONG _dwRefCount = 1;

	~IOPort()
	{
		if (HANDLE CompletionPort = _CompletionPort) NtClose(CompletionPort);
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if (riid == __uuidof(IUnknown))
		{
			AddRef();
			*ppvObject = static_cast<IExecTask*>(this);
			return S_OK;
		}

		*ppvObject = 0;
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( )
	{
		return InterlockedIncrementNoFence(&_dwRefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release( )
	{
		ULONG dwRefCount = InterlockedDecrement(&_dwRefCount);
		if (!dwRefCount) delete this;
		return dwRefCount;
	}

	static ULONG WINAPI PortThread(PVOID This)
	{
		union {
			ULONG_PTR CompletionKey;
			ITask* pTask;
		};

		ULONG NumberOfBytesTransferred;
		OVERLAPPED* lpOverlapped;

		HANDLE CompletionPort = reinterpret_cast<IOPort*>(This)->_CompletionPort;

		while (GetQueuedCompletionStatus(CompletionPort, &NumberOfBytesTransferred, 
			&CompletionKey, &lpOverlapped, INFINITE) && CompletionKey )
		{
			pTask->OnIoCompletion(lpOverlapped, NumberOfBytesTransferred);
		}

		reinterpret_cast<IOPort*>(This)->Release();

		FreeLibraryAndExitThread((HMODULE)&__ImageBase, 0);
	}

	virtual HRESULT STDMETHODCALLTYPE Start()
	{
		if (HWINEVENTHOOK hWinEventHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, 
			0, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT| WINEVENT_SKIPOWNPROCESS  ))
		{
			if (HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1))
			{
				_CompletionPort = CompletionPort;

				AddRef();
				LdrAddRefDll(0, (HMODULE)&__ImageBase);

				if (HANDLE hThread = CreateThread(0, 0, PortThread, this, 0, 0))
				{
					NtClose(hThread);

					_hWinEventHook = hWinEventHook;

					return S_OK;
				}

				LdrUnloadDll((HMODULE)&__ImageBase);
				Release();
			}

			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

			UnhookWinEvent(hWinEventHook);

			return hr;
		}

		return HRESULT_FROM_WIN32(GetLastError());
	}

	virtual HRESULT STDMETHODCALLTYPE Stop()
	{
		CleanupWinEvents(_hWinEventHook);
		return PostQueuedCompletionStatus(_CompletionPort, 0, 0, 0) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	}

	virtual HRESULT STDMETHODCALLTYPE Exec(_In_opt_ PCWSTR lpApplicationName, 
		_In_opt_ PCWSTR lpCommandLine, 
		_In_opt_ PCWSTR lpCurrentDirectory,
		_Out_ PPROCESS_INFORMATION lpProcessInformation)
	{
		STARTUPINFOW si = { sizeof(si) };
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESHOWWINDOW;

		return GetLastHr(CreateProcessW(lpApplicationName, const_cast<PWSTR>(lpCommandLine), 
			0, 0, 0, CREATE_SUSPENDED, 0, lpCurrentDirectory, &si, lpProcessInformation));
	}

	virtual HRESULT STDMETHODCALLTYPE EmbedTask(_In_ HWND hwnd, _In_ PPROCESS_INFORMATION lpProcessInformation, _Out_ ITask** ppTask)
	{
		HRESULT hr = E_OUTOFMEMORY;

		if (ITask* pTask = new ITask(hwnd))
		{
			if (0 > (hr = pTask->Create(_CompletionPort, lpProcessInformation)) ||
				0 > (hr = AddParentInfo(hwnd, lpProcessInformation)))
			{
				pTask->Release();
			}
			else
			{
				*ppTask = pTask;
			}
		}

		if (0 > hr)
		{
			TerminateProcess(lpProcessInformation->hProcess, 0);
		}

		return hr;
	}

	virtual void STDMETHODCALLTYPE Cleanup(_In_ PPROCESS_INFORMATION lpProcessInformation)
	{
		ZwResumeThread(lpProcessInformation->hThread, 0);
		NtClose(lpProcessInformation->hThread);
		NtClose(lpProcessInformation->hProcess);
	}
};

HRESULT CreateTaskMngr(_Out_ IExecTask** ppExec)
{
	if (IExecTask* pTaskMgr = new IOPort)
	{
		*ppExec = pTaskMgr;
		return S_OK;
	}

	*ppExec = 0;
	return E_OUTOFMEMORY;
}

_NT_END