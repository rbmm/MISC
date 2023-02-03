#pragma once

struct T_HOOK_ENTRY 
{
	_Inout_ void** pThunk;
	// pointer on variable which hold:
	// In: where to put the hook ( *pThunk -> func)
	// Out: pointer to place to execute original code ( *pThunk -> trump)
	_Inout_ PVOID hook;
	// In: pointer to hook function. so *pThunk redirected to hook
	// Out: original value of *pThunk ( func )
	union Z_DETOUR_TRAMPOLINE* pTramp;
};


NTSTATUS NTAPI TrInit();

void NTAPI TrHook(_In_ T_HOOK_ENTRY* entry, _In_ ULONG n);
void NTAPI TrUnHook(_In_ T_HOOK_ENTRY* entry, _In_ ULONG n);

NTSTATUS NTAPI TrHook(_Inout_ void** p__imp, _In_ PVOID hook);

#define _DECLARE_T_HOOK(pfn) EXTERN_C extern PVOID __imp_ ## pfn;

#define DECLARE_T_HOOK_X86(pfn, n) _DECLARE_T_HOOK(pfn) __pragma(comment(linker, _CRT_STRINGIZE(/alternatename:___imp_##pfn##=__imp__##pfn##@##n)))

#ifdef _M_IX86
#define DECLARE_T_HOOK(pfn, n) DECLARE_T_HOOK_X86(pfn, n)
#else
#define DECLARE_T_HOOK(pfn, n) _DECLARE_T_HOOK(pfn)
#endif


#define T_HOOKS_BEGIN(name) T_HOOK_ENTRY name[] = {
#define T_HOOK(pfn) { &__imp_ ## pfn, hook_ ## pfn }
#define T_HOOKS_END() };

