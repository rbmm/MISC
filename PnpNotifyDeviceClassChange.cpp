typedef struct NOTIFY_ENTRY_HEADER : LIST_ENTRY {
	IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
	ULONG SessionId;
	HANDLE SessionHandle;
	PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine;
	PVOID Context;
	PDRIVER_OBJECT DriverObject;
	USHORT RefCount;
	BOOLEAN Unregistered;
	PKGUARDED_MUTEX Lock;
	PERESOURCE EntryLock;
} *PNOTIFY_ENTRY_HEADER;

typedef struct DEVICE_CLASS_NOTIFY_ENTRY : NOTIFY_ENTRY_HEADER {
    GUID ClassGuid;
} *PDEVICE_CLASS_NOTIFY_ENTRY;

#define NOTIFY_DEVICE_CLASS_HASH_BUCKETS 13
#define PNP_NOTIFICATION_VERSION 1

inline ULONG PnpHashGuid(LPCGUID Guid)
{
    return (((PULONG)Guid)[0] + ((PULONG)Guid)[1] + ((PULONG)Guid)[2]
    + ((PULONG)Guid)[3]) % NOTIFY_DEVICE_CLASS_HASH_BUCKETS;
}

inline BOOLEAN IopCompareGuid(IN LPCGUID guid1, IN LPCGUID guid2)
{
    return guid1 == guid2 || RtlCompareMemory( guid1, guid2, sizeof(GUID) ) == sizeof(GUID);
}

FAST_MUTEX PnpDeviceClassNotifyLock;
LIST_ENTRY PnpDeviceClassNotifyList[NOTIFY_DEVICE_CLASS_HASH_BUCKETS];

ULONG IopGetSessionIdFromSymbolicName(PUNICODE_STRING SymbolicLinkName);

void PnpDereferenceNotify(PNOTIFY_ENTRY_HEADER NotificationEntry);

void PnpNotifyDriverCallback(PDEVICE_CLASS_NOTIFY_ENTRY NotificationEntry, 
                             PDEVICE_INTERFACE_CHANGE_NOTIFICATION notification, 
                             PNTSTATUS pstatus);


#define INVALID_SESSION MAXULONG

NTSTATUS
PnpNotifyDeviceClassChange(_In_ LPCGUID EventGuid,
                           _In_ LPCGUID ClassGuid,
                           _In_ PUNICODE_STRING SymbolicLinkName)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION notification = {
        PNP_NOTIFICATION_VERSION, sizeof(notification), *EventGuid, *ClassGuid, SymbolicLinkName
    };

    PLIST_ENTRY head = &PnpDeviceClassNotifyList[PnpHashGuid(ClassGuid)], entry = head;

    ULONG SessionId = INVALID_SESSION; // (0) bug !! ───────────────────────────────────────//┐
                                                                                            //│                                                                                           //│
    PDEVICE_CLASS_NOTIFY_ENTRY NotificationEntry = 0;                                       //│
                                                                                            //│
    goto __0;                                                                               //│
    do                                                                                      //│
    {                                                                                       //│
        NotificationEntry = static_cast<PDEVICE_CLASS_NOTIFY_ENTRY>(entry);                 //│
                                                                                            //│
        // ULONG SessionId = INVALID_SESSION; // (0) must be here !! <──────────────────────//┘

        if (NotificationEntry->SessionId != PsGetServerSiloServiceSessionId(PsGetCurrentServerSilo())) // (1)
        {
            SessionId = IopGetSessionIdFromSymbolicName(SymbolicLinkName);// (2)
        }

        NotificationEntry->RefCount++;

        ExReleaseFastMutex(&PnpDeviceClassNotifyLock);

        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(NotificationEntry->Resource, TRUE);

        if ((SessionId == INVALID_SESSION || SessionId != NotificationEntry->SessionId) && // (3)
            !NotificationEntry->Unregistered && 
            IopCompareGuid(ClassGuid, &NotificationEntry->ClassGuid))
        {
            NTSTATUS status;
            PnpNotifyDriverCallback(NotificationEntry, &notification, &status);
        }

        ExReleaseResourceLite(NotificationEntry->Resource);
        KeLeaveCriticalRegion();

__0:
        ExAcquireFastMutex(&PnpDeviceClassNotifyLock);
        entry = entry->Flink;
        if (NotificationEntry) PnpDereferenceNotify(NotificationEntry);

    } while (entry != head);

    ExReleaseFastMutex(&PnpDeviceClassNotifyLock);
}

/*

are NotificationEntry->CallbackRoutine called depended from (3)
if (SessionId != INVALID_SESSION && SessionId != NotificationEntry->SessionId)
will be no callback notify. so this is depended from SessionId variable.
but it value can be affected in (2) if previous NotificationEntry have (1).

so are callback will be called can depend from previous entry in list

example:
we register for GUID_DEVINTERFACE_KEYBOARD notification

IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
    const_cast<GUID*>(&GUID_DEVINTERFACE_KEYBOARD), DriverObject,
    KbdNotifyCallback, 0, &NotificationEntry);

the win32k[base].sys register GUID_DEVINTERFACE_KEYBOARD notification (RIMDeviceClassNotify) too and before us

RDP session connected

PnpNotifyDeviceClassChange(GUID_DEVICE_INTERFACE_ARRIVAL, GUID_DEVINTERFACE_KEYBOARD, SymbolicLinkName) called
SymbolicLinkName usualy \??\TERMINPUT_BUS#UMB#...&Session<X>Keyboard<Y>#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}

win32k[base].sys in session space - SessionId != 0

as result, after RIMDeviceClassNotify called,

SessionId changed from -1 (0xFFFFFFFF) to IopGetSessionIdFromSymbolicName(SymbolicLinkName) (!= 0)

and next entry (KbdNotifyCallback) will be not called,
because now SessionId != MAXULONG && SessionId != NotificationEntry->SessionId (==0)

--------------------------------------------------------
from another side

PsGetServerSiloServiceSessionId(PsGetCurrentServerSilo())

we can move out (before) of loop

ULONG ServiceSessionId = PsGetServerSiloServiceSessionId(PsGetCurrentServerSilo());


if (NotificationEntry->SessionId != ServiceSessionId) // (1)
*/