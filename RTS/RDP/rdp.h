#pragma once

class DECLSPEC_UUID("7CACBD7B-0D99-468F-AC33-22E495C0AFE5") MsRdpClientNotSafeForScripting;

// Typedefs
//
enum AutoReconnectContinueState {
	autoReconnectContinueAutomatic,
	autoReconnectContinueStop,
	autoReconnectContinueManual,
};

enum RemoteWindowDisplayedAttribute {
	remoteAppWindowNone,
	remoteAppWindowDisplayed,
	remoteAppShellIconDisplayed,
};

enum RemoteProgramResult {
	remoteAppResultOk,
	remoteAppResultLocked,
	remoteAppResultProtocolError,
	remoteAppResultNotInWhitelist,
	remoteAppResultNetworkPathDenied,
	remoteAppResultFileNotFound,
	remoteAppResultFailure,
	remoteAppResultHookNotLoaded,
};

enum ExtendedDisconnectReasonCode {
	exDiscReasonNoInfo,
	exDiscReasonAPIInitiatedDisconnect,
	exDiscReasonAPIInitiatedLogoff,
	exDiscReasonServerIdleTimeout,
	exDiscReasonServerLogonTimeout,
	exDiscReasonReplacedByOtherConnection,
	exDiscReasonOutOfMemory,
	exDiscReasonServerDeniedConnection,
	exDiscReasonServerDeniedConnectionFips,
	exDiscReasonServerInsufficientPrivileges,
	exDiscReasonServerFreshCredsRequired,
	exDiscReasonLicenseInternal = 0x100,
	exDiscReasonLicenseNoLicenseServer,
	exDiscReasonLicenseNoLicense,
	exDiscReasonLicenseErrClientMsg,
	exDiscReasonLicenseHwidDoesntMatchLicense,
	exDiscReasonLicenseErrClientLicense,
	exDiscReasonLicenseCantFinishProtocol,
	exDiscReasonLicenseClientEndedProtocol,
	exDiscReasonLicenseErrClientEncryption,
	exDiscReasonLicenseCantUpgradeLicense,
	exDiscReasonLicenseNoRemoteConnections,
	exDiscReasonLicenseCreatingLicStoreAccDenied,
	exDiscReasonRdpEncInvalidCredentials = 0x300,
	exDiscReasonProtocolRangeStart = 0x1000,
	exDiscReasonProtocolRangeEnd = 0x7FFF,
};

enum ControlCloseStatus {
	controlCloseCanProceed,
	controlCloseWaitForEvents,
};

enum RedirectionWarningType {
	RedirectionWarningTypeDefault,
	RedirectionWarningTypeUnsigned,
	RedirectionWarningTypeUnknown,
	RedirectionWarningTypeUser,
	RedirectionWarningTypeThirdPartySigned,
	RedirectionWarningTypeTrusted,
};

enum RemoteSessionActionType {
	RemoteSessionActionCharms,
	RemoteSessionActionAppbar,
	RemoteSessionActionSnap,
	RemoteSessionActionStartScreen,
	RemoteSessionActionAppSwitch,
};

enum ClientSpec {
	FullMode,
	ThinClientMode,
	SmallCacheMode,
};

enum ControlReconnectStatus {
	controlReconnectStarted,
	controlReconnectBlocked,
};

enum RemoteActionType {
	RemoteActionCharms,
	RemoteActionAppbar,
	RemoteActionSnap,
	RemoteActionStartScreen,
	RemoteActionAppSwitch,
};

enum SnapshotEncodingType {
	SnapshotEncodingDataUri,
};

enum SnapshotFormatType {
	SnapshotFormatPng,
	SnapshotFormatJpeg,
	SnapshotFormatBmp,
};

//
// Interfaces
//

MIDL_INTERFACE("336d5562-efa8-482e-8cb3-c5c0fc7a7db6") IMsTscAxEvents : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE OnConnecting() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnConnected() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnLoginComplete() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnDisconnected(LONG discReason) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnEnterFullScreenMode() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnLeaveFullScreenMode() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnChannelReceivedData(BSTR chanName, BSTR data) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRequestGoFullScreen() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRequestLeaveFullScreen() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnFatalError(LONG errorCode) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnWarning(LONG warningCode) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRemoteDesktopSizeChange(LONG width, LONG height) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnIdleTimeoutNotification() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRequestContainerMinimize() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnConfirmClose(_Out_ VARIANT_BOOL* pfAllowClose) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnReceivedTSPublicKey(BSTR publicKey, _Out_ VARIANT_BOOL* pfContinueLogon) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAutoReconnecting(LONG disconnectReason, LONG attemptCount, _Out_ AutoReconnectContinueState* pArcContinueStatus) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAuthenticationWarningDisplayed() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAuthenticationWarningDismissed() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRemoteProgramResult(BSTR bstrRemoteProgram, RemoteProgramResult lError, VARIANT_BOOL vbIsExecutable) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRemoteProgramDisplayed(VARIANT_BOOL vbDisplayed, ULONG uDisplayInformation) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRemoteWindowDisplayed(VARIANT_BOOL vbDisplayed, LPVOID hwnd, RemoteWindowDisplayedAttribute windowAttribute) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnLogonError(LONG lError) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnFocusReleased(INT iDirection) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnUserNameAcquired(BSTR bstrUserName) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnMouseInputModeChanged(VARIANT_BOOL fMouseModeRelative) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnServiceMessageReceived(BSTR serviceMessage) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnConnectionBarPullDown() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnNetworkStatusChanged(ULONG qualityLevel, LONG bandwidth, LONG rtt) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnDevicesButtonPressed() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAutoReconnected() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAutoReconnecting2(LONG disconnectReason, VARIANT_BOOL networkAvailable, LONG attemptCount, LONG maxAttemptCount) = 0;
};

MIDL_INTERFACE("c9d65442-a0f9-45b2-8f73-d61d2db8cbb6") IMsTscSecuredSettings : IDispatch {
    virtual HRESULT STDMETHODCALLTYPE put_StartProgram(BSTR pStartProgram) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_StartProgram(_Out_ BSTR* pStartProgram) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_WorkDir(BSTR pWorkDir) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_WorkDir(_Out_ BSTR* pWorkDir) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_FullScreen(LONG pfFullScreen) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_FullScreen(_Out_ LONG* pfFullScreen) = 0;
};

MIDL_INTERFACE("809945cc-4b3b-4a92-a6b0-dbf9b5f2ef2d") IMsTscAdvancedSettings : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE put_Compress(LONG pcompress) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Compress(_Out_ LONG* pcompress) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapPeristence(LONG pbitmapPeristence) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapPeristence(_Out_ LONG* pbitmapPeristence) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_allowBackgroundInput(LONG pallowBackgroundInput) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_allowBackgroundInput(_Out_ LONG* pallowBackgroundInput) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_KeyBoardLayoutStr(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PluginDlls(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_IconFile(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_IconIndex(LONG _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ContainerHandledFullScreen(LONG pContainerHandledFullScreen) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ContainerHandledFullScreen(_Out_ LONG* pContainerHandledFullScreen) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DisableRdpdr(LONG pDisableRdpdr) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DisableRdpdr(_Out_ LONG* pDisableRdpdr) = 0;
};

MIDL_INTERFACE("209d0eb9-6254-47b1-9033-a98dae55bb27") IMsTscDebug : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE put_HatchBitmapPDU(LONG phatchBitmapPDU) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HatchBitmapPDU(_Out_ LONG* phatchBitmapPDU) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HatchSSBOrder(LONG phatchSSBOrder) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HatchSSBOrder(_Out_ LONG* phatchSSBOrder) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HatchMembltOrder(LONG phatchMembltOrder) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HatchMembltOrder(_Out_ LONG* phatchMembltOrder) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HatchIndexPDU(LONG phatchIndexPDU) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HatchIndexPDU(_Out_ LONG* phatchIndexPDU) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_LabelMemblt(LONG plabelMemblt) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_LabelMemblt(_Out_ LONG* plabelMemblt) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapCacheMonitor(LONG pbitmapCacheMonitor) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapCacheMonitor(_Out_ LONG* pbitmapCacheMonitor) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MallocFailuresPercent(LONG pmallocFailuresPercent) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MallocFailuresPercent(_Out_ LONG* pmallocFailuresPercent) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MallocHugeFailuresPercent(LONG pmallocHugeFailuresPercent) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MallocHugeFailuresPercent(_Out_ LONG* pmallocHugeFailuresPercent) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_NetThroughput(LONG NetThroughput) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_NetThroughput(_Out_ LONG* NetThroughput) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_CLXCmdLine(BSTR pCLXCmdLine) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_CLXCmdLine(_Out_ BSTR* pCLXCmdLine) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_CLXDll(BSTR pCLXDll) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_CLXDll(_Out_ BSTR* pCLXDll) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteProgramsHatchVisibleRegion(LONG pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgramsHatchVisibleRegion(_Out_ LONG* pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteProgramsHatchVisibleNoDataRegion(LONG pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgramsHatchVisibleNoDataRegion(_Out_ LONG* pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteProgramsHatchNonVisibleRegion(LONG pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgramsHatchNonVisibleRegion(_Out_ LONG* pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteProgramsHatchWindow(LONG pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgramsHatchWindow(_Out_ LONG* pcbHatch) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteProgramsStayConnectOnBadCaps(LONG pcbStayConnected) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgramsStayConnectOnBadCaps(_Out_ LONG* pcbStayConnected) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ControlType(_Out_ UINT* pControlType) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DecodeGfx(VARIANT_BOOL _arg1) = 0;
};

MIDL_INTERFACE("8c11efae-92c3-11d1-bc1e-00c04fa31489") IMsTscAx : IDispatch
{
  virtual HRESULT STDMETHODCALLTYPE put_Server(BSTR pServer) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Server(_Out_ BSTR* pServer) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_Domain(BSTR pDomain) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Domain(_Out_ BSTR* pDomain) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_UserName(BSTR pUserName) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_UserName(_Out_ BSTR* pUserName) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DisconnectedText(BSTR pDisconnectedText) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DisconnectedText(_Out_ BSTR* pDisconnectedText) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectingText(BSTR pConnectingText) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectingText(_Out_ BSTR* pConnectingText) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Connected(_Out_ short* pIsConnected) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DesktopWidth(LONG pVal) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DesktopWidth(_Out_ LONG* pVal) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DesktopHeight(LONG pVal) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DesktopHeight(_Out_ LONG* pVal) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_StartConnected(LONG pfStartConnected) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_StartConnected(_Out_ LONG* pfStartConnected) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HorizontalScrollBarVisible(_Out_ LONG* pfHScrollVisible) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_VerticalScrollBarVisible(_Out_ LONG* pfVScrollVisible) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_FullScreenTitle(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_CipherStrength(_Out_ LONG* pCipherStrength) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Version(_Out_ BSTR* pVersion) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SecuredSettingsEnabled(_Out_ LONG* pSecuredSettingsEnabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SecuredSettings(_Out_ IMsTscSecuredSettings** ppSecuredSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings(_Out_ IMsTscAdvancedSettings** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Debugger(_Out_ IMsTscDebug** ppDebugger) = 0;
  virtual HRESULT STDMETHODCALLTYPE Connect() = 0;
  virtual HRESULT STDMETHODCALLTYPE Disconnect() = 0;
  virtual HRESULT STDMETHODCALLTYPE CreateVirtualChannels(BSTR newVal) = 0;
  virtual HRESULT STDMETHODCALLTYPE SendOnVirtualChannel(BSTR chanName, BSTR ChanData) = 0;
};

MIDL_INTERFACE("3c65b4ab-12b3-465b-acd4-b8dad3bff9e2") IMsRdpClientAdvancedSettings : IMsTscAdvancedSettings {
  virtual HRESULT STDMETHODCALLTYPE put_SmoothScroll(LONG psmoothScroll) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SmoothScroll(_Out_ LONG* psmoothScroll) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AcceleratorPassthrough(LONG pacceleratorPassthrough) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AcceleratorPassthrough(_Out_ LONG* pacceleratorPassthrough) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ShadowBitmap(LONG pshadowBitmap) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ShadowBitmap(_Out_ LONG* pshadowBitmap) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_TransportType(LONG ptransportType) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_TransportType(_Out_ LONG* ptransportType) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_SasSequence(LONG psasSequence) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SasSequence(_Out_ LONG* psasSequence) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EncryptionEnabled(LONG pencryptionEnabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EncryptionEnabled(_Out_ LONG* pencryptionEnabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DedicatedTerminal(LONG pdedicatedTerminal) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DedicatedTerminal(_Out_ LONG* pdedicatedTerminal) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RDPPort(LONG prdpPort) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RDPPort(_Out_ LONG* prdpPort) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EnableMouse(LONG penableMouse) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EnableMouse(_Out_ LONG* penableMouse) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DisableCtrlAltDel(LONG pdisableCtrlAltDel) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DisableCtrlAltDel(_Out_ LONG* pdisableCtrlAltDel) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EnableWindowsKey(LONG penableWindowsKey) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EnableWindowsKey(_Out_ LONG* penableWindowsKey) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DoubleClickDetect(LONG pdoubleClickDetect) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DoubleClickDetect(_Out_ LONG* pdoubleClickDetect) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MaximizeShell(LONG pmaximizeShell) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MaximizeShell(_Out_ LONG* pmaximizeShell) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyFullScreen(LONG photKeyFullScreen) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyFullScreen(_Out_ LONG* photKeyFullScreen) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyCtrlEsc(LONG photKeyCtrlEsc) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyCtrlEsc(_Out_ LONG* photKeyCtrlEsc) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyAltEsc(LONG photKeyAltEsc) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyAltEsc(_Out_ LONG* photKeyAltEsc) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyAltTab(LONG photKeyAltTab) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyAltTab(_Out_ LONG* photKeyAltTab) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyAltShiftTab(LONG photKeyAltShiftTab) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyAltShiftTab(_Out_ LONG* photKeyAltShiftTab) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyAltSpace(LONG photKeyAltSpace) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyAltSpace(_Out_ LONG* photKeyAltSpace) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyCtrlAltDel(LONG photKeyCtrlAltDel) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyCtrlAltDel(_Out_ LONG* photKeyCtrlAltDel) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_orderDrawThreshold(LONG porderDrawThreshold) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_orderDrawThreshold(_Out_ LONG* porderDrawThreshold) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapCacheSize(LONG pbitmapCacheSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapCacheSize(_Out_ LONG* pbitmapCacheSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapVirtualCacheSize(LONG pbitmapVirtualCacheSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapVirtualCacheSize(_Out_ LONG* pbitmapVirtualCacheSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ScaleBitmapCachesByBPP(LONG pbScale) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ScaleBitmapCachesByBPP(_Out_ LONG* pbScale) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_NumBitmapCaches(LONG pnumBitmapCaches) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_NumBitmapCaches(_Out_ LONG* pnumBitmapCaches) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_CachePersistenceActive(LONG pcachePersistenceActive) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_CachePersistenceActive(_Out_ LONG* pcachePersistenceActive) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PersistCacheDirectory(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_brushSupportLevel(LONG pbrushSupportLevel) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_brushSupportLevel(_Out_ LONG* pbrushSupportLevel) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_minInputSendInterval(LONG pminInputSendInterval) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_minInputSendInterval(_Out_ LONG* pminInputSendInterval) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_InputEventsAtOnce(LONG pinputEventsAtOnce) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_InputEventsAtOnce(_Out_ LONG* pinputEventsAtOnce) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_maxEventCount(LONG pmaxEventCount) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_maxEventCount(_Out_ LONG* pmaxEventCount) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_keepAliveInterval(LONG pkeepAliveInterval) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_keepAliveInterval(_Out_ LONG* pkeepAliveInterval) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_shutdownTimeout(LONG pshutdownTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_shutdownTimeout(_Out_ LONG* pshutdownTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_overallConnectionTimeout(LONG poverallConnectionTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_overallConnectionTimeout(_Out_ LONG* poverallConnectionTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_singleConnectionTimeout(LONG psingleConnectionTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_singleConnectionTimeout(_Out_ LONG* psingleConnectionTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_KeyboardType(LONG pkeyboardType) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_KeyboardType(_Out_ LONG* pkeyboardType) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_KeyboardSubType(LONG pkeyboardSubType) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_KeyboardSubType(_Out_ LONG* pkeyboardSubType) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_KeyboardFunctionKey(LONG pkeyboardFunctionKey) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_KeyboardFunctionKey(_Out_ LONG* pkeyboardFunctionKey) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_WinceFixedPalette(LONG pwinceFixedPalette) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_WinceFixedPalette(_Out_ LONG* pwinceFixedPalette) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectToServerConsole(VARIANT_BOOL pConnectToConsole) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectToServerConsole(_Out_ VARIANT_BOOL* pConnectToConsole) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapPersistence(LONG pbitmapPersistence) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapPersistence(_Out_ LONG* pbitmapPersistence) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MinutesToIdleTimeout(LONG pminutesToIdleTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MinutesToIdleTimeout(_Out_ LONG* pminutesToIdleTimeout) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_SmartSizing(VARIANT_BOOL pfSmartSizing) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SmartSizing(_Out_ VARIANT_BOOL* pfSmartSizing) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RdpdrLocalPrintingDocName(BSTR pLocalPrintingDocName) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RdpdrLocalPrintingDocName(_Out_ BSTR* pLocalPrintingDocName) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RdpdrClipCleanTempDirString(BSTR clipCleanTempDirString) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RdpdrClipCleanTempDirString(_Out_ BSTR* clipCleanTempDirString) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RdpdrClipPasteInfoString(BSTR clipPasteInfoString) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RdpdrClipPasteInfoString(_Out_ BSTR* clipPasteInfoString) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ClearTextPassword(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DisplayConnectionBar(VARIANT_BOOL pDisplayConnectionBar) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DisplayConnectionBar(_Out_ VARIANT_BOOL* pDisplayConnectionBar) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PinConnectionBar(VARIANT_BOOL pPinConnectionBar) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PinConnectionBar(_Out_ VARIANT_BOOL* pPinConnectionBar) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GrabFocusOnConnect(VARIANT_BOOL pfGrabFocusOnConnect) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GrabFocusOnConnect(_Out_ VARIANT_BOOL* pfGrabFocusOnConnect) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_LoadBalanceInfo(BSTR pLBInfo) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_LoadBalanceInfo(_Out_ BSTR* pLBInfo) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectDrives(VARIANT_BOOL pRedirectDrives) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectDrives(_Out_ VARIANT_BOOL* pRedirectDrives) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectPrinters(VARIANT_BOOL pRedirectPrinters) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectPrinters(_Out_ VARIANT_BOOL* pRedirectPrinters) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectPorts(VARIANT_BOOL pRedirectPorts) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectPorts(_Out_ VARIANT_BOOL* pRedirectPorts) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectSmartCards(VARIANT_BOOL pRedirectSmartCards) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectSmartCards(_Out_ VARIANT_BOOL* pRedirectSmartCards) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapVirtualCache16BppSize(LONG pBitmapVirtualCache16BppSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapVirtualCache16BppSize(_Out_ LONG* pBitmapVirtualCache16BppSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapVirtualCache24BppSize(LONG pBitmapVirtualCache24BppSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapVirtualCache24BppSize(_Out_ LONG* pBitmapVirtualCache24BppSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PerformanceFlags(LONG pDisableList) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PerformanceFlags(_Out_ LONG* pDisableList) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectWithEndpoint(VARIANT* _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_NotifyTSPublicKey(VARIANT_BOOL pfNotify) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_NotifyTSPublicKey(_Out_ VARIANT_BOOL* pfNotify) = 0;
};

MIDL_INTERFACE("605befcf-39c1-45cc-a811-068fb7be346d") IMsRdpClientSecuredSettings : IMsTscSecuredSettings {
  virtual HRESULT STDMETHODCALLTYPE put_KeyboardHookMode(LONG pkeyboardHookMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_KeyboardHookMode(_Out_ LONG* pkeyboardHookMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AudioRedirectionMode(LONG pAudioRedirectionMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AudioRedirectionMode(_Out_ LONG* pAudioRedirectionMode) = 0;
};

MIDL_INTERFACE("92b4a539-7115-4b7c-a5a9-e5d9efc2780a") IMsRdpClient : IMsTscAx {
  virtual HRESULT STDMETHODCALLTYPE put_ColorDepth(LONG pcolorDepth) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ColorDepth(_Out_ LONG* pcolorDepth) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings2(_Out_ IMsRdpClientAdvancedSettings** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SecuredSettings2(_Out_ IMsRdpClientSecuredSettings** ppSecuredSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ExtendedDisconnectReason(_Out_ ExtendedDisconnectReasonCode* pExtendedDisconnectReason) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_FullScreen(VARIANT_BOOL pfFullScreen) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_FullScreen(_Out_ VARIANT_BOOL* pfFullScreen) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetVirtualChannelOptions(BSTR chanName, LONG chanOptions) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetVirtualChannelOptions(BSTR chanName, _Out_ LONG* pChanOptions) = 0;
  virtual HRESULT STDMETHODCALLTYPE RequestClose(_Out_ ControlCloseStatus* pCloseStatus) = 0;
};

MIDL_INTERFACE("c1e6743a-41c1-4a74-832a-0dd06c1c7a0e") IMsTscNonScriptable : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE put_ClearTextPassword(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PortablePassword(BSTR pPortablePass) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PortablePassword(_Out_ BSTR* pPortablePass) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PortableSalt(BSTR pPortableSalt) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PortableSalt(_Out_ BSTR* pPortableSalt) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BinaryPassword(BSTR pBinaryPassword) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BinaryPassword(_Out_ BSTR* pBinaryPassword) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BinarySalt(BSTR pSalt) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BinarySalt(_Out_ BSTR* pSalt) = 0;
  virtual HRESULT STDMETHODCALLTYPE ResetPassword() = 0;
};

MIDL_INTERFACE("2f079c4c-87b2-4afd-97ab-20cdb43038ae") IMsRdpClientNonScriptable : IMsTscNonScriptable {
  virtual HRESULT STDMETHODCALLTYPE NotifyRedirectDeviceChange(UINT_PTR wParam, LONG_PTR lParam) = 0;
  virtual HRESULT STDMETHODCALLTYPE SendKeys(LONG numKeys, VARIANT_BOOL* pbArrayKeyUp, LONG* plKeyData) = 0;
};

MIDL_INTERFACE("9ac42117-2b76-4320-aa44-0e616ab8437b") IMsRdpClientAdvancedSettings2 : IMsRdpClientAdvancedSettings {
  virtual HRESULT STDMETHODCALLTYPE get_CanAutoReconnect(_Out_ VARIANT_BOOL* pfCanAutoReconnect) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EnableAutoReconnect(VARIANT_BOOL pfEnableAutoReconnect) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EnableAutoReconnect(_Out_ VARIANT_BOOL* pfEnableAutoReconnect) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MaxReconnectAttempts(LONG pMaxReconnectAttempts) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MaxReconnectAttempts(_Out_ LONG* pMaxReconnectAttempts) = 0;
};

MIDL_INTERFACE("e7e17dc4-3b71-4ba7-a8e6-281ffadca28f") IMsRdpClient2 : IMsRdpClient {
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings3(_Out_ IMsRdpClientAdvancedSettings2** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectedStatusText(BSTR pConnectedStatusText) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectedStatusText(_Out_ BSTR* pConnectedStatusText) = 0;
};

MIDL_INTERFACE("19cd856b-c542-4c53-acee-f127e3be1a59") IMsRdpClientAdvancedSettings3 : IMsRdpClientAdvancedSettings2 {
  virtual HRESULT STDMETHODCALLTYPE put_ConnectionBarShowMinimizeButton(VARIANT_BOOL pfShowMinimize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectionBarShowMinimizeButton(_Out_ VARIANT_BOOL* pfShowMinimize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectionBarShowRestoreButton(VARIANT_BOOL pfShowRestore) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectionBarShowRestoreButton(_Out_ VARIANT_BOOL* pfShowRestore) = 0;
};

MIDL_INTERFACE("91b7cbc5-a72e-4fa0-9300-d647d7e897ff") IMsRdpClient3 : IMsRdpClient2 {
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings4(_Out_ IMsRdpClientAdvancedSettings3** ppAdvSettings) = 0;
};

MIDL_INTERFACE("fba7f64e-7345-4405-ae50-fa4a763dc0de") IMsRdpClientAdvancedSettings4 : IMsRdpClientAdvancedSettings3 {
  virtual HRESULT STDMETHODCALLTYPE put_AuthenticationLevel(UINT puiAuthLevel) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AuthenticationLevel(_Out_ UINT* puiAuthLevel) = 0;
};

MIDL_INTERFACE("095e0738-d97d-488b-b9f6-dd0e8d66c0de") IMsRdpClient4 : IMsRdpClient3 {
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings5(_Out_ IMsRdpClientAdvancedSettings4** ppAdvSettings) = 0;
};

MIDL_INTERFACE("17a5e535-4072-4fa4-af32-c8d0d47345e9") IMsRdpClientNonScriptable2 : IMsRdpClientNonScriptable {
  virtual HRESULT STDMETHODCALLTYPE put_UIParentWindowHandle(LPVOID phwndUIParentWindowHandle) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_UIParentWindowHandle(_Out_ LPVOID* phwndUIParentWindowHandle) = 0;
};

MIDL_INTERFACE("720298c0-a099-46f5-9f82-96921bae4701") IMsRdpClientTransportSettings : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE put_GatewayHostname(BSTR pProxyHostname) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayHostname(_Out_ BSTR* pProxyHostname) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayUsageMethod(ULONG pulProxyUsageMethod) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayUsageMethod(_Out_ ULONG* pulProxyUsageMethod) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayProfileUsageMethod(ULONG pulProxyProfileUsageMethod) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayProfileUsageMethod(_Out_ ULONG* pulProxyProfileUsageMethod) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayCredsSource(ULONG pulProxyCredsSource) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayCredsSource(_Out_ ULONG* pulProxyCredsSource) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayUserSelectedCredsSource(ULONG pulProxyCredsSource) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayUserSelectedCredsSource(_Out_ ULONG* pulProxyCredsSource) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayIsSupported(_Out_ LONG* pfProxyIsSupported) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayDefaultUsageMethod(_Out_ ULONG* pulProxyDefaultUsageMethod) = 0;
};

MIDL_INTERFACE("fba7f64e-6783-4405-da45-fa4a763dabd0") IMsRdpClientAdvancedSettings5 : IMsRdpClientAdvancedSettings4 {
  virtual HRESULT STDMETHODCALLTYPE put_RedirectClipboard(VARIANT_BOOL pfRedirectClipboard) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectClipboard(_Out_ VARIANT_BOOL* pfRedirectClipboard) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AudioRedirectionMode(UINT puiAudioRedirectionMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AudioRedirectionMode(_Out_ UINT* puiAudioRedirectionMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectionBarShowPinButton(VARIANT_BOOL pfShowPin) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectionBarShowPinButton(_Out_ VARIANT_BOOL* pfShowPin) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PublicMode(VARIANT_BOOL pfPublicMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PublicMode(_Out_ VARIANT_BOOL* pfPublicMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectDevices(VARIANT_BOOL pfRedirectPnPDevices) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectDevices(_Out_ VARIANT_BOOL* pfRedirectPnPDevices) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectPOSDevices(VARIANT_BOOL pfRedirectPOSDevices) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectPOSDevices(_Out_ VARIANT_BOOL* pfRedirectPOSDevices) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_BitmapVirtualCache32BppSize(LONG pBitmapVirtualCache32BppSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BitmapVirtualCache32BppSize(_Out_ LONG* pBitmapVirtualCache32BppSize) = 0;
};

MIDL_INTERFACE("fdd029f9-467a-4c49-8529-64b521dbd1b4") ITSRemoteProgram : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE put_RemoteProgramMode(VARIANT_BOOL pvboolRemoteProgramMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgramMode(_Out_ VARIANT_BOOL* pvboolRemoteProgramMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE ServerStartProgram(BSTR bstrExecutablePath, BSTR bstrFilePath, BSTR bstrWorkingDirectory, VARIANT_BOOL vbExpandEnvVarInWorkingDirectoryOnServer, BSTR bstrArguments, VARIANT_BOOL vbExpandEnvVarInArgumentsOnServer) = 0;
};

MIDL_INTERFACE("d012ae6d-c19a-4bfe-b367-201f8911f134") IMsRdpClientShell : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE Launch() = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RdpFileContents(BSTR pszRdpFile) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RdpFileContents(_Out_ BSTR* pszRdpFile) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetRdpProperty(BSTR szProperty, VARIANT Value) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetRdpProperty(BSTR szProperty, _Out_ VARIANT* pValue) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_IsRemoteProgramClientInstalled(_Out_ VARIANT_BOOL* pbClientInstalled) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PublicMode(VARIANT_BOOL pfPublicMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PublicMode(_Out_ VARIANT_BOOL* pfPublicMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE ShowTrustedSitesManagementDialog() = 0;
};

MIDL_INTERFACE("4eb5335b-6429-477d-b922-e06a28ecd8bf") IMsRdpClient5 : IMsRdpClient4 {
  virtual HRESULT STDMETHODCALLTYPE get_TransportSettings(_Out_ IMsRdpClientTransportSettings** ppXportSet) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings6(_Out_ IMsRdpClientAdvancedSettings5** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetErrorDescription(UINT disconnectReason, UINT ExtendedDisconnectReason, _Out_ BSTR* pBstrErrorMsg) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgram(_Out_ ITSRemoteProgram** ppRemoteProgram) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MsRdpClientShell(_Out_ IMsRdpClientShell** ppLauncher) = 0;
};

MIDL_INTERFACE("60c3b9c8-9e92-4f5e-a3e7-604a912093ea") IMsRdpDevice : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE get_DeviceInstanceId(_Out_ BSTR* pDevInstanceId) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_FriendlyName(_Out_ BSTR* pFriendlyName) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DeviceDescription(_Out_ BSTR* pDeviceDescription) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectionState(VARIANT_BOOL pvboolRedirState) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectionState(_Out_ VARIANT_BOOL* pvboolRedirState) = 0;
};

MIDL_INTERFACE("56540617-d281-488c-8738-6a8fdf64a118") IMsRdpDeviceCollection : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE RescanDevices(VARIANT_BOOL vboolDynRedir) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DeviceByIndex(ULONG index, _Out_ IMsRdpDevice** ppDevice) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DeviceById(BSTR devInstanceId, _Out_ IMsRdpDevice** ppDevice) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DeviceCount(_Out_ ULONG* pDeviceCount) = 0;
};

MIDL_INTERFACE("d28b5458-f694-47a8-8e61-40356a767e46") IMsRdpDrive : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE get_Name(_Out_ BSTR* pName) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectionState(VARIANT_BOOL pvboolRedirState) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectionState(_Out_ VARIANT_BOOL* pvboolRedirState) = 0;
};

MIDL_INTERFACE("7ff17599-da2c-4677-ad35-f60c04fe1585") IMsRdpDriveCollection : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE RescanDrives(VARIANT_BOOL vboolDynRedir) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DriveByIndex(ULONG index, _Out_ IMsRdpDrive** ppDevice) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DriveCount(_Out_ ULONG* pDriveCount) = 0;
};

MIDL_INTERFACE("b3378d90-0728-45c7-8ed7-b6159fb92219") IMsRdpClientNonScriptable3 : IMsRdpClientNonScriptable2 {
  virtual HRESULT STDMETHODCALLTYPE put_ShowRedirectionWarningDialog(VARIANT_BOOL pfShowRdrDlg) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ShowRedirectionWarningDialog(_Out_ VARIANT_BOOL* pfShowRdrDlg) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PromptForCredentials(VARIANT_BOOL pfPrompt) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PromptForCredentials(_Out_ VARIANT_BOOL* pfPrompt) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_NegotiateSecurityLayer(VARIANT_BOOL pfNegotiate) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_NegotiateSecurityLayer(_Out_ VARIANT_BOOL* pfNegotiate) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EnableCredSspSupport(VARIANT_BOOL pfEnableSupport) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EnableCredSspSupport(_Out_ VARIANT_BOOL* pfEnableSupport) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectDynamicDrives(VARIANT_BOOL pfRedirectDynamicDrives) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectDynamicDrives(_Out_ VARIANT_BOOL* pfRedirectDynamicDrives) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectDynamicDevices(VARIANT_BOOL pfRedirectDynamicDevices) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectDynamicDevices(_Out_ VARIANT_BOOL* pfRedirectDynamicDevices) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DeviceCollection(_Out_ IMsRdpDeviceCollection** ppDeviceCollection) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DriveCollection(_Out_ IMsRdpDriveCollection** ppDeviceCollection) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_WarnAboutSendingCredentials(VARIANT_BOOL pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_WarnAboutSendingCredentials(_Out_ VARIANT_BOOL* pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_WarnAboutClipboardRedirection(VARIANT_BOOL pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_WarnAboutClipboardRedirection(_Out_ VARIANT_BOOL* pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectionBarText(BSTR pConnectionBarText) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectionBarText(_Out_ BSTR* pConnectionBarText) = 0;
};

MIDL_INTERFACE("222c4b5d-45d9-4df0-a7c6-60cf9089d285") IMsRdpClientAdvancedSettings6 : IMsRdpClientAdvancedSettings5 {
  virtual HRESULT STDMETHODCALLTYPE put_RelativeMouseMode(VARIANT_BOOL pfRelativeMouseMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RelativeMouseMode(_Out_ VARIANT_BOOL* pfRelativeMouseMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AuthenticationServiceClass(_Out_ BSTR* pbstrAuthServiceClass) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AuthenticationServiceClass(BSTR pbstrAuthServiceClass) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PCB(_Out_ BSTR* bstrPCB) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PCB(BSTR bstrPCB) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyFocusReleaseLeft(LONG HotKeyFocusReleaseLeft) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyFocusReleaseLeft(_Out_ LONG* HotKeyFocusReleaseLeft) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_HotKeyFocusReleaseRight(LONG HotKeyFocusReleaseRight) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_HotKeyFocusReleaseRight(_Out_ LONG* HotKeyFocusReleaseRight) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EnableCredSspSupport(VARIANT_BOOL pfEnableSupport) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EnableCredSspSupport(_Out_ VARIANT_BOOL* pfEnableSupport) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AuthenticationType(_Out_ UINT* puiAuthType) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ConnectToAdministerServer(VARIANT_BOOL pConnectToAdministerServer) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ConnectToAdministerServer(_Out_ VARIANT_BOOL* pConnectToAdministerServer) = 0;
};

MIDL_INTERFACE("67341688-d606-4c73-a5d2-2e0489009319") IMsRdpClientTransportSettings2 : IMsRdpClientTransportSettings {
  virtual HRESULT STDMETHODCALLTYPE put_GatewayCredSharing(ULONG pulProxyCredSharing) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayCredSharing(_Out_ ULONG* pulProxyCredSharing) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayPreAuthRequirement(ULONG pulProxyPreAuthRequirement) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayPreAuthRequirement(_Out_ ULONG* pulProxyPreAuthRequirement) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayPreAuthServerAddr(BSTR pbstrProxyPreAuthServerAddr) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayPreAuthServerAddr(_Out_ BSTR* pbstrProxyPreAuthServerAddr) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewaySupportUrl(BSTR pbstrProxySupportUrl) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewaySupportUrl(_Out_ BSTR* pbstrProxySupportUrl) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayEncryptedOtpCookie(BSTR pbstrEncryptedOtpCookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayEncryptedOtpCookie(_Out_ BSTR* pbstrEncryptedOtpCookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayEncryptedOtpCookieSize(ULONG pulEncryptedOtpCookieSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayEncryptedOtpCookieSize(_Out_ ULONG* pulEncryptedOtpCookieSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayUsername(BSTR pProxyUsername) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayUsername(_Out_ BSTR* pProxyUsername) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayDomain(BSTR pProxyDomain) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayDomain(_Out_ BSTR* pProxyDomain) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayPassword(BSTR _arg1) = 0;
};

MIDL_INTERFACE("d43b7d80-8517-4b6d-9eac-96ad6800d7f2") IMsRdpClient6 : IMsRdpClient5 {
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings7(_Out_ IMsRdpClientAdvancedSettings6** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_TransportSettings2(_Out_ IMsRdpClientTransportSettings2** ppXportSet2) = 0;
};

MIDL_INTERFACE("f50fa8aa-1c7d-4f59-b15c-a90cacae1fcb") IMsRdpClientNonScriptable4 : IMsRdpClientNonScriptable3 {
  virtual HRESULT STDMETHODCALLTYPE put_RedirectionWarningType(RedirectionWarningType pWrnType) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectionWarningType(_Out_ RedirectionWarningType* pWrnType) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_MarkRdpSettingsSecure(VARIANT_BOOL pfRdpSecure) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_MarkRdpSettingsSecure(_Out_ VARIANT_BOOL* pfRdpSecure) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PublisherCertificateChain(VARIANT* pVarCert) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PublisherCertificateChain(_Out_ VARIANT* pVarCert) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_WarnAboutPrinterRedirection(VARIANT_BOOL pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_WarnAboutPrinterRedirection(_Out_ VARIANT_BOOL* pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AllowCredentialSaving(VARIANT_BOOL pfAllowSave) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AllowCredentialSaving(_Out_ VARIANT_BOOL* pfAllowSave) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PromptForCredsOnClient(VARIANT_BOOL pfPromptForCredsOnClient) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PromptForCredsOnClient(_Out_ VARIANT_BOOL* pfPromptForCredsOnClient) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_LaunchedViaClientShellInterface(VARIANT_BOOL pfLaunchedViaClientShellInterface) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_LaunchedViaClientShellInterface(_Out_ VARIANT_BOOL* pfLaunchedViaClientShellInterface) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_TrustedZoneSite(VARIANT_BOOL pfIsTrustedZone) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_TrustedZoneSite(_Out_ VARIANT_BOOL* pfIsTrustedZone) = 0;
};

MIDL_INTERFACE("26036036-4010-4578-8091-0db9a1edf9c3") IMsRdpClientAdvancedSettings7 : IMsRdpClientAdvancedSettings6 {
  virtual HRESULT STDMETHODCALLTYPE put_AudioCaptureRedirectionMode(VARIANT_BOOL pfRedir) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AudioCaptureRedirectionMode(_Out_ VARIANT_BOOL* pfRedir) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_VideoPlaybackMode(UINT pVideoPlaybackMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_VideoPlaybackMode(_Out_ UINT* pVideoPlaybackMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EnableSuperPan(VARIANT_BOOL pfEnableSuperPan) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EnableSuperPan(_Out_ VARIANT_BOOL* pfEnableSuperPan) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_SuperPanAccelerationFactor(ULONG puAccelFactor) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SuperPanAccelerationFactor(_Out_ ULONG* puAccelFactor) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_NegotiateSecurityLayer(VARIANT_BOOL pfNegotiate) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_NegotiateSecurityLayer(_Out_ VARIANT_BOOL* pfNegotiate) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AudioQualityMode(UINT pAudioQualityMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AudioQualityMode(_Out_ UINT* pAudioQualityMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RedirectDirectX(VARIANT_BOOL pfRedirectDirectX) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RedirectDirectX(_Out_ VARIANT_BOOL* pfRedirectDirectX) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_NetworkConnectionType(UINT pConnectionType) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_NetworkConnectionType(_Out_ UINT* pConnectionType) = 0;
};

MIDL_INTERFACE("3d5b21ac-748d-41de-8f30-e15169586bd4") IMsRdpClientTransportSettings3 : IMsRdpClientTransportSettings2 {
  virtual HRESULT STDMETHODCALLTYPE put_GatewayCredSourceCookie(ULONG pulProxyCredSourceCookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayCredSourceCookie(_Out_ ULONG* pulProxyCredSourceCookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayAuthCookieServerAddr(BSTR pbstrProxyAuthCookieServerAddr) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayAuthCookieServerAddr(_Out_ BSTR* pbstrProxyAuthCookieServerAddr) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayEncryptedAuthCookie(BSTR pbstrEncryptedAuthCookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayEncryptedAuthCookie(_Out_ BSTR* pbstrEncryptedAuthCookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayEncryptedAuthCookieSize(ULONG pulEncryptedAuthCookieSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayEncryptedAuthCookieSize(_Out_ ULONG* pulEncryptedAuthCookieSize) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_GatewayAuthLoginPage(BSTR pbstrProxyAuthLoginPage) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_GatewayAuthLoginPage(_Out_ BSTR* pbstrProxyAuthLoginPage) = 0;
};

MIDL_INTERFACE("25f2ce20-8b1d-4971-a7cd-549dae201fc0") IMsRdpClientSecuredSettings2 : IMsRdpClientSecuredSettings {
  virtual HRESULT STDMETHODCALLTYPE get_PCB(_Out_ BSTR* bstrPCB) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PCB(BSTR bstrPCB) = 0;
};

MIDL_INTERFACE("92c38a7d-241a-418c-9936-099872c9af20") ITSRemoteProgram2 : ITSRemoteProgram {
  virtual HRESULT STDMETHODCALLTYPE put_RemoteApplicationName(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteApplicationProgram(BSTR _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_RemoteApplicationArgs(BSTR _arg1) = 0;
};

MIDL_INTERFACE("b2a5b5ce-3461-444a-91d4-add26d070638") IMsRdpClient7 : IMsRdpClient6 {
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings8(_Out_ IMsRdpClientAdvancedSettings7** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_TransportSettings3(_Out_ IMsRdpClientTransportSettings3** ppXportSet3) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetStatusText(UINT statusCode, _Out_ BSTR* pBstrStatusText) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_SecuredSettings3(_Out_ IMsRdpClientSecuredSettings2** ppSecuredSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteProgram2(_Out_ ITSRemoteProgram2** ppRemoteProgram) = 0;
};

MIDL_INTERFACE("4f6996d5-d7b1-412c-b0ff-063718566907") IMsRdpClientNonScriptable5 : IMsRdpClientNonScriptable4 {
  virtual HRESULT STDMETHODCALLTYPE put_UseMultimon(VARIANT_BOOL pfUseMultimon) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_UseMultimon(_Out_ VARIANT_BOOL* pfUseMultimon) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteMonitorCount(_Out_ ULONG* pcRemoteMonitors) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetRemoteMonitorsBoundingBox(/*_Out_*/ LONG* pLeft, /*_Out_*/ LONG* pTop, /*_Out_*/ LONG* pRight, /*_Out_*/ LONG* pBottom) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_RemoteMonitorLayoutMatchesLocal(_Out_ VARIANT_BOOL* pfRemoteMatchesLocal) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DisableConnectionBar(VARIANT_BOOL _arg1) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_DisableRemoteAppCapsCheck(VARIANT_BOOL pfDisableRemoteAppCapsCheck) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_DisableRemoteAppCapsCheck(_Out_ VARIANT_BOOL* pfDisableRemoteAppCapsCheck) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_WarnAboutDirectXRedirection(VARIANT_BOOL pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_WarnAboutDirectXRedirection(_Out_ VARIANT_BOOL* pfWarn) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_AllowPromptingForCredentials(VARIANT_BOOL pfAllow) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AllowPromptingForCredentials(_Out_ VARIANT_BOOL* pfAllow) = 0;
};

MIDL_INTERFACE("fdd029f9-9574-4def-8529-64b521cccaa4") IMsRdpPreferredRedirectionInfo : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE put_UseRedirectionServerName(VARIANT_BOOL pVal) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_UseRedirectionServerName(_Out_ VARIANT_BOOL* pVal) = 0;
};

MIDL_INTERFACE("302d8188-0052-4807-806a-362b628f9ac5") IMsRdpExtendedSettings : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE put_Property(BSTR bstrPropertyName, VARIANT* pValue) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Property(BSTR bstrPropertyName, _Out_ VARIANT* pValue) = 0;
};

MIDL_INTERFACE("89acb528-2557-4d16-8625-226a30e97e9a") IMsRdpClientAdvancedSettings8 : IMsRdpClientAdvancedSettings7 {
  virtual HRESULT STDMETHODCALLTYPE put_BandwidthDetection(VARIANT_BOOL pfAutodetect) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_BandwidthDetection(_Out_ VARIANT_BOOL* pfAutodetect) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_ClientProtocolSpec(ClientSpec pClientMode) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ClientProtocolSpec(_Out_ ClientSpec* pClientMode) = 0;
};

MIDL_INTERFACE("4247e044-9271-43a9-bc49-e2ad9e855d62") IMsRdpClient8 : IMsRdpClient7 {
  virtual HRESULT STDMETHODCALLTYPE SendRemoteAction(RemoteSessionActionType actionType) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_AdvancedSettings9(_Out_ IMsRdpClientAdvancedSettings8** ppAdvSettings) = 0;
  virtual HRESULT STDMETHODCALLTYPE Reconnect(ULONG ulWidth, ULONG ulHeight, _Out_ ControlReconnectStatus* pReconnectStatus) = 0;
};

MIDL_INTERFACE("079863b7-6d47-4105-8bfe-0cdcb360e67d") IRemoteDesktopClientEvents : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE OnConnecting() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnConnected() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnLoginCompleted() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnDisconnected(LONG disconnectReason, LONG ExtendedDisconnectReason, BSTR disconnectErrorMessage) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnStatusChanged(LONG statusCode, BSTR statusMessage) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAutoReconnecting(LONG disconnectReason, LONG ExtendedDisconnectReason, BSTR disconnectErrorMessage, VARIANT_BOOL networkAvailable, LONG attemptCount, LONG maxAttemptCount) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAutoReconnected() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnDialogDisplaying() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnDialogDismissed() = 0;
  virtual HRESULT STDMETHODCALLTYPE OnNetworkStatusChanged(ULONG qualityLevel, LONG bandwidth, LONG rtt) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnAdminMessageReceived(BSTR adminMessage) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnKeyCombinationPressed(LONG keyCombination) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnRemoteDesktopSizeChanged(LONG width, LONG height) = 0;
  virtual HRESULT STDMETHODCALLTYPE OnTouchPointerCursorMoved(LONG x, LONG y) = 0;
};

MIDL_INTERFACE("48a0f2a7-2713-431f-bbac-6f4558e7d64d") IRemoteDesktopClientSettings : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE ApplySettings(BSTR RdpFileContents) = 0;
  virtual HRESULT STDMETHODCALLTYPE RetrieveSettings(_Out_ BSTR* RdpFileContents) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetRdpProperty(BSTR propertyName, _Out_ VARIANT* Value) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetRdpProperty(BSTR propertyName, VARIANT Value) = 0;
};

MIDL_INTERFACE("7d54bc4e-1028-45d4-8b0a-b9b6bffba176") IRemoteDesktopClientActions : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE SuspendScreenUpdates() = 0;
  virtual HRESULT STDMETHODCALLTYPE ResumeScreenUpdates() = 0;
  virtual HRESULT STDMETHODCALLTYPE ExecuteRemoteAction(RemoteActionType remoteAction) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetSnapshot(SnapshotEncodingType snapshotEncoding, SnapshotFormatType snapshotFormat, ULONG snapshotWidth, ULONG snapshotHeight, _Out_ BSTR* snapshotData) = 0;
};

MIDL_INTERFACE("260ec22d-8cbc-44b5-9e88-2a37f6c93ae9") IRemoteDesktopClientTouchPointer : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE put_Enabled(VARIANT_BOOL Enabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Enabled(_Out_ VARIANT_BOOL* Enabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_EventsEnabled(VARIANT_BOOL EventsEnabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_EventsEnabled(_Out_ VARIANT_BOOL* EventsEnabled) = 0;
  virtual HRESULT STDMETHODCALLTYPE put_PointerSpeed(ULONG PointerSpeed) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_PointerSpeed(_Out_ ULONG* PointerSpeed) = 0;
};

MIDL_INTERFACE("57d25668-625a-4905-be4e-304caa13f89c") IRemoteDesktopClient : IDispatch {
  virtual HRESULT STDMETHODCALLTYPE Connect() = 0;
  virtual HRESULT STDMETHODCALLTYPE Disconnect() = 0;
  virtual HRESULT STDMETHODCALLTYPE Reconnect(ULONG width, ULONG height) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Settings(_Out_ IRemoteDesktopClientSettings** Settings) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Actions(_Out_ IRemoteDesktopClientActions** Actions) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_TouchPointer(_Out_ IRemoteDesktopClientTouchPointer** TouchPointer) = 0;
  virtual HRESULT STDMETHODCALLTYPE DeleteSavedCredentials(BSTR serverName) = 0;
};

MIDL_INTERFACE("A0B2DD9A-7F53-4E65-8547-851952EC8C96") IMsRdpSessionManager : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE StartRemoteApplication(SAFEARRAY* psaCreds, SAFEARRAY* psaParams, LONG lFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProcessId(_Out_ ULONG* pulProcessId) = 0;
};