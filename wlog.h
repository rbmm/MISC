#pragma once

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
	void operator >> (HWND hwnd)
	{
		PVOID pv = (PVOID)SendMessage(hwnd, EM_GETHANDLE, 0, 0);
		SendMessage(hwnd, EM_SETHANDLE, (WPARAM)_BaseAddress, 0);
		_BaseAddress = 0;
		if (pv)
		{
			LocalFree(pv);
		}
	}

	ULONG Init(SIZE_T RegionSize)
	{
		if (_BaseAddress = LocalAlloc(0, RegionSize))
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
			LocalFree(_BaseAddress);
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

	WLog& operator << (PCWSTR str)
	{
		PWSTR buf = _buf();
		if (!wcscpy_s(buf, _cch(), str))
		{
			_Ptr += (ULONG)wcslen(buf) * sizeof(WCHAR);
		}
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

		if (dwFlags = FormatMessageW(dwFlags, lpSource, dwError, 0, _buf(), _cch(), 0))
		{
			_Ptr += dwFlags * sizeof(WCHAR);
		}
		return *this;
	}
};
