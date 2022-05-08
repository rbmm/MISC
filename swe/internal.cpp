#include "stdafx.h"

_NT_BEGIN

#include "task.h"
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

#include "../inc/initterm.h"

BOOLEAN DllMain(_In_ HMODULE DllHandle, _In_ ULONG Reason, _In_ PVOID /*Reserved*/)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(DllHandle);
		initterm();
		break;
	case DLL_PROCESS_DETACH:
		destroyterm();
		break;
	}

	return TRUE;
}

_NT_END