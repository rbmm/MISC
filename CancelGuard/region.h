#pragma once

inline bool IsBigDelta(LONG_PTR rel32)
{
	return rel32 != (LONG)rel32;
}

#ifdef _WIN64
#define IF_BIG_DELTA(delta) if (IsBigDelta(delta)) return 0;
#else
#define IF_BIG_DELTA(delta)
#endif

struct Z_DETOUR_REGION 
{
	friend Z_DETOUR_TRAMPOLINE;

	Z_DETOUR_REGION* next;
	PVOID BaseAddress;
	Z_DETOUR_TRAMPOLINE* First;

	inline static Z_DETOUR_REGION* spRegion = 0;

	void* operator new(size_t, Z_DETOUR_REGION* This)
	{
		return This;
	}

	Z_DETOUR_REGION()
	{
	}

	Z_DETOUR_REGION(Z_DETOUR_TRAMPOLINE* Next, ULONG n);

	Z_DETOUR_TRAMPOLINE* alloc();

	void free(Z_DETOUR_TRAMPOLINE* Next)
	{
		Next->Next = First, First = Next;
	}

	static void _free(Z_DETOUR_TRAMPOLINE* pTramp);

	static Z_DETOUR_TRAMPOLINE* _alloc(void* pvTarget);
};