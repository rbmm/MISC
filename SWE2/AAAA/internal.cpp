#include "stdafx.h"

_NT_BEGIN

#include "internal.h"

bool ITask::OnIoCompletion(OVERLAPPED* dwProcessId, ULONG MessageId)
{
	DbgPrint("%s<%p>(%x %p)\n", __FUNCTION__, this, MessageId, dwProcessId);
	switch (MessageId)
	{
	case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
		DbgPrint("%p - ACTIVE_PROCESS_ZERO\n", dwProcessId);
		PostMessageW(_hwnd, WM_CLOSE, JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO, 0);
		Release();
		break;
	case JOB_OBJECT_MSG_NEW_PROCESS:
		DbgPrint("%p - NEW_PROCESS\n", dwProcessId);
		break;
	case JOB_OBJECT_MSG_EXIT_PROCESS:
		DbgPrint("%p - EXIT_PROCESS\n", dwProcessId);
		break;
	case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
		DbgPrint("%p - ABNORMAL_EXIT_PROCESS\n", dwProcessId);
		break;
	}
	return true;
}

HRESULT ITask::Create(_In_ HANDLE CompletionPort, _In_ PPROCESS_INFORMATION lpProcessInformation)
{
	if (HANDLE hJob = CreateJobObjectW(0, 0))
	{
		JOBOBJECT_ASSOCIATE_COMPLETION_PORT jacp = { this, CompletionPort };
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jbli;
		jbli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

		AddRef();

		HANDLE hProcess = lpProcessInformation->hProcess;
		if (SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &jacp, sizeof(jacp)) &&
			SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jbli, sizeof(jbli)) &&
			AssignProcessToJobObject(hJob, hProcess))
		{
			_hJob = hJob;
			return S_OK;
		}

		Release();

		NtClose(hJob);
	}

	return HRESULT_FROM_WIN32(GetLastError());
}

//////////////////////////////////////////////////////////////////////////

LIST_ENTRY gList = { &gList, &gList };

ULONG GetSecondsCount()
{
	return (ULONG)(GetTickCount64() / 1000);
}

struct PARENT_INFO : LIST_ENTRY
{
	HWND hwndParent;
	ULONG dwThreadId;
	ULONG dwProcessId;
	ULONG time;

	PARENT_INFO(HWND hwndParent, ULONG dwThreadId, ULONG dwProcessId) : hwndParent(hwndParent), dwThreadId(dwThreadId), dwProcessId(dwProcessId)
	{
		InsertHeadList(&gList, this);
		time = GetSecondsCount() + 8;
	}

	~PARENT_INFO()
	{
		RemoveEntryList(this);
		DbgPrint("%s<%p>(%x)\n", __FUNCTION__, this, dwProcessId);
	}
};

void CleanupWinEvents(HWINEVENTHOOK hWinEventHook)
{
	if (hWinEventHook)
	{
		UnhookWinEvent(hWinEventHook);
	}

	PLIST_ENTRY entry = gList.Flink;

	while (entry != &gList)
	{
		PARENT_INFO* pi = static_cast<PARENT_INFO*>(entry);

		entry = entry->Flink;

		delete pi;
	}
}

HRESULT AddParentInfo(_In_ HWND hwnd, _In_ PPROCESS_INFORMATION lpProcessInformation)
{
	if (PARENT_INFO* p = new PARENT_INFO(hwnd, lpProcessInformation->dwThreadId, lpProcessInformation->dwProcessId))
	{
		return S_OK;
	}

	return E_OUTOFMEMORY;
}

void OnCreateWnd(HWND hwnd, DWORD dwEventThread)
{
	WCHAR clsn[128];
	PLIST_ENTRY entry = gList.Flink;

	//++ kill too old entry
	while (entry != &gList)
	{
		PARENT_INFO* pi = static_cast<PARENT_INFO*>(entry);
		entry = entry->Flink;

		if (pi->time < GetSecondsCount() )
		{
			delete pi;
		}
	}
	//++ kill too old entry

	while ((entry = entry->Flink) != &gList)
	{
		if (static_cast<PARENT_INFO*>(entry)->dwThreadId == dwEventThread)
		{
			if (!GetClassNameW(hwnd, clsn, _countof(clsn)))
			{
				swprintf_s(clsn, _countof(clsn), L"!! %u", GetLastError());
			}

			DbgPrint("++%p:%p(%p) %x.%x %S\n", hwnd, GetParent(hwnd), 
				GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT),
				GetWindowLongW(hwnd, GWL_STYLE), GetWindowLongW(hwnd, GWL_EXSTYLE), clsn);

			if (IsWindow(hwnd) &&
				!(GetWindowLongW(hwnd, GWL_STYLE) & (WS_CHILD|WS_POPUP)) &&
				!(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) &&
				!GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT) &&
				!GetParent(hwnd))
			{
__0:
				PostMessageW(static_cast<PARENT_INFO*>(entry)->hwndParent, WM_NULL, aRedirect, (LPARAM)hwnd);

				delete static_cast<PARENT_INFO*>(entry);

				return;
			}
		}
	}

	if (GetClassNameW(hwnd, clsn, _countof(clsn)) && !wcscmp(clsn, L"ConsoleWindowClass"))
	{
		ULONG dwProcessId;
		if (GetWindowThreadProcessId(hwnd, &dwProcessId))
		{
			entry = &gList;

			while ((entry = entry->Flink) != &gList)
			{
				if (static_cast<PARENT_INFO*>(entry)->dwProcessId == dwProcessId)
				{
					goto __0;
				}
			}

			if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, dwProcessId))
			{
				PROCESS_BASIC_INFORMATION pbi;
				NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), 0);
				NtClose(hProcess);

				if (0 <= status)
				{
					entry = &gList;

					while ((entry = entry->Flink) != &gList)
					{
						if (static_cast<PARENT_INFO*>(entry)->dwProcessId == (ULONG)pbi.InheritedFromUniqueProcessId)
						{
							goto __0;
						}
					}
				}
			}
		}
	}
}

VOID CALLBACK WinEventProc(
						   HWINEVENTHOOK /*hWinEventHook*/,
						   DWORD event,
						   HWND hwnd,
						   LONG idObject,
						   LONG idChild,
						   DWORD dwEventThread,
						   DWORD /*dwmsEventTime*/
						   )
{
	if (hwnd && event == EVENT_OBJECT_CREATE && idObject == OBJID_WINDOW && idChild == CHILDID_SELF )
	{
		OnCreateWnd(hwnd, dwEventThread);
	}
}

_NT_END