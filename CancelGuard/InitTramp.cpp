#include "StdAfx.h"

_NT_BEGIN

#include "trampoline.h"
#include "region.h"
#include "../inc/amd64plat.h"

_PTE* GetPteAddress(_In_ ULONG_PTR p)
{
	_PTE* pte = PXE_X64_L(p);

	if (pte->Valid)
	{
		pte = PPE_X64_L(p);

		if (pte->Valid)
		{
			pte = PDE_X64_L(p);

			return pte->Valid && !pte->LargePage ? PTE_X64_L(p) : 0;
		}
	}

	return 0;
}

ULONGLONG PTE_BASE_X64, PDE_BASE_X64, PPE_BASE_X64, PXE_BASE_X64, PX_SELFMAP;

#define CR3_MASK 0x000FFFFFFFFFF000

BOOL FoundSelfMapIndex()
{
	ULONGLONG cr3 = __readcr3() & CR3_MASK;

	DbgPrint("cr3=%I64x\n", cr3);

	int n = 0x100;

	PX_SELFMAP = PX_SELFMAP_MIN;

	ULONGLONG _PX_SELFMAP = 0;
	do 
	{
		_PTE* pte = PXE(PX_SELFMAP);

		if (MmIsAddressValid(pte))
		{
			DbgPrint("%03x %p %I64x\n", PX_SELFMAP, pte, pte->Value);

			if ((pte->Value & CR3_MASK) == cr3)
			{
				if (_PX_SELFMAP)
				{
					PX_SELFMAP = 0;
					return FALSE;
				}
				else
				{
					_PX_SELFMAP = PX_SELFMAP;
				}
			}
		}

	} while (++PX_SELFMAP, --n);

	if (_PX_SELFMAP)
	{
		INIT_PTE_CONSTS(_PX_SELFMAP);

		DbgPrint("%I64x\n%I64x\n%I64x\n%I64x\n%I64x\n", PX_SELFMAP, PTE_BASE_X64, PDE_BASE_X64, PPE_BASE_X64, PXE_BASE_X64);

		return TRUE;
	}

	return FALSE;
}

NTSTATUS GetNtos(_Out_ void** ImageBase, _Out_ ULONG* ImageSize)
{
	NTSTATUS status;
	ULONG cb = 0x10000;
	union {
		PVOID buf;
		PRTL_PROCESS_MODULES ppm;
	};

	do 
	{
		status = STATUS_NO_MEMORY;

		if (buf = ExAllocatePool(PagedPool, cb += PAGE_SIZE))
		{
			if (0 <= (status = NtQuerySystemInformation(SystemModuleInformation, buf, cb, &cb)))
			{
				if (ppm->NumberOfModules)
				{
					PRTL_PROCESS_MODULE_INFORMATION Module = ppm->Modules;
					*ImageBase = Module->ImageBase;
					*ImageSize = Module->ImageSize;

					ExFreePool(buf);

					return STATUS_SUCCESS;
				}
			}

			ExFreePool(buf);
		}

	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	return status;
}

#pragma bss_seg("INITKDBG")
UCHAR gbuf[4*PAGE_SIZE];
#pragma bss_seg()

#pragma comment(linker, "/SECTION:INITKDBG,ERW!P")

Z_DETOUR_REGION gdr[2];

NTSTATUS InitRegions()
{
	PVOID ImageBase;
	ULONG ImageSize;
	NTSTATUS status = GetNtos(&ImageBase, &ImageSize);

	DbgPrint("ntos=%p %X [%x]\n", ImageBase, ImageSize, status);

	if (0 > status)
	{
		return status;
	}

	if (ImageBase < gbuf)
	{
		if ((ULONG_PTR)gbuf - (ULONG_PTR)ImageBase < 0x80000000 - sizeof(gbuf))
		{
__ok:
			DbgPrint("single range !\n");
			new(gdr) Z_DETOUR_REGION((Z_DETOUR_TRAMPOLINE*)gbuf, sizeof(gbuf) / sizeof(Z_DETOUR_TRAMPOLINE));
			return STATUS_SUCCESS;
		}
	}
	else
	{
		if ((ULONG_PTR)ImageBase - (ULONG_PTR)gbuf < 0x80000000 - ImageSize)
		{
			goto __ok;
		}
	}

	if (FoundSelfMapIndex())
	{
		ULONG size;
		if (PVOID pIAT = RtlImageDirectoryEntryToData(&__ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IAT, &size))
		{
			if (_PTE* pte = GetPteAddress((ULONG_PTR)pIAT))
			{
				DbgPrint("iat:%p %p %p\n", pIAT, pte, pte->Value);
			}
		}

		PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(ImageBase);

		if (ULONG NumberOfSections = pinth->FileHeader.NumberOfSections)
		{
			ULONG VirtualSize = 0;
			ULONG_PTR VirtualAddress = 0;

			PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth);

			do 
			{
				if (!memcmp(pish->Name, "INITKDBG", sizeof(pish->Name)))
				{
					VirtualSize = pish->Misc.VirtualSize;
					VirtualAddress = (ULONG_PTR)ImageBase + pish->VirtualAddress;
					break;
				}
			} while (pish++, --NumberOfSections);

			if (VirtualSize >= PAGE_SIZE)
			{
				DbgPrint("INITKDBG: %p %08X\n", VirtualAddress, VirtualSize);

				if (_PTE* pteFrom = GetPteAddress((ULONG_PTR)gbuf))
				{
					if (_PTE* pte = GetPteAddress(VirtualAddress))
					{
						DbgPrint("map: %p <- %p\n", pte, pteFrom);
						pte->Value = pteFrom->Value;

						new(gdr) Z_DETOUR_REGION((Z_DETOUR_TRAMPOLINE*)VirtualAddress, PAGE_SIZE / sizeof(Z_DETOUR_TRAMPOLINE));
						new(gdr + 1) Z_DETOUR_REGION((Z_DETOUR_TRAMPOLINE*)(gbuf + PAGE_SIZE), (sizeof(gbuf) - PAGE_SIZE) / sizeof(Z_DETOUR_TRAMPOLINE));

						return STATUS_SUCCESS;
					}
				}
			}
		}
	}

	return STATUS_UNSUCCESSFUL;
}

_NT_END