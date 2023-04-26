BOOL IsSameImage(PIMAGE_EXPORT_DIRECTORY pied, ULONG size, PVOID BaseAddress)
{
	ULONG size2;
	PIMAGE_EXPORT_DIRECTORY pied2 = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
		BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size2);

	return pied2 && size2 == size && !memcmp(pied, pied2, size);
}

BOOL IsCode(ULONG rva, DWORD NumberOfSections, PIMAGE_SECTION_HEADER pish)
{
	do 
	{
		if (rva - pish->VirtualAddress < pish->Misc.VirtualSize)
		{
			return (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE) ==
				(pish->Characteristics & (IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_WRITE));
		}
	} while (pish++, --NumberOfSections);

	return FALSE;
}

NTSTATUS DetectHooks(PVOID BaseAddress, PVOID BaseAddress2)
{
	ULONG size;
	PIMAGE_EXPORT_DIRECTORY pied = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
		BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);

	if (!pied || size < sizeof(IMAGE_EXPORT_DIRECTORY))
	{
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (!IsSameImage(pied, size, BaseAddress2))
	{
		return STATUS_NOT_SAME_OBJECT;
	}

	if (DWORD NumberOfFunctions = pied->NumberOfFunctions)
	{
		PULONG AddressOfFunctions = (PULONG)RtlOffsetToPointer(BaseAddress, pied->AddressOfFunctions);
		PULONG AddressOfFunctions2 = (PULONG)RtlOffsetToPointer(BaseAddress2, pied->AddressOfFunctions);

		if (memcmp(AddressOfFunctions, AddressOfFunctions2, NumberOfFunctions*sizeof(ULONG)))
		{
			return STATUS_NOT_SAME_OBJECT;
		}

		ULONG ExportRva = RtlPointerToOffset(BaseAddress, pied);

		PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(BaseAddress);

		if (!pinth)
		{
			return STATUS_INVALID_IMAGE_FORMAT;
		}

		DWORD NumberOfSections = pinth->FileHeader.NumberOfSections;
		if (!NumberOfSections)
		{
			return STATUS_INVALID_IMAGE_FORMAT;
		}

		PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth);

		BOOL fOk = TRUE;
		do 
		{
			ULONG rva = *AddressOfFunctions++;

			PCSTR pv1 = RtlOffsetToPointer(BaseAddress, rva), pv2 = RtlOffsetToPointer(BaseAddress2, rva);
			
			if (rva - ExportRva < size)
			{
				// forward export
				if (strcmp(pv1, pv2))
				{
					fOk = FALSE;
				}
			}
			else
			{
				if (*pv1 != *pv2)
				{
					if (IsCode(rva, NumberOfSections, pish))
					{
						fOk = FALSE;
					}
				}
			}
			
		} while (--NumberOfFunctions);

		return fOk ? STATUS_SUCCESS : STATUS_IMAGE_CHECKSUM_MISMATCH;
	}

	return STATUS_INVALID_IMAGE_FORMAT;
}

NTSTATUS DetectHooks()
{
	PCWSTR psz = L"\\KnownDlls\\ntdll.dll";

	NTSTATUS status;

#ifndef _WIN64
	PVOID wow;
	if (0 > (status = NtQueryInformationProcess(NtCurrentProcess(), ProcessWow64Information, &wow, sizeof(wow), 0)))
	{
		return status;
	}

	if (wow)
	{
		psz = L"\\KnownDlls32\\ntdll.dll";
	}
#endif

	UNICODE_STRING ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };
	RtlInitUnicodeString(&ObjectName, psz);

	HANDLE hSection;
	if (0 <= (status = ZwOpenSection(&hSection, SECTION_MAP_READ, &oa)))
	{
		PVOID BaseAddress = 0;
		SIZE_T ViewSize = 0;
		status = ZwMapViewOfSection(hSection, NtCurrentProcess(), &BaseAddress, 0, 0, 0, &ViewSize, ViewUnmap, 0, PAGE_READONLY);
		NtClose(hSection);
		if (0 <= status)
		{
			if (HMODULE hmod = GetModuleHandleW(1 + wcschr(1 + psz, '\\')))
			{
				status = DetectHooks(BaseAddress, hmod);
			}
			ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
		}
	}

	return status;
}
