NTSTATUS IsUserInAdminGroup(PCUNICODE_STRING ServerName, PSID UserSid, BOOLEAN* pb)
{
	*pb = FALSE;

	SAM_HANDLE ServerHandle, DomainHandle;

	NTSTATUS status = SamConnect(ServerName, &ServerHandle, SAM_SERVER_LOOKUP_DOMAIN, 0);

	if (0 <= status)
	{
		ULONG len = GetSidLengthRequired(1);

		PSID BuiltIn = (PSID)alloca(len);
		static const SID_IDENTIFIER_AUTHORITY NT_AUTHORITY = SECURITY_NT_AUTHORITY;

		InitializeSid(BuiltIn, const_cast<SID_IDENTIFIER_AUTHORITY*>(&NT_AUTHORITY), 1);
		*GetSidSubAuthority(BuiltIn, 0) = SECURITY_BUILTIN_DOMAIN_RID;

		status = SamOpenDomain(ServerHandle, DOMAIN_READ, BuiltIn, &DomainHandle);

		SamCloseHandle(ServerHandle);

		if (0 <= status)
		{
			ULONG MembershipCount, *Aliases;

			status = SamGetAliasMembership(DomainHandle, 1, &UserSid, &MembershipCount, &Aliases);

			SamCloseHandle(DomainHandle);

			if (0 <= status)
			{
				PVOID buf = Aliases;
				if (MembershipCount)
				{
					do 
					{
						if (*Aliases++ == DOMAIN_ALIAS_RID_ADMINS)
						{
							*pb = TRUE;
							break;
						}
					} while (--MembershipCount);
				}
				SamFreeMemory(buf);
			}
		}
	}

	return status;
}

NTSTATUS QueryUser(PCWSTR UserName, BOOLEAN* pb)
{
	ULONG f = FACILITY_NT_BIT;

	LSA_OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) };
	LSA_HANDLE PolicyHandle;

	NTSTATUS status = LsaOpenPolicy(0, &ObjectAttributes,
		POLICY_VIEW_LOCAL_INFORMATION|POLICY_LOOKUP_NAMES, &PolicyHandle);
	
	if (0 <= status)
	{
		PLSA_TRANSLATED_SID2 Sids = 0;
		PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = 0;

		UNICODE_STRING us;
		RtlInitUnicodeString(&us, UserName);

		if (0 <= (status = LsaLookupNames2(PolicyHandle, 0, 1, &us, &ReferencedDomains, &Sids)))
		{
			if (Sids->Use == SidTypeUser)
			{
				PPOLICY_DNS_DOMAIN_INFO pddi;
				if (0 <= (status = LsaQueryInformationPolicy(PolicyHandle, PolicyDnsDomainInformation, (void**)&pddi)))
				{
					BOOLEAN bInDomain = pddi->Sid != 0;
					LsaFreeMemory(pddi);

					NT_PRODUCT_TYPE NtProductType;

					if (bInDomain && RtlGetNtProductType(&NtProductType) && NtProductType != NtProductLanManNt)
					{
						ULONG DomainIndex = Sids->DomainIndex;

						if (DomainIndex < ReferencedDomains->Entries)
						{
							PLSA_TRUST_INFORMATION Domain = ReferencedDomains->Domains + DomainIndex;

							ULONG Length = Domain->Name.Length;

							PWSTR DomainName = Domain->Name.Buffer;

							if (Domain->Name.MaximumLength <= Length)
							{
								DomainName = (PWSTR)memcpy(alloca(Length + sizeof(WCHAR)), DomainName, Length);
							}

							*(WCHAR*)RtlOffsetToPointer(DomainName, Length) = 0;

							PDOMAIN_CONTROLLER_INFO DomainControllerInfo;

							if (status = DsGetDcName(0, DomainName, 0, 0, DS_PDC_REQUIRED, &DomainControllerInfo))
							{
								f = 0;
								status = HRESULT_FROM_WIN32(status);
							}
							else
							{
								if (!(DomainName = DomainControllerInfo->DomainControllerAddress))
								{
									DomainName = DomainControllerInfo->DomainControllerName;
								}

								if (DomainName)
								{
									RtlInitUnicodeString(&Domain->Name, DomainName);

									status = IsUserInAdminGroup(&Domain->Name, Sids->Sid, pb);
								}
								else
								{
									status = STATUS_UNSUCCESSFUL;
								}

								NetApiBufferFree(DomainControllerInfo);
							}
						}
						else
						{
							status = STATUS_NO_SUCH_DOMAIN;
						}
					}
					else
					{
						// not in domain or at DC
						status = IsUserInAdminGroup(0, Sids->Sid, pb);
					}
				}
			}
			else
			{
				status = STATUS_NO_SUCH_USER;
			}
		}

		LsaFreeMemory(Sids);
		LsaFreeMemory(ReferencedDomains);

		LsaClose(PolicyHandle);
	}

	return status ? status | f : S_OK;
}
