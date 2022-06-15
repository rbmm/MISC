#include "stdafx.h"

_NT_BEGIN

struct CFilter : public UNICODE_STRING
{
	CFilter* next;
	WCHAR buf[];

	enum { PoolTag = '>00<'};
	CFilter(CFilter* next) : next(next)
	{
		DbgPrint("++%p> %wZ\n", this, static_cast<PUNICODE_STRING>(this));
	}

	~CFilter()
	{
		DbgPrint("--%p> %wZ\n", this, static_cast<PUNICODE_STRING>(this));
	}

	void* operator new(size_t s, _In_ PCUNICODE_STRING Source1, _In_ PCUNICODE_STRING Source2 )
	{
		union {
			PVOID pv;
			CFilter* This;
		};

		ULONG MaximumLength = Source1->MaximumLength + Source1->MaximumLength;

		if (MaximumLength < MAXUSHORT)
		{
			if (pv = ExAllocatePoolWithTag(PagedPool, s + MaximumLength, PoolTag))
			{
				This->Buffer = This->buf;
				This->Length = 0;
				This->MaximumLength = (USHORT)MaximumLength;

				if (0 <= RtlAppendUnicodeStringToString(This, Source1) &&
					0 <= RtlAppendUnicodeStringToString(This, Source2) &&
					0 <= RtlUpcaseUnicodeString(This, This, FALSE))
				{
					return pv;
				}

				ExFreePoolWithTag(pv, PoolTag);
			}
		}

		return 0;
	}

	void operator delete(void* pv)
	{
		ExFreePoolWithTag(pv, PoolTag);
	}
};

CFilter* InitFilter(PCWSTR name, CFilter* next)
{
	UNICODE_STRING ObjectName = { 0, 0, const_cast<PWSTR>(name) };
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName };

	DbgPrint("Filter: %S\n", name);

	for (; name = wcschr(name, OBJ_NAME_PATH_SEPARATOR); ++name)
	{
		ObjectName.MaximumLength = ObjectName.Length = (USHORT)RtlPointerToOffset(ObjectName.Buffer, name);

		if (!ObjectName.Length)
		{
			continue;
		}

		HANDLE hFile;
		IO_STATUS_BLOCK iosb;

		if (0 <= NtOpenFile(&hFile, FILE_READ_ATTRIBUTES, &oa, &iosb, 0, FILE_DIRECTORY_FILE))
		{
			PVOID Object;
			NTSTATUS status = ObReferenceObjectByHandle(hFile, 0, 0, KernelMode, &Object, 0);
			NtClose(hFile);

			if (0 <= status)
			{
				union {
					WCHAR buf[0x100];
					OBJECT_NAME_INFORMATION oni;
				};

				ULONG cb;

				status = ObQueryNameString(Object, &oni, sizeof(buf), &cb);

				ObfDereferenceObject(Object);

				if (0 <= status)
				{
					DbgPrint(">> %wZ\n", &oni.Name);
					if (0 <= RtlInitUnicodeStringEx(&ObjectName, name))
					{
						return new(&oni.Name, &ObjectName) CFilter(next);
					}
				}
			}
		}
	}

	return next;
}

CFilter* InitFilters(PKEY_VALUE_PARTIAL_INFORMATION pkvpi)
{
	if (pkvpi->Type != REG_MULTI_SZ)
	{
		return 0;
	}

	ULONG DataLength = pkvpi->DataLength;

	if (DataLength <= 2 * sizeof(WCHAR) || (DataLength & (sizeof(WCHAR) - 1)))
	{
		return 0;
	}

	union {
		PUCHAR pb;
		PWSTR psz;
	};

	pb = pkvpi->Data;

	PWSTR end = (PWSTR)(pb + DataLength);

	if (!*psz || *--end || *--end)
	{
		return 0;
	}

	CFilter* next = 0;

	do 
	{
		next = InitFilter(psz, next);
	} while (*(psz += wcslen(psz) + 1));

	return next;
}

CFilter* g_pFirst = 0;

BOOLEAN InitFilters(PUNICODE_STRING DriverKey)
{
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, DriverKey, OBJ_CASE_INSENSITIVE };

	NTSTATUS status = ZwOpenKey(&oa.RootDirectory, KEY_READ, &oa);

	if (0 <= status)
	{
		STATIC_UNICODE_STRING_(Protect);

		ULONG cb = 0x100;

		do 
		{
			if (PVOID pv = ExAllocatePoolWithTag(PagedPool, cb, 'init'))
			{
				if (0 <= (status = ZwQueryValueKey(oa.RootDirectory, &Protect, KeyValuePartialInformation, pv, cb, &cb)))
				{
					g_pFirst = InitFilters((PKEY_VALUE_PARTIAL_INFORMATION)pv);
				}

				ExFreePoolWithTag(pv, 'init');
			}

		} while (status == STATUS_BUFFER_OVERFLOW);

		NtClose(oa.RootDirectory);
	}

	DbgPrint("InitFilters = %x, %p\n", status, g_pFirst);

	return g_pFirst != 0;
}

void DeleteFilters()
{
	if (CFilter* next = g_pFirst)
	{
		do 
		{
			CFilter* cur = next;
			next = next->next;
			delete cur;
		} while (next);
	}
}

BOOLEAN IsProtectPath(PUNICODE_STRING Name)
{
	CFilter* Expression = g_pFirst;
	do 
	{
		if (FsRtlIsNameInExpression(Expression, Name, TRUE, 0))
		{
			return TRUE;
		}
	} while (Expression = Expression->next);

	return FALSE;
}

_NT_END