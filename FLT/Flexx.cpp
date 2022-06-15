// Flexx.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

_NT_BEGIN

PDRIVER_OBJECT g_DriverObject;
PFLT_FILTER g_Filter;

BOOLEAN InitFilters(PUNICODE_STRING DriverKey);
void DeleteFilters();
BOOLEAN IsProtectPath(PUNICODE_STRING Name);

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
	DeleteFilters();
	DbgPrint("DriverUnload(%p)\n", DriverObject);
}

NTSTATUS FilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	DbgPrint("FilterUnload(%x) from %p\n", Flags, _ReturnAddress());

	DbgPrint("FltUnregisterFilter(%p)\n", g_Filter);

	if (g_Filter) 
	{
		FltUnregisterFilter(g_Filter);
	}
	else
	{
		DriverUnload(g_DriverObject);
	}

	return STATUS_SUCCESS;
}

NTSTATUS InstanceSetup(
					   IN PCFLT_RELATED_OBJECTS FltObjects,
					   IN FLT_INSTANCE_SETUP_FLAGS /*Flags*/,
					   IN DEVICE_TYPE VolumeDeviceType,
					   IN FLT_FILESYSTEM_TYPE VolumeFilesystemType
					   )
{
	CONST PFLT_VOLUME Volume = FltObjects->Volume; 

	DbgPrint("InstanceSetup(%p %p, %x, %x)\n", FltObjects->Instance, Volume, VolumeDeviceType, VolumeFilesystemType);

	return VolumeDeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ? STATUS_SUCCESS : STATUS_FLT_DO_NOT_ATTACH;
}

void PrintFileName(PFLT_FILE_NAME_INFORMATION FileNameInformation, PFILE_OBJECT FileObject)
{
	DbgPrint(">%wZ\n%p::%wZ\n", 
		&FileNameInformation->Name,
		FileObject->RelatedFileObject, FileObject->FileName
		);
}

FLT_PREOP_CALLBACK_STATUS PreCreate(
									IN OUT PFLT_CALLBACK_DATA Data,
									IN PCFLT_RELATED_OBJECTS FltObjects,
									OUT PVOID * /*CompletionContext*/
									)
{
	PFLT_PARAMETERS Parameters = &Data->Iopb->Parameters;

	ULONG Options = Parameters->Create.Options;
	ULONG CreateDisposition = Options >> 24;

	Options &= 0x00ffffff;

	PIO_SECURITY_CONTEXT SecurityContext = Parameters->Create.SecurityContext;
	PACCESS_STATE AccessState = SecurityContext->AccessState;

	ULONG DesiredAccess = SecurityContext->DesiredAccess;

	enum { 
		_ModifyAccess = DELETE|FILE_DELETE_CHILD|FILE_WRITE_DATA|FILE_APPEND_DATA|FILE_WRITE_ATTRIBUTES|FILE_WRITE_EA
	};

	if ((CreateDisposition == FILE_CREATE) || !(DesiredAccess & _ModifyAccess))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	Data->IoStatus.Status = STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT;
	Data->IoStatus.Information = 0;

	ULONG ProcessId = FltGetRequestorProcessId(Data);

	BOOLEAN bProtect = FALSE;

	if (KeGetCurrentIrql() <= APC_LEVEL)
	{
		PFLT_FILE_NAME_INFORMATION FileNameInformation;
		
		NTSTATUS status = FltGetFileNameInformation(Data, 
			FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &FileNameInformation);
		
		if (0 <= status)
		{
			if (bProtect = IsProtectPath(&FileNameInformation->Name))
			{
				DbgPrint("++[%x]>(%08x) %08x | %08x\n", 
					ProcessId, DesiredAccess, Options, CreateDisposition);

				PrintFileName(FileNameInformation, FltObjects->FileObject);
			}
			
			FltReleaseFileNameInformation(FileNameInformation);
		}
		else
		{
			DbgPrint("FltGetFileNameInformation=%x < %wZ >\n", status, &FltObjects->FileObject->FileName);
		}
	}

	if (bProtect)
	{
		if (Options & FILE_DELETE_ON_CLOSE)
		{
			DbgPrint("block !! DELETE_ON_CLOSE\n");
			return FLT_PREOP_COMPLETE;
		}

		switch (CreateDisposition)
		{
		case FILE_OPEN_IF:
		case FILE_OVERWRITE_IF:
			Parameters->Create.Options = Options | (FILE_CREATE << 24);
			FltSetCallbackDataDirty(Data);
			DbgPrint("!! Disposition %x -> CREATE\n", CreateDisposition);
			break;

		case FILE_OPEN:
			// not need call FltSetCallbackDataDirty(Data) when we modify data from SecurityContext->
			// because we not change this pointer(!) inside IO_STACK_LOCATION
			// we change data by pointer
			SecurityContext->DesiredAccess &= ~_ModifyAccess;
			AccessState->PreviouslyGrantedAccess &= ~_ModifyAccess;
			AccessState->RemainingDesiredAccess &= ~_ModifyAccess;
			DbgPrint("!! ModifyAccess -> %x\n", SecurityContext->DesiredAccess);
			break;
		default:
			DbgPrint("block !! %x\n", CreateDisposition);
			return FLT_PREOP_COMPLETE;
		}
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS RegisterFilter(PUNICODE_STRING ObjectName)
{
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, ObjectName, OBJ_CASE_INSENSITIVE };

	STATIC_UNICODE_STRING_(Instances);
	OBJECT_ATTRIBUTES oa1 = { sizeof(oa1), 0, const_cast<PUNICODE_STRING>(&Instances), OBJ_CASE_INSENSITIVE };

	NTSTATUS status;

	if (0 <= (status = ZwOpenKey(&oa1.RootDirectory, KEY_ALL_ACCESS, &oa)))
	{
		STATIC_UNICODE_STRING_(DefaultInstance);
		OBJECT_ATTRIBUTES oa2 = { sizeof(oa2), 0, const_cast<PUNICODE_STRING>(&DefaultInstance), OBJ_CASE_INSENSITIVE };

		if (0 <= (status = ZwCreateKey(&oa2.RootDirectory, KEY_ALL_ACCESS, &oa1, 0, 0, 0, 0)))// \Instances
		{
			if (0 <= (status = ZwSetValueKey(oa2.RootDirectory, &DefaultInstance, 0, REG_SZ, DefaultInstance.Buffer, DefaultInstance.MaximumLength)))
			{
				if (0 <= (status = ZwCreateKey(&oa.RootDirectory, KEY_ALL_ACCESS, &oa2, 0, 0, 0, 0)))// \Instances\DefaultInstance
				{
					STATIC_UNICODE_STRING_(Flags);
					STATIC_UNICODE_STRING_(Altitude);
					STATIC_WSTRING(alt, "370000");

					static ULONG f;
					0 <= (status = ZwSetValueKey(oa.RootDirectory, &Flags, 0, REG_DWORD, &f, sizeof(f))) &&
						0 <= (status = ZwSetValueKey(oa.RootDirectory, &Altitude, 0, REG_SZ, (void*)alt, sizeof(alt)));

					ZwClose(oa.RootDirectory);
				}
			}
			ZwClose(oa2.RootDirectory);
		}
		ZwClose(oa1.RootDirectory);
	}

	return status;
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING DriverKey)
{
	//initterm();

	g_DriverObject = DriverObject;

	DbgPrint("%s:DriverEntry(%p) %p %wZ\n", __FUNCTION__, DriverObject, DriverEntry, &DriverObject->DriverName);

	DriverObject->DriverUnload = DriverUnload;

	RegisterFilter(DriverKey);

	static const FLT_OPERATION_REGISTRATION Callbacks[] = {
		{ IRP_MJ_CREATE, 0, PreCreate },
		{ IRP_MJ_OPERATION_END }
	};

	static const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {
		{ FLT_CONTEXT_END }
	};

	static FLT_REGISTRATION fr = {
		sizeof(FLT_REGISTRATION), FLT_REGISTRATION_VERSION, 0, ContextRegistration, Callbacks, FilterUnload, InstanceSetup
	};

	NTSTATUS status = STATUS_INVALID_PARAMETER;

	if (
		!InitFilters(DriverKey) ||
		0 > (status = FltRegisterFilter(DriverObject, &fr, &g_Filter)) ||
		0 > (status = FltStartFiltering(g_Filter))
		)
	{
		FilterUnload(MAXDWORD);
	}

	DbgPrint("error=%x, unl=%p\n", status, DriverObject->DriverUnload);

	return status;
}

_NT_END