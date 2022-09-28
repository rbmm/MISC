#include <Windows.h>
#include <wincred.h>
#include <stdio.h>
#include <malloc.h>

#if 0

#include <freerdp/settings.h>

#else

#define FreeRDP_ServerHostname (20)
#define FreeRDP_Username (21)
#define FreeRDP_Password (22)
#define FreeRDP_GatewayHostname (1986)
#define FreeRDP_GatewayUsername (1987)
#define FreeRDP_GatewayPassword (1988)

typedef struct rdp_settings rdpSettings;

const char* freerdp_settings_get_string(const rdpSettings* settings, size_t id);
BOOL freerdp_settings_set_string(rdpSettings* settings, size_t id, const char* param);

#endif

#pragma warning( disable : 4706 ) // assignment within conditional expression

#define malloca(size) ((size) < _ALLOCA_S_THRESHOLD ? _alloca(size) : LocalAlloc(0, size))

_CRTNOALIAS inline void freea(PVOID pv)
{
	if (pv)
	{
		PNT_TIB tib = (PNT_TIB)NtCurrentTeb();
		if (pv < tib->StackLimit || tib->StackBase <= pv) LocalFree(pv);
	}
}

BOOL freerdp_settings_set_string_W(rdpSettings* settings, size_t id, PCWSTR param)
{
	BOOL fOk = FALSE;
	PSTR lpMultiByteStr = 0;
	int cbMultiByte = 0;

	while (cbMultiByte = WideCharToMultiByte(CP_UTF8, 0, param, -1, lpMultiByteStr, cbMultiByte, 0, 0))
	{
		if (lpMultiByteStr)
		{
			fOk = freerdp_settings_set_string(settings, id, lpMultiByteStr);
			break;
		}

		if (!(lpMultiByteStr = (PSTR)malloca(cbMultiByte)))
		{
			break;
		}
	}

	freea(lpMultiByteStr);

	return fOk;
}

static PWSTR ValidateString(const BYTE* pb, ULONG cb)
{
	if (pb)
	{
		if (cb)
		{
			if (!(cb & (sizeof(WCHAR) - 1)))
			{
				if (!*(WCHAR*)(pb + cb - sizeof(WCHAR)))
				{
					return (PWSTR)pb;
				}
			}
		}
	}

	return 0;					
}

static void AddDefaultSettings_IT(rdpSettings* settings, PWSTR TargetName, size_t idUsername, size_t idPassword)
{
	BOOL bNoUserName = !freerdp_settings_get_string(settings, idUsername);
	BOOL bNoPassword = !freerdp_settings_get_string(settings, idPassword);

	if (bNoUserName || bNoPassword)
	{
		PCREDENTIALW Credential;

		if (CredReadW(TargetName, CRED_TYPE_GENERIC, 0, &Credential))
		{
			if (bNoPassword)
			{
				if (TargetName = ValidateString(Credential->CredentialBlob, Credential->CredentialBlobSize))
				{
					freerdp_settings_set_string_W(settings, idPassword, TargetName);
				}
			}

			if (bNoUserName)
			{
				if (TargetName = Credential->UserName)
				{
					freerdp_settings_set_string_W(settings, idUsername, TargetName);
				}
			}

			CredFree(Credential);
		}
	}
}

static const WCHAR TERMSRV[] = L"TERMSRV/";

static void AddDefaultSettings_I(rdpSettings* settings, size_t idHostname, size_t idUsername, size_t idPassword)
{
	PCSTR ServerHostname = freerdp_settings_get_string(settings, idHostname);

	if (ServerHostname)
	{
		PWSTR ServerHostnameW = 0;
		PWSTR TargetName = 0;
		ULONG len = 0;

		while (len = MultiByteToWideChar(CP_UTF8, 0, ServerHostname, -1, ServerHostnameW, len))
		{
			if (TargetName)
			{
				AddDefaultSettings_IT(settings, TargetName, idUsername, idPassword);
				break;
			}

			if (!(TargetName = malloca(sizeof(TERMSRV) - sizeof(WCHAR) + len * sizeof(WCHAR))))
			{
				break;
			}

			memcpy(TargetName, TERMSRV, sizeof(TERMSRV) - sizeof(WCHAR));

			ServerHostnameW = TargetName + _countof(TERMSRV) - 1;
		}

		freea(TargetName);
	}
}

void AddDefaultSettings(rdpSettings* settings)
{
	PWSTR TargetName = 0;
	ULONG cch = 0;

	while (cch = GetEnvironmentVariableW(L"FREERDP_CRED_TARGET_NAME", TargetName, cch))
	{
		if (TargetName)
		{
			if (memcmp(TargetName, TERMSRV, sizeof(TERMSRV) - sizeof(WCHAR)))
			{
				break;
			}

			AddDefaultSettings_IT(settings, TargetName, FreeRDP_Username, FreeRDP_Password);

			goto exit;
		}

		if (!(TargetName = (PWSTR)malloca(cch * sizeof(WCHAR))))
		{
			break;
		}
	}

	AddDefaultSettings_I(settings, FreeRDP_ServerHostname, FreeRDP_Username, FreeRDP_Password);
exit:
	freea(TargetName);
	AddDefaultSettings_I(settings, FreeRDP_GatewayHostname, FreeRDP_GatewayUsername, FreeRDP_GatewayPassword);
}