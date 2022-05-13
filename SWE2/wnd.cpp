#include "stdafx.h"

_NT_BEGIN

#include "task.h"
#include "../winZ/window.h"

LIST_ENTRY gwndList = { &gwndList, &gwndList };
HWINEVENTHOOK hWinEventHook = 0;

LONG dwListRef;

struct FOCUS_INFO : LIST_ENTRY
{
	HWND hwndChild;
	HWND hwndParent;

	FOCUS_INFO(HWND hwndParent, HWND hwndChild) : hwndParent(hwndParent), hwndChild(hwndChild)
	{
		InsertHeadList(&gwndList, this);
	}

	~FOCUS_INFO()
	{
		RemoveEntryList(this);
		DbgPrint("%s<%p>(%x)\n", __FUNCTION__, this);
	}
};

void RefList()
{
	++dwListRef;
}

void ReleaseList()
{
	if (!--dwListRef)
	{
		PLIST_ENTRY entry = gwndList.Flink;

		while (entry != &gwndList)
		{
			FOCUS_INFO* pi = static_cast<FOCUS_INFO*>(entry);

			entry = entry->Flink;

			delete pi;
		}
	}
}

void WINAPI DoneShl()
{
	if (hWinEventHook)
	{
		UnhookWinEvent(hWinEventHook);
	}

	ReleaseList();
}

class ShellWnd : public ZWnd
{
	HWND _hwnd = 0;
	FOCUS_INFO* _pfi = 0;
	POINT _pt;
	SIZE _s;

	BOOL RedirectParent(_In_ HWND hwnd, _In_ HWND hwndForEmbed, _In_ int cx, _In_ int cy)
	{
		if (SetParent(hwndForEmbed, hwnd))
		{
			RECT rc { 0, 0, cx, cy };

			if (AdjustWindowRectEx(&rc, GetWindowLongW(hwndForEmbed, GWL_STYLE), FALSE, GetWindowLongW(hwndForEmbed, GWL_EXSTYLE)))
			{
				_pt.x = rc.left, _pt.y = rc.top, _s.cx = (rc.right -= rc.left) - cx, _s.cy = (rc.bottom -= rc.top) - cy;

				InvalidateRect(hwndForEmbed, 0, TRUE);

				return SetWindowPos(hwndForEmbed, 0, rc.left, rc.top, rc.right, rc.bottom, SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_NOZORDER);
			}
		}
		return FALSE;
	}

	LRESULT OnPaint(HWND hwnd)
	{
		PAINTSTRUCT ps;
		if (BeginPaint(hwnd, &ps))
		{
			if (!IsRectEmpty(&ps.rcPaint))
			{
				FillRect(ps.hdc, &ps.rcPaint, (HBRUSH)(1 + COLOR_INFOBK));
			}
			EndPaint(hwnd, &ps);
		}
		return 0;
	}

	virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_SIZE:
			if (hwnd = _hwnd)
			{
				switch (wParam)
				{
				case SIZE_RESTORED:
				case SIZE_MAXIMIZED:
					//DbgPrint("(%d, %d) [%u * %u]\n", _pt.x, _pt.y, _s.cx + LOWORD(lParam), _s.cy + HIWORD(lParam));
					MoveWindow(hwnd, _pt.x, _pt.y, _s.cx + LOWORD(lParam), _s.cy + HIWORD(lParam), TRUE);
					break;
				}
			}
			break;

		case WM_NCDESTROY:

			if (FOCUS_INFO* pfi = _pfi)
			{
				delete pfi;
			}
			ReleaseList();
			break;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			return OnPaint(hwnd);

		case WM_NCCREATE:
			RefList();
			if (RedirectParent(hwnd, _hwnd, reinterpret_cast<CREATESTRUCTW*>(lParam)->cx, reinterpret_cast<CREATESTRUCTW*>(lParam)->cy))
			{
				if (_pfi = new FOCUS_INFO(reinterpret_cast<CREATESTRUCTW*>(lParam)->hwndParent, _hwnd))
				{
					break;
				}
			}
			return 0;
		}

		return __super::WindowProc(hwnd, uMsg, wParam, lParam);
	}
public:
	ShellWnd(HWND hwnd) : _hwnd(hwnd)
	{
	}
};

HWND WINAPI CreateShlWnd(_In_ HWND hWndParent, _In_ HWND hwndExternal, _In_ int nWidth, _In_ int nHeight)
{
	if (ShellWnd* p = new ShellWnd(hwndExternal))
	{
		hWndParent = p->Create(0, 0, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0, 0, nWidth, nHeight, hWndParent, 0, 0);
		
		p->Release();

		return hWndParent;
	}

	return 0;
}

VOID CALLBACK WinEventProc(
						   HWINEVENTHOOK /*hWinEventHook*/,
						   DWORD event,
						   HWND hwnd,
						   LONG /*idObject*/,
						   LONG /*idChild*/,
						   DWORD /*dwEventThread*/,
						   DWORD /*dwmsEventTime*/
						   )
{
	if (hwnd && event == EVENT_OBJECT_FOCUS)
	{
		ULONG dwProcessId;

		if (GetWindowThreadProcessId(hwnd, &dwProcessId))
		{
			static ULONG s_dwLastActive;

			PLIST_ENTRY entry = &gwndList;

			while ((entry = entry->Flink) != &gwndList)
			{
				if (static_cast<FOCUS_INFO*>(entry)->hwndChild == hwnd)
				{
					int n = 32;
					BringWindowToTop(GetAncestor(hwnd = static_cast<FOCUS_INFO*>(entry)->hwndParent, GA_ROOT));
					SetFocus(hwnd);
					do 
					{
						SendMessageW(hwnd, WM_NCACTIVATE, TRUE, 0);
					} while (--n && (hwnd = GetAncestor(hwnd, GA_PARENT)));

					s_dwLastActive = dwProcessId;
					return;
				}
			}

			if (dwProcessId != GetCurrentProcessId() && dwProcessId != s_dwLastActive)
			{
				GUITHREADINFO gui = { sizeof(gui) };
				if (GetGUIThreadInfo(GetCurrentThreadId(), &gui))
				{
					SendMessageW(gui.hwndActive, WM_NCACTIVATE, FALSE, 0);
				}
			}
		}
	}
}

HRESULT WINAPI InitShl()
{
	dwListRef = 1;

	if (hWinEventHook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, 
		0, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT| WINEVENT_SKIPOWNPROCESS  ))
	{
		return S_OK;
	}

	return GetLastHr();
}

_NT_END