#pragma once

//struct IO_MINI_COMPLETION_PACKET_USER {
//	LIST_ENTRY ListEntry;
//	ULONG PacketType;
//	PVOID KeyContext;
//	PVOID ApcContext;
//	NTSTATUS IoStatus;
//	ULONG_PTR IoStatusInformation;
//	void (NTAPI * MiniPacketCallback)( IO_MINI_COMPLETION_PACKET_USER * , PVOID Context );
//	PVOID Context;
//	BOOLEAN Allocated;
//};

struct IO_MINI_COMPLETION_PACKET_USER;

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
typedef
VOID
NTAPI
IO_MINI_COMPLETION_PACKET_CALLBACK (_In_ IO_MINI_COMPLETION_PACKET_USER * NotificationPacket, _In_ PVOID Context);

typedef IO_MINI_COMPLETION_PACKET_CALLBACK *PIO_MINI_COMPLETION_PACKET_CALLBACK;


EXTERN_C
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
IO_MINI_COMPLETION_PACKET_USER* 
NTAPI
IoAllocateMiniCompletionPacket(
							   PIO_MINI_COMPLETION_PACKET_CALLBACK MiniPacketCallback,
							   PVOID Context
							   );

EXTERN_C
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
IoSetIoCompletionEx (
					 IN PVOID IoCompletion,
					 IN PVOID KeyContext,
					 IN PVOID ApcContext,
					 IN NTSTATUS IoStatus,
					 IN ULONG_PTR IoStatusInformation,
					 IN BOOLEAN Quota,
					 IN IO_MINI_COMPLETION_PACKET_USER * NotificationPacket
					 );
