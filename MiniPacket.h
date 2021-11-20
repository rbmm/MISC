#pragma once

struct IO_MINI_COMPLETION_PACKET_USER {
	LIST_ENTRY ListEntry;
	ULONG PacketType;
	PVOID KeyContext;
	PVOID ApcContext;
	NTSTATUS IoStatus;
	ULONG_PTR IoStatusInformation;
	void (NTAPI * MiniPacketCallback)( IO_MINI_COMPLETION_PACKET_USER * , PVOID Context );
	PVOID Context;
	BOOLEAN Allocated;
};

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
typedef
VOID
NTAPI
IO_MINI_COMPLETION_PACKET_CALLBACK (_In_ IO_MINI_COMPLETION_PACKET_USER * NotificationPacket, _In_ PVOID Context);

typedef IO_MINI_COMPLETION_PACKET_CALLBACK *PIO_MINI_COMPLETION_PACKET_CALLBACK;

EXTERN_C_START

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
IO_MINI_COMPLETION_PACKET_USER* 
NTAPI
IoAllocateMiniCompletionPacket(
	_In_ PIO_MINI_COMPLETION_PACKET_CALLBACK MiniPacketCallback,
	_In_ PVOID Context
	);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
void 
NTAPI
IoInitializeMiniCompletionPacket(
	_In_ IO_MINI_COMPLETION_PACKET_USER * NotificationPacket,
	_In_ PIO_MINI_COMPLETION_PACKET_CALLBACK MiniPacketCallback,
	_In_ PVOID Context);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoSetIoCompletionEx (
	_In_ PVOID IoCompletion,
	_In_ PVOID KeyContext,
	_In_ PVOID ApcContext,
	_In_ NTSTATUS IoStatus,
	_In_ ULONG_PTR IoStatusInformation,
	_In_ BOOLEAN Quota,
	_In_ IO_MINI_COMPLETION_PACKET_USER * NotificationPacket
	);

EXTERN_C_END