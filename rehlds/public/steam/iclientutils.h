//==========================  Open Steamworks  ================================
//
// This file is part of the Open Steamworks project. All individuals associated
// with this project do not claim ownership of the contents
//
// The code, comments, and all related files, projects, resources,
// redistributables included with this project are Copyright Valve Corporation.
// Additionally, Valve, the Valve logo, Half-Life, the Half-Life logo, the
// Lambda logo, Steam, the Steam logo, Team Fortress, the Team Fortress logo,
// Opposing Force, Day of Defeat, the Day of Defeat logo, Counter-Strike, the
// Counter-Strike logo, Source, the Source logo, and Counter-Strike Condition
// Zero are trademarks and or registered trademarks of Valve Corporation.
// All other trademarks are property of their respective owners.
//
//=============================================================================
//
// Based on https://github.com/SteamRE/open-steamworks (IClientUtils.h).
// Internal steamclient interface, not part of the Steamworks SDK.
// Method order follows the IClientUtils IPC name table of the current steamclient
// (checked on the Windows and Linux builds); recheck when steamclient changes.
// Methods marked unknown_ret have unknown signatures, don't call them.
//
//=============================================================================

#ifndef ICLIENTUTILS_H
#define ICLIENTUTILS_H
#ifdef _WIN32
#pragma once
#endif

#include "steam_api_common.h"

#ifndef OSW_UNKNOWN_RET
#define OSW_UNKNOWN_RET
typedef int unknown_ret;
#endif

class IClientUtils
{
public:
	virtual const char *GetInstallPath() = 0;
	virtual const char *GetUserBaseFolderInstallImage() = 0;
	virtual const char *GetUserBaseFolderPersistentStorage() = 0;
	virtual const char *GetManagedContentRoot() = 0;

	// return the number of seconds since the user
	virtual uint32 GetSecondsSinceAppActive() = 0;
	virtual uint32 GetSecondsSinceComputerActive() = 0;
	virtual void SetComputerActive() = 0;

	// the universe this client is connecting to
	virtual EUniverse GetConnectedUniverse() = 0;
	virtual unknown_ret GetSteamRealm() = 0;

	// server time - in PST, number of seconds since January 1, 1970 (i.e unix time)
	virtual uint32 GetServerRealTime() = 0;

	// returns the 2 digit ISO 3166-1-alpha-2 format country code this client is running in (as looked up via an IP-to-location database)
	// e.g "US" or "UK".
	virtual const char *GetIPCountry() = 0;

	// returns true if the image exists, and valid sizes were filled out
	virtual bool GetImageSize( int32 iImage, uint32 *pnWidth, uint32 *pnHeight ) = 0;

	// returns true if the image exists, and the buffer was successfully filled out
	// results are returned in RGBA format
	// the destination buffer size should be 4 * height * width * sizeof(char)
	virtual bool GetImageRGBA( int32 iImage, uint8 *pubDest, int32 nDestBufferSize ) = 0;

	virtual uint32 GetNumRunningApps() = 0;

	// return the amount of battery power left in the current system in % [0..100], 255 for being on AC power
	virtual uint8 GetCurrentBatteryPower() = 0;
	virtual unknown_ret GetBatteryInformation() = 0;

	virtual void SetOfflineMode( bool bOffline ) = 0;
	virtual bool GetOfflineMode() = 0;

	virtual AppId_t SetAppIDForCurrentPipe( AppId_t nAppID, bool bTrackProcess ) = 0;
	virtual AppId_t GetAppID() = 0;

	virtual void SetAPIDebuggingActive( bool bActive, bool bVerbose ) = 0;

	// API asynchronous call results
	// can be used directly, but more commonly used via the callback dispatch API (see steam_api.h)
	virtual bool IsAPICallCompleted( SteamAPICall_t hSteamAPICall, bool *pbFailed ) = 0;
	virtual ESteamAPICallFailure GetAPICallFailureReason( SteamAPICall_t hSteamAPICall ) = 0;
	virtual bool GetAPICallResult( SteamAPICall_t hSteamAPICall, void *pCallback, int32 cubCallback, int32 iCallbackExpected, bool *pbFailed ) = 0;
	virtual unknown_ret AllocPendingAPICallHandle() = 0;
	virtual unknown_ret SetAPICallResultWithoutPostingCallback() = 0;

	virtual bool SignalAppsToShutDown() = 0;
	virtual unknown_ret SignalServiceAppsToDisconnect() = 0;
	virtual unknown_ret TerminateAllApps() = 0;

	virtual unknown_ret GetCellID() = 0;

	virtual bool BIsGlobalInstance() = 0;

	// Asynchronous call to check if file is signed, result is returned in CheckFileSignature_t
	virtual SteamAPICall_t CheckFileSignature( const char *szFileName ) = 0;

	virtual uint64 GetBuildID() = 0;

	virtual unknown_ret SetCurrentUIMode() = 0;
	virtual unknown_ret GetCurrentUIMode() = 0;
	virtual unknown_ret BIsWebBasedUIMode() = 0;
	virtual unknown_ret SetDisableOverlayScaling() = 0;

	virtual unknown_ret ShowGamepadTextInput() = 0;
	virtual uint32 GetEnteredGamepadTextLength() = 0;
	virtual bool GetEnteredGamepadTextInput( char *pchValue, uint32 cchValueMax ) = 0;
	virtual void GamepadTextInputClosed( HSteamPipe hSteamPipe, bool, const char * ) = 0;

	virtual unknown_ret SetSpew() = 0;

	virtual bool BDownloadsDisabled() = 0;

	virtual unknown_ret SetFocusedWindow() = 0;
	virtual const char *GetSteamUILanguage() = 0;

	virtual uint64 CheckSteamReachable() = 0;
	virtual unknown_ret SetLastGameLaunchMethod() = 0;
	virtual void SetVideoAdapterInfo( int32, int32, int32, int32, int32 ) = 0;
	virtual unknown_ret SetLauncherType() = 0;
	virtual unknown_ret GetLauncherType() = 0;
	virtual unknown_ret ShutdownLauncher() = 0;
	virtual unknown_ret SetOverlayWindowFocusForPipe() = 0;
	virtual unknown_ret GetGameOverlayUIInstanceFocusGameID() = 0;
	virtual unknown_ret GetFocusedGameWindow() = 0;

	virtual bool SetControllerConfigFileForAppID( AppId_t unAppID, const char * pszControllerConfigFile ) = 0;
	virtual bool GetControllerConfigFileForAppID( AppId_t unAppID, const char * pszControllerConfigFile, uint32 cubControllerConfigFile ) = 0;

	virtual bool IsSteamRunningInVR() = 0;
	virtual unknown_ret StartVRDashboard() = 0;
	virtual unknown_ret IsVRHeadsetStreamingEnabled() = 0;
	virtual unknown_ret SetVRHeadsetStreamingEnabled() = 0;
	virtual unknown_ret GenerateSupportSystemReport() = 0;
	virtual unknown_ret GetSupportSystemReport() = 0;
	virtual unknown_ret GetAppIdForPid() = 0;
	virtual unknown_ret BIsClientUIInForeground() = 0;
	virtual unknown_ret AllowSetForegroundThroughWebhelper() = 0;
	virtual unknown_ret SetClientUIProcess() = 0;
	virtual unknown_ret SetOverlayBrowserInfo() = 0;
	virtual unknown_ret ClearOverlayBrowserInfo() = 0;
	virtual unknown_ret GetOverlayBrowserInfo() = 0;
	virtual unknown_ret SetOverlayNotificationPosition() = 0;
	virtual unknown_ret SetOverlayNotificationInset() = 0;
	virtual unknown_ret DispatchClientUINotification() = 0;
	virtual unknown_ret RespondToClientUINotification() = 0;
	virtual unknown_ret DispatchClientUICommand() = 0;
	virtual unknown_ret DispatchComputerActiveStateChange() = 0;
	virtual unknown_ret DispatchOpenURLInClient() = 0;
	virtual unknown_ret UpdateWideVineCDM() = 0;
	virtual unknown_ret DispatchClearAllBrowsingData() = 0;
	virtual unknown_ret DispatchClientSettingsChanged() = 0;
	virtual unknown_ret DispatchClientPostMessage() = 0;
	virtual unknown_ret IsSteamChina() = 0;
	virtual unknown_ret NeedsSteamChinaWorkshop() = 0;
	virtual unknown_ret InitFilterText() = 0;
	virtual unknown_ret FilterText() = 0;
	virtual unknown_ret GetIPv6ConnectivityState() = 0;
	virtual unknown_ret RecordSteamInterfaceCreation() = 0;
	virtual unknown_ret GetCloudGamingPlatform() = 0;
	virtual unknown_ret TestHTTP() = 0;
	virtual unknown_ret DumpJobs() = 0;
	virtual unknown_ret SteamRuntimeSystemInfo() = 0;
	virtual unknown_ret BGetMacAddresses() = 0;
	virtual unknown_ret BGetDiskSerialNumber() = 0;
	virtual unknown_ret GetSteamEnvironmentForApp() = 0;
	virtual unknown_ret ShowFloatingGamepadTextInput() = 0;
	virtual unknown_ret DismissFloatingGamepadTextInput() = 0;
	virtual unknown_ret FloatingGamepadTextInputDismissed() = 0;
	virtual unknown_ret SetGameLauncherMode() = 0;
	virtual unknown_ret ScheduleConnectivityTest() = 0;
	virtual unknown_ret GetConnectivityTestState() = 0;
	virtual unknown_ret GetCaptivePortalURL() = 0;
	virtual unknown_ret ClearAllHTTPCaches() = 0;
	virtual unknown_ret ShowControllerLayoutPreview() = 0;
	virtual unknown_ret GetFocusedGameID() = 0;
	virtual unknown_ret GetFocusedWindowPID() = 0;
	virtual unknown_ret RecordFakeReactRouteMetric() = 0;
	virtual unknown_ret SetWebUITransportWebhelperPID() = 0;
	virtual unknown_ret GetWebUITransportInfo() = 0;
	virtual unknown_ret DumpHTTPClients() = 0;
	virtual unknown_ret BGetMachineID() = 0;
	virtual unknown_ret NotifyMissingInterface() = 0;
};

#endif // ICLIENTUTILS_H
