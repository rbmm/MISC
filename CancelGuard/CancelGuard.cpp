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

	DbgPrint("%p:%p:%p>%s => DPC=%p(%p(%p))\n", PsGetCurrentThreadId(), Pc, Stack, txt, Dpc, Dpc->DeferredRoutine, Dpc->DeferredContext);

	//DoSleep();

	return TRUE;
}

void checkDpc(_Inout_ PKDPC * pDpc)
{
	if (PKDPC Dpc = *pDpc)
	{
		DbgPrint("!! DPC=%p(%p(%p))\n", Dpc, Dpc->DeferredRoutine, Dpc->DeferredContext);
		*pDpc = 0;
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
		LARGE_INTEGER li = { (ULONG)(-10000000*120), -1 };
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

	if (Dpc && IsBadDpc(Dpc, __FUNCTION__, _ReturnAddress(), _AddressOfReturnAddress()))
	{
		return FALSE;
	}

	return KeSetCoalescableTimer(Timer, DueTime, Period, TolerableDelay, Dpc);
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
	return PsCreateSystemThread(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, StartRoutine, StartContext);
}

EXTERN_C PVOID __imp_KiDispatchCallout = 0;

VOID
NTAPI
hook_KiDispatchCallout (
				 __in PKAPC Apc,
				 __deref_inout_opt PKNORMAL_ROUTINE *NormalRoutine,
				 __deref_inout_opt PVOID *NormalContext,
				 __deref_inout_opt PVOID *SystemArgument1,
				 __deref_inout_opt PVOID *SystemArgument2
				 )
{
	DbgPrint("KiDispatchCallout(%p, %p, %p, %p, %p)\n", Apc, NormalRoutine, NormalContext, SystemArgument1, SystemArgument2);
	//ExFreePool(Apc);
}

DECLARE_T_HOOK(PsCreateSystemThread, 28);
DECLARE_T_HOOK(KeInsertQueueApc, 16);
DECLARE_T_HOOK(KeSetCoalescableTimer, 20);

T_HOOKS_BEGIN(yy)
T_HOOK(PsCreateSystemThread),
T_HOOK(KeSetCoalescableTimer),
T_HOOKS_END()

T_HOOK_ENTRY KiDisp { &__imp_KiDispatchCallout, hook_KiDispatchCallout };
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
	DbgPrint("DriverUnload \n");
	if (gCallbackRegistration) IoUnregisterContainerNotification(gCallbackRegistration);
	
	//if (__imp_KiDispatchCallout)
	{
		//TrUnHook(&KiDisp, 1);
	}
	//TrUnHook(yy, _countof(yy));
	//HookGenericFault(KiGenericProtectionFault);
}

NTSTATUS InitRegions();
#include "..\kpdb\module.h"

EXTERN_C PVOID __imp_ExIsSafeWorkItem = 0;

EXTERN_C
NTKERNELAPI
BOOLEAN NTAPI ExIsSafeWorkItem(_In_ PWORKER_THREAD_ROUTINE WorkerRoutine);

EXTERN_C PVOID testSafeWorkItem();

EX_PUSH_LOCK PushLock {};

VOID
NTAPI
InApc (
		__in_opt PVOID StartAddress,
		__in_opt PVOID UniqueThread,
		__in_opt PVOID
		)
{
	char sz[64];
	sprintf_s(sz, _countof(sz), "%p>%p", UniqueThread, StartAddress);
	KeEnterGuardedRegion();
	ExAcquirePushLockExclusive(&PushLock);
	DumpStack(sz);
	ExReleasePushLockExclusive(&PushLock);
	KeLeaveGuardedRegion();
}

VOID NTAPI InApc2 ( _In_opt_ PVOID , _In_opt_ PVOID pg, _In_opt_ PVOID time );

VOID
NTAPI
KernelRoutine (
			   __in PKAPC Apc,
			   __deref_inout_opt PKNORMAL_ROUTINE * ,
			   __deref_inout_opt PVOID * Context,
			   __deref_inout_opt PVOID *S1,
			   __deref_inout_opt PVOID *S2
			   )
{
	ExFreePool(Apc);
	InApc2(*Context, *S1, *S2);
	//DbgPrint("%p>KernelRoutine(%p)\n", PsGetCurrentThreadId(), Apc);
}

void EnumSysThreads()
{
	NTSTATUS status;

	ULONG cb = 0x10000;
	do 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;

		if (PVOID buf = ExAllocatePool(PagedPool, cb += PAGE_SIZE))
		{
			if (0 <= (status = NtQuerySystemInformation(SystemProcessInformation, buf, cb, &cb)))
			{
				PSYSTEM_PROCESS_INFORMATION pspi = (PSYSTEM_PROCESS_INFORMATION)buf;

				ULONG NextEntryOffset = 0;
				do 
				{
					(PUCHAR&)pspi += NextEntryOffset;
					if (pspi->UniqueProcessId && !pspi->InheritedFromUniqueProcessId)
					{
						if (ULONG NumberOfThreads = pspi->NumberOfThreads)
						{
							PSYSTEM_THREAD_INFORMATION TH = pspi->TH;
							do 
							{
								if (TH->ThreadState == StateWait && TH->WaitReason == Executive)
								{
									PETHREAD Thread;
									if (0 <= PsLookupProcessThreadByCid(&TH->ClientId, 0, &Thread))
									{
										if (PKAPC Apc = (PKAPC)ExAllocatePool(NonPagedPool, sizeof(KAPC)))
										{
											KeInitializeApc(Apc, Thread, OriginalApcEnvironment, KernelRoutine, 0, 
												(PKNORMAL_ROUTINE)InApc, KernelMode, TH->StartAddress);

											if (!KeInsertQueueApc(Apc, TH->ClientId.UniqueThread, 0, IO_NO_INCREMENT))
											{
												ExFreePool(Apc);
											}
										}
										ObfDereferenceObject(Thread);
									}
								}
							} while (++TH, --NumberOfThreads);
						}
						break;
					}
				} while (NextEntryOffset = pspi->NextEntryOffset);
			}

			ExFreePool(buf);
		}
	} while (status == STATUS_INFO_LENGTH_MISMATCH);
}


typedef NTSTATUS (NTAPI * PROCESS_ENUM_ROUTINE)(_In_ PEPROCESS Process, _In_ PVOID Context);
typedef NTSTATUS (NTAPI * THREAD_ENUM_ROUTINE)(_In_ PEPROCESS Process, _In_ PETHREAD Thread, _In_ PVOID Context);

EXTERN_C PVOID __imp_PsEnumProcesses = 0, __imp_PsEnumProcessThreads = 0;

EXTERN_C
NTKERNELAPI
NTSTATUS
NTAPI
PsEnumProcesses (
				 _In_ PROCESS_ENUM_ROUTINE CallBack,
				 _In_ PVOID Context
				 );

EXTERN_C
NTKERNELAPI
NTSTATUS
NTAPI
PsEnumProcessThreads (
					  _In_ PEPROCESS Process,
					  _In_ THREAD_ENUM_ROUTINE CallBack,
					  _In_ PVOID Context
					  );

struct TCL : CONTEXT, SLIST_ENTRY
{
};

ULONG64 KeGetSecondCount()
{
	return (SharedUserData->TickCountQuad * SharedUserData->TickCountMultiplier >> 24) / 1000;
}

struct GCT 
{
	SLIST_HEADER ListHead;
	KEVENT LowEvent, HighEvent;
	LONG CtxOk = 0, CtxFail = 0, nWorks = 1;
	ULONG dwThreadCount = 0, nInsertOk = 0, nInsertFail = 0;
	ULONG time = (ULONG)KeGetSecondCount() + 2;

	GCT()
	{
		InitializeSListHead(&ListHead);
		KeInitializeEvent(&LowEvent, NotificationEvent, FALSE);
		KeInitializeEvent(&HighEvent, NotificationEvent, FALSE);
		DbgPrint("GCT<%p>\n", this);
	}

	void WorkReady()
	{
		if (!InterlockedDecrement(&nWorks))
		{
			DbgPrint("WorkReady (%p)\n", PsGetCurrentThreadId());
			KeSetEvent(&LowEvent, IO_NO_INCREMENT, TRUE);
		}
	}

	void Worker()
	{
		TCL Context{};
		Context.ContextFlags = CONTEXT_CONTROL;
		ULONG_PTR Rip = 0;
		NTSTATUS status = PsGetContextThread(KeGetCurrentThread(), &Context, KernelMode);
		//DbgPrint("++%p> rip=%p [%x]\n", PsGetCurrentThreadId(), Context.Rip, status);
		if (0 <= status)
		{
			InterlockedIncrement(&CtxOk);
			Rip = Context.Rip;
			ExpInterlockedPushEntrySList(&ListHead, &Context);
		}
		else
		{
			InterlockedIncrement(&CtxFail);
		}

		WorkReady();

		KeWaitForSingleObject(&HighEvent, WrExecutive, KernelMode, FALSE, 0);

		if (Rip != Context.Rip)
		{
			if (0 > PsSetContextThread(KeGetCurrentThread(), &Context, KernelMode))
			{
				__debugbreak();
			}

			DbgPrint("%p> %p -> %p !!!\n", PsGetCurrentThreadId(), Rip, Context.Rip);
		}
		//DbgPrint("--%p> rip=%p [%x]\n", PsGetCurrentThreadId(), Context.Rip, status);
	}

	void Main()
	{
		WorkReady();
		LARGE_INTEGER li = {-10000000*4,-1};
		NTSTATUS s = KeWaitForSingleObject(&LowEvent, WrExecutive, KernelMode, FALSE, &li);

		DbgPrint("Main(%p) =%x\n", PsGetCurrentThreadId(), s);

		if (PSLIST_ENTRY ListEntry = FirstEntrySList(&ListHead))
		{
			ULONG n = 16;
			do 
			{
				if (n)
				{
					--n;
					DbgPrint("Rip= %p\n", static_cast<TCL*>(ListEntry)->Rip);
				}
				
			} while (ListEntry = ListEntry->Next);
		}

		DbgPrint("T=%u %u/%u %u/%u\n", dwThreadCount, CtxOk, CtxFail, nInsertOk, nInsertFail);

		KeSetEvent(&HighEvent, IO_NO_INCREMENT, FALSE);
	}
};


void Scd()
{
	CONTEXT Context{};
	Context.ContextFlags = CONTEXT_CONTROL;
	NTSTATUS status = PsGetContextThread(KeGetCurrentThread(), &Context, KernelMode);
	DbgPrint("++%p> rip=%p [%x]\n", PsGetCurrentThreadId(), Context.Rip, status);
}

VOID
NTAPI
InApc2 (
	   _In_opt_ PVOID ,
	   _In_opt_ PVOID pg,
	   _In_opt_ PVOID time
	   )
{
	ULONG64 t = KeGetSecondCount();
	if (t > (ULONG_PTR)time)
	{
		DbgPrint("%p:%p> %p [%x] Time !! %u\n", 
			PsGetCurrentProcessId(), PsGetCurrentThreadId(), KeGetCurrentThread(), 
			ExGetPreviousMode(), t -= (ULONG_PTR)time);

		static LONG s = 0;

		if (InterlockedIncrement(&s) < 32 || t > 4)
		{
			char sz[32];
			KeEnterGuardedRegion();
			ExAcquirePushLockExclusive(&PushLock);
			Scd();
			sprintf_s(sz, _countof(sz), "## %p", PsGetCurrentThreadId());
			DumpStack(sz);
			ExReleasePushLockExclusive(&PushLock);
			KeLeaveGuardedRegion();
		}
		return ;
	}

	//DbgPrint("%p>InApc2(%x)\n", PsGetCurrentThreadId(), *((PULONG)ptid));
	reinterpret_cast<GCT*>(pg)->Worker();
}

VOID
NTAPI
RundownRoutine2 (_In_ PKAPC Apc)
{
	DbgPrint("%p> RundownRoutine2(%p)\n", PsGetCurrentThreadId(), Apc);
	//reinterpret_cast<GCT*>(Apc->NormalContext)->Worker();
	InApc2(Apc->NormalContext, Apc->SystemArgument1, Apc->SystemArgument2);
	ExFreePool(Apc);
}

NTSTATUS NTAPI ThreadCB(_In_ PEPROCESS Process, _In_ PETHREAD Thread, _In_ PVOID Context)
{
	if (Thread != KeGetCurrentThread())
	{
		reinterpret_cast<GCT*>(Context)->dwThreadCount++;

		if (PKAPC Apc = (PKAPC)ExAllocatePool(NonPagedPool, sizeof(KAPC)))
		{
			KeInitializeApc(Apc, Thread, OriginalApcEnvironment, 
				KernelRoutine, RundownRoutine2, 0, KernelMode, 0);

			InterlockedIncrementNoFence(&reinterpret_cast<GCT*>(Context)->nWorks);

			if (!KeInsertQueueApc(Apc, Context, (PVOID)(ULONG_PTR)reinterpret_cast<GCT*>(Context)->time, IO_NO_INCREMENT))
			{
				DbgPrint("!Insert %p:%p\n", PsGetProcessId(Process), PsGetThreadId(Thread));
				reinterpret_cast<GCT*>(Context)->nInsertFail++;
				InterlockedDecrementNoFence(&reinterpret_cast<GCT*>(Context)->nWorks);
				ExFreePool(Apc);
			}
			else
			{
				//DbgPrint("%p> ++%p\n", PsGetThreadId(Thread), Apc);
				reinterpret_cast<GCT*>(Context)->nInsertOk++;
			}
		}
	}
	else
	{
		DbgPrint("Skip Self\n");
	}

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI ProcessCB(_In_ PEPROCESS Process, _In_ PVOID Context)
{
	PsEnumProcessThreads(Process, ThreadCB, Context);
	return STATUS_SUCCESS;
}

EXTERN_C
NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	DbgPrint("ep(%p, %wZ)", DriverObject, RegistryPath);
	
	if (!ExIsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128)) return STATUS_NOT_IMPLEMENTED;

	struct  
	{
		UCHAR pad[6];
		USHORT n;
		_KIDTENTRY64* pidte;
	} s;

	__sidt(&s.n);

	s.pidte += 13;

	KiGenericProtectionFault.OffsetLow = s.pidte->OffsetLow;
	KiGenericProtectionFault.OffsetMiddle = s.pidte->OffsetMiddle;
	KiGenericProtectionFault.OffsetHigh = s.pidte->OffsetHigh;

	DbgPrint("KiGenericProtectionFault = %p\n", KiGenericProtectionFault.Value);

	DriverObject->DriverUnload = DriverUnload;

	NTSTATUS status = InitRegions();

	DbgPrint("InitRegions = %x\n", status);

	if (0 <= status)
	{
		status = TrInit();
		DbgPrint("TrInit = %x\n", status);
	}

	ULONG h = 0x9A57BC6B; // "ntoskrnl.exe"
	LoadNtModule(1, &h);

	if (CModule* nt = CModule::ByName("ntoskrnl.exe"))
	{
		if (PKDPC KiBalanceSetManagerPeriodicDpc = (PKDPC)nt->GetVaFromName("KiBalanceSetManagerPeriodicDpc"))
		{
			DbgPrint("KiBalanceSetManagerPeriodicDpc=%p(%p(%p))", 
				KiBalanceSetManagerPeriodicDpc, KiBalanceSetManagerPeriodicDpc->DeferredRoutine,
				KiBalanceSetManagerPeriodicDpc->DeferredContext);

			if (PKDEFERRED_ROUTINE KiBalanceSetManagerDeferredRoutine = (PKDEFERRED_ROUTINE)nt->GetVaFromName("KiBalanceSetManagerDeferredRoutine"))
			{
				if (PVOID KiBalanceSetManagerPeriodicEvent = (PKDEFERRED_ROUTINE)nt->GetVaFromName("KiBalanceSetManagerPeriodicEvent"))
				{
					DbgPrint("%p**%p\n", KiBalanceSetManagerDeferredRoutine, KiBalanceSetManagerPeriodicEvent);

					KiBalanceSetManagerPeriodicDpc->DeferredRoutine = KiBalanceSetManagerDeferredRoutine;
					// raise!
					KiBalanceSetManagerPeriodicDpc->DeferredContext = KiBalanceSetManagerPeriodicEvent;

				}
			}
		}

		if ((__imp_PsEnumProcesses = nt->GetVaFromName("PsEnumProcesses")) &&
			(__imp_PsEnumProcessThreads = nt->GetVaFromName("PsEnumProcessThreads")))
		{
			GCT ctx;
			PsEnumProcesses(ProcessCB, &ctx);
			ctx.Main();
		}

		if (__imp_KiDispatchCallout = nt->GetVaFromName("KiDispatchCallout"))
		{
			//TrHook(&KiDisp, 1);
		}
	}

	//HookGenericFault({ MyGenericProtectionFault });

	if (0 <= status)
	{
		//TrHook(yy, _countof(yy));
	}

	//IO_SESSION_STATE_NOTIFICATION ssn = { sizeof(ssn), 0, DriverObject, IO_SESSION_STATE_ALL_EVENTS, };
	//status = IoRegisterContainerNotification(IoSessionStateNotification, 
	//	(PIO_CONTAINER_NOTIFICATION_FUNCTION)OnSessionNotify, &ssn, sizeof(ssn), &gCallbackRegistration);


	//if (__imp_ExIsSafeWorkItem = CModule::GetVaFromName("ntoskrnl.exe", "ExIsSafeWorkItem"))
	//{
	//	DbgPrint("ExIsSafeWorkItem = %p\n", __imp_ExIsSafeWorkItem);

	//	PVOID pv = testSafeWorkItem();;
	//	DbgPrint("SafeWorkItem = %p\n", pv);
	//}

	//PVOID pv = CModule::GetVaFromName("ntoskrnl.exe", "MmGetSessionById");
	//DbgPrint("===%p\n", pv);
	//EnumSysThreads();
	return STATUS_SUCCESS;
}

_NT_END
