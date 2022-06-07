#include "stdafx.h"
#include <initguid.h>
#include <wincred.h >
#include <credentialprovider.h>
#include <Userenv.h>

MIDL_INTERFACE("9D5F2149-DE8C-45F2-B313-6587A04F771D") IConnectedUser : public IUnknown
{
	virtual STDMETHODIMP GetConnectedUserInfo(IPropertyStore ** ppProps);
	
	virtual STDMETHODIMP DeleteLocalUser();

	virtual STDMETHODIMP DisconnectLocalUser(
		_In_ PCWSTR pszLocalUsername, 
		_In_ PCWSTR pszProtectedPassword,	// from CredProtectW ( password for current user )
		_In_ PUCHAR pbPackedCredentials, 	// CredPackAuthenticationBufferW ( name + password for ConnectedUser)
		_In_ ULONG cbPackedCredentials		// CredPackAuthenticationBufferW
		);
};

MIDL_INTERFACE("9EC044BC-B01D-4C18-8634-59BD3FF5DCC1") IConnectedUserStore : public IUnknown 
{
	virtual STDMETHODIMP GetConnectedUserEnum(ULONG, LPCGUID ProviderId, IEnumUnknown **);

	virtual long CreateConnectedUser(
		_In_ PCWSTR pszLocalUsername,
		_In_ PCWSTR pszInternetUserName,	
		_In_ LPCGUID ProviderId,			// one from HKEY_LOCAL_MACHINE\Software\Microsoft\IdentityStore\Providers
		_In_ BOOL bAdminAccount,			// make user member of DOMAIN_ALIAS_RID_ADMINS
		_In_ PUCHAR pbPackedCredentials,	// CredPackAuthenticationBufferW ( name + password for ConnectedUser)
		_In_ ULONG cbPackedCredentials		// CredPackAuthenticationBufferW
		);
	
	// to current user (by token)
	virtual STDMETHODIMP ConnectLocalUser(
		_In_ PCWSTR pszProtectedPassword,	// from CredProtectW ( password for current user )
		_In_ LPCGUID ProviderId,			// one from HKEY_LOCAL_MACHINE\Software\Microsoft\IdentityStore\Providers
		_In_ PUCHAR pbPackedCredentials,	// CredPackAuthenticationBufferW ( name + password for ConnectedUser)
		_In_ ULONG cbPackedCredentials		// CredPackAuthenticationBufferW
		);

	virtual STDMETHODIMP FindConnectedUserByName(
		_In_ PCWSTR pszInternetUserName, 
		_In_opt_ PCWSTR pszDomain, // L"MicrosoftAccount", mutually exclusive with ProviderId
		_In_opt_ LPCGUID ProviderId, // &__uuidof(MicrosoftAccount), mutually exclusive with pszDomain
		_Out_ IConnectedUser ** ppUser);

	virtual STDMETHODIMP FindConnectedUserBySid(PSID UserSid, ULONG SidLength, IConnectedUser ** ppUser);
};

class DECLSPEC_UUID("40AFA0B6-3B2F-4654-8C3F-161DE85CF80E") ConnectedUserStore;
class DECLSPEC_UUID("B16898C6-A148-4967-9171-64D755DA8520") AzureAD;
class DECLSPEC_UUID("D7F9888F-E3FC-49b0-9EA6-A85B5F392A4F") MicrosoftAccount;

static const WCHAR MicrosoftDomain[] = L"MicrosoftAccount\\";

HRESULT ConnectUser(_In_ PCWSTR pszProtectedPassword, 
					_In_ PUCHAR pbPackedCredentials, 
					_In_ ULONG cbPackedCredentials)
{
	IConnectedUserStore* pUserStore;
	
	HRESULT hr = CoCreateInstance(__uuidof(ConnectedUserStore), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pUserStore));
	
	if (0 <= hr)
	{
		hr = pUserStore->ConnectLocalUser(pszProtectedPassword, &__uuidof(MicrosoftAccount), pbPackedCredentials, cbPackedCredentials);
		pUserStore->Release();
	}

	return hr;
}

HRESULT ConnectUser(_In_ PCWSTR lpszPassword, 
					_In_ PCWSTR pszInternetUserName, 
					_In_ PCWSTR pszConnectedPassword)
{
	CRED_PROTECTION_TYPE ProtectionType;
	ULONG cch = 1 + (ULONG)wcslen(lpszPassword);
	PWSTR pszProtectedPassword = 0;
	ULONG cchMaxChars = 0;

	HRESULT hr;

__loop_1:

	switch (hr = BOOL_TO_ERROR(CredProtectW(FALSE, const_cast<PWSTR>(lpszPassword), 
		cch, pszProtectedPassword, &cchMaxChars, &ProtectionType)))
	{
	case ERROR_INSUFFICIENT_BUFFER:
		if (!pszProtectedPassword)
		{
			pszProtectedPassword = (PWSTR)alloca(cchMaxChars * sizeof(WCHAR));
			goto __loop_1;
		}
		break;

	case NOERROR:
		if (pszProtectedPassword)
		{
			size_t len = _countof(MicrosoftDomain) + wcslen(pszInternetUserName);
			PWSTR FullName = (PWSTR)alloca(len * sizeof(WCHAR));
			wcscat(wcscpy(FullName, MicrosoftDomain), pszInternetUserName);

			PUCHAR pb = 0;
			ULONG cb = 0;
__loop_2:
			switch (hr = BOOL_TO_ERROR(CredPackAuthenticationBufferW(
				CRED_PACK_ID_PROVIDER_CREDENTIALS, 
				FullName, 
				const_cast<PWSTR>(pszConnectedPassword), 
				pb, &cb)))
			{
			case ERROR_INSUFFICIENT_BUFFER:
				if (!pb)
				{
					pb = (PUCHAR)alloca(cb);
					goto __loop_2;
				}
				break;

			case NOERROR:
				if (pb)
				{
					hr = ConnectUser(pszProtectedPassword, pb, cb);
				}
				break;
			}
		}
		break;
	}

	return hr;
}

HRESULT ConnectUser(_In_ PCWSTR lpszUsername,
					_In_ PCWSTR lpszDomain,
					_In_ PCWSTR lpszPassword,
					_In_ PCWSTR pszInternetUserName, 
					_In_ PCWSTR pszConnectedPassword)
{
	HANDLE hToken;

	HRESULT hr = BOOL_TO_ERROR(LogonUserW(lpszUsername, lpszDomain, 
		lpszPassword, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken));

	if (NOERROR == hr)
	{
		PROFILEINFO ProfileInfo = { sizeof(PROFILEINFO), 0, const_cast<PWSTR>(lpszUsername) };

		if (NOERROR == (hr = BOOL_TO_ERROR(LoadUserProfileW(hToken, &ProfileInfo))))
		{
			if (NOERROR == (hr = BOOL_TO_ERROR(ImpersonateLoggedOnUser(hToken))))
			{
				hr = ConnectUser(lpszPassword, pszInternetUserName, pszConnectedPassword);

				SetThreadToken(0, 0);
			}

			UnloadUserProfile(hToken, ProfileInfo.hProfile);
		}
		
		NtClose(hToken);
	}

	return HRESULT_FROM_WIN32(hr);
}

//////////////////////////////////////////////////////////////////////////

HRESULT CreateConnectedUser(_In_ PCWSTR pszLocalUsername, 
							_In_ PCWSTR pszInternetUserName,
							_In_ BOOL bAdminAccount,
							_In_ PUCHAR pbPackedCredentials, 
							_In_ ULONG cbPackedCredentials)
{
	IConnectedUserStore* pUserStore;

	HRESULT hr = CoCreateInstance(__uuidof(ConnectedUserStore), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pUserStore));

	if (0 <= hr)
	{
		hr = pUserStore->CreateConnectedUser(pszLocalUsername, 
			pszInternetUserName, &__uuidof(MicrosoftAccount), bAdminAccount, pbPackedCredentials, cbPackedCredentials);
		
		pUserStore->Release();
	}

	return hr;
}

HRESULT CreateConnectedUser(_In_ PCWSTR pszLocalUsername, 
							_In_ BOOL bAdminAccount,
							_In_ PCWSTR pszInternetUserName, 
							_In_ PCWSTR pszConnectedPassword)
{
	size_t len = _countof(MicrosoftDomain) + wcslen(pszInternetUserName);
	PWSTR FullName = (PWSTR)alloca(len * sizeof(WCHAR));
	wcscat(wcscpy(FullName, MicrosoftDomain), pszInternetUserName);

	HRESULT hr;

	PUCHAR pb = 0;
	ULONG cb = 0;
__loop:
	switch (hr = BOOL_TO_ERROR(CredPackAuthenticationBufferW(
		CRED_PACK_ID_PROVIDER_CREDENTIALS, 
		FullName, 
		const_cast<PWSTR>(pszConnectedPassword), 
		pb, &cb)))
	{
	case ERROR_INSUFFICIENT_BUFFER:
		if (!pb)
		{
			pb = (PUCHAR)alloca(cb);
			goto __loop;
		}
		break;

	case NOERROR:
		if (pb)
		{
			hr = CreateConnectedUser(pszLocalUsername, pszInternetUserName, bAdminAccount, pb, cb);
		}
		break;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////

void DumpUser(_In_ IConnectedUser * pUser)
{
	IPropertyStore* pProp;

	if (0 <= pUser->GetConnectedUserInfo(&pProp))
	{
		// https://docs.microsoft.com/en-us/windows/win32/properties/identity-buffer
		ULONG n;
		if (0 <= pProp->GetCount(&n) && n)
		{
			do 
			{
				PROPERTYKEY key;
				if (0 <= pProp->GetAt(--n, &key))
				{
					PROPVARIANT v;
					if (0 <= pProp->GetValue(key, &v))
					{
						DbgPrint("\nformatID = %08X-%04X-%04X-%02X%02x-%02X%02X%02X%02X%02X%02X\npropID = %u\n", 
							key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
							key.fmtid.Data4[0], key.fmtid.Data4[1],key.fmtid.Data4[2], key.fmtid.Data4[3],
							key.fmtid.Data4[4], key.fmtid.Data4[5],key.fmtid.Data4[6], key.fmtid.Data4[7], key.pid);

						switch(v.vt)
						{
						case VT_BSTR:
						case VT_LPWSTR:
							DbgPrint("%S\n", v.bstrVal);
							break;

						case VT_I4:
						case VT_UI4:
						case VT_UINT:
						case VT_INT:
							DbgPrint("%x\n", v.uintVal);
							break;

						case VT_CLSID:
							DbgPrint("{%08X-%04X-%04X-%02X%02x-%02X%02X%02X%02X%02X%02X}\n", 
								v.puuid->Data1, v.puuid->Data2, v.puuid->Data3,
								v.puuid->Data4[0], v.puuid->Data4[1],v.puuid->Data4[2], v.puuid->Data4[3],
								v.puuid->Data4[4], v.puuid->Data4[5],v.puuid->Data4[6], v.puuid->Data4[7]);
							break;

						default: 
							DbgPrint("[%x] !!\n", v.vt);
						}
						PropVariantClear(&v);
					}
				}

			} while (n);
		}
		pProp->Release();
	}
}

void DumpUser(_In_ PCWSTR pszInternetUserName)
{
	IConnectedUserStore* pUserStore;

	HRESULT hr = CoCreateInstance(__uuidof(ConnectedUserStore), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pUserStore));

	if (0 <= hr)
	{
		IConnectedUser * pUser;

		hr = pUserStore->FindConnectedUserByName(pszInternetUserName, L"MicrosoftAccount", 0, &pUser);

		pUserStore->Release();

		if (0 <= hr)
		{
			DumpUser(pUser);

			pUser->Release();
		}
	}
}

HRESULT FindUserEx(_In_ PCWSTR pszInternetUserName)
{
	ULONG f = FACILITY_NT_BIT;

	LSA_OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) };
	LSA_HANDLE PolicyHandle;

	NTSTATUS status = LsaOpenPolicy(0, &ObjectAttributes,
		POLICY_VIEW_LOCAL_INFORMATION|POLICY_LOOKUP_NAMES, &PolicyHandle);

	if (0 <= status)
	{
		PLSA_TRANSLATED_SID2 Sids = 0;
		PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = 0;

		UNICODE_STRING us;
		RtlInitUnicodeString(&us, pszInternetUserName);

		if (0 <= (status = LsaLookupNames2(PolicyHandle, 0, 1, &us, &ReferencedDomains, &Sids)))
		{
			if (Sids->Use == SidTypeUser)
			{
				f = 0;

				IConnectedUserStore* pUserStore;

				if (0 <= (status = CoCreateInstance(__uuidof(ConnectedUserStore), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pUserStore))))
				{
					IConnectedUser * pUser;

					status = pUserStore->FindConnectedUserBySid(Sids->Sid, RtlLengthSid(Sids->Sid), &pUser);

					pUserStore->Release();

					if (0 <= status)
					{
						DumpUser(pUser);

						pUser->Release();
					}
				}
			}
			else
			{
				status = STATUS_NO_SUCH_USER;
			}
		}

		LsaFreeMemory(Sids);
		LsaFreeMemory(ReferencedDomains);

		LsaClose(PolicyHandle);
	}

	return status ? status | f : S_OK;
}

void IdTest()
{
	if (0 <= CoInitializeEx(0, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE))
	{
		// create new local account ("ConnectedUser") and connect it to Internet account (Microsoft)
		CreateConnectedUser(L"ConnectedUser",
			TRUE,
			L"<internet name>@keemail.me", 
			L"<password>");
		
		// connect Internet account (Microsoft) to already existing local account ( "LocalUser" )
		ConnectUser(L"LocalUser",
			L".", // local domain
			L"0", 
			L"<internet name>@keemail.me", 
			L"<password>");
		
		FindUserEx(L"<internet name>@keemail.me");
		DumpUser(L"<internet name>@keemail.me");

		CoUninitialize();
	}
}

/*

// PKEY_Identity_ProviderID
formatID = 74A7DE49-FA11-4D3D-A006-DB7E08675916
propID = 100
{D7F9888F-E3FC-49B0-9Ea6-A85B5F392A4F}

// PKEY_Identity_UserName
formatID = C4322503-78CA-49C6-9Acc-A68E2AFD7B6B
propID = 100
<internet name>@keemail.me

// PKEY_Identity_DisplayName
formatID = 7D683FC9-D155-45A8-BB1f-89D19BCB792F
propID = 100
John Doe

// PKEY_Identity_PrimarySid
formatID = 2B1B801E-C0C1-4987-9Ec5-72FA89814787
propID = 100
S-1-5-21-3032776714-2974785564-2375659916-1030

// PKEY_Identity_UniqueID
formatID = E55FC3B0-2B60-4220-918e-B21E8BF16016
propID = 100
7f116444ea20c5e3

// PKEY_Identity_InternetSid
formatID = 6D6D5D49-265D-4688-9F4e-1FDD33E7CC83
propID = 100
S-1-11-96-3623454863-58364-18864-2661722203-1597581903-3044051589-2523024296-3311815836-3860865233-3092034861

// PKEY_IdentityProvider_Name
formatID = B96EFF7B-35CA-4A35-8607-29E3A54C46EA
propID = 100
MicrosoftAccount

*/