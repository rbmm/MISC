_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS NTAPI IopReplaceCompletionPort(_In_ PFILE_OBJECT FileObject, _In_ PVOID Port, _In_ PVOID Key)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	KIRQL irql = KeAcquireSpinLockRaiseToDpc(&FileObject->IrpListLock);
	if (PIO_COMPLETION_CONTEXT CompletionContext = FileObject->CompletionContext)
	{
		if (IsListEmpty(&FileObject->IrpList) && !CompletionContext->UsageCount)
		{
			ObfDereferenceObjectWithTag(CompletionContext->Port, 'tlfD');
			FileObject->Flags &= ~(FO_SKIP_COMPLETION_PORT|FO_SKIP_SET_EVENT|FO_SKIP_SET_FAST_IO);

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
			}
			status = STATUS_SUCCESS;
		}
	}
	KeReleaseSpinLock(&FileObject->IrpListLock, irql);
	return status;
}