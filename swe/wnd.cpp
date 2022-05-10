#include "stdafx.h"

_NT_BEGIN

#include "task.h"
#include "internal.h"
#include "../winZ/window.h"

enum { aRedirect = 0x782303DE, aSetTask = 0x353481A5 };

LIST_ENTRY gList = { &gList, &gList };
LIST_ENTRY gwndList = { &gwndList, &gwndList };

ULONG GetSecondsCount()
{
	return (ULONG)(GetTickCount64() / 1000);
}

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

struct PARENT_INFO : LIST_ENTRY
{
	HWND hwndParent;
	ULONG dwThreadId;
	ULONG dwProcessId;
	ULONG time;

	PARENT_INFO(HWND hwndParent, ULONG dwThreadId, ULONG dwProcessId) : hwndParent(hwndParent), dwThreadId(dwThreadId), dwProcessId(dwProcessId)
	{
		InsertHeadList(&gList, this);
		time = GetSecondsCount() + 8;
	}

	~PARENT_INFO()
	{
		RemoveEntryList(this);
		DbgPrint("%s<%p>(%x)\n", __FUNCTION__, this, dwProcessId);
	}
};

void CleanupWinEvents(HWINEVENTHOOK hWinEventHook)
{
	if (hWinEventHook)
	{
		UnhookWinEvent(hWinEventHook);
	}

	PLIST_ENTRY entry = gList.Flink;

	while (entry != &gList)
	{
		PARENT_INFO* pi = static_cast<PARENT_INFO*>(entry);

		entry = entry->Flink;

		delete pi;
	}

	entry = gwndList.Flink;

	while (entry != &gwndList)
	{
		FOCUS_INFO* pi = static_cast<FOCUS_INFO*>(entry);

		entry = entry->Flink;

		delete pi;
	}
}

HRESULT AddParentInfo(_In_ HWND hwnd, _In_ PPROCESS_INFORMATION lpProcessInformation, _In_ ITask* pTask)
{
	if (PARENT_INFO* p = new PARENT_INFO(hwnd, lpProcessInformation->dwThreadId, lpProcessInformation->dwProcessId))
	{
		SendMessageW(hwnd, WM_NULL, aSetTask, (LPARAM)pTask);

		return S_OK;
	}

	return E_OUTOFMEMORY;
}

class ShellWnd : public ZWnd
{
	ITask* _pTask = 0;
	HWND _hwnd = 0;
	FOCUS_INFO* _pfi = 0;
	POINT _pt;
	SIZE _s;

	BOOL RedirectParent(_In_ HWND hwnd, _In_ HWND hwndForEmbed)
	{
		if (!SetParent(hwndForEmbed, hwnd))
		{
			GetLastError();
			return FALSE;
		}

		RECT rc, rcw;

		GetClientRect(hwnd, &rc);

		//ULONG dwStyle = GetWindowLongW(hwndForEmbed, GWL_EXSTYLE);

		//SetWindowLongW(hwndForEmbed, GWL_EXSTYLE, (dwStyle & ~(WS_EX_APPWINDOW|WS_EX_CONTROLPARENT)) | WS_EX_TOOLWINDOW);

		//dwStyle = GetWindowLongW(hwndForEmbed, GWL_STYLE);
		//SetWindowLongW(hwndForEmbed, GWL_STYLE, (dwStyle & ~(WS_GROUP|WS_TABSTOP)));

		GetWindowRect(hwndForEmbed, &rcw);
		POINT pt = { rcw.left, rcw.top };
		SIZE s = { rcw.right - rcw.left, rcw.bottom - rcw.top };
		ScreenToClient(hwndForEmbed, &pt);
		GetClientRect(hwndForEmbed, &rcw);
		s.cx -= rcw.right, s.cy -= rcw.bottom;

		if (GetWindowLongW(hwndForEmbed, GWL_STYLE) & WS_VSCROLL)
		{
			s.cx -= GetSystemMetrics(SM_CXVSCROLL);
		}

		_pt = pt, _s = s;

		return SetWindowPos(hwndForEmbed, 0, pt.x, pt.y, rc.right + s.cx, rc.bottom + s.cy, 
			SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_NOZORDER);
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
					MoveWindow(hwnd, _pt.x, _pt.y, _s.cx + LOWORD(lParam), _s.cy + HIWORD(lParam), TRUE);
					break;
				}
			}
			break;

		case WM_NULL:
			switch (wParam)
			{
			case aRedirect:
				DbgPrint("====== SetParent(%p)\n", lParam);
				_hwnd = (HWND)lParam;
				if (RedirectParent(hwnd, (HWND)lParam))
				{
					_pfi = new FOCUS_INFO(GetParent(hwnd), (HWND)lParam);
				}
				else
				{
					DestroyWindow(hwnd);
				}
				break;
			case aSetTask:
				_pTask = (ITask*)lParam;
				break;
			}
			break;

		case WM_CLOSE:
			PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
			break;

		case WM_DESTROY:
			if (ITask* pTask = _pTask)
			{
				pTask->Close();
				pTask->Release();
			}

			if (FOCUS_INFO* pfi = _pfi)
			{
				delete pfi;
			}
			break;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			return OnPaint(hwnd);
		}

		return __super::WindowProc(hwnd, uMsg, wParam, lParam);
	}
};

HWND CreateShlWnd(_In_ HWND hWndParent, _In_ int nWidth, _In_ int nHeight)
{
	if (ShellWnd* p = new ShellWnd)
	{
		hWndParent = p->Create(0, 0, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0, 0, nWidth, nHeight, hWndParent, 0, 0);
		p->Release();

		return hWndParent;
	}

	return 0;
}

void OnCreateWnd(HWND hwnd, DWORD dwEventThread)
{
	WCHAR clsn[128];
	PLIST_ENTRY entry = gList.Flink;

	//++ kill too old entry
	while (entry != &gList)
	{
		PARENT_INFO* pi = static_cast<PARENT_INFO*>(entry);
		entry = entry->Flink;

		if (pi->time < GetSecondsCount() )
		{
			delete pi;
		}
	}
	//++ kill too old entry

	while ((entry = entry->Flink) != &gList)
	{
		if (static_cast<PARENT_INFO*>(entry)->dwThreadId == dwEventThread)
		{
			if (!GetClassNameW(hwnd, clsn, _countof(clsn)))
			{
				swprintf_s(clsn, _countof(clsn), L"!! %u", GetLastError());
			}

			DbgPrint("++%p:%p(%p) %x.%x %S\n", hwnd, GetParent(hwnd), 
				GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT),
				GetWindowLongW(hwnd, GWL_STYLE), GetWindowLongW(hwnd, GWL_EXSTYLE), clsn);

			if (IsWindow(hwnd) &&
				!(GetWindowLongW(hwnd, GWL_STYLE) & (WS_CHILD|WS_POPUP)) &&
				!(GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) &&
				!GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT) &&
				!GetParent(hwnd))
			{
__0:
				PostMessageW(static_cast<PARENT_INFO*>(entry)->hwndParent, WM_NULL, aRedirect, (LPARAM)hwnd);

				delete static_cast<PARENT_INFO*>(entry);

				return;
			}
		}
	}

	if (GetClassNameW(hwnd, clsn, _countof(clsn)) && !wcscmp(clsn, L"ConsoleWindowClass"))
	{
		ULONG dwProcessId;
		if (GetWindowThreadProcessId(hwnd, &dwProcessId))
		{
			entry = &gList;

			while ((entry = entry->Flink) != &gList)
			{
				if (static_cast<PARENT_INFO*>(entry)->dwProcessId == dwProcessId)
				{
					goto __0;
				}
			}

			if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, dwProcessId))
			{
				PROCESS_BASIC_INFORMATION pbi;
				NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), 0);
				NtClose(hProcess);

				if (0 <= status)
				{
					entry = &gList;

					while ((entry = entry->Flink) != &gList)
					{
						if (static_cast<PARENT_INFO*>(entry)->dwProcessId == (ULONG)pbi.InheritedFromUniqueProcessId)
						{
							goto __0;
						}
					}
				}
			}
		}
	}
}

void OnFocus(HWND hwnd)
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

VOID CALLBACK WinEventProc(
						   HWINEVENTHOOK /*hWinEventHook*/,
						   DWORD event,
						   HWND hwnd,
						   LONG idObject,
						   LONG idChild,
						   DWORD dwEventThread,
						   DWORD /*dwmsEventTime*/
						   )
{
	if (hwnd )
	{
		switch (event)
		{
		case EVENT_OBJECT_CREATE:
			if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF) OnCreateWnd(hwnd, dwEventThread);
			break;
		case EVENT_OBJECT_FOCUS:
			OnFocus(hwnd);
			break;
		}
	}
}

_NT_END