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

union _KPRIORITY_STATE {

	/*0000*/ UCHAR AllFields;

	struct {
	/*0000*/ UCHAR Priority : 07; // 0x7f;
	/*0000*/ UCHAR IsolationWidth : 01; // 0x80;
	};
	/*0001*/
};

struct _KPRCB {
	/*0000*/ ULONG MxCsr;
	/*0004*/ UCHAR LegacyNumber;
	/*0005*/ UCHAR ReservedMustBeZero;
	/*0006*/ UCHAR InterruptRequest;
	/*0007*/ UCHAR IdleHalt;
	/*0008*/ PKTHREAD CurrentThread;
	/*0010*/ PKTHREAD NextThread;
	/*0018*/ PKTHREAD IdleThread;
	/*0020*/ UCHAR NestingLevel;
	/*0021*/ UCHAR ClockOwner;
	union {
	/*0022*/ UCHAR PendingTickFlags;
		struct {
		/*0022*/ UCHAR PendingTick : 01; // 0x01;
		/*0022*/ UCHAR PendingBackupTick : 01; // 0x02;
		};
	};
	/*0023*/ UCHAR IdleState;
	/*0024*/ ULONG Number;
	/*0028*/ ULONGLONG RspBase;
	/*0030*/ ULONGLONG PrcbLock;
	/*0038*/ _KPRIORITY_STATE * PriorityState;
	/*0040*/ CHAR CpuType;
	/*0041*/ CHAR CpuID;
	union {
	/*0042*/ USHORT CpuStep;
		struct {
		/*0042*/ UCHAR CpuStepping;
		/*0043*/ UCHAR CpuModel;
		};
	};
	/*0044*/ ULONG MHz;
	/*0048*/ ULONGLONG HalReserved[0x8];
	/*0088*/ USHORT MinorVersion;
	/*008a*/ USHORT MajorVersion;
	/*008c*/ UCHAR BuildType;
	/*008d*/ UCHAR CpuVendor;
	/*008e*/ UCHAR LegacyCoresPerPhysicalProcessor;
	/*008f*/ UCHAR LegacyLogicalProcessorsPerCore;
	/*0090*/ ULONGLONG TscFrequency;
	/*0098*/ void * TracepointLog;
	/*00a0*/ ULONG CoresPerPhysicalProcessor;
	/*00a4*/ ULONG LogicalProcessorsPerCore;
	/*00a8*/ ULONGLONG PrcbPad04[0x3];
	/*00c0*/ void * SchedulerSubNode;
	/*00c8*/ ULONGLONG GroupSetMember;
	/*00d0*/ UCHAR Group;
	/*00d1*/ UCHAR GroupIndex;
	/*00d2*/ UCHAR PrcbPad05[0x2];
	/*00d4*/ ULONG InitialApicId;
	/*00d8*/ ULONG ScbOffset;
	/*00dc*/ ULONG ApicMask;
	/*00e0*/ void * AcpiReserved;
};

C_ASSERT(FIELD_OFFSET(_KPRCB, HalReserved) + 7*sizeof(ULONGLONG) == 0x80);
C_ASSERT(FIELD_OFFSET(_KPRCB, AcpiReserved) == 0xe0);

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

BOOLEAN IsBadDpc(_In_ PKDPC Dpc, _In_ PCSTR txt, _In_ PVOID Pc, _In_ PVOID Stack)
{
	switch (0xFFFF800000000000 & (ULONG_PTR)Dpc->DeferredContext)
	{
	case 0:
	case 0xFFFF800000000000:
		return FALSE;
	}

	DbgPrint("%p:%p:%p>%s(dpc=%p ctx=%p f=%p)\n", PsGetCurrentThreadId(), Pc, Stack, txt, Dpc, Dpc->DeferredContext, Dpc->DeferredRoutine);

	//DoSleep();

	return TRUE;
}

void checkDpc(_Inout_ PKDPC * pDpc)
{
	if (PKDPC Dpc = *pDpc)
	{
		if (!IsBadDpc(Dpc, __FUNCTION__, 0, 0))
		{
			DbgPrint("!! DPC=%p(%p %p)\n", Dpc, Dpc->DeferredContext, Dpc->DeferredRoutine);
			*pDpc = 0;
		}
	}
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

						PKPCR Pcr = KeGetPcr();

						_KPRCB *Prcb = Pcr->CurrentPrcb;
						_KIDTENTRY64* pidte = &Pcr->IdtBase[0xd], idte_new, idte_cmp = *pidte;

						DbgPrint("Pcr = %p(%p)\nAcpiReserved=%p HalReserved=%p\n", Pcr, Prcb, Prcb->AcpiReserved, Prcb->HalReserved[7]);

						checkDpc((PKDPC*)&Prcb->AcpiReserved);
						checkDpc((PKDPC*)&Prcb->HalReserved[7]);

						IdtOffset v;
						v.OffsetLow = pidte->OffsetLow;
						v.OffsetMiddle = pidte->OffsetMiddle;
						v.OffsetHigh = pidte->OffsetHigh;

						DbgPrint("int0D = %p KiGenericProtectionFault = %p [%x]\n", 
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

struct NonVRegs {
	ULONG_PTR Rsp, Rbx, Rdi, Rsi, Rbp, R12, R13, R14, R15;
};

EXTERN_C void GetRegs(NonVRegs* p);

void DoSleep()
{
	NonVRegs r;
	GetRegs(&r);

	if (KeGetCurrentIrql() < DISPATCH_LEVEL)
	{
		DbgPrint("!!! Sleep rsp=%p\nrbx=%p rdi=%p\nrsi=%p rbp=%p\nr12=%p r13=%p\nr14=%p r15=%p\n", 
			r.Rsp, r.Rbx, r.Rdi, r.Rsi, r.Rbp, r.R12, r.R13, r.R14, r.R15);
		LARGE_INTEGER li = {0, MINLONG };
		KeDelayExecutionThread(KernelMode, FALSE, &li);
	}
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

	if (Dpc)
	{
		IsBadDpc(Dpc, __FUNCTION__, _ReturnAddress(), _AddressOfReturnAddress());
	}

	return KeSetCoalescableTimer(Timer, DueTime, Period, TolerableDelay, Dpc);
}

PRTL_PROCESS_MODULES g_ppm;

void CheckReturnAddress(bool bNotReturn, PVOID Pc, PVOID Stack, PCSTR func, PVOID a = 0, PVOID b = 0, PVOID c = 0, PVOID d = 0)
{
	if (PRTL_PROCESS_MODULES ppm = g_ppm)
	{
		if (ULONG NumberOfModules = ppm->NumberOfModules)
		{
			PRTL_PROCESS_MODULE_INFORMATION Module = ppm->Modules;
			do 
			{
				if ((ULONG_PTR)Pc - (ULONG_PTR)Module->ImageBase < Module->ImageSize)
				{

					break;
				}
			} while (Module++, --NumberOfModules);

			if (!NumberOfModules)
			{
				static LONG n = 0;

				InterlockedIncrement(&n);

				if (n < 4)
				{
					DbgPrint("%p:%p:%p>%s(%p %p %p %p)\n", PsGetCurrentThreadId(), Pc, Stack, func, a, b, c, d);

					if (bNotReturn)
					{
						DoSleep();
					}
				}
			}
		}
	}
}

NTSTATUS
NTAPI
hook_PsCreateSystemThread(
						  _Out_ PHANDLE ThreadHandle,
						  _In_ ULONG DesiredAccess,
						  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
						  _In_opt_  HANDLE ProcessHandle,
						  _Out_opt_ PCLIENT_ID ClientId,
						  _In_ PKSTART_ROUTINE StartRoutine,
						  _In_opt_ _When_(return >= 0, __drv_aliasesMem) PVOID StartContext
						  )
{
	CheckReturnAddress(true, _ReturnAddress(), _AddressOfReturnAddress(), __FUNCTION__, StartRoutine, StartContext);
	return PsCreateSystemThread(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, StartRoutine, StartContext);
}

VOID
NTAPI
hook_ExQueueWorkItem(
				_Inout_ __drv_aliasesMem PWORK_QUEUE_ITEM WorkItem,
				_In_ WORK_QUEUE_TYPE QueueType
				)
{
	CheckReturnAddress(true, _ReturnAddress(), _AddressOfReturnAddress(), __FUNCTION__, WorkItem->WorkerRoutine, WorkItem->Parameter);
	ExQueueWorkItem(WorkItem, QueueType);
}

VOID
NTAPI
hook_KeSetSystemGroupAffinityThread (
								_In_ PGROUP_AFFINITY Affinity,
								_Out_opt_ PGROUP_AFFINITY PreviousAffinity
								)
{
	CheckReturnAddress(true, _ReturnAddress(), _AddressOfReturnAddress(), __FUNCTION__);
	KeSetSystemGroupAffinityThread (Affinity, PreviousAffinity);
}

BOOLEAN 
NTAPI
hook_KeInsertQueueApc ( IN PKAPC Apc, IN PVOID Argument1, IN PVOID Argument2, IN ULONG PriorityIncrement )
{
	CheckReturnAddress(false, _ReturnAddress(), _AddressOfReturnAddress(), __FUNCTION__, 
		Apc->Reserved[0], Apc->Reserved[2], Apc->NormalContext, Argument1);

	return KeInsertQueueApc(Apc, Argument1, Argument2, PriorityIncrement);
}

DECLARE_T_HOOK(PsCreateSystemThread, 28);
DECLARE_T_HOOK(KeInsertQueueApc, 16);
DECLARE_T_HOOK(KeSetCoalescableTimer, 20);
DECLARE_T_HOOK(ExQueueWorkItem, 8);
DECLARE_T_HOOK(KeSetSystemGroupAffinityThread, 8);

T_HOOKS_BEGIN(yy)
T_HOOK(PsCreateSystemThread),
T_HOOK(KeInsertQueueApc),
T_HOOK(ExQueueWorkItem),
T_HOOK(KeSetCoalescableTimer),
T_HOOK(KeSetSystemGroupAffinityThread),
T_HOOKS_END()

// -- Demo
//////////////////////////////////////////////////////////////////////////

PVOID gCallbackRegistration = 0;

NTSTATUS NTAPI OnSessionNotify (
								_In_ PVOID SessionObject,
								_In_ PDRIVER_OBJECT IoObject,
								_In_ IO_SESSION_EVENT Event,
								_In_ PVOID Context,
								_In_reads_bytes_opt_(PayloadLength) PIO_SESSION_CONNECT_INFO NotificationPayload,
								_In_ ULONG PayloadLength
								)
{
	DbgPrint("OnSessionNotify(%p %p %x %p %p %x)\n", SessionObject, IoObject, Event, Context, NotificationPayload, PayloadLength);
	return 0;
}

void NTAPI DriverUnload(PDRIVER_OBJECT /*DriverObject*/)
{
	DbgPrint("DriverUnload\n");
	if (gCallbackRegistration) IoUnregisterContainerNotification(gCallbackRegistration);
	TrUnHookA(yy);
	HookGenericFault(KiGenericProtectionFault);
	if (g_ppm)
	{
		ExFreePool(g_ppm);
	}
}

NTSTATUS InitRegions();

#include "..\kpdb\module.h"

EXTERN_C PVOID __imp_ExIsSafeWorkItem = 0;

EXTERN_C
NTKERNELAPI
BOOLEAN NTAPI ExIsSafeWorkItem(_In_ PWORKER_THREAD_ROUTINE WorkerRoutine);

EXTERN_C PVOID testSafeWorkItem();


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

	IO_SESSION_STATE_NOTIFICATION ssn = { sizeof(ssn), 0, DriverObject, IO_SESSION_STATE_ALL_EVENTS, };
	status = IoRegisterContainerNotification(IoSessionStateNotification, 
		(PIO_CONTAINER_NOTIFICATION_FUNCTION)OnSessionNotify, &ssn, sizeof(ssn), &gCallbackRegistration);

	//ULONG h = 0x9A57BC6B; // "ntoskrnl.exe"
	//LoadNtModule(1, &h);

	//if (__imp_ExIsSafeWorkItem = CModule::GetVaFromName("ntoskrnl.exe", "ExIsSafeWorkItem"))
	//{
	//	DbgPrint("ExIsSafeWorkItem = %p\n", __imp_ExIsSafeWorkItem);

	//	PVOID pv = testSafeWorkItem();;
	//	DbgPrint("SafeWorkItem = %p\n", pv);
	//}

	//PVOID pv = CModule::GetVaFromName("ntoskrnl.exe", "MmGetSessionById");
	//DbgPrint("===%p\n", pv);
	return STATUS_SUCCESS;
}

_NT_END
