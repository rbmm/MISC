_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS NTAPI IopReplaceCompletionPort(PFILE_OBJECT FileObject, PVOID Port, PVOID Key)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PKSPIN_LOCK IrpListLock = &FileObject->IrpListLock;
	KIRQL irql = KeAcquireSpinLockRaiseToDpc(IrpListLock);
	if (PIO_COMPLETION_CONTEXT CompletionContext = FileObject->CompletionContext)
	{
		if (IsListEmpty(&FileObject->IrpList))
		{
			ObfDereferenceObject(CompletionContext->Port);
			FileObject->Flags &= ~FO_SKIP_COMPLETION_PORT;

			if (Port)
			{
				ObfReferenceObject(Port);
				CompletionContext->Port = Port;
				CompletionContext->Key = Key;
			}
			else
			{
				ExFreePool(CompletionContext);
				FileObject->CompletionContext = 0;
				FileObject->Flags |= FO_QUEUE_IRP_TO_THREAD;
			}
			status = STATUS_SUCCESS;
		}
	}
	KeReleaseSpinLock(IrpListLock, irql);
	return status;
}