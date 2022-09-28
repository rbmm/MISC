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

BOOL freerdp_settings_set_string_W(rdpSettings* settings, size_t id, PCWSTR param)
{
	PSTR lpMultiByteStr = 0;
	int cbMultiByte = 0;

	while (cbMultiByte = WideCharToMultiByte(CP_UTF8, 0, param, -1, lpMultiByteStr, cbMultiByte, 0, 0))
	{
		if (lpMultiByteStr)
		{
			return freerdp_settings_set_string(settings, id, lpMultiByteStr);
		}

		lpMultiByteStr = (PSTR)alloca(cbMultiByte);
	}

	return FALSE;
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

static void AddDefaultSettings_I(rdpSettings* settings, size_t idHostname, size_t idUsername, size_t idPassword)
{
	PCSTR ServerHostname = freerdp_settings_get_string(settings, idHostname);

	if (!ServerHostname)
	{
		return ;
	}

	BOOL bExistUserName = 0 != freerdp_settings_get_string(settings, idUsername);
	BOOL bExistPassword = 0 != freerdp_settings_get_string(settings, idPassword);

	if (bExistUserName && bExistPassword)
	{
		return ;
	}

	PWSTR ServerHostnameW = 0;
	PWSTR TargetName = 0;
	ULONG len = 0;

	while (len = MultiByteToWideChar(CP_UTF8, 0, ServerHostname, -1, ServerHostnameW, len))
	{
		if (TargetName)
		{
			PCREDENTIALW Credential;

			if (CredReadW(TargetName, CRED_TYPE_GENERIC, 0, &Credential))
			{
				if (!bExistPassword)
				{
					ULONG cch = Credential->CredentialBlobSize;

					if (TargetName = ValidateString(Credential->CredentialBlob, cch))
					{
						freerdp_settings_set_string_W(settings, idPassword, TargetName);
					}
				}

				if (!bExistUserName)
				{
					if (TargetName = Credential->UserName)
					{
						freerdp_settings_set_string_W(settings, idUsername, TargetName);
					}
				}

				CredFree(Credential);
			}

			break;
		}

		static const WCHAR TERMSRV[] = L"TERMSRV/";

		TargetName = alloca(sizeof(TERMSRV) - sizeof(WCHAR) + len * sizeof(WCHAR));

		memcpy(TargetName, TERMSRV, sizeof(TERMSRV) - sizeof(WCHAR));

		ServerHostnameW = TargetName + _countof(TERMSRV) - 1;
	}
}

void WINAPI AddDefaultSettings(_Inout_ rdpSettings* settings)
{
	AddDefaultSettings_I(settings, FreeRDP_ServerHostname, FreeRDP_Username, FreeRDP_Password);
	AddDefaultSettings_I(settings, FreeRDP_GatewayHostname, FreeRDP_GatewayUsername, FreeRDP_GatewayPassword);
}