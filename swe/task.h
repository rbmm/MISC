#pragma once

struct __declspec(novtable) IExecTask : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Start() = 0;
	
	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;

	virtual HRESULT STDMETHODCALLTYPE Exec(_In_opt_ PCWSTR lpApplicationName, 
		_In_opt_ PCWSTR lpCommandLine, 
		_In_opt_ PCWSTR lpCurrentDirectory,
		_Out_ PPROCESS_INFORMATION lpProcessInformation) = 0;
	
	virtual HRESULT STDMETHODCALLTYPE EmbedTask(_In_ HWND hwnd, _In_ PPROCESS_INFORMATION lpProcessInformation) = 0;

	virtual void STDMETHODCALLTYPE Cleanup(_In_ PPROCESS_INFORMATION lpProcessInformation) = 0; 
};

HRESULT CreateTaskMngr(_Out_ IExecTask** ppExec);

HWND CreateShlWnd(_In_ HWND hWndParent, _In_ int nWidth, _In_ int nHeight);