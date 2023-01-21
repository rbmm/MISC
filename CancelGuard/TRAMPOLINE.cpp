#include "StdAfx.h"
#include "LDasm.h"

_NT_BEGIN

#include <ntintsafe.h>
#include "trampoline.h"

struct Z_DETOUR_REGION 
{
	friend Z_DETOUR_TRAMPOLINE;

	Z_DETOUR_REGION* next;
	PVOID BaseAddress;
	Z_DETOUR_TRAMPOLINE* First;

	inline static Z_DETOUR_REGION* spRegion = 0;

	void* operator new(size_t, Z_DETOUR_REGION* This)
	{
		return This;
	}

	Z_DETOUR_REGION()
	{
	}

	Z_DETOUR_REGION(Z_DETOUR_TRAMPOLINE* Next, ULONG n)
	{
		DbgPrint("%s<%p>(%p, %x)\n", __FUNCTION__, this, Next, n);

		BaseAddress = Next;

		next = spRegion, spRegion = this;

		Z_DETOUR_TRAMPOLINE* Prev = 0;

		do 
		{
			Next->Next = Prev, Prev = Next++;
		} while (--n);

		First = Prev;
	}

	Z_DETOUR_TRAMPOLINE* alloc()
	{
		if (Z_DETOUR_TRAMPOLINE* Next = First)
		{
			First = Next->Next;

			return Next;
		}

		return 0;
	}

	void free(Z_DETOUR_TRAMPOLINE* Next)
	{
		Next->Next = First, First = Next;
	}

	static void _free(Z_DETOUR_TRAMPOLINE* pTramp)
	{
		if (Z_DETOUR_REGION* pRegion = spRegion)
		{
			do 
			{
				if ((ULONG_PTR)pTramp - (ULONG_PTR)pRegion->BaseAddress < 0x10000)
				{
					pRegion->free(pTramp);

					return ;
				}
			} while (pRegion = pRegion->next);
		}

		__debugbreak();
	}

	static Z_DETOUR_TRAMPOLINE* _alloc(void* pvTarget)
	{
		if (Z_DETOUR_REGION* pRegion = spRegion)
		{
			do 
			{
				PVOID BaseAddress = pRegion->BaseAddress;

				if (BaseAddress < pvTarget)
				{
					if ((ULONG_PTR)pvTarget - (ULONG_PTR)BaseAddress < 0x80000000 - 0x10000)
					{
						return pRegion->alloc();
					}
				}
				else
				{
					if ((ULONG_PTR)BaseAddress - (ULONG_PTR)pvTarget < 0x80000000 - 0x10000)
					{
						return pRegion->alloc();
					}
				}

			} while (pRegion = pRegion->next);
		}

		return 0;
	}
};

void* Z_DETOUR_TRAMPOLINE::operator new(size_t, void* pvTarget)
{
	return Z_DETOUR_REGION::_alloc(pvTarget);
}

void Z_DETOUR_TRAMPOLINE::operator delete(PVOID pv)
{
	Z_DETOUR_REGION::_free((Z_DETOUR_TRAMPOLINE*)pv);
}

struct MDL_EX : MDL 
{
	PFN_NUMBER Pages[2];
};

NTSTATUS Z_DETOUR_TRAMPOLINE::Set()
{
	struct {
		ULONG op;
		BYTE pad[3];
		BYTE jmp_e9;
		union {
			ULONG64 rel;

			struct {
				ULONG rel32;
				USHORT fbe9; // jmp $-7
			};
		};
	} j;

	// first try direct jmp -> pvDetour
	j.rel = (ULONG64)(ULONG_PTR)pvDetour - (ULONG64)((ULONG_PTR)pvJmp + 5);

	if (j.rel + 0x80000000 >= 0x100000000ULL)
	{
		// try jmp -> jmp [pvDetour] in Z_DETOUR_TRAMPOLINE

		j.rel = (ULONG64)(ULONG_PTR)&ff25 - (ULONG64)((ULONG_PTR)pvJmp + 5);

		if (j.rel + 0x80000000 >= 0x100000000ULL)
		{
			return STATUS_UNSUCCESSFUL;
		}
	}

#ifdef _M_IX86
	j.fbe9 = 0xf9eb;
#endif//#ifdef _M_IX86

	j.jmp_e9 = 0xe9;

	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

	MDL_EX mdl;

	MmInitializeMdl(&mdl, pvJmp, cbRestore);
	MmProbeAndLockPages(&mdl, KernelMode, IoReadAccess);

	if (PVOID BaseAddress = MmGetSystemAddressForMdlSafe(&mdl, LowPagePriority))
	{
		memcpy(BaseAddress, &j.jmp_e9, cbRestore);

		MmUnmapLockedPages(BaseAddress, &mdl);
		status = STATUS_SUCCESS;
	}

	MmUnlockPages(&mdl);

	return status;
}

NTSTATUS Z_DETOUR_TRAMPOLINE::Remove()
{
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

	MDL_EX mdl;

	MmInitializeMdl(&mdl, pvJmp, cbRestore);
	MmProbeAndLockPages(&mdl, KernelMode, IoReadAccess);

	if (PVOID BaseAddress = MmGetSystemAddressForMdlSafe(&mdl, LowPagePriority))
	{
		memcpy(BaseAddress, rbRestore, cbRestore);

		MmUnmapLockedPages(BaseAddress, &mdl);
		status = STATUS_SUCCESS;
	}

	MmUnlockPages(&mdl);

	return STATUS_SUCCESS;
}

PVOID Z_DETOUR_TRAMPOLINE::Init(PVOID pvTarget)
{
	ldasm_data ld;
	PBYTE code = (PBYTE)pvTarget, pDst = rbCode;
	BYTE len, cb = 0;
	cbRestore = 5;

#ifdef _M_IX86
	if (code[0] == 0x8b && code[1] == 0xff && // mov edi,edi
		((code[-1] == 0x90 && code[-2] == 0x90 && code[-3] == 0x90 && code[-4] == 0x90 && code[-5] == 0x90) || // nop
		(code[-1] == 0xcc && code[-2] == 0xcc && code[-3] == 0xcc && code[-4] == 0xcc && code[-5] == 0xcc))) // int 3
	{
		pvJmp = code - 5;
		pvRemain = 0;
		cbRestore = 7;
		memcpy(rbRestore, pvJmp, 7);
		return code + 2;
	}
#endif//_M_IX86

	do 
	{
__0:
		len = ldasm( code, &ld, is_x64 );

		if (ld.flags & F_INVALID)
		{
			return 0;
		}

		memcpy(pDst, code, len);

		if (ld.flags & F_RELATIVE)
		{
			LONG_PTR delta;

			if (ld.flags & F_DISP)
			{
				if (ld.disp_size != 4)
				{
					return 0;
				}

				delta = *(PLONG)(code + ld.disp_offset);
__1:
				delta += code - pDst;

				if ((ULONG64)delta + 0x80000000 >= 0x100000000ULL)
				{
					return 0;
				}

				*(PLONG)(pDst + ld.disp_offset) = (LONG)delta;
			}
			else if (ld.flags & F_IMM)
			{
				BYTE opcode = code[ld.opcd_offset];

				switch (ld.imm_size)
				{
				default:
					return 0;
				case 4:
					delta = *(PLONG)(code + ld.imm_offset);

					if (ld.opcd_size == 1 && opcode == 0xe9 && code == pvTarget) // jmp +imm32
					{
						memcpy(rbRestore, pvTarget, 5);

						pvJmp = pvTarget, pvRemain = 0;

						return code + len + delta;
					}
					ld.disp_offset = ld.imm_offset;
					goto __1;
				case 1:
					if (ld.opcd_size != 1)
					{
						return 0;
					}

					delta = *(PCHAR)(code+ld.imm_offset);

					if (opcode == 0xeb) // jmp +imm8
					{
						if (code == pvTarget)
						{
							pvTarget = code = code + len + delta, cb = 0;
							goto __0;
						}
						pDst[ld.opcd_offset]=0xe9;// -> jmp +imm32

						delta += code - pDst - 3;

						if ((ULONG64)delta + 0x80000000 >= 0x100000000ULL)
						{
							return 0;
						}

						*(PLONG)(pDst + ld.imm_offset) = (LONG)delta;

						pDst += 3;
						break;
					}

					if (opcode - 0x70 > 0xf) // jxx
					{
						return 0;
					}

					pDst[ld.opcd_offset]=0x0f;
					pDst[ld.opcd_offset+1]=0x10+opcode;

					delta += code - pDst - 4;

					if ((ULONG_PTR)delta + 0x80000000 >= 0x100000000UL)
					{
						return 0;
					}

					*(PLONG)(pDst + ld.imm_offset + 1) = (LONG)delta;

					pDst += 4;
					break;
				}
			}
		}

		pDst += len;

	} while (code += len, (cb += len) < 5);

	pvRemain = code;

	*pDst++ = 0xff, *pDst++ = 0x25; // jmp [pvRemain]

	ULONG delta;

#if defined(_M_X64)  
	delta = RtlPointerToOffset(pDst + 4, &pvRemain);
#elif defined (_M_IX86)
	delta = (ULONG)&pvRemain;
#else
#error ##
#endif
	memcpy(pDst, &delta, sizeof(ULONG));

	memcpy(rbRestore, pvTarget, 5);

	pvJmp = pvTarget;

	return rbCode;
}

//////////////////////////////////////////////////////////////////////////

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

extern PRTL_PROCESS_MODULES g_ppm;

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

		if (buf = ExAllocatePool(NonPagedPool, cb += PAGE_SIZE))
		{
			if (0 <= (status = NtQuerySystemInformation(SystemModuleInformation, buf, cb, &cb)))
			{
				if (ppm->NumberOfModules)
				{
					PRTL_PROCESS_MODULE_INFORMATION Module = ppm->Modules;
					*ImageBase = Module->ImageBase;
					*ImageSize = Module->ImageSize;

					g_ppm = ppm;

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