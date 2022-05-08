#include "stdafx.h"

_NT_BEGIN

#include "../inc/initterm.h"
#include "../winZ/mdi.h"
#include "resource.h"
#include "../inc/idcres.h"
#include "../swe/task.h"

HRESULT GetLastErrorEx()
{
	NTSTATUS status = RtlGetLastNtStatus();
	ULONG dwError = GetLastError();
	return dwError == RtlNtStatusToDosErrorNoTeb(status) ? HRESULT_FROM_NT(status) : HRESULT_FROM_WIN32(dwError);
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
		hWnd,
		(HINSTANCE)&__ImageBase,
		lpText, 
		lpszCaption, 
		(uType & ~MB_ICONMASK)|MB_USERICON,
		MAKEINTRESOURCE(IDR_MENU1)
	};

	return MessageBoxIndirect(&mbp);
}

int ShowErrorBox(HWND hWnd, PCWSTR lpCaption, HRESULT dwError, UINT uType)
{
	int r = 0;
	LPCVOID lpSource = 0;
	ULONG dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

	if (dwError & FACILITY_NT_BIT)
	{
		dwError &= ~FACILITY_NT_BIT;
		dwFlags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

		static HMODULE ghnt;
		if (!ghnt && !(ghnt = GetModuleHandle(L"ntdll"))) return 0;
		lpSource = ghnt;
	}

	PWSTR lpText;
	if (FormatMessageW(dwFlags, lpSource, dwError, 0, (PWSTR)&lpText, 0, 0))
	{
		r = CustomMessageBox(hWnd, lpText, lpCaption, uType);
		LocalFree(lpText);
	}

	return r;
}

class ZShellFrame : public ZMDIChildFrame
{
	virtual HWND CreateView(HWND hWndParent, int nWidth, int nHeight, PVOID /*lpCreateParams*/)
	{
		return CreateShlWnd(hWndParent, nWidth, nHeight);
	}
};

class CRun : public ZDlg
{
	IExecTask* _pExec;

	void OnInitDialog(HWND hwndDlg)
	{
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconW((HINSTANCE)&__ImageBase, MAKEINTRESOURCE(IDR_MENU1)));
		SendDlgItemMessageW(hwndDlg, IDC_EDIT1, EM_SETCUEBANNER, FALSE, (LPARAM)L"Application Name");
		SendDlgItemMessageW(hwndDlg, IDC_EDIT2, EM_SETCUEBANNER, FALSE, (LPARAM)L"Command Line");
		SendDlgItemMessageW(hwndDlg, IDC_EDIT3, EM_SETCUEBANNER, FALSE, (LPARAM)L"Directory");
	}

	BOOL OnOk(HWND hwndDlg)
	{
		static const UINT id[] = { IDC_EDIT1, IDC_EDIT2, IDC_EDIT3 };
		
		HWND hwnd;
		PWSTR psz[3] = {}, sz, pc;

		ULONG i = _countof(id);
		do 
		{
			if (hwnd = GetDlgItem(hwndDlg, id[--i]))
			{
				if (ULONG len = GetWindowTextLengthW(hwnd))
				{
					sz = (PWSTR)alloca(++len * sizeof(wchar_t));
					GetWindowTextW(hwnd, sz, len);
					psz[i] = sz;
				}
			}

		} while (i);

		if (!(sz = psz[0]))
		{
			sz = psz[1];
		}
		
		if (!sz)
		{
			SetFocus(hwnd);
			return FALSE;
		}

		if (pc = wcsrchr(sz, '\\'))
		{
			sz = pc + 1;
		}

		IExecTask* pExec = _pExec;
		PROCESS_INFORMATION pi;

		HRESULT hr = pExec->Exec(psz[0], psz[1], psz[2], &pi);

		if (0 <= hr)
		{
			hr = E_OUTOFMEMORY, hwnd = 0;

			if (ZShellFrame* p = new ZShellFrame)
			{
				if (p->Create(sz, 0))
				{
					hr = pExec->EmbedTask(p->GetPane(), &pi);
				}
				else
				{
					if (!(hr = GetLastHr()))
					{
						hr = E_FAIL;
					}
				}

				p->Release();
			}

			pExec->Cleanup(&pi);
		}

		if (0 > hr)
		{
			ShowErrorBox(hwndDlg, 0, hr, 0);
			return FALSE;
		}

		return TRUE;
	}

	virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
			OnInitDialog(hwndDlg);
			break;
		case WM_COMMAND:
			switch (wParam)
			{
			case IDOK:
				if (!OnOk(hwndDlg))
				{
					break;
				}
				[[fallthrough]];
			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				break;
			}
			break;
		}
		return __super::DialogProc(hwndDlg, uMsg, wParam, lParam);
	}
public:
	CRun(IExecTask* pExec) : _pExec(pExec)
	{
	}
};

class ZMainWnd : public ZMDIFrameWnd
{
	IExecTask* _pExec;

	void OnNewWindow(HWND hwnd)
	{
		CRun dlg(_pExec);
		dlg.DoModal((HINSTANCE)&__ImageBase, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, 0);
	}

	virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CREATE:
			if (0 > CreateTaskMngr(&_pExec) || 0 > _pExec->Start())
			{
				return -1;
			}
			break;

		case WM_DESTROY:
			if (IExecTask* pExec = _pExec)
			{
				pExec->Stop();
				pExec->Release();
			}
			break;

		case WM_COMMAND:
			switch (wParam)
			{
			case ID_FILE_EXIT:
				DestroyWindow(hwnd);
				break;
			case ID_WINDOW_NEW:
				OnNewWindow(hwnd);
				break;
			}
			break;
		}
		return __super::WindowProc(hwnd, uMsg, wParam, lParam);
	}

	virtual PCUNICODE_STRING getPosName()
	{
		STATIC_UNICODE_STRING_(sMainWnd);
		return &sMainWnd;
	}
};

void zmain()
{
	ZGLOBALS globals;
	ZApp app;
	ZRegistry reg;
	ZFont font(TRUE);

	if (0 <= reg.Create(L"Software\\{FD9A4B3B-9DF6-44dd-965A-A2D8738AC69F}") && font.Init())
	{
		HWND hwnd = 0;

		if (ZMainWnd* p = new ZMainWnd)
		{
			hwnd = p->ZSDIFrameWnd::Create(L"Demo", (HINSTANCE)&__ImageBase, MAKEINTRESOURCE(IDR_MENU1));//

			p->Release();
		}

		if (hwnd)
		{
			app.Run();
		}
	}
}

void WINAPI ep(void*)
{
	initterm();
	zmain();
	destroyterm();
	ExitProcess(0);
}

_NT_END