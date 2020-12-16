#include "stdafx.h"

_NT_BEGIN

#include <ntlsa.h>
#include "../inc/ntsam2.h"

class WLog
{
  PVOID _BaseAddress;
  ULONG _RegionSize, _Ptr;

  PWSTR _buf()
  {
    return (PWSTR)((PBYTE)_BaseAddress + _Ptr);
  }

  ULONG _cch()
  {
    return (_RegionSize - _Ptr) / sizeof(WCHAR);
  }

public:
  ULONG Init(SIZE_T RegionSize)
  {
    if (_BaseAddress = new BYTE[RegionSize])
    {
      _RegionSize = (ULONG)RegionSize, _Ptr = 0;
      return NOERROR;
    }
    return GetLastError();
  }

  ~WLog()
  {
    if (_BaseAddress)
    {
      delete [] _BaseAddress;
    }
  }

  WLog(WLog&&) = delete;
  WLog(WLog&) = delete;
  WLog(): _BaseAddress(0) {  }

  operator PCWSTR()
  {
    return (PCWSTR)_BaseAddress;
  }

  WLog& operator ()(PCWSTR format, ...)
  {
    va_list args;
    va_start(args, format);

    int len = _vsnwprintf_s(_buf(), _cch(), _TRUNCATE, format, args);

    if (0 < len)
    {
      _Ptr += len * sizeof(WCHAR);
    }

    va_end(args);

    return *this;
  }

  WLog& operator[](HRESULT dwError)
  {
    LPCVOID lpSource = 0;
    ULONG dwFlags = FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS;

    if (dwError & FACILITY_NT_BIT)
    {
      dwError &= ~FACILITY_NT_BIT;
      dwFlags = FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS;

      static HMODULE ghnt;
      if (!ghnt && !(ghnt = GetModuleHandle(L"ntdll"))) return *this;
      lpSource = ghnt;
    }

    FormatMessageW(dwFlags, lpSource, dwError, 0, _buf(), _cch(), 0);
    return *this;
  }
};

PCSTR GetSidNameUseName(SID_NAME_USE snu)
{
  switch (snu)
  {
  case SidTypeUser: return "User";
  case SidTypeGroup: return "Group";
  case SidTypeDomain: return "Domain";
  case SidTypeAlias: return "Alias";
  case SidTypeWellKnownGroup: return "WellKnownGroup";
  case SidTypeDeletedAccount: return "DeletedAccount";
  case SidTypeInvalid: return "Invalid";
  case SidTypeUnknown: return "Unknown";
  case SidTypeComputer: return "Computer";
  case SidTypeLabel: return "Label";
  case SidTypeLogonSession: return "LogonSession";
  }
  return "?";
}

void DumpSid(WLog& log, PSID Sid)
{
  UNICODE_STRING StringSid;
  if (0 <= RtlConvertSidToUnicodeString(&StringSid, Sid, TRUE))
  {
    log(L"%wZ", &StringSid);
    RtlFreeUnicodeString(&StringSid);
  }
}

void DisplayAliases(WLog& log, SAM_HANDLE DomainHandle)
{
  log(L"\t\t<Aliases>\r\n");
  NTSTATUS status;
  ULONG CountReturned;
  SAM_ENUMERATE_HANDLE EnumerationContext = 0;
  PVOID Buffer;

  do 
  {
    if (0 <= (status = SamEnumerateAliasesInDomain(
      DomainHandle,
      &EnumerationContext,
      &Buffer, // 
      0x10000,
      &CountReturned
      )))
    {
      if (CountReturned)
      {
        PSAM_RID_ENUMERATION psre = (PSAM_RID_ENUMERATION)Buffer;
        do 
        {
          log(L"\t\t\t<%08x '%wZ' />\r\n", psre->RelativeId, &psre->Name);
        } while (psre++, --CountReturned);
      }
      SamFreeMemory(Buffer);
    }
    else
    {
      log(L"!!! EnumerateAliasesInDomain = %x\r\n", status)[status];
    }
  } while (status == STATUS_MORE_ENTRIES);
  log(L"\t\t</Aliases>\r\n");
}

void DisplayGroups(WLog& log, SAM_HANDLE DomainHandle)
{
  log(L"\t\t<Groups>\r\n");
  NTSTATUS status;
  ULONG Index = 0, TotalAvailable, TotalReturned, ReturnedEntryCount;

  do 
  {
    PVOID Buffer;
    if (0 <= (status = SamQueryDisplayInformation(DomainHandle, DomainDisplayGroup, 
      Index, 0x40, 0x10000, &TotalAvailable, &TotalReturned, &ReturnedEntryCount, &Buffer)))
    {
      if (ReturnedEntryCount)
      {
        PDOMAIN_DISPLAY_GROUP pdg = (PDOMAIN_DISPLAY_GROUP)Buffer;
        do 
        {
          log(L"\t\t\n<%08x '[%08x]' '%wZ' '%wZ' />\r\n", pdg->Rid, pdg->Attributes, &pdg->Group, &pdg->Comment);

          Index = pdg->Index;

        } while (pdg++, --ReturnedEntryCount);
      }
      SamFreeMemory(Buffer);
    }
    else
    {
      log(L"!!! DomainDisplayGroup = %x\r\n", status)[status];
    }

  } while (status == STATUS_MORE_ENTRIES);
  log(L"\t\t</Groups>\r\n");
}

void DisplayUsers(WLog& log, SAM_HANDLE DomainHandle)
{
  log(L"\t\t<Users>\r\n");
  NTSTATUS status;
  ULONG Index = 0, TotalAvailable, TotalReturned, ReturnedEntryCount;

  do 
  {
    PVOID Buffer;
    if (0 <= (status = SamQueryDisplayInformation(DomainHandle, DomainDisplayUser, 
      Index, 0x40, 0x10000, &TotalAvailable, &TotalReturned, &ReturnedEntryCount, &Buffer)))
    {
      if (ReturnedEntryCount)
      {
        PDOMAIN_DISPLAY_USER pdu = (PDOMAIN_DISPLAY_USER)Buffer;
        do 
        {
          log(L"\t\t\t<%08x '[%08x]' '%wZ' '%wZ' />\r\n", pdu->Rid, pdu->AccountControl, &pdu->LogonName, &pdu->FullName);

          Index = pdu->Index;

        } while (pdu++, --ReturnedEntryCount);
      }
      SamFreeMemory(Buffer);
    }
    else
    {
      log(L"!!! DisplayUsers = %x\r\n", status)[status];
    }

  } while (status == STATUS_MORE_ENTRIES);
  log(L"\t\t</Users>\r\n");
}

NTSTATUS QueryUser(WLog& log, SAM_HANDLE DomainHandle, ULONG UserId)
{
  SAM_HANDLE UserHandle, GroupHandle;

  NTSTATUS status = SamOpenUser(DomainHandle, USER_READ, UserId, &UserHandle);

  if (0 <= status)
  {
    ULONG MembershipCount;

    PGROUP_MEMBERSHIP Groups;

    status = SamGetGroupsForUser(UserHandle, &Groups, &MembershipCount);

    if (0 <= status)
    {
      log(L"\t\t<GroupsForUser>\r\n");

      PVOID buf = Groups;

      if (MembershipCount)
      {
        do 
        {
          status = SamOpenGroup(DomainHandle, GROUP_READ|GROUP_EXECUTE, Groups->RelativeId, &GroupHandle);
          
          if (0 <= status)
          {
            PGROUP_NAME_INFORMATION GroupName;
            
            status = SamQueryInformationGroup(GroupHandle, GroupNameInformation, (void**)&GroupName);
            
            if (0 <= status)
            {
              log(L"\t\t\t<%08x '[%08x]' '%wZ' />\r\n", Groups->RelativeId, Groups->Attributes, GroupName);
              SamFreeMemory(GroupName);
            }
            else
            {
              log(L"!!! GroupNameInformation = %x\r\n", status)[status];
            }
            SamCloseHandle(GroupHandle);
          }
          else
          {
            log(L"!!! OpenGroup(%08x) = %x\r\n", Groups->RelativeId, status)[status];
          }

        } while (Groups++, --MembershipCount);
      }

      SamFreeMemory(buf);
      log(L"\t\t</GroupsForUser>\r\n");
    }
    else
    {
      log(L"GetGroupsForUser(%08x) = %x\r\n", UserId, status)[status];
    }

    SamCloseHandle(UserHandle);
  }
  else
  {
    log(L"OpenUser(%08x) = %x\r\n", UserId, status)[status];
  }

  return status;
}

NTSTATUS QuerySam(WLog& log, PUNICODE_STRING ServerName, PUNICODE_STRING DomainName, PUNICODE_STRING UserName)
{
  SAM_HANDLE ServerHandle, DomainHandle;

  NTSTATUS status = SamConnect(ServerName, &ServerHandle, SAM_SERVER_ENUMERATE_DOMAINS|SAM_SERVER_LOOKUP_DOMAIN, 0);

  if (0 <= status)
  {
    log(L"<Server '%wZ'>\r\n", ServerName);

    PSID DomainSid;

    status = SamLookupDomainInSamServer(ServerHandle, DomainName, &DomainSid);

    if (0 <= status)
    {
      log(L"\tDomainSid=");
      DumpSid(log, DomainSid);
      log(L"\r\n");

      status = SamOpenDomain(ServerHandle, DOMAIN_READ|DOMAIN_EXECUTE, DomainSid, &DomainHandle);

      UCHAR SubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid);
      ULONG DestinationSidLength = RtlLengthRequiredSid(SubAuthorityCount + 1);

      PSID UserSid = alloca(DestinationSidLength);
      RtlCopySid(DestinationSidLength, UserSid, DomainSid);
      ++*RtlSubAuthorityCountSid(UserSid);
      PULONG pUserRid = RtlSubAuthoritySid(UserSid, SubAuthorityCount);

      SamFreeMemory(DomainSid);

      if (0 <= status)
      {
        log(L"\t<Domain '%wZ'>\r\n", DomainName);

        DisplayAliases(log, DomainHandle);
        DisplayGroups(log, DomainHandle);
        DisplayUsers(log, DomainHandle);

        SID_NAME_USE *pUse, Use;
        ULONG UserId, *UserRid;

        status = SamLookupNamesInDomain(DomainHandle, 1, UserName, &UserRid, &pUse);

        log(L"\t<!-- Lookup(%wZ) -->\r\n", UserName);

        if (0 <= status)
        {
          Use = *pUse, *pUserRid = UserId = *UserRid;
          SamFreeMemory(pUse);
          SamFreeMemory(UserRid);

          log(L"\t\t<User id='%08x' use='%S'>\r\n", UserId, GetSidNameUseName(Use));

          status = STATUS_NOT_FOUND;

          if (Use == SidTypeUser)
          {
            status = QueryUser(log, DomainHandle, UserId);
          }

          log(L"\t\t</User>\r\n");
        }
        else
        {
          log(L"Lookup(%wZ) = %x\r\n", UserName, status)[status];
        }

        SamCloseHandle(DomainHandle);

        log(L"\t</Domain '%wZ'>\r\n", DomainName);

        if (0 <= status)
        {
          DomainSid = alloca(RtlLengthRequiredSid(1));
          static const SID_IDENTIFIER_AUTHORITY IdentifierAuthority = SECURITY_NT_AUTHORITY;
          InitializeSid(DomainSid, const_cast<SID_IDENTIFIER_AUTHORITY*>(&IdentifierAuthority), 1);
          *RtlSubAuthoritySid(DomainSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;

          status = SamOpenDomain(ServerHandle, DOMAIN_READ|DOMAIN_EXECUTE, DomainSid, &DomainHandle);

          if (0 <= status)
          {
            log(L"\t<BUILTIN>\r\n");

            DisplayAliases(log, DomainHandle);

            ULONG n;
            PULONG Aliases;
            status = SamGetAliasMembership(DomainHandle, 1, &UserSid, &n, &Aliases);

            if (0 <= status)
            {
              log(L"\t\t<AliasMembership for='%wZ' >\r\n", UserName);

              if (n)
              {
                do 
                {
                  log(L"\t\t\t<%08x />\r\n", Aliases[--n]);
                } while (n);
              }
              log(L"\t\t</AliasMembership>\r\n");

              SamFreeMemory(Aliases);
            }
            else
            {
              log(L"GetAliasMembership = %x\r\n", status)[status];
            }

            SamCloseHandle(DomainHandle);

            log(L"\t</BUILTIN>\r\n");
          }
          else
          {
            log(L"OpenDomain(%wZ\\BUILTIN) = %x\r\n", ServerName, status)[status];
          }
        }
      }
      else
      {
        log(L"OpenDomain(%wZ) = %x\r\n", DomainName, status)[status];
      }
    }
    else
    {
      log(L"LookupDomain(%wZ) = %x\r\n", DomainName, status)[status];
    }

    SamCloseHandle(ServerHandle);

    log(L"</Server '%wZ'>\r\n", ServerName);
  }
  else
  {
    log(L"Connect(%wZ) = %x\r\n", ServerName, status)[status];
  }

  return status;
}

class LSA
{
  UNICODE_STRING _CN, _DN, _DNS;
  bool _isDC;
public:
  LSA()
  {
    RtlInitUnicodeString(&_CN, 0);
    RtlInitUnicodeString(&_DN, 0);
    RtlInitUnicodeString(&_DNS, 0);
  }

  ~LSA()
  {
    RtlFreeUnicodeString(&_CN);
    RtlFreeUnicodeString(&_DN);
    RtlFreeUnicodeString(&_DNS);
  }

  NTSTATUS Init(WLog& log)
  {
    _isDC = false;

    LSA_HANDLE PolicyHandle;

    LSA_OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) };

    NTSTATUS status = LsaOpenPolicy(0, &ObjectAttributes, POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);

    if (0 <= status)
    {
      union {
        PVOID buf;
        PPOLICY_DNS_DOMAIN_INFO ppdi;
        PPOLICY_ACCOUNT_DOMAIN_INFO padi;
      };

      status = LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation, &buf);

      if (0 <= status)
      {
        log(L"<!-- CN='%wZ' -->\r\n", &padi->DomainName);

        status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &padi->DomainName, &_CN);

        LsaFreeMemory(buf);

        if (0 <= status)
        {
          status = LsaQueryInformationPolicy(PolicyHandle, PolicyDnsDomainInformation, &buf);

          if (0 <= status)
          {
            if (ppdi->Sid)
            {
              log(L"<!-- DN='%wZ' Dns='%wZ' -->\r\n", &ppdi->Name, &ppdi->DnsDomainName);

              if (0 <= (status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &ppdi->Name, &_DN)))
              {
                _isDC = RtlEqualUnicodeString(&_CN, &_DN, TRUE);

                status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &ppdi->DnsDomainName, &_DNS);
              }
            }

            LsaFreeMemory(buf);
          }
          else
          {
            log(L"PolicyDnsDomainInformation=%x\r\n", status)[status];
          }
        }
      }
      else
      {
        log(L"PolicyAccountDomainInformation=%x\r\n", status)[status];
      }

      LsaClose(PolicyHandle);
    }
    else
    {
      log(L"OpenPolicy=%x\r\n", status)[status];
    }

    return status;
  }

  PCUNICODE_STRING operator[](PCUNICODE_STRING DomainName)
  {
    if (!DomainName || !DomainName->Length)
    {
      return _isDC ? &_DN : &_CN;
    }

    if (DomainName->Length == sizeof(WCHAR) && *DomainName->Buffer == '.')
    {
      return &_CN;
    }

    if (RtlEqualUnicodeString(DomainName, &_DNS, TRUE))
    {
      return &_DN;
    }

    return DomainName;
  }

  PCUNICODE_STRING operator()(PCUNICODE_STRING DomainName)
  {
    return RtlEqualUnicodeString(DomainName, &_DN, TRUE) ? &_DNS : DomainName;
  }
};

NTSTATUS QuerySam(WLog& log, LSA& policy, PCWSTR szUserName, PCWSTR szDomain)
{
  UNICODE_STRING UserName, DomainName;
  RtlInitUnicodeString(&UserName, szUserName);
  RtlInitUnicodeString(&DomainName, szDomain);

  log(L"\r\n<!-- %wZ\\%wZ -->\r\n\r\n", &DomainName, &UserName);

  PCUNICODE_STRING pcDomainName = policy[&DomainName];

  return QuerySam(log, const_cast<PUNICODE_STRING>(policy(pcDomainName)), const_cast<PUNICODE_STRING>(pcDomainName), &UserName);
}

#define LAA(se) {{se},SE_PRIVILEGE_ENABLED|SE_PRIVILEGE_ENABLED_BY_DEFAULT}

#define BEGIN_PRIVILEGES(tp, n) static const struct {ULONG PrivilegeCount;LUID_AND_ATTRIBUTES Privileges[n];} tp = {n,{
#define END_PRIVILEGES }};

static OBJECT_ATTRIBUTES zoa = { sizeof(zoa) };

NTSTATUS GetSystemToken(PVOID buf)
{
  NTSTATUS status;

  union {
    PVOID pv;
    PBYTE pb;
    PSYSTEM_PROCESS_INFORMATION pspi;
  };

  pv = buf;
  ULONG NextEntryOffset = 0;

  do 
  {
    pb += NextEntryOffset;

    HANDLE hProcess, hToken, hNewToken;

    if (pspi->InheritedFromUniqueProcessId && pspi->UniqueProcessId && pspi->NumberOfThreads)
    {
      static SECURITY_QUALITY_OF_SERVICE sqos = {
        sizeof sqos, SecurityImpersonation, SECURITY_DYNAMIC_TRACKING, FALSE
      };

      static OBJECT_ATTRIBUTES soa = { sizeof(soa), 0, 0, 0, 0, &sqos };

      if (0 <= NtOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &zoa, &pspi->TH->ClientId))
      {
        status = NtOpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken);

        NtClose(hProcess);

        if (0 <= status)
        {
          status = NtDuplicateToken(hToken, TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE, 
            &soa, FALSE, TokenImpersonation, &hNewToken);

          NtClose(hToken);

          if (0 <= status)
          {
            BEGIN_PRIVILEGES(tp, 2)
              LAA(SE_TCB_PRIVILEGE),
              LAA(SE_DEBUG_PRIVILEGE),
            END_PRIVILEGES  

            status = NtAdjustPrivilegesToken(hNewToken, FALSE, (PTOKEN_PRIVILEGES)&tp, 0, 0, 0);

            if (STATUS_SUCCESS == status) 
            {
              status = ZwSetInformationThread(NtCurrentThread(), ThreadImpersonationToken, &hNewToken, sizeof(hNewToken));
            }

            NtClose(hNewToken);

            if (STATUS_SUCCESS == status)
            {
              return STATUS_SUCCESS;
            }
          }
        }
      }
    }

  } while (NextEntryOffset = pspi->NextEntryOffset);

  return STATUS_UNSUCCESSFUL;
}

NTSTATUS Impersonate()
{
  BOOLEAN b;
  NTSTATUS status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &b);

  if (0 > status)
  {
    return status;
  }

  ULONG cb = 0x10000;

  do 
  {
    status = STATUS_INSUFFICIENT_RESOURCES;

    if (PBYTE buf = new BYTE[cb])
    {
      if (0 <= (status = ZwQuerySystemInformation(SystemProcessInformation, buf, cb, &cb)))
      {
        status = GetSystemToken(buf);

        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
          status = STATUS_UNSUCCESSFUL;
        }
      }

      delete [] buf;
    }

  } while(status == STATUS_INFO_LENGTH_MISMATCH);

  return status;
}

void WINAPI EpZ(void*)
{
  WLog log;
  if (!log.Init(0x100000))
  {
    if (HWND hwnd = CreateWindowExW(0, WC_EDIT, L"SAM Query", WS_OVERLAPPEDWINDOW|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, 0, 0, 0))
    {
      HFONT hFont = 0;
      NONCLIENTMETRICS ncm = { sizeof(ncm) };
      if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
      {
        wcscpy(ncm.lfMessageFont.lfFaceName, L"Courier New");
        ncm.lfMessageFont.lfHeight = -ncm.iMenuHeight;
        if (hFont = CreateFontIndirectW(&ncm.lfMessageFont))
        {
          SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0);
        }
      }

      ULONG n = 8;
      SendMessage(hwnd, EM_SETTABSTOPS, 1, (LPARAM)&n);

      NTSTATUS status = Impersonate();
      if (0 > status)
      {
        log(L"!!! PRIVILEGE = %x\r\n", status);
      }

      {
        LSA policy;
        if (0 <= policy.Init(log))
        {
          PWSTR psz = GetCommandLineW(), pszUser, pszDomain;
          while (psz = wcschr(psz, '['))
          {
            pszDomain = psz + 1;
            
            if (!(psz = wcschr(pszDomain, ']')) )
            {
              break;
            }

            *psz++ = 0;

            if (pszUser = wcschr(pszDomain, '\\'))
            {
              *pszUser++ = 0;
            }
            else
            {
              pszUser = pszDomain, pszDomain = 0;
            }

            QuerySam(log, policy, pszUser, pszDomain);
          }

          HANDLE hToken = 0;
          NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken, &hToken, sizeof(hToken));
        }
      }

      SetWindowTextW(hwnd, log);

      ShowWindow(hwnd, SW_SHOWNORMAL);

      MSG msg;

      while (0 < GetMessage(&msg, 0, 0, 0))
      {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(hwnd))
        {
          break;
        }
      }

      if (hFont)
      {
        DeleteObject(hFont);
      }
    }
  }

  ExitProcess(0);
}

_NT_END