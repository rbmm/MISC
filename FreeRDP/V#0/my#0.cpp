#include <Windows.h>
#include <wincred.h>
#include <shellapi.h>
#include <stdio.h>
#include <malloc.h>

#pragma warning(disable : 4706) // assignment within conditional expression

namespace {

	enum { fNameNeed, fPasswordNeed };

	PCWSTR GetServerName(_In_ PWSTR* args, _In_ ULONG NumArgs, _Inout_ PLONG bits)
	{
		PCWSTR ServerName = 0;

		if (NumArgs)
		{
			do 
			{
				PCWSTR arg = *args++;

				switch (*arg++)
				{
				case '/':
				case '-':
					switch (*arg++)
					{
					case 'v': // Server hostname
					case 'g': // Gateway Hostname ?
						if (*arg == ':')
						{
							ServerName = arg + 1;
						}
						break;
					case 'p':
						if (*arg == ':')
						{
							_bittestandreset(bits, fPasswordNeed);
						}
						break;
					case 'u':
						if (*arg == ':')
						{
							_bittestandreset(bits, fNameNeed);
						}
						break;
					}
					break;
				}

			} while (--NumArgs);
		}

		return ServerName;
	}

	PCWSTR ValidateString(const BYTE* pb, ULONG cb)
	{
		return pb &&							// present ?
			cb &&								// size not 0 ?
			!(cb & (sizeof(WCHAR) - 1)) &&		// cb == n * sizeof(WCHAR) ?
			!*(WCHAR*)(pb + cb - sizeof(WCHAR))	// 0 terminated ?
			? (PCWSTR)pb : 0;					
	}
}

EXTERN_C PWSTR* WINAPI hook_CommandLineToArgvW(_In_ PCWSTR lpCmdLine, _Out_ int* pNumArgs)
{
	int argc;
	PWSTR* args = CommandLineToArgvW(lpCmdLine, &argc);

	if (args)
	{
		LONG mask = (1 << fNameNeed) | (1 << fPasswordNeed);
		PCWSTR ServerName = GetServerName(args, argc, &mask);

		if (ServerName && mask)
		{
			PWSTR TargetName = 0;
			int cch = 0;
			while (0 < (cch = _vsnwprintf(TargetName, cch, L"TERMSRV/%s", (va_list)&ServerName)))
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
						PCWSTR UserName, Password;

						SIZE_T cbExtra = 0, cbUserName = 0, cbPassword = 0, uBytes, PointersSize = argc * sizeof(PVOID);
						ULONG n = 0;

						if (UserName = _bittest(&mask, fNameNeed) ? Credential->UserName : 0)
						{
							cbExtra += sizeof(PVOID) + sizeof(L"/u") + (cbUserName = (ULONG)(1 + wcslen(UserName)) * sizeof(WCHAR));
							n++;
						}

						if (Password = _bittest(&mask, fPasswordNeed) ? 
							ValidateString(Credential->CredentialBlob, Credential->CredentialBlobSize) : 0)
						{
							cbExtra += sizeof(PVOID) + sizeof(L"/p") + (cbPassword = (ULONG)(1 + wcslen(Password)) * sizeof(WCHAR));
							n++;
						}

						if (cbExtra && (uBytes = LocalSize(args)) > PointersSize)
						{
							if (PVOID buf = LocalAlloc(0, cbExtra + uBytes))
							{
								PWSTR pszOldBase = (PWSTR)(args + argc);

								PWSTR pszNewBase = (PWSTR)((ULONG_PTR)buf + PointersSize + cbExtra);

								memcpy(pszNewBase, pszOldBase, uBytes -= PointersSize);

								ULONG_PTR Delta = (ULONG_PTR)pszNewBase - (ULONG_PTR)pszOldBase;

								PWSTR* ppsz = (PWSTR*)buf;
								PULONG_PTR pu = (PULONG_PTR)args;

								ULONG m = argc;
								do 
								{
									*ppsz++ = (PWSTR)(Delta + *pu++);

								} while (--m);

								pszNewBase = (PWSTR)(ppsz + n);

								if (UserName)
								{
									*ppsz++ = pszNewBase;
									*pszNewBase++ = '/';
									*pszNewBase++ = 'u';
									*pszNewBase++ = ':';
									memcpy(pszNewBase, UserName, cbUserName);
									(ULONG_PTR&)pszNewBase += cbUserName;
								}

								if (Password)
								{
									*ppsz++ = pszNewBase;
									*pszNewBase++ = '/';
									*pszNewBase++ = 'p';
									*pszNewBase++ = ':';
									memcpy(pszNewBase, Password, cbPassword);
								}

								LocalFree(args);
								args = (PWSTR*)buf;
								argc += n;
							}
						}

						CredFree(Credential);
					}

					break;
				}

				TargetName = (PWSTR)alloca(++cch * sizeof(WCHAR));
			}
		}
	}

	*pNumArgs = argc;
	return args;
}