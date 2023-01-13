void TestYY()
{
	PWSTR pc = GetCommandLineW();
	MessageBoxW(0, pc, 0, MB_ICONINFORMATION);
	HANDLE hDbgObj;

	if (*pc == '\n')
	{
		ULONG dwProcessId = wcstoul(pc + 1, &pc, 16);
		if (*pc == '\n')
		{
			hDbgObj = (HANDLE)(ULONG_PTR)wcstoul(pc + 1, &pc, 16);

			if (!*pc)
			{
				if (HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE|PROCESS_SUSPEND_RESUME, FALSE, dwProcessId))
				{
					if (0 <= ZwDuplicateObject(hProcess, hDbgObj, NtCurrentProcess(), &hDbgObj, 0, 0, DUPLICATE_SAME_ACCESS))
					{
						NtDebugActiveProcess(hProcess, hDbgObj);
						NtClose(hDbgObj);
					}
					NtClose(hProcess);
				}
			}
		}
	}
	else
	{
		if (0 <= NtCreateDebugObject(&hDbgObj, DEBUG_ALL_ACCESS, 0, 0))
		{
			WCHAR sz[MAX_PATH], cmd[64];
			GetModuleFileNameW(0, sz, _countof(sz));
			if (GetLastError() == NOERROR)
			{
				if (0 < swprintf_s(cmd, _countof(cmd), L"\n%x\n%x", GetCurrentProcessId(), (ULONG)(ULONG_PTR)hDbgObj))
				{
					STARTUPINFO si = { sizeof(si) };
					PROCESS_INFORMATION pi;
					if (CreateProcessW(sz, cmd, 0, 0, 0, 0, 0, 0, &si, &pi))
					{
						NtClose(pi.hThread);
						NtClose(pi.hProcess);

						DbgUiSetThreadDebugObject(hDbgObj);
						DBGUI_WAIT_STATE_CHANGE StateChange;
						DbgUiWaitStateChange(&StateChange, 0);
					}
				}
			}
			NtClose(hDbgObj);
		}
	}

	MessageBoxW(0, pc, 0, MB_ICONWARNING);
	ExitProcess(0);
}
