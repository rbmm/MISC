#include "stdafx.h"

_NT_BEGIN
#include "../winz/Container.h"
#include "../winZ/Frame.h"
#include "../winz/app.h"
#include "../inc/initterm.h"
#include "resource.h"
extern unsigned char const volatile guz = 0;

#include "rdp.h"

#pragma warning(disable : 4100)

class MyMsTscAxEvents : public IMsTscAxEvents {

	LONG _dwRefCount = 1;

	virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if (riid == __uuidof(IUnknown) || 
			riid == __uuidof(IDispatch)|| 
			riid == __uuidof(IMsTscAxEvents))
		{
			AddRef();

			*ppvObject = static_cast<IMsTscAxEvents*>(this);
			return S_OK;
		}

		*ppvObject = 0;
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( )
	{
		return InterlockedIncrementNoFence(&_dwRefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release( )
	{
		ULONG dwRefCount = InterlockedDecrement(&_dwRefCount);
		if (!dwRefCount)
		{
			delete this;
		}
		return dwRefCount;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
		/* [out] */ __RPC__out UINT *pctinfo)
	{
		*pctinfo = 0;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
		/* [in] */ UINT iTInfo,
		/* [in] */ LCID lcid,
		/* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
		/* [in] */ __RPC__in REFIID riid,
		/* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
		/* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
		/* [in] */ LCID lcid,
		/* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
		/* [annotation][in] */ 
		_In_  DISPID dispIdMember,
		/* [annotation][in] */ 
		_In_  REFIID riid,
		/* [annotation][in] */ 
		_In_  LCID lcid,
		/* [annotation][in] */ 
		_In_  WORD wFlags,
		/* [annotation][out][in] */ 
		_In_  DISPPARAMS *pDispParams,
		/* [annotation][out] */ 
		_Out_opt_  VARIANT *pVarResult,
		/* [annotation][out] */ 
		_Out_opt_  EXCEPINFO *pExcepInfo,
		/* [annotation][out] */ 
		_Out_opt_  UINT *puArgErr)
	{
		DbgPrint("%s<%p>(%x %x|%x)\n", __FUNCTION__, this, dispIdMember, pDispParams->cArgs, pDispParams->cNamedArgs);
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE OnConnecting()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnConnected()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnLoginComplete()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnDisconnected(LONG discReason)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnEnterFullScreenMode()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnLeaveFullScreenMode()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnChannelReceivedData(BSTR chanName, BSTR data)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRequestGoFullScreen()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRequestLeaveFullScreen()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnFatalError(LONG errorCode)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnWarning(LONG warningCode)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRemoteDesktopSizeChange(LONG width, LONG height)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnIdleTimeoutNotification()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRequestContainerMinimize()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnConfirmClose(_Out_ VARIANT_BOOL* pfAllowClose)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnReceivedTSPublicKey(BSTR publicKey, _Out_ VARIANT_BOOL* pfContinueLogon)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnAutoReconnecting(LONG disconnectReason, LONG attemptCount, _Out_ AutoReconnectContinueState* pArcContinueStatus)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnAuthenticationWarningDisplayed()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnAuthenticationWarningDismissed()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRemoteProgramResult(BSTR bstrRemoteProgram, RemoteProgramResult lError, VARIANT_BOOL vbIsExecutable)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRemoteProgramDisplayed(VARIANT_BOOL vbDisplayed, ULONG uDisplayInformation)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnRemoteWindowDisplayed(VARIANT_BOOL vbDisplayed, LPVOID hwnd, RemoteWindowDisplayedAttribute windowAttribute)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnLogonError(LONG lError)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnFocusReleased(INT iDirection)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnUserNameAcquired(BSTR bstrUserName)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnMouseInputModeChanged(VARIANT_BOOL fMouseModeRelative)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnServiceMessageReceived(BSTR serviceMessage)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnConnectionBarPullDown()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnNetworkStatusChanged(ULONG qualityLevel, LONG bandwidth, LONG rtt)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnDevicesButtonPressed()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnAutoReconnected()
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnAutoReconnecting2(LONG disconnectReason, VARIANT_BOOL networkAvailable, LONG attemptCount, LONG maxAttemptCount)
	{
		DbgPrint("%s<%p>\n", __FUNCTION__, this);
		return S_OK;
	}
};

#pragma warning(default : 4100)

class MyFrame : public ZContainer
{
	ULONG _dwCookie = 0;
	BOOLEAN _bSinkSet = FALSE;

	BOOL CreateControl(IUnknown** ppControl)
	{
		return S_OK == CoCreateInstance(__uuidof(MsRdpClientNotSafeForScripting), 0, CLSCTX_INPROC_SERVER, 
			__uuidof(IMsRdpClient), (void**)ppControl);
	}

	HRESULT SetSink(IUnknown* pControl, BOOL bSet)
	{
		HRESULT hr;
		IConnectionPoint* pcp;
		IConnectionPointContainer* pcpc;

		if (0 <= (hr = pControl->QueryInterface(IID_PPV_ARGS(&pcpc))))
		{
			hr = pcpc->FindConnectionPoint(__uuidof(IMsTscAxEvents), &pcp);

			pcpc->Release();

			if (0 <= hr)
			{
				if (bSet)
				{
					hr = E_OUTOFMEMORY;

					if (IMsTscAxEvents* pSink = new MyMsTscAxEvents)
					{
						if (0 <= (hr = pcp->Advise(pSink, &_dwCookie)))
						{
							_bSinkSet = TRUE;
						}

						pSink->Release();
					}
				}
				else
				{
					hr = pcp->Unadvise(_dwCookie);
				}

				pcp->Release();
			}
		}

		return hr;
	}

	STDMETHOD(OnShowWindow)(BOOL fShow)
	{
		DbgPrint("%s<%p>(%x)\n", __FUNCTION__, this, fShow);
		ShowWindow(getHWND(), fShow ? SW_SHOW : SW_HIDE);
		return S_OK;
	}

	BOOL OnControlActivate(IUnknown* pControl, PVOID lpCreateParams)
	{
		// 0 Apply key combinations only locally at the client computer.
		// 1 Apply key combinations at the remote server.
		// 2 Apply key combinations to the remote server only when the client is running in full-screen mode. This is the default value.
		// IMsRdpClientSecuredSettings::put_KeyboardHookMode(1);

		IMsRdpClientNonScriptable3* p;
		IMsRdpClientSecuredSettings* pSecuredSettings;

		if (0 <= reinterpret_cast<IMsRdpClient*>(pControl)->get_SecuredSettings2(&pSecuredSettings))
		{
			pSecuredSettings->put_KeyboardHookMode(1);
			pSecuredSettings->Release();
		}

		if (0 <= pControl->QueryInterface(IID_PPV_ARGS(&p)))
		{
			VARIANT_BOOL fNegotiate;

			p->get_NegotiateSecurityLayer(&fNegotiate);

			if (!fNegotiate)
			{
				p->put_NegotiateSecurityLayer(VARIANT_TRUE);
			}

			p->get_PromptForCredentials(&fNegotiate);

			if (!fNegotiate)
			{
				p->put_PromptForCredentials(VARIANT_TRUE);
			}

			p->get_EnableCredSspSupport(&fNegotiate);
			if (!fNegotiate)
			{
				p->put_EnableCredSspSupport(VARIANT_TRUE);
			}

			//if (BSTR sz = SysAllocString(L"***"))
			//{
			//	p->put_ClearTextPassword(sz);

			//	SysFreeString(sz);
			//}

			p->Release();
		}

		HRESULT hr = SetSink(pControl, TRUE);

		if (0 <= hr)
		{
			reinterpret_cast<IMsRdpClient*>(pControl)->put_DesktopWidth(GetSystemMetrics(SM_CXSCREEN));
			reinterpret_cast<IMsRdpClient*>(pControl)->put_DesktopHeight(GetSystemMetrics(SM_CYSCREEN));
			reinterpret_cast<IMsRdpClient*>(pControl)->put_FullScreen(VARIANT_TRUE);
			0 <= (hr = reinterpret_cast<IMsRdpClient*>(pControl)->put_Server((BSTR)lpCreateParams)) &&
			0 <= (hr = reinterpret_cast<IMsRdpClient*>(pControl)->Connect());
			//if (sz = SysAllocString(L"Administrator"))
			//{
			//	hr = reinterpret_cast<IMsRdpClient*>(pControl)->put_UserName(sz);

			//	SysFreeString(sz);
			//}
		}

		return 0 <= hr;
	}

	virtual void DettachControl(IUnknown* pControl, HWND hwnd)
	{
		reinterpret_cast<IMsRdpClient*>(pControl)->Disconnect();
		SetSink(pControl, FALSE);
		__super::DettachControl(pControl, hwnd);
	}

	LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_NCLBUTTONDBLCLK:
			goto __0;
		case WM_SYSCOMMAND:
			switch (wParam)
			{
			case SC_MAXIMIZE:
__0:
				reinterpret_cast<IMsRdpClient*>(getControl())->put_FullScreen(VARIANT_TRUE);
				return 0;
			}
			break;
		}

		return __super::WindowProc(hwnd, uMsg, wParam, lParam);
	}

	~MyFrame()
	{
		RundownGUI();
	}
};

#define DNS_MAX_NAME_BUFFER_LENGTH      (256)
#include "resource.h"

class CSrvAddr : public ZDlg
{
	BSTR* ppServer;

	PCWSTR OnOk(HWND hwndDlg)
	{
		WCHAR sz[DNS_MAX_NAME_BUFFER_LENGTH];
		if (GetDlgItemTextW(hwndDlg, IDC_EDIT1, sz, _countof(sz)))
		{
			*ppServer = SysAllocString(sz);

			return 0;
		}

		return L"server name is empty";
	}

	virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_COMMAND:
			switch (wParam)
			{
			case IDOK:
				if (PCWSTR psz = OnOk(hwndDlg))
				{
					MessageBoxW(hwndDlg, psz, 0, MB_ICONWARNING);
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

	CSrvAddr(BSTR* ppServer) : ppServer(ppServer)
	{
	}
};

void WINAPI ep(void*)
{
	initterm();

	if (0 <= CoInitializeEx(0, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE))
	{
		BSTR pServer = 0;

		if (ZDlg* p = new CSrvAddr(&pServer))
		{
			p->DoModal((HINSTANCE)&__ImageBase, MAKEINTRESOURCEW(IDD_DIALOG1), 0, 0);
			p->Release();
		}

		if (pServer)
		{
			if (MyFrame* p = new MyFrame)
			{
				ZGLOBALS globals;
				ZApp app;

				HWND hwnd = p->Create(WS_EX_CLIENTEDGE, L"ms-rdp", WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, 0, pServer);

				p->Release();

				if (hwnd)
				{
					app.Run();
				}
			}

			SysFreeString(pServer);
		}
		CoUninitialize();
	}

	destroyterm();
	ExitProcess(0);
}

_NT_END