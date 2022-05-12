#include "stdafx.h"

_NT_BEGIN

#include "task.h"

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