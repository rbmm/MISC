#include "stdafx.h"

_NT_BEGIN

#include "../winz/Container.h"
#include "../winZ/Frame.h"
#include "../winz/app.h"
#include "../inc/initterm.h"
#include "resource.h"

class ShellWnd : public ZWnd
{
	HWND _hwnd = 0;
	POINT _pt;
	SIZE _s;

	static BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
	{
		WCHAR clsname[32];
		if (GetClassNameW(hwnd, clsname, _countof(clsname)))
		{
			if (!wcscmp(L"PuTTY", clsname))
			{
				*(HWND*)lParam = hwnd;

				return FALSE;
			}
		}
		return TRUE;
	}

	void OnCreate(HWND hwnd)
	{
		WCHAR putty[MAX_PATH];
		if (SearchPathW(0, L"putty.exe", 0, _countof(putty), putty, 0))
		{
			PROCESS_INFORMATION pi;
			STARTUPINFOW si = { sizeof(si) };

			si.wShowWindow = SW_HIDE;
			si.dwFlags = STARTF_USESHOWWINDOW;

			PWSTR cmdline = wcschr(GetCommandLineW(), '*');

			if (!cmdline)
			{
				cmdline = const_cast<PWSTR>(L"* 127.0.0.1");
			}

			if (CreateProcessW(putty, cmdline, 0, 0, 0, 0, 0, 0, &si, &pi))
			{		
				WaitForInputIdle(pi.hProcess, 1000);
				NtClose(pi.hProcess);

				HWND hwndEmbeded = 0;
				EnumThreadWindows(pi.dwThreadId, EnumThreadWndProc, (LPARAM)&hwndEmbeded);
				AttachThreadInput(pi.dwThreadId, GetCurrentThreadId(), TRUE);
				NtClose(pi.hThread);

				if (hwndEmbeded)
				{
					_hwnd = hwndEmbeded;
					RECT rc, rcw;

					ShowWindow(hwndEmbeded, SW_SHOW);
					GetClientRect(hwnd, &rc);

					ULONG dwStyle = GetWindowLongW(hwndEmbeded, GWL_STYLE);

					SetWindowLongW(hwndEmbeded, GWL_STYLE, 
						(dwStyle & ~(WS_GROUP|WS_TABSTOP) | WS_CHILD));

					SetParent(hwndEmbeded, hwnd);

					GetWindowRect(hwndEmbeded, &rcw);
					POINT pt = { rcw.left, rcw.top };
					SIZE s = { rcw.right - rcw.left, rcw.bottom - rcw.top };
					ScreenToClient(hwndEmbeded, &pt);
					GetClientRect(hwndEmbeded, &rcw);
					s.cx -= rcw.right, s.cy -= rcw.bottom;

					if (dwStyle & WS_VSCROLL)
					{
						s.cx -= GetSystemMetrics(SM_CXVSCROLL);
					}

					_pt = pt, _s = s;

					SetWindowPos(hwndEmbeded, 0, pt.x, pt.y, rc.right + s.cx, rc.bottom + s.cy, 
						SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_NOZORDER);
				}
			}
		}
	}

	virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_NCDESTROY:
			RundownGUI();
			break;
		case WM_CREATE:
			OnCreate(hwnd);
			break;
		case WM_SIZE:
			if (hwnd = _hwnd)
			{
				switch (wParam)
				{
				case SIZE_RESTORED:
				case SIZE_MAXIMIZED:
					MoveWindow(hwnd, _pt.x, _pt.y, _s.cx + LOWORD(lParam), _s.cy + HIWORD(lParam), FALSE);
					break;
				}
			}
			break;
		case WM_DESTROY:
			break;
		}

		return __super::WindowProc(hwnd, uMsg, wParam, lParam);
	}
};

void WINAPI ep(void*)
{
	initterm();

	ZGLOBALS globals;
	ZApp app;

	if (0 <= CoInitializeEx(0, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE))
	{
		if (ShellWnd* p = new ShellWnd)
		{
			HWND hwnd = p->Create(WS_EX_CLIENTEDGE, L"[ putty ]", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, 0, 0);

			p->Release();

			if (hwnd)
			{
				app.Run();
			}
		}
		CoUninitialize();
	}
	destroyterm();
	ExitProcess(0);
}

_NT_END