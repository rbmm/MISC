#include "stdafx.h"

_NT_BEGIN

#include "trampoline.h"
#include "region.h"

Z_DETOUR_REGION::Z_DETOUR_REGION(Z_DETOUR_TRAMPOLINE* Next, ULONG n)
{
	DbgPrint("%s<%p>(%p, %x)\n", __FUNCTION__, this, Next, n);

	BaseAddress = Next;

	next = spRegion, spRegion = this;

	Z_DETOUR_TRAMPOLINE* Prev = 0;

	do 
	{
		Next->Next = Prev, Prev = Next++;
	} while (--n);

	First = Prev;
}

Z_DETOUR_TRAMPOLINE* Z_DETOUR_REGION::alloc()
{
	if (Z_DETOUR_TRAMPOLINE* Next = First)
	{
		First = Next->Next;

		return Next;
	}

	return 0;
}

void Z_DETOUR_REGION::_free(Z_DETOUR_TRAMPOLINE* pTramp)
{
	if (Z_DETOUR_REGION* pRegion = spRegion)
	{
		do 
		{
			if ((ULONG_PTR)pTramp - (ULONG_PTR)pRegion->BaseAddress < 0x10000)
			{
				pRegion->free(pTramp);

				return ;
			}
		} while (pRegion = pRegion->next);
	}

	__debugbreak();
}

Z_DETOUR_TRAMPOLINE* Z_DETOUR_REGION::_alloc(void* pvTarget)
{
	if (Z_DETOUR_REGION* pRegion = spRegion)
	{
		do 
		{
			if (!IsBigDelta((ULONG_PTR)pvTarget - (ULONG_PTR)pRegion->BaseAddress))
			{
				return pRegion->alloc();
			}

		} while (pRegion = pRegion->next);
	}

	return 0;
}

_NT_END