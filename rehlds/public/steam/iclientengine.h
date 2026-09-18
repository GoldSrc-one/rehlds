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
// Based on https://github.com/SteamRE/open-steamworks (IClientEngine.h).
// Internal steamclient interface, not part of the Steamworks SDK.
// Obtained from steamclient's CreateInterface( CLIENTENGINE_INTERFACE_VERSION ).
// Only slots 0-14 were checked against the current steamclient (Windows and Linux),
// the rest of the interface is left out; recheck when steamclient changes.
//
//=============================================================================

#ifndef ICLIENTENGINE_H
#define ICLIENTENGINE_H
#ifdef _WIN32
#pragma once
#endif

#include "steam_api_common.h"
#include "steamclientpublic.h"

#ifndef OSW_UNKNOWN_RET
#define OSW_UNKNOWN_RET
typedef int unknown_ret;
#endif

#define CLIENTENGINE_INTERFACE_VERSION "CLIENTENGINE_INTERFACE_VERSION005"

class IClientUser;
class IClientGameServer;
class IClientFriends;
class IClientUtils;

class IClientEngine
{
public:
	virtual HSteamPipe CreateSteamPipe() = 0;
	virtual bool BReleaseSteamPipe( HSteamPipe hSteamPipe ) = 0;

	virtual HSteamUser CreateGlobalUser( HSteamPipe* phSteamPipe ) = 0;
	virtual HSteamUser ConnectToGlobalUser( HSteamPipe hSteamPipe ) = 0;

	virtual HSteamUser CreateLocalUser( HSteamPipe* phSteamPipe, EAccountType eAccountType ) = 0;
	virtual void CreatePipeToLocalUser( HSteamUser hSteamUser, HSteamPipe* phSteamPipe ) = 0; // obsolete, asserts

	virtual void ReleaseUser( HSteamPipe hSteamPipe, HSteamUser hUser ) = 0;

	virtual bool IsValidHSteamUserPipe( HSteamPipe hSteamPipe, HSteamUser hUser ) = 0;

	// Signatures of the getters below are unverified, except GetIClientUtils
	virtual IClientUser *GetIClientUser( HSteamUser hSteamUser, HSteamPipe hSteamPipe ) = 0;
	virtual IClientGameServer *GetIClientGameServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe ) = 0;

	virtual void SetLocalIPBinding( const SteamIPAddress_t &unIP, uint16 usPort ) = 0;
	virtual char const *GetUniverseName( EUniverse eUniverse ) = 0;
	virtual unknown_ret Unknown12() = 0; // not in Open Steamworks

	virtual IClientFriends *GetIClientFriends( HSteamUser hSteamUser, HSteamPipe hSteamPipe ) = 0;
	virtual IClientUtils *GetIClientUtils( HSteamPipe hSteamPipe ) = 0;
};

#endif // ICLIENTENGINE_H
