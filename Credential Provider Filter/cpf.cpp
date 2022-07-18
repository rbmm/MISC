#include "stdafx.h"

_NT_BEGIN

#include "log.h"

volatile UCHAR guz;

NTSTATUS OpenLsass(_Out_ PHANDLE ProcessHandle, _In_ PBYTE buf)
{
	union {
		PBYTE pb;
		PSYSTEM_PROCESS_INFORMATION pspi;
	};

	pb = buf;
	ULONG NextEntryOffset = 0;

	do 
	{
		pb += NextEntryOffset;

		STATIC_UNICODE_STRING(lsass, "lsass.exe");

		CLIENT_ID ClientId = { pspi->UniqueProcessId };

		if (ClientId.UniqueProcess && RtlEqualUnicodeString(&lsass, &pspi->ImageName, TRUE))
		{
			OBJECT_ATTRIBUTES oa = { sizeof(oa) };

			return NtOpenProcess(ProcessHandle, PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_CREATE_THREAD, &oa, &ClientId);
		}

	} while (NextEntryOffset = pspi->NextEntryOffset);

	return STATUS_NOT_FOUND;
}

NTSTATUS OpenLsass(_Out_ PHANDLE ProcessHandle)
{
	NTSTATUS status;

	ULONG cb = 0x10000;

	do 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;

		if (PBYTE buf = new BYTE[cb += PAGE_SIZE])
		{
			if (0 <= (status = NtQuerySystemInformation(SystemProcessInformation, buf, cb, &cb)))
			{
				status = OpenLsass(ProcessHandle, buf), cb = 0;
			}

			delete [] buf;
		}

	} while(cb && status == STATUS_INFO_LENGTH_MISMATCH);

	return status;
}

const SECURITY_DESCRIPTOR sd = { SECURITY_DESCRIPTOR_REVISION, 0, SE_DACL_PRESENT|SE_DACL_PROTECTED };
STATIC_OBJECT_ATTRIBUTES_EX(oa_Event, "\\BaseNamedObjects\\{DF023840-676C-4334-8650-A94DC54EC59B}", 
							OBJ_CASE_INSENSITIVE, const_cast<SECURITY_DESCRIPTOR*>(&sd), 0);

void InjectSelfToLsass()
{
	union {
		NTSTATUS status;
		ULONG hr;
	};

	union {
		HANDLE hProcess;
		HANDLE hEvent;
	};

	status = ZwOpenEvent(&hEvent, SYNCHRONIZE, &oa_Event);
	DbgPrint("OpenEvent=%x\r\n", status);
	if (0 <= status)
	{
		NtClose(hEvent);
	}

	if (status != STATUS_OBJECT_NAME_NOT_FOUND)
	{
		return ;
	}

	PVOID stack = alloca(guz);
	PWSTR szPath;
	ULONG cb = 0x20, cch;
	do 
	{
		szPath = (PWSTR)alloca(cb <<= 1);
		cch = GetModuleFileNameW((HMODULE)&__ImageBase, szPath, RtlPointerToOffset(szPath, stack) / sizeof(WCHAR));
	} while ((hr = GetLastError()) == ERROR_INSUFFICIENT_BUFFER);

	if (hr == NOERROR)
	{
		DbgPrint("MyPath=%S\r\n", szPath);
		status = OpenLsass(&hProcess);
		DbgPrint("OpenLsass=%x\r\n", status);
		if (0 <= status)
		{
			if (PVOID pv = VirtualAllocEx(hProcess, 0, cch = (cch + 1)*sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE))
			{
				if (WriteProcessMemory(hProcess, pv, szPath, cch, 0))
				{
					if (HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (PTHREAD_START_ROUTINE)ExitThread, 0, CREATE_SUSPENDED, 0))
					{
						if (0 <= ZwQueueApcThread(hThread, (PKNORMAL_ROUTINE)LoadLibraryExW, pv, 0, 0))
						{
							ZwQueueApcThread(hThread, (PKNORMAL_ROUTINE)VirtualFree, pv, 0, (PVOID)MEM_RELEASE);
							pv = 0;
						}

						ResumeThread(hThread);

						NtClose(hThread);
					}
				}
				if (pv) VirtualFreeEx(hProcess, pv, 0, MEM_RELEASE);
			}
			NtClose(hProcess);
		}
	}
	else
	{
		DbgPrint("GetModuleFileNameW=%u\r\n", hr);
	}
}

class __declspec(uuid("{DF023840-676C-4334-8650-A94DC54EC59B}")) CSampleFilter : public ICredentialProviderFilter
{
	LONG _cRef = 1;

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(__in REFIID riid, __in void** ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	// ICredentialProviderFilter
	virtual HRESULT STDMETHODCALLTYPE Filter( 
		/* [in] */ CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
		/* [in] */ DWORD dwFlags,
		/* [annotation][size_is][in] */ 
		_In_reads_(cProviders)  GUID *rgclsidProviders,
		/* [annotation][size_is][out][in] */ 
		_Inout_updates_(cProviders)  BOOL *rgbAllow,
		/* [in] */ DWORD cProviders);

	virtual HRESULT STDMETHODCALLTYPE UpdateRemoteCredential( 
		/* [annotation][in] */ 
		_In_  const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsIn,
		/* [annotation][out] */ 
		_Out_  CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsOut);

	~CSampleFilter();

public:
	CSampleFilter();
};

LONG g_cRef;

LONG DllAddRef()
{
	return InterlockedIncrement(&g_cRef);
}

LONG DllRelease()
{
	return InterlockedDecrement(&g_cRef);
}

HRESULT STDMETHODCALLTYPE CSampleFilter::QueryInterface(__in REFIID riid, __in void** ppv)
{
	if (riid == __uuidof(IUnknown) || riid == __uuidof(ICredentialProviderFilter))
	{
		*ppv = static_cast<ICredentialProviderFilter*>(this);
		AddRef();
		return S_OK;
	}

	*ppv = 0;
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CSampleFilter::AddRef()
{
	return InterlockedIncrementNoFence(&_cRef);
}

ULONG STDMETHODCALLTYPE CSampleFilter::Release()
{
	LONG cRef = InterlockedDecrement(&_cRef);
	if (!cRef)
	{
		delete this;
	}
	return cRef;
}

CSampleFilter::~CSampleFilter()
{
	DbgPrint("%s<%p>\r\n", __FUNCTION__, this);
	DllRelease();
}

CSampleFilter::CSampleFilter()
{
	DllAddRef();
	DbgPrint("%s<%p>\r\n", __FUNCTION__, this);
}

HRESULT STDMETHODCALLTYPE CSampleFilter::Filter( 
	/* [in] */ CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
	/* [in] */ DWORD dwFlags,
	/* [annotation][size_is][in] */ 
	_In_reads_(cProviders)  GUID *rgclsidProviders,
	/* [annotation][size_is][out][in] */ 
	_Inout_updates_(cProviders)  BOOL *rgbAllow,
	/* [in] */ DWORD cProviders)
{
	DbgPrint("%s<%p>(%x, %x, %x)\r\n", __FUNCTION__, this, cpus, dwFlags, cProviders);

	if (cProviders)
	{
		do 
		{

			DbgPrint("\t@%x = {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\r\n", 
				*rgbAllow, rgclsidProviders->Data1, rgclsidProviders->Data2, rgclsidProviders->Data3,
				rgclsidProviders->Data4[0], rgclsidProviders->Data4[1], rgclsidProviders->Data4[2], rgclsidProviders->Data4[3], 
				rgclsidProviders->Data4[4], rgclsidProviders->Data4[5], rgclsidProviders->Data4[6], rgclsidProviders->Data4[7]);

		} while (rgbAllow++, rgclsidProviders++, --cProviders);
	}

	InjectSelfToLsass();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSampleFilter::UpdateRemoteCredential( 
	/* [annotation][in] */ 
	_In_  const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsIn,
	/* [annotation][out] */ 
	_Out_  CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * /*pcpcsOut*/)
{
	DbgPrint("%s<%p>(%x %08x)\r\n", __FUNCTION__, this, pcpcsIn->clsidCredentialProvider.Data1);

	// we dont know format of serialization
	return E_UNEXPECTED;
}

STDAPI DllCanUnloadNow()
{
	return g_cRef > 0 ? S_FALSE : S_OK;
}

struct CClassFactory : public IClassFactory
{
	// IUnknown
	IFACEMETHODIMP QueryInterface(__in REFIID riid, __deref_out void **ppv)
	{
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IClassFactory))
		{
			*ppv = static_cast<IClassFactory*>(this);
		}
		else
		{
			*ppv = 0;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		DllAddRef();
		return 1;
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		DllRelease();
		return 1;
	}

	// IClassFactory
	IFACEMETHODIMP CreateInstance(__in IUnknown* pUnkOuter, __in REFIID riid, __deref_out void **ppv)
	{
		*ppv = NULL;
		HRESULT hr;
		if (pUnkOuter)
		{
			hr = CLASS_E_NOAGGREGATION;
		}
		else
		{
			hr = E_OUTOFMEMORY;

			if (IUnknown * pUnk = new CSampleFilter)
			{
				hr = pUnk->QueryInterface(riid, ppv);
				pUnk->Release();
			}
		}
		return hr;
	}

	IFACEMETHODIMP LockServer(__in BOOL bLock)
	{
		DbgPrint("%s<%p>\r\n", __FUNCTION__, this);
		bLock ? DllAddRef() : DllRelease();
		return S_OK;
	}
} cf_filter;

STDAPI DllGetClassObject(
						 _In_ REFCLSID rclsid,
						 _In_ REFIID riid,
						 _Out_ void** ppv
						 )
{
	*ppv = 0;

	return rclsid == __uuidof(CSampleFilter) ? cf_filter.QueryInterface(riid, ppv) : CLASS_E_CLASSNOTAVAILABLE;
}

struct SB : public OBJECT_ATTRIBUTES, UNICODE_STRING 
{
	SB(PWSTR Buf)
	{
		Buffer = Buf;
		OBJECT_ATTRIBUTES::Length = sizeof(OBJECT_ATTRIBUTES);
		UNICODE_STRING::Length = 0;
		RootDirectory = 0;
		ObjectName = this;
		Attributes = OBJ_CASE_INSENSITIVE;
		SecurityDescriptor = 0;
		SecurityQualityOfService = 0;
	}

	OBJECT_ATTRIBUTES* operator()(PCWSTR str, ...)
	{
		PCWSTR* ppsz = &str;
		PBYTE Buf = (PBYTE)Buffer;
		size_t cb;

		do 
		{
			memcpy(Buf, str, cb = wcslen(str) * sizeof(WCHAR)), Buf += cb;
		} while (str = *++ppsz);

		*(PWSTR)Buf = 0;
		cb = Buf - (PBYTE)Buffer;
		MaximumLength = (USHORT)cb;
		UNICODE_STRING::Length = (USHORT)cb;

		return this;
	}
};

NTSTATUS DeleteKey(PCOBJECT_ATTRIBUTES poa)
{
	UNICODE_STRING ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

	NTSTATUS status = ZwOpenKey(&oa.RootDirectory, KEY_ALL_ACCESS, const_cast<POBJECT_ATTRIBUTES>(poa));

	if (0 <= status)
	{
		KEY_FULL_INFORMATION kfi;

		switch (ZwQueryKey(oa.RootDirectory, KeyFullInformation, &kfi, sizeof(kfi), &kfi.TitleIndex))
		{
		case 0:
		case STATUS_BUFFER_OVERFLOW:

			if (kfi.SubKeys)
			{
				PKEY_BASIC_INFORMATION pkni = (PKEY_BASIC_INFORMATION)alloca(
					kfi.MaxNameLen += sizeof(KEY_BASIC_INFORMATION));

				ObjectName.Buffer = pkni->Name;
				ObjectName.MaximumLength = (USHORT)kfi.MaxNameLen - sizeof(ULONG);

				do 
				{
					if (0 <= ZwEnumerateKey(oa.RootDirectory, 
						--kfi.SubKeys, KeyBasicInformation, pkni, 
						kfi.MaxNameLen, &kfi.TitleIndex))
					{
						ObjectName.Length = (USHORT)pkni->NameLength;
						DeleteKey(&oa);
					}

				} while (kfi.SubKeys);
			}
		}

		status = ZwDeleteKey(oa.RootDirectory);

		NtClose(oa.RootDirectory);
	}

	return status;
}

STATIC_WSTRING(SOFTWARE, "\\registry\\MACHINE\\SOFTWARE\\");
STATIC_WSTRING(Classes, "Classes\\CLSID");
STATIC_WSTRING(Filters, "Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Provider Filters");
STATIC_WSTRING(My, "\\{DF023840-676C-4334-8650-A94DC54EC59B}");

#define LENGTH_OF(s) (RTL_NUMBER_OF(s) - 1)

STDAPI DllUnregisterServer()
{
	HANDLE hEvent;
	NTSTATUS status = ZwOpenEvent(&hEvent, EVENT_MODIFY_STATE, &oa_Event);
	DbgPrint("OpenEvent=%x\r\n", status);

	if (0 <= status)
	{
		SetEvent(hEvent);
		NtClose(hEvent);
	}

	WCHAR Buf[
		LENGTH_OF(SOFTWARE) + LENGTH_OF(Filters) + RTL_NUMBER_OF(My)
	];

	SB oa(Buf);

	DeleteKey(oa(SOFTWARE, Filters, My, 0));
	DeleteKey(oa(SOFTWARE, Classes, My, 0));

	return S_OK;
}

NTSTATUS CreateCLSIDKeyRoot(_Out_ PHANDLE KeyHandle)
{
	WCHAR Buf[
		LENGTH_OF(SOFTWARE) + LENGTH_OF(Classes) + RTL_NUMBER_OF(My)
	];

	SB oa(Buf);

	return ZwCreateKey(KeyHandle, KEY_ALL_ACCESS, oa(SOFTWARE, Classes, My, 0), 0, 0, 0, 0);
}

UNICODE_STRING g_emptyUS;

STATIC_WSTRING_(Demo);

NTSTATUS CreateCLSIDKey(PWSTR DllPath, ULONG cb)
{
	STATIC_UNICODE_STRING_(InprocServer32);
	STATIC_UNICODE_STRING_(ThreadingModel);
	STATIC_WSTRING_(Apartment);

	NTSTATUS status;
	HANDLE hKey;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, (PUNICODE_STRING)&InprocServer32, OBJ_CASE_INSENSITIVE };

	if (0 <= (status = CreateCLSIDKeyRoot(&oa.RootDirectory)))
	{
		ZwSetValueKey(oa.RootDirectory, &g_emptyUS, 0, REG_SZ, const_cast<PWSTR>(Demo), sizeof(Demo));

		if (0 <= (status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, &oa, 0, 0, 0, 0)))
		{
			status = ZwSetValueKey(hKey, &g_emptyUS, 0, REG_SZ, DllPath, cb);

			ZwSetValueKey(hKey, &ThreadingModel, 0, REG_SZ, const_cast<PWSTR>(Apartment), sizeof(Apartment));

			NtClose(hKey);
		}
		NtClose(oa.RootDirectory);
	}

	return status;
}

NTSTATUS CreateCPKey(POBJECT_ATTRIBUTES ObjectAttributes)
{
	HANDLE hKey;

	NTSTATUS status = ZwCreateKey(&hKey, KEY_ALL_ACCESS, ObjectAttributes, 0, 0, 0, 0);

	if (0 <= status)
	{
		status = ZwSetValueKey(hKey, &g_emptyUS, 0, REG_SZ, const_cast<PWSTR>(Demo), sizeof(Demo));
		NtClose(hKey);
	}

	return status;
}

NTSTATUS RegisterFilter(PWSTR DllPath, ULONG cb)
{
	WCHAR Buf[
		LENGTH_OF(SOFTWARE) + LENGTH_OF(Filters) + RTL_NUMBER_OF(My)
	];

	SB oa(Buf);

	NTSTATUS status;

	if (0 <= (status = CreateCLSIDKey(DllPath, cb)))
	{
		if (0 <= (status = CreateCPKey(oa(SOFTWARE, Filters, My, 0))))
		{
			return STATUS_SUCCESS;
		}

		DeleteKey(oa(SOFTWARE, Classes, My, 0));
	}

	return status;
}

STDAPI DllRegisterServer()
{
	BOOLEAN b;
	if (0 <= RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &b))
	{
		InjectSelfToLsass();
	}

	ULONG hr;
	PVOID stack = alloca(guz);
	PWSTR szPath;
	ULONG cb = 0x20, cch;
	do 
	{
		szPath = (PWSTR)alloca(cb <<= 1);
		cch = GetModuleFileNameW((HMODULE)&__ImageBase, szPath, RtlPointerToOffset(szPath, stack) / sizeof(WCHAR));
	} while ((hr = GetLastError()) == ERROR_INSUFFICIENT_BUFFER);

	return hr ? HRESULT_FROM_WIN32(hr) : RegisterFilter(szPath, (cch + 1) * sizeof(WCHAR));
}

#include "../inc/initterm.h"

ULONG WINAPI InLsass(HANDLE hEvent)
{
	NTSTATUS status = ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, &oa_Event, NotificationEvent, FALSE);
	DbgPrint("CreateEvent=%x\r\n", status);

	if (0 <= status)
	{
		WaitForSingleObject(hEvent, INFINITE);
		NtClose(hEvent);
	}

	DbgPrint("Exit...\r\n");

	FreeLibraryAndExitThread((HMODULE)&__ImageBase, 0);
}

BOOLEAN APIENTRY DllMain( HMODULE hModule,
						 DWORD  ul_reason_for_call,
						 PVOID hThread
						 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		initterm();
		LdrDisableThreadCalloutsForDll(hModule);
		LOG(Init());
		DbgPrint("PROCESS_ATTACH\r\n");
		if (GetModuleHandleW(L"lsass.exe"))
		{
			if (hThread = CreateThread(0, 0, InLsass, 0, 0, 0))
			{
				NtClose(hThread);
			}
		}
		break;
	case DLL_PROCESS_DETACH:
		DbgPrint("PROCESS_DETACH\r\n");
		destroyterm();
		break;
	}
	return TRUE;
}

_NT_END