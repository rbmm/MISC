 struct _SILO_USER_SHARED_DATA {
	/*0000*/ ULONG ServiceSessionId;
	/*0004*/ ULONG ActiveConsoleId;
	/*0008*/ LONGLONG ConsoleSessionForegroundProcessId;
	/*0010*/ _NT_PRODUCT_TYPE NtProductType;
	/*0014*/ ULONG SuiteMask;
	/*0018*/ ULONG SharedUserSessionId;
	/*001c*/ UCHAR IsMultiSessionSku;
	/*001e*/ WCHAR NtSystemRoot[0x104];
	/*0226*/ USHORT UserModeGlobalLogger[0x10];
	/*0248*/
};

 struct _ESERVERSILO_GLOBALS {
	/*0000*/ _OBP_SILODRIVERSTATE ObSiloState;
	/*02e0*/ _SEP_SILOSTATE SeSiloState;
	/*0310*/ _SEP_RM_LSA_CONNECTION_STATE SeRmSiloState;
	/*0360*/ _ETW_SILODRIVERSTATE * EtwSiloState;
	/*0368*/ _EPROCESS * MiSessionLeaderProcess;
	/*0370*/ _EPROCESS * ExpDefaultErrorPortProcess;
	/*0378*/ void * ExpDefaultErrorPort;
	/*0380*/ ULONG HardErrorState;
	/*0388*/ _WNF_SILODRIVERSTATE WnfSiloState;
	/*03c0*/ _DBGK_SILOSTATE DbgkSiloState;
	/*03e0*/ _UNICODE_STRING PsProtectedCurrentDirectory;
	/*03f0*/ _UNICODE_STRING PsProtectedEnvironment;
	/*0400*/ void * ApiSetSection;
	/*0408*/ void * ApiSetSchema;
	/*0410*/ UCHAR OneCoreForwardersEnabled;
	/*0418*/ _UNICODE_STRING NtSystemRoot;
	/*0428*/ _UNICODE_STRING SiloRootDirectoryName;
	/*0438*/ _PSP_STORAGE * Storage;
	/*0440*/ _SERVERSILO_STATE State;
	/*0444*/ LONG ExitStatus;
	/*0448*/ _KEVENT * DeleteEvent;
	/*0450*/ _SILO_USER_SHARED_DATA * UserSharedData;
	/*0458*/ void * UserSharedSection;
	/*0460*/ _WORK_QUEUE_ITEM TerminateWorkItem;
	/*0480*/
};

ESERVERSILO_GLOBALS PspHostSiloGlobals;

BOOLEAN PsIsServerSilo(EJOB* Silo)
{
  return Silo ? Silo->ServerSiloGlobals != 0 : TRUE;
}

EJOB* PsGetCurrentServerSilo()
{
  PETHREAD Thread = PsGetCurrentThread();
  
  switch(EJOB* Silo = Thread->Silo)
  {
  case -3:
    return Thread->Process->ServerSilo;
  case 0:
    return 0;
  default:
    while (!PsIsServerSilo(Silo)) Silo = Silo->ParentJob;
    return Silo;
  }  
}

ESERVERSILO_GLOBALS* PsGetServerSiloGlobals(EJOB* Silo)
{
  if (Silo == -1) Silo = PsGetCurrentServerSilo();
  
  if (Silo)
  {
    return Silo->ServerSiloGlobals;
  }
  
  return &PspHostSiloGlobals;
}

ULONG PsGetServerSiloServiceSessionId(EJOB* Silo)
{
  PsGetServerSiloGlobals(Silo)->UserSharedData->ServiceSessionId;
}
