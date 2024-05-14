#include "stdafx.h"

_NT_BEGIN

#include "ioctl.h"

EXTERN_C
NTSYSAPI
BOOLEAN
NTAPI
PsIsProtectedProcess(_In_ PEPROCESS Process);

HANDLE _G_ProcessId;
PVOID _G_Process;

void RemoveProtected()
{
	_G_ProcessId = 0;
	if (PVOID Process = InterlockedExchangePointer((void**)&_G_Process, 0))
	{
		DbgPrint("--remove (%p)\n", Process);
		ObfDereferenceObject(Process);
	}
}

VOID CALLBACK OnCreateProcess(HANDLE /*ParentId*/, HANDLE ProcessId, BOOLEAN Create)
{
	//DbgPrint("OnCreateProcess<%p>(%p->%p %x)\n", PsGetCurrentProcessId(), ParentId, ProcessId, Create);

	if (!Create && _G_ProcessId == ProcessId)
	{
		RemoveProtected();
	}
}

OB_PREOP_CALLBACK_STATUS ObjectPreCallback(PVOID /*RegistrationContext*/, POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	ACCESS_MASK DesiredAccess, *pDesiredAccess;

	switch (OperationInformation->Operation)
	{
	case OB_OPERATION_HANDLE_CREATE:
		pDesiredAccess = &OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
		break;
	case OB_OPERATION_HANDLE_DUPLICATE:
		pDesiredAccess = &OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
		break;
	default:
		// unknown operation ?!
		return OB_PREOP_SUCCESS;
	}

	DesiredAccess = *pDesiredAccess;

	union {
		PVOID Object;
		PEPROCESS Process;
		PETHREAD Thread;
	};

	Object = OperationInformation->Object;

	const ULONG ThreadAccess = SYNCHRONIZE|READ_CONTROL|THREAD_QUERY_LIMITED_INFORMATION|THREAD_QUERY_INFORMATION|THREAD_GET_CONTEXT;

	const ULONG ProcessAccess = SYNCHRONIZE|READ_CONTROL|PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ;

	POBJECT_TYPE ObjectType = OperationInformation->ObjectType;

	if (*PsThreadType == ObjectType)
	{
		if (!(DesiredAccess & ~ThreadAccess))
		{
			// nothing todo
			return OB_PREOP_SUCCESS;
		}

		DesiredAccess &= ThreadAccess;

		Process = PsGetThreadProcess(Thread);
	}
	else if (*PsProcessType == ObjectType)
	{
		if (!(DesiredAccess & ~ProcessAccess))
		{
			// nothing todo
			return OB_PREOP_SUCCESS;
		}

		DesiredAccess &= ProcessAccess;
	}
	else
	{
		// not thread or process
		return OB_PREOP_SUCCESS;
	}

	PEPROCESS CurrentProcess = IoGetCurrentProcess();

	if (Process != CurrentProcess && Process == _G_Process && !PsIsProtectedProcess(CurrentProcess))
	{
		DbgPrint("%x> %p,%s [%x] (%p,%s %08x -> %08x)\n", OperationInformation->Operation, 
			PsGetProcessId(CurrentProcess), PsGetProcessImageFileName(CurrentProcess),
			*PsProcessType == ObjectType,
			PsGetProcessId(Process), PsGetProcessImageFileName(Process), 
			*pDesiredAccess, DesiredAccess);

		*pDesiredAccess = DesiredAccess;
	}

	return OB_PREOP_SUCCESS;
}

ULONG g_protOffset, g_protMask, g_protMax;

void FindProtectedBits(PEPROCESS Process)
{
	RtlZeroMemory(Process, PAGE_SIZE);
	PLONG pu = (PLONG)Process, qu, ru = 0;
	ULONG n = PAGE_SIZE >> 2, i;

	__try 
	{
		do 
		{
			*(qu = pu++) = ~0;

			if (PsIsProtectedProcess(Process))
			{
				if (ru)
				{
					return ;
				}
				ru = qu;
			}

			*qu = 0;

		} while (--n);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}

	if (!ru)
	{
		return ;
	}

	i = 31;
	LONG mask = 0;
	do 
	{
		_bittestandset(ru, i);

		if (PsIsProtectedProcess(Process))
		{
			mask |= *ru;
			n++;
		}

		_bittestandreset(ru, i);

	} while (--i);

	g_protOffset = RtlPointerToOffset(Process, ru);

	switch (n)
	{
	case 1:
		g_protMask = mask;
		break;
	case 3:
		switch (mask)
		{
		case 0x07000000:
			g_protMask = 0xff000000;
			break;
		case 0x00070000:
			g_protMask = 0x00ff0000;
			break;
		case 0x00000700:
			g_protMask = 0x0000ff00;
			break;
		case 0x00000007:
			g_protMask = 0x000000ff;
			break;
		}
		break;
	}

	g_protMax = (*(PULONG)RtlOffsetToPointer(PsInitialSystemProcess, g_protOffset) & g_protMask) | ~g_protMask;
}

#pragma warning(disable: 4996)
void FindProtectedBits()
{
	if (PEPROCESS Process = (PEPROCESS)ExAllocatePool(PagedPool, PAGE_SIZE))
	{
		FindProtectedBits(Process);

		ExFreePool(Process);

		DbgPrint("ProtectedBits: %08x %08x %08x\n", g_protOffset, g_protMask, g_protMax);
	}
}

NTSTATUS SetProtectedProcess(BOOL bSet)
{
	if (g_protMask)
	{
		PLONG pl = (PLONG)RtlOffsetToPointer(IoGetCurrentProcess(), g_protOffset);
		LONG oldValue, newValue;
		do 
		{
			oldValue = *pl;
			newValue = bSet ? ((oldValue | g_protMask) & g_protMax)  : (oldValue & ~g_protMask);
		} while (InterlockedCompareExchange(pl, newValue, oldValue) != oldValue);

		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}

BOOLEAN NTAPI FastIoDeviceControl (
								   _In_ PFILE_OBJECT,
								   _In_ BOOLEAN ,
								   _In_ [[maybe_unused]] PVOID InputBuffer,
								   _In_ [[maybe_unused]] ULONG InputBufferLength,
								   _Out_ [[maybe_unused]] PVOID OutputBuffer,
								   _In_ [[maybe_unused]] ULONG OutputBufferLength,
								   _In_ ULONG IoControlCode,
								   _Out_ PIO_STATUS_BLOCK IoStatus,
								   _In_ PDEVICE_OBJECT
								   )
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

	PEPROCESS Process;

	switch (IoControlCode)
	{
	case IOCTL_AddProtected:
		Process = IoGetCurrentProcess();
		ObfReferenceObject(Process);
		if (0 == InterlockedCompareExchangePointer(&_G_Process, Process, 0))
		{
			_G_ProcessId = PsGetCurrentProcessId();
			status = STATUS_SUCCESS;
		}
		else
		{
			ObfDereferenceObject(Process);
			status = STATUS_TOO_LATE;
		}
		break;
	
	case IOCTL_DelProtected:
		if (_G_Process == IoGetCurrentProcess())
		{
			RemoveProtected();
			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_ACCESS_DENIED;
		}
		break;

	case IOCTL_SetProtectedProcess:
		status = SetProtectedProcess(TRUE);
		break;
	case IOCTL_DelProtectedProcess:
		status = SetProtectedProcess(FALSE);
		break;
	}

	IoStatus->Status = status;
	IoStatus->Information = 0;

	return TRUE;
}

PVOID _G_RegistrationHandle;

NTSTATUS NTAPI OnCloseCleanup(PDEVICE_OBJECT , PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI OnCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
	if (FileObject->FileName.Length || FileObject->RelatedFileObject)
	{
		Irp->IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
		IofCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_OBJECT_NAME_INVALID;
	}
	return OnCloseCleanup(DeviceObject, Irp);
}

void NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{	
	if (_G_RegistrationHandle)
	{
		ObUnRegisterCallbacks(_G_RegistrationHandle);
	}

	PsSetCreateProcessNotifyRoutine(OnCreateProcess, TRUE);

	while (PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject)
	{
		IoDeleteDevice(DeviceObject);
	}

	RemoveProtected();

	DbgPrint("DriverUnload(%p)\n", DriverObject);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	DbgPrint("DriverLoad(%p, %wZ)\n", DriverObject, RegistryPath);

	static const FAST_IO_DISPATCH s_fiod = { 
		sizeof (FAST_IO_DISPATCH), 0, 0, 0, 0, 0, 0, 0, 0, 0, FastIoDeviceControl 
	};

	DriverObject->FastIoDispatch = (PFAST_IO_DISPATCH)&s_fiod;
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = OnCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = OnCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = OnCloseCleanup;

	FindProtectedBits();

	STATIC_UNICODE_STRING(DeviceName, "\\device\\{534A4F3B-A655-4a9e-8E9C-3488B2265ADD}");

	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, const_cast<PUNICODE_STRING>(&DeviceName),
		FILE_DEVICE_UNKNOWN, 0, 0, &DeviceObject);

	if (0 <= status)
	{
		DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

		if (0 <= (status = PsSetCreateProcessNotifyRoutine(OnCreateProcess, FALSE)))
		{
			OB_OPERATION_REGISTRATION oor[] = {
				{ PsThreadType, OB_OPERATION_HANDLE_CREATE|OB_OPERATION_HANDLE_DUPLICATE, ObjectPreCallback },
				{ PsProcessType, OB_OPERATION_HANDLE_CREATE|OB_OPERATION_HANDLE_DUPLICATE, ObjectPreCallback },
			};

			OB_CALLBACK_REGISTRATION ocr = { OB_FLT_REGISTRATION_VERSION, RTL_NUMBER_OF(oor), {}, 0, oor };

			RtlInitUnicodeString(&ocr.Altitude, L"425000");

			status = ObRegisterCallbacks(&ocr, &_G_RegistrationHandle);

			DbgPrint("ObRegisterCallbacks=%x\n", status);
		}
	}

	if (0 > status)
	{
		DriverUnload(DriverObject);
	}

	return status;
}

_NT_END