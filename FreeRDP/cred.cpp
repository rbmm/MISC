#include "stdafx.h"

_NT_BEGIN

PWSTR* WINAPI hook_CommandLineToArgvW(_In_ PCWSTR lpCmdLine, _Out_ int* pNumArgs)
{
	int argc;

	if (PWSTR* args = CommandLineToArgvW(lpCmdLine, &argc))
	{
		PWSTR* buf = args;
		*pNumArgs = argc;

		if (argc)
		{
			do 
			{
				PWSTR arg = *args++;

				if (*arg++ == '/' && *arg++ == 'v' && *arg++ == ':')
				{
					PWSTR TargetName = 0;
					int cch = 0;
					while (0 < (cch = _vsnwprintf(TargetName, cch, L"TERMSRV/%s", (va_list)&arg)))
					{
						if (TargetName)
						{
							if (PWSTR psz = wcschr(TargetName, ':'))
							{
								*psz = 0;
							}

							PCREDENTIALW Credential;

							if (CredReadW(TargetName, CRED_TYPE_GENERIC, 0, &Credential))
							{
								if (PWSTR UserName = Credential->UserName)
								{
									if ((arg = (PWSTR)Credential->CredentialBlob) &&
										(cch = Credential->CredentialBlobSize) && 
										!(cch & (sizeof(WCHAR) - 1)) &&
										!arg[cch - 1]
									)
									{
										PWSTR aa[] = { GetCommandLineW(), UserName, arg };
										cch = 0;
										TargetName = 0;

										while (0 < (cch = _vsnwprintf(TargetName, cch, L"%s \"/u:%s\" \"/p:%s\"", (va_list)aa)))
										{
											if (TargetName)
											{
												CredFree(Credential);
												LocalFree(buf);
												return CommandLineToArgvW(TargetName, pNumArgs);
											}

											TargetName = (PWSTR)alloca(++cch * sizeof(WCHAR));
										}
									}
								}

								CredFree(Credential);
							}

							break;
						}

						TargetName = (PWSTR)alloca(++cch * sizeof(WCHAR));
					}

					break;
				}

			} while (--argc);
		}

		return buf;
	}

	*pNumArgs = 0;
	return 0;
}

EXTERN_C void** __fastcall findCTA(ULONG size, void** ppv);

BOOLEAN DoHook()
{
	ULONG size;
	if (void** ppv = (void**)RtlImageDirectoryEntryToData(GetModuleHandleW(0), TRUE, IMAGE_DIRECTORY_ENTRY_IAT, &size))
	{
		if (ppv = findCTA(size, ppv))
		{
			if (VirtualProtect(ppv, sizeof(PVOID), PAGE_READWRITE, &size))
			{
				*ppv = hook_CommandLineToArgvW;
				if (size != PAGE_READWRITE) VirtualProtect(ppv, sizeof(PVOID), size, &size);
				return TRUE;
			}
		}
	}

	return FALSE;
}

BOOLEAN APIENTRY DllMain( HMODULE hModule,
						 DWORD  ul_reason_for_call,
						 PVOID 
						 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		LdrDisableThreadCalloutsForDll(hModule);
		return DoHook();
	}
	return FALSE;
}
_NT_END