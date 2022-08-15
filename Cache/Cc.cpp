#include "stdafx.h"

struct FCB : FSRTL_ADVANCED_FCB_HEADER, FAST_MUTEX, SECTION_OBJECT_POINTERS
{
	ERESOURCE m_Resource;
	ERESOURCE m_PagingIoResource;

	FCB(LONGLONG Size)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		RtlZeroMemory(this, sizeof(*this));

		NodeTypeCode = 0x3333;
		NodeByteSize = sizeof(FCB);
		IsFastIoPossible = FastIoIsPossible;

		ExInitializeResourceLite(Resource = &m_Resource);
		ExInitializeResourceLite(PagingIoResource = &m_PagingIoResource);

		AllocationSize.QuadPart = Size;
		FileSize.QuadPart = Size;
		ValidDataLength.QuadPart = Size;

		ExInitializeFastMutex(this);
		FsRtlSetupAdvancedHeader(this, this);
	}
	
	~FCB()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		ExDeleteResourceLite(&m_PagingIoResource);
		ExDeleteResourceLite(&m_Resource);
	}

	enum { PT = '*BCF' };

	void* operator new(SIZE_T NumberOfBytes)
	{
		return ExAllocatePoolWithTag(NonPagedPoolNx, NumberOfBytes, PT);
	}

	void operator delete(PVOID p)
	{
		ExFreePoolWithTag(p, PT);
	}
};

#define MyTopIrp ((PIRP)&__ImageBase)

BOOLEAN CALLBACK AcquireForLazyWrite(_In_ PVOID Context, _In_ BOOLEAN Wait)
{
	DbgPrint("%s(%p %x)\n", __FUNCTION__, Context, Wait);
	return ExAcquireResourceSharedLite(&reinterpret_cast<FCB*>(Context)->m_Resource, Wait);
}

BOOLEAN CALLBACK AcquireForReadAhead(_In_ PVOID Context, _In_ BOOLEAN Wait)
{
	DbgPrint("%s(%p %x)\n", __FUNCTION__, Context, Wait);
	return ExAcquireResourceSharedLite(&reinterpret_cast<FCB*>(Context)->m_Resource, Wait);
}

VOID CALLBACK ReleaseFromLazyWrite(_In_ PVOID Context)
{
	DbgPrint("%s(%p %x)\n", __FUNCTION__, Context);
	ExReleaseResourceLite(&reinterpret_cast<FCB*>(Context)->m_Resource);
}

VOID CALLBACK ReleaseFromReadAhead(_In_ PVOID Context)
{
	DbgPrint("%s(%p %x)\n", __FUNCTION__, Context);
	ExReleaseResourceLite(&reinterpret_cast<FCB*>(Context)->m_Resource);
}

NTSTATUS CALLBACK OnClose(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

	DbgPrint("%s(%p %08x fo=%p %p)\n", __FUNCTION__, Irp, Irp->Flags, FileObject, FileObject->FsContext2);

	CcFlushCache(FileObject->SectionObjectPointer, 0, 0, &Irp->IoStatus);

	BOOLEAN b = CcPurgeCacheSection(FileObject->SectionObjectPointer, 0, 0, 0);

	DbgPrint("CcPurgeCacheSection=%x\n", b);

	CACHE_UNINITIALIZE_EVENT UninitializeEvent {};
	KeInitializeEvent(&UninitializeEvent.Event, NotificationEvent, FALSE);
	CcUninitializeCacheMap(FileObject, 0, &UninitializeEvent);
	DbgPrint("SignalState=%x\n", UninitializeEvent.Event.Header.SignalState);

	if (!KeReadStateEvent(&UninitializeEvent.Event))
	{
		KeWaitForSingleObject(&UninitializeEvent.Event, WrExecutive, KernelMode, FALSE, 0);
	}

	DbgPrint("Flags=%x %p\n", FileObject->Flags, FileObject->PrivateCacheMap);

	PVOID pv;
	if (pv = FileObject->FsContext)
	{
		delete static_cast<FCB*>(reinterpret_cast<PFSRTL_COMMON_FCB_HEADER>(pv));
		FileObject->FsContext = 0;
	}

	if (pv = FileObject->FsContext2)
	{
		ExFreePoolWithTag(pv, '<<<<');
		FileObject->FsContext2 = 0;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS CALLBACK OnCleanup(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

	DbgPrint("%s(%p %08x fo=%p %p)\n", __FUNCTION__, Irp, Irp->Flags, FileObject, FileObject->FsContext2);

	CcFlushCache(FileObject->SectionObjectPointer, 0, 0, &Irp->IoStatus);

	BOOLEAN b = CcPurgeCacheSection(FileObject->SectionObjectPointer, 0, 0, 0);

	DbgPrint("CcPurgeCacheSection=%x\n", b);

	CcUninitializeCacheMap(FileObject, 0, 0);

	DbgPrint("Flags=%x %p\n", FileObject->Flags, FileObject->PrivateCacheMap);
	FileObject->Flags |= FO_CLEANUP_COMPLETE;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR Information, CCHAR PriorityBoost = IO_NO_INCREMENT)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = Information;

	IofCompleteRequest(Irp, PriorityBoost);

	return status;
}

NTSTATUS CALLBACK OnCreate(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

	DbgPrint("%s(%p %08x fo=%p)\n", __FUNCTION__, Irp, Irp->Flags, FileObject);

	NTSTATUS status = STATUS_NO_MEMORY;
	ULONG_PTR Information = 0;

	if (PVOID buf = ExAllocatePoolWithTag(PagedPool, 0x400000, '<<<<'))
	{
		if (FCB* Fcb = new FCB(0x400000))
		{
			FileObject->FsContext = static_cast<PFSRTL_COMMON_FCB_HEADER>(Fcb);
			FileObject->SectionObjectPointer = Fcb;
			FileObject->FsContext2 = buf;

			static const CACHE_MANAGER_CALLBACKS cmc = {
				AcquireForLazyWrite, ReleaseFromLazyWrite, AcquireForReadAhead, ReleaseFromReadAhead,
			};

			CcInitializeCacheMap(FileObject, (PCC_FILE_SIZES)&Fcb->AllocationSize, FALSE, 
				const_cast<CACHE_MANAGER_CALLBACKS*>(&cmc), Fcb);

			DbgPrint("PrivateCacheMap=%p, buf = %p %x\n", FileObject->PrivateCacheMap, buf, FileObject->Flags);

			FileObject->Flags |= FO_CACHE_SUPPORTED;

			status = STATUS_SUCCESS;
			Information = FILE_CREATED;
		}
		else
		{
			ExFreePoolWithTag(buf, '<<<<');
		}
	}

	return CompleteIrp(Irp, status, Information);
}

NTSTATUS IsIoRangeValid (ULONG64 ByteOffset, ULONG Length)
{
	if (!Length)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if ((Length & 0x1FF) || (ByteOffset & 0x1FF))
	{
		return STATUS_DATATYPE_MISALIGNMENT;
	}

	if ((ByteOffset += Length) < Length)
	{
		return STATUS_INVALID_PARAMETER_MIX;
	}

	if (ByteOffset > 0x400000)
	{
		return STATUS_DISK_FULL;
	}

	return STATUS_SUCCESS;
}

BOOLEAN CALLBACK DoWrite (
						  _In_ PFILE_OBJECT FileObject,
						  _In_ PLARGE_INTEGER FileOffset,
						  _In_ ULONG Length,
						  _In_ BOOLEAN Wait,
						  _In_ ULONG Flags,
						  _In_ PVOID UserBuffer,
						  _Out_ PIO_STATUS_BLOCK IoStatus,
						  _In_ PMDL MdlAddress
						  )
{
	ULONG64 ByteOffset = FileOffset->QuadPart;

	PIRP Irp = IoGetTopLevelIrp();

	DbgPrint("%s[%x]<%p>(%08x mdl=%p ub=%p w=%x fo=%p %x -> %I64x)\n", 
		__FUNCTION__, (ULONG)(ULONG_PTR)PsGetCurrentProcessId(), Irp, 
		Flags, MdlAddress, UserBuffer, Wait, FileObject, Length, ByteOffset);

	NTSTATUS status;
	ULONG_PTR Information = 0;

	if (0 <= (status = IsIoRangeValid(ByteOffset, Length)))
	{
		if (MdlAddress)
		{
			UserBuffer = MmGetSystemAddressForMdlSafe(MdlAddress, LowPagePriority);
			DbgPrint("MmGetSystemAddressForMdlSafe=%p\n", UserBuffer);
		}

		if (UserBuffer)
		{
			Information = Length;
			status = STATUS_SUCCESS;

			if ((Flags & IRP_NOCACHE) || (Irp == MyTopIrp))
			{
				DbgPrint("%p << %x\n", (PUCHAR)FileObject->FsContext2 + ByteOffset, Length);
				__try
				{
					if (!MdlAddress) ProbeForRead(UserBuffer, Length, 1);
					memcpy((PUCHAR)FileObject->FsContext2 + (ULONG_PTR)ByteOffset, UserBuffer, Length);
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{
					status = GetExceptionCode();
				}
			}
			else
			{
				IoSetTopLevelIrp(MyTopIrp);
				DbgPrint("call CcCopyWrite...\n");
				BOOLEAN b = CcCopyWrite(FileObject, FileOffset, Length, Wait, UserBuffer);
				IoSetTopLevelIrp(Irp);
				DbgPrint("CcCopyWrite=%x\n", b);
				status = b ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
			}
		}
		else
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	IoStatus->Status = status;
	IoStatus->Information = Information;

	DbgPrint("%s=%x, %p\n", __FUNCTION__, status, Information);

	return TRUE;
}

BOOLEAN CALLBACK FastIoWrite (
			   _In_ PFILE_OBJECT FileObject,
			   _In_ PLARGE_INTEGER FileOffset,
			   _In_ ULONG Length,
			   _In_ BOOLEAN Wait,
			   _In_ ULONG /*LockKey*/,
			   _In_ PVOID Buffer,
			   _Out_ PIO_STATUS_BLOCK IoStatus,
			   _In_ PDEVICE_OBJECT /*DeviceObject*/
			   )
{
	DbgPrint("FastIoWrite\n");
	return DoWrite(FileObject, FileOffset, Length, Wait, 0, Buffer, IoStatus, 0);
}

NTSTATUS CALLBACK OnWrite(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	PFILE_OBJECT FileObject = IrpSp->FileObject;

	DbgPrint("%s(%p %08x m=%x)\n", __FUNCTION__, Irp, Irp->Flags, IrpSp->MinorFunction);

	DoWrite(FileObject, 
		&IrpSp->Parameters.Write.ByteOffset, 
		IrpSp->Parameters.Write.Length, IoIsOperationSynchronous(Irp), Irp->Flags, Irp->UserBuffer, &Irp->IoStatus, Irp->MdlAddress);

	NTSTATUS status = Irp->IoStatus.Status;
	
	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

BOOLEAN CALLBACK DoRead(
						_In_ PFILE_OBJECT FileObject,
						_In_ PLARGE_INTEGER FileOffset,
						_In_ ULONG Length,
						_In_ BOOLEAN Wait,
						_In_ ULONG Flags,
						_Out_ PVOID UserBuffer,
						_Out_ PIO_STATUS_BLOCK IoStatus,
						_In_ PMDL MdlAddress
						)
{
	ULONG64 ByteOffset = FileOffset->QuadPart;

	PIRP Irp = IoGetTopLevelIrp();

	DbgPrint("%s[%x]<%p>(%08x mdl=%p ub=%p w=%x fo=%p %x -> %I64x)\n", 
		__FUNCTION__, (ULONG)(ULONG_PTR)PsGetCurrentProcessId(), Irp, 
		Flags, MdlAddress, UserBuffer, Wait, FileObject, Length, ByteOffset);

	NTSTATUS status;
	ULONG_PTR Information = 0;

	if (0 <= (status = IsIoRangeValid(ByteOffset, Length)))
	{
		if (MdlAddress)
		{
			UserBuffer = MmGetSystemAddressForMdlSafe(MdlAddress, LowPagePriority);
			DbgPrint("MmGetSystemAddressForMdlSafe=%p\n", UserBuffer);
		}

		if (UserBuffer)
		{
			Information = Length;

			if ((Flags & IRP_NOCACHE) || Irp == MyTopIrp)
			{
				DbgPrint("%p << %x\n", (PUCHAR)FileObject->FsContext2 + ByteOffset, Length);
				status = STATUS_SUCCESS;
				__try
				{
					if (!MdlAddress) ProbeForWrite(UserBuffer, Length, 1);
					memcpy(UserBuffer, (PUCHAR)FileObject->FsContext2 + (ULONG_PTR)ByteOffset, Length);
				}
				__except(EXCEPTION_EXECUTE_HANDLER)
				{
					status = GetExceptionCode();
				}
			}
			else
			{
				IoSetTopLevelIrp(MyTopIrp);
				DbgPrint("call CcCopyRead...\n");
				BOOLEAN b = CcCopyRead(FileObject, FileOffset, Length, Wait, UserBuffer, IoStatus);
				IoSetTopLevelIrp(Irp);
				DbgPrint("CcCopyRead=%x (%x, %p)\n", b, IoStatus->Status, IoStatus->Information);
				return b;
			}
		}
		else
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	IoStatus->Status = status;
	IoStatus->Information = Information;

	DbgPrint("%s=%x, %p\n", __FUNCTION__, status, Information);

	return TRUE;
}

BOOLEAN CALLBACK FastIoRead(
							_In_ PFILE_OBJECT FileObject,
							_In_ PLARGE_INTEGER FileOffset,
							_In_ ULONG Length,
							_In_ BOOLEAN Wait,
							_In_ ULONG /*LockKey*/,
							_Out_ PVOID Buffer,
							_Out_ PIO_STATUS_BLOCK IoStatus,
							_In_ PDEVICE_OBJECT /*DeviceObject*/
							)
{
	DbgPrint("FastIoRead\n");
	return DoRead(FileObject, FileOffset, Length, Wait, 0, Buffer, IoStatus, 0);
}

NTSTATUS CALLBACK OnRead(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	PFILE_OBJECT FileObject = IrpSp->FileObject;

	DbgPrint("%s(%p %08x m=%x)\n", __FUNCTION__, Irp, Irp->Flags, IrpSp->MinorFunction);

	DoRead(FileObject, 
		&IrpSp->Parameters.Read.ByteOffset, 
		IrpSp->Parameters.Read.Length, IoIsOperationSynchronous(Irp), Irp->Flags, Irp->UserBuffer, &Irp->IoStatus, Irp->MdlAddress);

	NTSTATUS status = Irp->IoStatus.Status;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS CALLBACK OnFlush(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	PFILE_OBJECT FileObject = IrpSp->FileObject;

	PIRP IrpTop = IoGetTopLevelIrp();

	IoSetTopLevelIrp(MyTopIrp);

	DbgPrint("%s(%p %08x) ...\n", __FUNCTION__, Irp, Irp->Flags);

	CcFlushCache(FileObject->SectionObjectPointer, 0, 0, &Irp->IoStatus);

	DbgPrint("%s(%p %08x)=%x,%p\n", __FUNCTION__, Irp, Irp->Flags, Irp->IoStatus.Status, Irp->IoStatus.Information);

	IoSetTopLevelIrp(IrpTop);

	NTSTATUS status = Irp->IoStatus.Status;

	IofCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

BOOLEAN CALLBACK FastIoQueryBasicInfo (
	_In_ PFILE_OBJECT /*FileObject*/,
	_In_ BOOLEAN /*Wait*/,
	_Out_ PFILE_BASIC_INFORMATION Buffer,
	_Out_ PIO_STATUS_BLOCK IoStatus,
	_In_ PDEVICE_OBJECT /*DeviceObject*/
	)
{
	DbgPrint("FastIoQueryBasicInfo\n");
	RtlZeroMemory(Buffer, sizeof(FILE_BASIC_INFORMATION));
	IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);
	IoStatus->Status = STATUS_SUCCESS;
	return TRUE;
}

BOOLEAN CALLBACK FastIoQueryStandardInfo (
	_In_ PFILE_OBJECT FileObject,
	_In_ BOOLEAN /*Wait*/,
	_Out_ PFILE_STANDARD_INFORMATION Buffer,
	_Out_ PIO_STATUS_BLOCK IoStatus,
	_In_ PDEVICE_OBJECT /*DeviceObject*/
	)
{
	DbgPrint("FastIoQueryStandardInfo\n");
	PFSRTL_COMMON_FCB_HEADER Fcb = (PFSRTL_COMMON_FCB_HEADER)FileObject->FsContext;
	Buffer->DeletePending = FALSE;
	Buffer->Directory = FALSE;
	Buffer->NumberOfLinks = 1;
	Buffer->EndOfFile.QuadPart = Fcb->FileSize.QuadPart;
	Buffer->AllocationSize.QuadPart = Fcb->AllocationSize.QuadPart;
	IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);
	IoStatus->Status = STATUS_SUCCESS;

	return TRUE;
}

NTSTATUS CALLBACK OnQueryInfo(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	PFILE_OBJECT FileObject = IrpSp->FileObject;

	ULONG Length = IrpSp->Parameters.QueryFile.Length;
	DbgPrint("%s(%p %08x c=%x len=%x)\n", __FUNCTION__, Irp, Irp->Flags, IrpSp->Parameters.QueryFile.FileInformationClass, Length);

	union {
		PVOID SystemBuffer;
		PFILE_STANDARD_INFORMATION pfsi;
		PFILE_BASIC_INFORMATION pfbi;
	};

	SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	ULONG_PTR Information = 0;

	switch (IrpSp->Parameters.QueryFile.FileInformationClass)
	{
	case FileBasicInformation:
		if (Length < sizeof(FILE_BASIC_INFORMATION))
		{
			status = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			FastIoQueryBasicInfo(FileObject, 0, pfbi, &Irp->IoStatus, 0);
			goto __0;
		}
		break;

	case FileStandardInformation:
		if (Length < sizeof(FILE_STANDARD_INFORMATION))
		{
			status = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			FastIoQueryStandardInfo(FileObject, 0, pfsi, &Irp->IoStatus, 0);
__0:
			IofCompleteRequest(Irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}
		break;
	}

	return CompleteIrp(Irp, status, Information);
}

NTSTATUS CALLBACK OnSetInfo(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

	//PFILE_OBJECT FileObject = IrpSp->FileObject;

	ULONG Length = IrpSp->Parameters.QueryFile.Length;

	DbgPrint("%s(%p %08x c=%x len=%x)\n", __FUNCTION__, Irp, Irp->Flags, IrpSp->Parameters.SetFile.FileInformationClass, Length);

	union {
		PVOID SystemBuffer;
		PFILE_END_OF_FILE_INFORMATION peof;
	};

	SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	ULONG_PTR Information = 0;

	switch (IrpSp->Parameters.SetFile.FileInformationClass)
	{
	case FileEndOfFileInformation:
		if (Length < sizeof(FILE_END_OF_FILE_INFORMATION))
		{
			status = STATUS_BUFFER_TOO_SMALL;
		}
		else
		{
			DbgPrint("EOF: %I64x\n", peof->EndOfFile.QuadPart);
			status = STATUS_SUCCESS;
			Information = sizeof(FILE_END_OF_FILE_INFORMATION);
		}
		break;
	}

	return CompleteIrp(Irp, status, Information);
}

BOOLEAN CALLBACK FastIoCheckIfPossible(
						   _In_ PFILE_OBJECT FileObject,
						   _In_ PLARGE_INTEGER FileOffset,
						   _In_ ULONG Length,
						   _In_ BOOLEAN Wait,
						   _In_ ULONG /*LockKey*/,
						   _In_ BOOLEAN CheckForReadOperation,
						   _Out_ PIO_STATUS_BLOCK IoStatus,
						   _In_ PDEVICE_OBJECT /*DeviceObject*/
						   )
{
	IoStatus->Information = 0;
	IoStatus->Status = 0;

	DbgPrint("%s(fo=%p %08x-%016I64x w=%x r=%x)\n", __FUNCTION__, FileObject, Length, FileOffset->QuadPart, Wait, CheckForReadOperation);
	return TRUE; 
}

VOID CALLBACK AcquireFileForNtCreateSection(
					  _In_ PFILE_OBJECT FileObject
					  )
{
	DbgPrint("%s(%p)\n", __FUNCTION__, FileObject);
}

VOID CALLBACK ReleaseFileForNtCreateSection (
					  _In_ PFILE_OBJECT FileObject
					  )
{
	DbgPrint("%s(%p)\n", __FUNCTION__, FileObject);
}

NTSTATUS CALLBACK AcquireForCcFlush (
					  _In_ PFILE_OBJECT FileObject,
					  _In_ PDEVICE_OBJECT /*DeviceObject*/
					  )
{
	DbgPrint("%s(%p)\n", __FUNCTION__, FileObject);
	ExAcquireResourceSharedLite(reinterpret_cast<PFSRTL_COMMON_FCB_HEADER>(FileObject->FsContext)->Resource, TRUE);
	ExAcquireResourceSharedLite(reinterpret_cast<PFSRTL_COMMON_FCB_HEADER>(FileObject->FsContext)->PagingIoResource, TRUE);
	return STATUS_SUCCESS;
}

NTSTATUS CALLBACK ReleaseForCcFlush (
					  _In_ PFILE_OBJECT FileObject,
					  _In_ PDEVICE_OBJECT /*DeviceObject*/
					  )
{
	DbgPrint("%s(%p)\n", __FUNCTION__, FileObject);
	ExReleaseResourceLite(reinterpret_cast<PFSRTL_COMMON_FCB_HEADER>(FileObject->FsContext)->PagingIoResource);
	ExReleaseResourceLite(reinterpret_cast<PFSRTL_COMMON_FCB_HEADER>(FileObject->FsContext)->Resource);
	return STATUS_SUCCESS;
}

BOOLEAN CALLBACK MdlRead (
				 _In_ PFILE_OBJECT FileObject,
				 _In_ PLARGE_INTEGER FileOffset,
				 _In_ ULONG Length,
				 _In_ ULONG LockKey,
				 _Outptr_ PMDL *MdlChain,
				 _Out_ PIO_STATUS_BLOCK IoStatus,
				 _In_opt_ PDEVICE_OBJECT DeviceObject
				 )
{
	PIRP IrpTop = IoGetTopLevelIrp();
	DbgPrint("%s<%p>(fo=%p %016I64x - %08x)\n", __FUNCTION__, IrpTop, FileObject, FileOffset->QuadPart, Length);

	if (IrpTop == MyTopIrp)
	{
		return FALSE;
	}

	IoSetTopLevelIrp(MyTopIrp);

	BOOLEAN b = FsRtlMdlReadDev(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject);

	IoSetTopLevelIrp(IrpTop);

	DbgPrint("%s<%p>=%x %x,%p\n", __FUNCTION__, IrpTop, b, IoStatus->Status, IoStatus->Information);

	return b;
}

BOOLEAN CALLBACK PrepareMdlWrite (
						 _In_ PFILE_OBJECT FileObject,
						 _In_ PLARGE_INTEGER FileOffset,
						 _In_ ULONG Length,
						 _In_ ULONG LockKey,
						 _Outptr_ PMDL *MdlChain,
						 _Out_ PIO_STATUS_BLOCK IoStatus,
						 _In_ PDEVICE_OBJECT DeviceObject
						 )
{
	PIRP IrpTop = IoGetTopLevelIrp();
	DbgPrint("%s<%p>(fo=%p %016I64x - %08x)\n", __FUNCTION__, IrpTop, FileObject, FileOffset->QuadPart, Length);

	if (IrpTop == MyTopIrp)
	{
		return FALSE;
	}

	IoSetTopLevelIrp(MyTopIrp);

	BOOLEAN b = FsRtlPrepareMdlWriteDev(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatus, DeviceObject);

	IoSetTopLevelIrp(IrpTop);

	DbgPrint("%s<%p>=%x %x,%p\n", __FUNCTION__, IrpTop, b, IoStatus->Status, IoStatus->Information);

	return b;
}

BOOLEAN CALLBACK MdlReadComplete (
								  _In_ PFILE_OBJECT FileObject,
								  _In_ PMDL MdlChain,
								  _In_opt_ PDEVICE_OBJECT DeviceObject
								  )
{
	DbgPrint("%s<%p>(fo=%p)\n", __FUNCTION__, IoGetTopLevelIrp(), FileObject);
	return FsRtlMdlReadCompleteDev(FileObject, MdlChain, DeviceObject);
}

BOOLEAN CALLBACK MdlWriteComplete (
								  _In_ PFILE_OBJECT FileObject,
								  _In_ PLARGE_INTEGER FileOffset,
								  _In_ PMDL MdlChain,
								  _In_opt_ PDEVICE_OBJECT DeviceObject
								  )
{
	DbgPrint("%s<%p>(fo=%p)\n", __FUNCTION__, IoGetTopLevelIrp(), FileObject);
	return FsRtlMdlWriteCompleteDev(FileObject, FileOffset, MdlChain, DeviceObject);
}

void CALLBACK DriverUnload(PDRIVER_OBJECT DriverObject)
{
	IoDeleteDevice(DriverObject->DeviceObject);
	DbgPrint("DriverUnload(%p)\r\n", DriverObject);
}

NTSTATUS CALLBACK NotImpl(PDEVICE_OBJECT /*DeviceObject*/, PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	DbgPrint("NotImpl(%.x.%x %x)\n", IrpSp->MajorFunction, IrpSp->MinorFunction, IrpSp->Flags);
	Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
	Irp->IoStatus.Information = 0;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_NOT_IMPLEMENTED;
}

#ifdef _WIN64
#define __stosp __stosq
#else
#define __stosp __stosd
#endif

NTSTATUS CALLBACK DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING Str)
{		
	DbgPrint("DriverEntry(%p, %wZ)\r\n", DriverObject, Str);

	DriverObject->DriverUnload = DriverUnload;

	__stosp((PULONG_PTR)DriverObject->MajorFunction, (ULONG_PTR)NotImpl, _countof(DriverObject->MajorFunction));

	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = OnCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = OnClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = OnRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = OnWrite;
	DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = OnFlush;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = OnCreate;
	DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = OnQueryInfo;
	DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = OnSetInfo;

	static const FAST_IO_DISPATCH FastIoDispatch = {
		sizeof(FastIoDispatch), FastIoCheckIfPossible, FastIoRead, FastIoWrite,
		FastIoQueryBasicInfo, FastIoQueryStandardInfo, 0, 0, 0, 0, 0,
		AcquireFileForNtCreateSection, ReleaseFileForNtCreateSection, 0, 0, 0,
		MdlRead, MdlReadComplete, PrepareMdlWrite, MdlWriteComplete, 0, 0, 0, 0, 0, 0,
		AcquireForCcFlush, ReleaseForCcFlush
	};

	DriverObject->FastIoDispatch = const_cast<PFAST_IO_DISPATCH>(&FastIoDispatch);

	STATIC_UNICODE_STRING(DeviceName, "\\Device\\@@@@@@@@");
	PDEVICE_OBJECT DeviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, const_cast<PUNICODE_STRING>(&DeviceName), 
		FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

	if (0 <= status)
	{
		DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	}

	DbgPrint("DO=%p, %x\n", DeviceObject, status);

	return STATUS_SUCCESS;
}
