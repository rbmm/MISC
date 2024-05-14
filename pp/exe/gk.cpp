#include "stdafx.h"
#include <CommCtrl.h>

_NT_BEGIN

#include "../inc/rtlframe.h"

ULONG GetLastErrorEx()
{
	ULONG dwError = GetLastError();
	NTSTATUS status = RtlGetLastNtStatus();
	return RtlNtStatusToDosErrorNoTeb(status) == dwError ? HRESULT_FROM_NT(status) : dwError;
}

struct FICON
{
	HICON hIcon;
};

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_CREATEWND)
	{
		CBT_CREATEWND* pccw = reinterpret_cast<CBT_CREATEWND*>(lParam);

		if (pccw->lpcs->lpszClass == WC_DIALOG)
		{
			if (FICON* p = RTL_FRAME<FICON>::get())
			{
				SendMessageW((HWND)wParam, WM_SETICON, ICON_SMALL, (LPARAM)p->hIcon);
			}
		}
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

int CustomMessageBox(HWND hWnd, PCWSTR lpText, PCWSTR lpszCaption, UINT uType)
{
	PCWSTR pszName = 0;

	switch (uType & MB_ICONMASK)
	{
	case MB_ICONINFORMATION:
		pszName = IDI_INFORMATION;
		break;
	case MB_ICONQUESTION:
		pszName = IDI_QUESTION;
		break;
	case MB_ICONWARNING:
		pszName = IDI_WARNING;
		break;
	case MB_ICONERROR:
		pszName = IDI_ERROR;
		break;
	}

	MSGBOXPARAMS mbp = {
		sizeof(mbp),
		IsWindowVisible(hWnd) ? hWnd : 0,
		(HINSTANCE)&__ImageBase,
		lpText,
		lpszCaption,
		(uType & ~MB_ICONMASK) | MB_USERICON,
		MAKEINTRESOURCE(1)
	};

	HHOOK hhk = 0;
	RTL_FRAME<FICON> frame;
	frame.hIcon = 0;

	if (pszName && 0 <= LoadIconWithScaleDown(0, pszName,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), &frame.hIcon))
	{
		hhk = SetWindowsHookExW(WH_CBT, CBTProc, 0, GetCurrentThreadId());
	}

	int i = MessageBoxIndirect(&mbp);

	if (hhk) UnhookWindowsHookEx(hhk);
	if (frame.hIcon) DestroyIcon(frame.hIcon);

	return i;
}

HMODULE GetNtMod()
{
	static HMODULE s_hntmod;
	if (!s_hntmod)
	{
		s_hntmod = GetModuleHandle(L"ntdll");
	}

	return s_hntmod;
}

int ShowErrorBox(HWND hwnd, HRESULT dwError, PCWSTR lpCaption)
{
	int r = 0;
	LPCVOID lpSource = 0;
	ULONG dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

	if ((dwError & FACILITY_NT_BIT) || (0 > dwError && HRESULT_FACILITY(dwError) == FACILITY_NULL))
	{
		dwError &= ~FACILITY_NT_BIT;
__nt:
		dwFlags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

		lpSource = GetNtMod();
	}

	PWSTR lpText;
	if (FormatMessageW(dwFlags, lpSource, dwError, 0, (PWSTR)&lpText, 0, 0))
	{
		r = CustomMessageBox(hwnd, lpText, lpCaption, dwError ? MB_ICONERROR : MB_ICONINFORMATION);
		LocalFree(lpText);
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
	{
		goto __nt;
	}

	return r;
}

NTSTATUS DoIoControl(HANDLE hFile, ULONG code)
{
	IO_STATUS_BLOCK iosb;
	return NtDeviceIoControlFile(hFile, 0, 0, 0, &iosb, code, 0, 0, 0, 0);
}

#include "../DrvTest/ioctl.h"

void WINAPI ep(PWSTR lpCmdLine)
{
	lpCmdLine = GetCommandLineW();

	IO_STATUS_BLOCK iosb;
	STATIC_OBJECT_ATTRIBUTES(oa, "\\device\\{534A4F3B-A655-4a9e-8E9C-3488B2265ADD}");

	HANDLE hFile;
	NTSTATUS status = NtOpenFile(&hFile, SYNCHRONIZE, &oa, &iosb, FILE_SHARE_VALID_FLAGS, FILE_SYNCHRONOUS_IO_NONALERT);

	if (0 > status)
	{
		ShowErrorBox(0, HRESULT_FROM_NT(status), L"Open Device FAIL");
	}
	else
	{
		if (0 > (status = DoIoControl(hFile, IOCTL_AddProtected)))
		{
			ShowErrorBox(0, HRESULT_FROM_NT(status), L"Set Protected FAIL");
		}
		else
		{
			BOOL bSetProtected = wcschr(lpCmdLine, '*') != 0;

			if (bSetProtected) DoIoControl(hFile, IOCTL_SetProtectedProcess);

			MessageBoxW(0, lpCmdLine, L"Protected", MB_ICONINFORMATION);

			if (bSetProtected) DoIoControl(hFile, IOCTL_DelProtectedProcess);

			DoIoControl(hFile, IOCTL_DelProtected);

		}

		NtClose(hFile);
	}

	ExitProcess(0);
}

_NT_END
