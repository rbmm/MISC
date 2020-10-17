typedef struct NOTIFY_ENTRY_HEADER : LIST_ENTRY {
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    ULONG SessionId; // if (MmIsSessionAddress(Callback)) SessionId = MmGetSessionIdEx(PsGetCurrentProcess())
    HANDLE hSession; // ZwOpenSession (\KernelObjects\Session<SessionId>)
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Callback;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    USHORT RefCount;
    BOOLEAN Unregistered;
    PFAST_MUTEX Lock;
    PERESOURCE Resource;
} *PNOTIFY_ENTRY_HEADER;

typedef struct DEVICE_CLASS_NOTIFY_ENTRY : NOTIFY_ENTRY_HEADER {
    GUID ClassGuid;
} *PDEVICE_CLASS_NOTIFY_ENTRY;

#define NOTIFY_DEVICE_CLASS_HASH_BUCKETS 13
#define PNP_NOTIFICATION_VERSION 1

inline ULONG IopHashGuid(LPCGUID Guid)
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


NTSTATUS
PnpNotifyDeviceClassChange(LPCGUID EventGuid,
                           LPCGUID ClassGuid,
                           PUNICODE_STRING SymbolicLinkName)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION notification = {
        PNP_NOTIFICATION_VERSION, sizeof(notification), *EventGuid, *ClassGuid, SymbolicLinkName
    };

    PLIST_ENTRY head = &PnpDeviceClassNotifyList[IopHashGuid(ClassGuid)], entry = head;

    ULONG SessionId = MAXULONG; // (0) bug !! ──────────────────────────────────────────────//┐
                                                                                            //│
    ULONG ServiceSessionId = PsGetServerSiloServiceSessionId(PsGetCurrentServerSilo());     //│
                                                                                            //│
    PDEVICE_CLASS_NOTIFY_ENTRY NotificationEntry = 0;                                       //│
                                                                                            //│
    goto __0;                                                                               //│
    do                                                                                      //│
    {                                                                                       //│
        NotificationEntry = static_cast<PDEVICE_CLASS_NOTIFY_ENTRY>(entry);                 //│
                                                                                            //│
        // ULONG SessionId = MAXULONG; // (0) must be here !! <─────────────────────────────//┘

        if (NotificationEntry->SessionId != ServiceSessionId) // (1)
        {
            SessionId = IopGetSessionIdFromSymbolicName(SymbolicLinkName);// (2)
        }

        NotificationEntry->RefCount++;

        ExReleaseFastMutex(&PnpDeviceClassNotifyLock);

        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(NotificationEntry->Resource, TRUE);

        if ((SessionId == MAXULONG || SessionId != NotificationEntry->SessionId) && // (3)
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

are NotificationEntry->Callback called depended from (3)
if (SessionId != MAXULONG && SessionId != NotificationEntry->SessionId)
will be no callback notify. so this is depended from SessionId variable.
but it value can be affected in (2) if previous NotificationEntry have (1).

so are callback will be called can depend from previous entry in list

*/