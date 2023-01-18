// discovery.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

_NT_BEGIN

#include "../inc/rtf.h"

union _KIDTENTRY64
{
	struct  
	{
		/*0000*/USHORT OffsetLow;
		/*0002*/USHORT Selector;
		/*0004*/USHORT IstIndex : 03;//00
		/*0004*/USHORT Reserved0 : 05;//03
		/*0004*/USHORT Type : 05;//08
		/*0004*/USHORT Dpl : 02;//13
		/*0004*/USHORT Present : 01;//15
		/*0006*/USHORT OffsetMiddle;
		/*0008*/ULONG OffsetHigh;
		/*000C*/ULONG Reserved1;
	};

	M128A m128;
};

union IdtOffset {
	PVOID pValue;
	ULONG_PTR Value;
	struct  
	{
		USHORT OffsetLow;
		USHORT OffsetMiddle;
		ULONG OffsetHigh;
	};
};

C_ASSERT(sizeof(IdtOffset)==8);

EXTERN_C_START

void XMM(UCHAR i, PVOID);
void XMM128(UCHAR i, PVOID);

IdtOffset KiGenericProtectionFault;
void MyGenericProtectionFault();
_KIDTENTRY64* GetIdtEntry(int i);

EXTERN_C_END

struct GENERIC_REGISTERS
{
	ULONG_PTR grV[16];
};

struct MACHFRAME 
{
	ULONG_PTR ErrorCode, Rip, Cs, RFlasg, Rsp, Ss;
};

struct INTERRUPT_FRAME : public GENERIC_REGISTERS, public MACHFRAME
{
};
/*
	return:
	TRUE - goto original handler
	FALSE - goto exit interrupt
*/
EXTERN_C BOOLEAN GenericProtectionHandler(INTERRUPT_FRAME& ifr)
{
	static const PCSTR szRegs[] = {
		"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
		"r8" , "r9" , "r10", "r11", "r12", "r13", "r14", "r15",
	};

	ULONG_PTR ImageBase, Rip = ifr.Rip, *Rsp = (ULONG_PTR*)ifr.Rsp, *Frame;

	PRUNTIME_FUNCTION pRT = RtlLookupFunctionEntry(Rip, &ImageBase, 0);

	DbgPrint(">GenericProtectionHandler %p/%p\n", Rip, pRT);

	if (!pRT) 
	{
		__debugbreak();
		return TRUE;
	}

	GENERIC_REGISTERS gr;

	memcpy(&gr, static_cast<GENERIC_REGISTERS*>(&ifr), sizeof(GENERIC_REGISTERS));

	ULONG RipRva = RtlPointerToOffset(ImageBase, Rip);

__loop:

	ULONG UnwindData;
	while (1 & (UnwindData = pRT->UnwindData))
	{
		pRT = (PRUNTIME_FUNCTION)RtlOffsetToPointer(ImageBase, UnwindData - 1);
	}

	PUNWIND_INFO pui = (PUNWIND_INFO)RtlOffsetToPointer(ImageBase, pRT->UnwindData);
	ULONG BeginAddress = pRT->BeginAddress;
	ULONG CodeOffset = RipRva - BeginAddress;
	UCHAR CountOfCodes = pui->CountOfCodes, Version = pui->Version, FrameRegister = pui->FrameRegister;
	PUNWIND_CODE UnwindCode = pui->UnwindCode;

	if (FrameRegister)
	{
		Frame = (ULONG_PTR*)ifr.grV[FrameRegister];
	}
	else
	{
		Frame = Rsp;
		FrameRegister = 4;
	}

	while (CountOfCodes--)
	{
		UCHAR i;
		ULONG cb;
		BOOL doUnwind = UnwindCode->CodeOffset <= CodeOffset;

		ULONG_PTR pc = ImageBase + BeginAddress + UnwindCode->CodeOffset;

		switch(UnwindCode->UnwindOp)
		{
		case UWOP_PUSH_NONVOL:

			if (doUnwind) gr.grV[UnwindCode->OpInfo] = *Rsp++;

			DbgPrint("%p>\tpop %s\t\t\t\t;%u\n", pc, szRegs[UnwindCode->OpInfo], doUnwind);
			break;

		case UWOP_ALLOC_LARGE:

			if (!CountOfCodes--) return TRUE;

			if (UnwindCode++->OpInfo)
			{
				if (!CountOfCodes--) return TRUE;
				cb = *(ULONG*)UnwindCode;
				UnwindCode++;
			}
			else
			{
				cb = *(WORD*)UnwindCode << 3;
			}

			if (doUnwind) Rsp = (ULONG_PTR*)RtlOffsetToPointer(Rsp, cb);
			DbgPrint("%p>\tadd rsp,%x\t\t\t\t;%u\n", pc, cb, doUnwind);
			break;

		case UWOP_ALLOC_SMALL:
			if (doUnwind) Rsp += UnwindCode->OpInfo + 1;
			DbgPrint("%p>\tadd rsp,%x\t\t\t\t;%u\n", pc, (UnwindCode->OpInfo + 1) << 3, doUnwind);
			break;

		case UWOP_SET_FPREG:
			if (doUnwind) Rsp = Frame - (pui->FrameOffset << 1);
			DbgPrint("%p>\tlea rsp,[%s - %x]\t\t\t\t\n", pc, szRegs[FrameRegister], pui->FrameOffset << 4, doUnwind);
			break;

		case UWOP_SAVE_NONVOL:
			if (!CountOfCodes--) return TRUE;
			i = UnwindCode->OpInfo;
			cb = (++UnwindCode)->FrameOffset;
			if (doUnwind) gr.grV[i] = Rsp[cb];
			DbgPrint("%p>\tmov %s,[rsp + %x]\t\t\t\t;%u\n", pc, szRegs[i], cb << 3, doUnwind);
			break;

		case UWOP_SAVE_NONVOL_FAR:
			if (!CountOfCodes-- || !CountOfCodes--) return TRUE;
			i = UnwindCode->OpInfo;
			cb = *(ULONG*)(UnwindCode + 1);
			if (doUnwind) gr.grV[i] = *(ULONG_PTR*)RtlOffsetToPointer(Rsp, cb);
			UnwindCode += 2;
			DbgPrint("%p>\tmov %s,[rsp + %x]\t\t\t\t;%u\n", pc, szRegs[i], cb, doUnwind);
			break;

		case UWOP_SAVE_XMM:
			if (!CountOfCodes--) return TRUE;
			if (Version > 1)
			{
				++UnwindCode;
				//DbgPrint("SAVE_XMM ??? %x\n", UnwindCode->FrameOffset);
			}
			else
			{
				i = UnwindCode->OpInfo;
				cb = (++UnwindCode)->FrameOffset;
				if (doUnwind) XMM(i, Rsp + cb);
				DbgPrint("%p>\tmov xmm%u,[rsp + %x]\t\t\t\t;%u\n", pc, i, cb << 3, doUnwind);
			}
			break;

		case UWOP_SAVE_XMM128:
			if (!CountOfCodes--) return TRUE;
			i = UnwindCode->OpInfo;
			cb = (++UnwindCode)->FrameOffset << 1;
			if (doUnwind) XMM128(i, Rsp + cb);
			DbgPrint("%p>\tmov xmm%u,[rsp + %x]\t\t\t\t;%u\n", pc, i, cb << 3, doUnwind);
			break;

		case UWOP_SAVE_XMM_FAR:
			if (!CountOfCodes-- || !CountOfCodes--) return TRUE;
			i = UnwindCode->OpInfo;
			cb = *(ULONG*)(UnwindCode + 1);
			UnwindCode += 2;
			if (doUnwind) XMM(i, RtlOffsetToPointer(Rsp, cb));
			DbgPrint("%p>\tmov xmm%u,[rsp + %x]\t\t\t\t;%u\n", pc, i, cb, doUnwind);
			break;

		case UWOP_SAVE_XMM128_FAR:
			if (!CountOfCodes-- || !CountOfCodes--) return TRUE;
			i = UnwindCode->OpInfo;
			cb = *(ULONG*)(UnwindCode + 1);
			UnwindCode += 2;
			if (doUnwind) XMM128(i, RtlOffsetToPointer(Rsp, cb));
			DbgPrint("%p>\tmov xmm%u,[rsp + %x]\t\t\t\t;%u\n", pc, i, cb, doUnwind);
			break;

		case UWOP_PUSH_MACHFRAME:
			__debugbreak();
			return TRUE;
		default:
			__debugbreak();
			return TRUE;
		}
		UnwindCode++;
	}

	if (pui->Flags & UNW_FLAG_CHAININFO)
	{
		pRT = GetChainedFunctionEntry(pui);
		goto __loop;
	}

	memcpy(static_cast<GENERIC_REGISTERS*>(&ifr), &gr, sizeof GENERIC_REGISTERS);

	ifr.Rip = *Rsp++;
	ifr.Rsp = (ULONG_PTR)Rsp;

	DbgPrint("exit from %p to %p\n", Rip, ifr.Rip);

	return FALSE;
}

BOOLEAN HookGenericFault(IdtOffset Handler)
{
	GROUP_AFFINITY Affinity {}, PreviousAffinity {}, *pPreviousAffinity = &PreviousAffinity;

	if (Affinity.Group = KeQueryActiveGroupCount())
	{
		do 
		{
			if (KAFFINITY ActiveProcessorMask = KeQueryGroupAffinity(--Affinity.Group))
			{
				DbgPrint("G[%x]=%p\n", Affinity.Group, ActiveProcessorMask);

				Affinity.Mask = 1ULL << 63;

				do 
				{
					if (Affinity.Mask & ActiveProcessorMask)
					{
						KeSetSystemGroupAffinityThread(&Affinity, pPreviousAffinity);
						pPreviousAffinity = 0;

						PROCESSOR_NUMBER ProcNumber;
						ULONG i = KeGetCurrentProcessorNumberEx(&ProcNumber);
						DbgPrint(">> G[%x][%x.%x] %p/%p\n", 
							i, ProcNumber.Group, ProcNumber.Number, 1ULL << ProcNumber.Number, Affinity.Mask);

						_KIDTENTRY64* pidte = GetIdtEntry(13), idte_new, idte_cmp = *pidte;

						IdtOffset v;
						v.OffsetLow = pidte->OffsetLow;
						v.OffsetMiddle = pidte->OffsetMiddle;
						v.OffsetHigh = pidte->OffsetHigh;

						DbgPrint("int0D=%p KiGenericProtectionFault = %p [%x]\n", 
							pidte, v.Value, v.Value == KiGenericProtectionFault.Value);

						struct MDL_EX : MDL 
						{
							PFN_NUMBER Pages[2];
						};

						MDL_EX mdl;

						MmInitializeMdl(&mdl, pidte, sizeof(_KIDTENTRY64));

						MmBuildMdlForNonPagedPool(&mdl);
						mdl.MdlFlags |= MDL_PAGES_LOCKED;
						mdl.MdlFlags &= ~MDL_SOURCE_IS_NONPAGED_POOL;
						//MmProbeAndLockPages(&mdl, KernelMode, IoReadAccess);

						if (PVOID BaseAddress = MmGetSystemAddressForMdlSafe(&mdl, LowPagePriority))
						{
							do 
							{
								idte_new = idte_cmp;

								idte_new.OffsetHigh = Handler.OffsetHigh;
								idte_new.OffsetMiddle = Handler.OffsetMiddle;
								idte_new.OffsetLow = Handler.OffsetLow;

							} while (!InterlockedCompareExchange128((PLONG64)BaseAddress, 
								idte_new.m128.High, idte_new.m128.Low, (PLONG64)&idte_cmp));

							MmUnmapLockedPages(BaseAddress, &mdl);
						}

						//MmUnlockPages(&mdl);
					}

				} while (Affinity.Mask >>= 1);
			}

		} while (Affinity.Group);

		if (!pPreviousAffinity)
		{
			DbgPrint(">> G[%x]=%p\n", PreviousAffinity.Group, PreviousAffinity.Mask);
			KeRevertToUserGroupAffinityThread(&PreviousAffinity);
			return TRUE;
		}
	}

	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
// ++ demo

#include "detour.h"

NTSTATUS
NTAPI
hook_ZwOpenKey(
			   _Out_ PHANDLE KeyHandle,
			   _In_ ACCESS_MASK DesiredAccess,
			   _In_ POBJECT_ATTRIBUTES ObjectAttributes
			   )
{
	static LONG n = 0;
	if (InterlockedIncrement(&n) < 16)
	{
		DbgPrint("%p>OpenKey(%wZ)\n", _ReturnAddress(), ObjectAttributes->ObjectName);
	}
	return ZwOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
}

BOOLEAN IsBad(_In_ PKDPC Dpc, _In_ PCSTR txt, _In_ PVOID Pc)
{
	switch (0xFFFF800000000000 & (ULONG_PTR)Dpc->DeferredContext)
	{
	case 0:
	case 0xFFFF800000000000:
		return FALSE;
	}

	static volatile LONG n = 0;
	static volatile PVOID sPc = 0;
	static volatile HANDLE tid = 0;
	static volatile PVOID DeferredContext = 0;
	static volatile PKDEFERRED_ROUTINE DeferredRoutine = 0;

	InterlockedIncrement(&n);
	InterlockedExchangePointer(&sPc, Pc);
	InterlockedExchangePointer(&tid, PsGetCurrentThreadId());
	InterlockedExchangePointer(&DeferredContext, Dpc->DeferredContext);
	InterlockedExchangePointer((void**)&DeferredRoutine, Dpc->DeferredRoutine);

	DbgPrint("%p:%p>%s(%p, %p)\n", PsGetCurrentThreadId(), Pc, txt, Dpc, Dpc->DeferredContext);

	return TRUE;
}

BOOLEAN
NTAPI
hook_KeSetCoalescableTimer (
					   _Inout_ PKTIMER Timer,
					   _In_ LARGE_INTEGER DueTime,
					   _In_ ULONG Period,
					   _In_ ULONG TolerableDelay,
					   _In_opt_ PKDPC Dpc
					   )
{
	static LONG n = 0;

	InterlockedIncrement(&n);

	return Dpc && IsBad(Dpc, __FUNCTION__, _ReturnAddress()) ? TRUE : KeSetCoalescableTimer(Timer, DueTime, Period, TolerableDelay, Dpc);
}

DECLARE_T_HOOK(KeSetCoalescableTimer, 20);
DECLARE_T_HOOK(ZwOpenKey, 12);

T_HOOKS_BEGIN(yy)
T_HOOK(ZwOpenKey),
T_HOOK(KeSetCoalescableTimer),
T_HOOKS_END()

// -- Demo
//////////////////////////////////////////////////////////////////////////

void NTAPI DriverUnload(PDRIVER_OBJECT /*DriverObject*/)
{
	DbgPrint("DriverUnload\n");
	TrUnHookA(yy);
	HookGenericFault(KiGenericProtectionFault);
}

NTSTATUS InitRegions();

NTSTATUS NTAPI ep(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	DbgPrint("ep(%p, %wZ)", DriverObject, RegistryPath);
	
	if (!ExIsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128)) return STATUS_NOT_IMPLEMENTED;

	_KIDTENTRY64* pidte = GetIdtEntry(13);

	KiGenericProtectionFault.OffsetLow = pidte->OffsetLow;
	KiGenericProtectionFault.OffsetMiddle = pidte->OffsetMiddle;
	KiGenericProtectionFault.OffsetHigh = pidte->OffsetHigh;

	DbgPrint("KiGenericProtectionFault = %p\n", KiGenericProtectionFault.Value);

	DriverObject->DriverUnload = DriverUnload;

	NTSTATUS status = InitRegions();

	DbgPrint("InitRegions = %x\n", status);

	if (0 <= status)
	{
		status = TrInit();
		DbgPrint("TrInit = %x\n", status);
	}

	HookGenericFault({ MyGenericProtectionFault });

	if (0 <= status)
	{
		TrHookA(yy);
	}
	return STATUS_SUCCESS;
}

_NT_END
