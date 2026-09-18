/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "precompiled.h"

int gCurrentCallbackGame = -1;
static uint64 gExtraSteamIDs[MAX_CLIENTS][MAX_EXTRA_GAMES];

static void Net_RequestSteamFakeIP(int sock);
static void Net_CloseFakeUDPPorts();

void CSteam3Server::OnGSPolicyResponse(GSPolicyResponse_t *pPolicyResponse)
{
	ISteamGameServer* sgs = num_extra_games == 0 ? CRehldsPlatformHolder::get()->SteamGameServer() : CRehldsPlatformHolder::get()->SteamGameServerExtra(gCurrentCallbackGame);
	if (sgs->BSecure())
		Con_Printf("   VAC secure mode is activated (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
	else
		Con_Printf("   VAC secure mode disabled (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
}

void CSteam3Server::OnLogonSuccess(SteamServersConnected_t *pLogonSuccess)
{
	auto& bLogOnResult = num_extra_games == 0 ? m_bLogOnResult : m_bLogOnResultExtra[gCurrentCallbackGame];
	if (bLogOnResult)
	{
		if (!m_bLanOnly)
			Con_Printf("Reconnected to Steam servers (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
	}
	else
	{
		bLogOnResult = true;
		if (!m_bLanOnly)
			Con_Printf("Connection to Steam servers successful (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
	}

	ISteamGameServer* sgs = num_extra_games == 0 ? CRehldsPlatformHolder::get()->SteamGameServer() : CRehldsPlatformHolder::get()->SteamGameServerExtra(gCurrentCallbackGame);
	m_SteamIDGS = sgs->GetSteamID();

	CSteam3Server::SendUpdatedServerDetails();
}

uint64 CSteam3Server::GetSteamID()
{
	if (m_bLanOnly)
		return CSteamID(0, k_EUniversePublic, k_EAccountTypeInvalid).ConvertToUint64();
	else
		return m_SteamIDGS.ConvertToUint64();
}

void CSteam3Server::OnLogonFailure(SteamServerConnectFailure_t *pLogonFailure)
{
	auto& bLogOnResult = num_extra_games == 0 ? m_bLogOnResult : m_bLogOnResultExtra[gCurrentCallbackGame];
	if (!bLogOnResult)
	{
		if (pLogonFailure->m_eResult == k_EResultServiceUnavailable)
		{
			if (!m_bLanOnly)
			{
				Con_Printf("Connection to Steam servers successful (SU) (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
				if (m_bWantToBeSecure)
				{
					Con_Printf("   VAC secure mode not available (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
					bLogOnResult = true;
					return;
				}
			}
		}
		else
		{
			if (!m_bLanOnly)
				Con_Printf("Could not establish connection to Steam servers (%s).\n", num_extra_games ? extra_games[gCurrentCallbackGame] : com_gamedir);
		}
	}

	bLogOnResult = true;
}

void CSteam3Server::OnGSClientDeny(GSClientDeny_t *pGSClientDeny)
{
	client_t* cl = CSteam3Server::ClientFindFromSteamID(pGSClientDeny->m_SteamID);
	if (cl)
		OnGSClientDenyHelper(cl, pGSClientDeny->m_eDenyReason, pGSClientDeny->m_rgchOptionalText);
}

void CSteam3Server::OnGSClientDenyHelper(client_t *cl, EDenyReason eDenyReason, const char *pchOptionalText)
{
	switch (eDenyReason)
	{
	case k_EDenyInvalidVersion:
		SV_DropClient(cl, 0, "Client version incompatible with server. \nPlease exit and restart");
		break;

	case k_EDenyNotLoggedOn:
		if (!m_bLanOnly)
			SV_DropClient(cl, 0, "No Steam logon\n");
		break;

	case k_EDenyLoggedInElseWhere:
		if (!m_bLanOnly)
			SV_DropClient(cl, 0, "This Steam account is being used in another location\n");
		break;

	case k_EDenyNoLicense:
		SV_DropClient(cl, 0, "This Steam account does not own this game. \nPlease login to the correct Steam account.");
		break;

	case k_EDenyCheater:
		SV_DropClient(cl, 0, "VAC banned from secure server\n");
		break;

	case k_EDenyUnknownText:
		if (pchOptionalText && *pchOptionalText)
			SV_DropClient(cl, 0, pchOptionalText);
		else
			SV_DropClient(cl, 0, "Client dropped by server");
		break;

	case k_EDenyIncompatibleAnticheat:
		SV_DropClient(cl, 0, "You are running an external tool that is incompatible with Secure servers.");
		break;

	case k_EDenyMemoryCorruption:
		SV_DropClient(cl, 0, "Memory corruption detected.");
		break;

	case k_EDenyIncompatibleSoftware:
		SV_DropClient(cl, 0, "You are running software that is not compatible with Secure servers.");
		break;

	case k_EDenySteamConnectionLost:
		if (!m_bLanOnly)
			SV_DropClient(cl, 0, "Steam connection lost\n");
		break;

	case k_EDenySteamConnectionError:
		if (!m_bLanOnly)
			SV_DropClient(cl, 0, "Unable to connect to Steam\n");
		break;

	case k_EDenySteamResponseTimedOut:
		SV_DropClient(cl, 0, "Client timed out while answering challenge.\n---> Please make sure that you have opened the appropriate ports on any firewall you are connected behind.\n---> See http://support.steampowered.com for help with firewall configuration.");
		break;

	case k_EDenySteamValidationStalled:
		if (m_bLanOnly)
			cl->network_userid.m_SteamID = 1;
		break;

	default:
		SV_DropClient(cl, 0, "Client dropped by server");
		break;
	}
}

void CSteam3Server::OnGSClientApprove(GSClientApprove_t *pGSClientSteam2Accept)
{
	client_t* cl = ClientFindFromSteamID(pGSClientSteam2Accept->m_SteamID);
	if (!cl)
		return;

	char msg[512];
	Q_snprintf(msg, ARRAYSIZE(msg), "\"%s<%i><%s><>\" STEAM USERID validated\n", cl->name, cl->userid, SV_GetClientIDString(cl));
#ifdef REHLDS_CHECKS
	msg[ARRAYSIZE(msg) - 1] = 0;
#endif
	Con_DPrintf("%s", msg);
	Log_Printf("%s", msg);
}

void CSteam3Server::OnGSClientKick(GSClientKick_t *pGSClientKick)
{
	client_t* cl = CSteam3Server::ClientFindFromSteamID(pGSClientKick->m_SteamID);
	if (cl)
		CSteam3Server::OnGSClientDenyHelper(cl, pGSClientKick->m_eDenyReason, 0);
}

client_t *CSteam3Server::ClientFindFromSteamID(CSteamID &steamIDFind)
{
	for (int i = 0; i < g_psvs.maxclients; i++)
	{
		auto cl = &g_psvs.clients[i];
		if (!cl->connected && !cl->active && !cl->spawned)
			continue;

		if (cl->network_userid.idtype != AUTH_IDTYPE_STEAM)
			continue;

		if (steamIDFind == cl->network_userid.m_SteamID)
			return cl;
	}

	return NULL;
}

CSteam3Server::CSteam3Server() :
	m_CallbackGSClientApprove(this, &CSteam3Server::OnGSClientApprove),
	m_CallbackGSClientDeny(this, &CSteam3Server::OnGSClientDeny),
	m_CallbackGSClientKick(this, &CSteam3Server::OnGSClientKick),
	m_CallbackGSPolicyResponse(this, &CSteam3Server::OnGSPolicyResponse),
	m_CallbackLogonSuccess(this, &CSteam3Server::OnLogonSuccess),
	m_CallbackLogonFailure(this, &CSteam3Server::OnLogonFailure),
	m_SteamIDGS(1, 0, k_EUniverseInvalid, k_EAccountTypeInvalid)
{
#ifdef REHLDS_FIXES
	m_GameTagsData[0] = '\0';
#endif

	m_bHasActivePlayers = false;
	m_bWantToBeSecure = false;
	m_bLanOnly = false;
}

void CSteam3Server::Activate()
{
	bool bLanOnly;
	EServerMode eSMode;
	int gamePort;
	char gamedir[MAX_PATH];
	uint32 unIP;

	if (m_bLoggedOn)
	{
		bLanOnly = sv_lan.value != 0.0;
		if (m_bLanOnly != bLanOnly)
		{
			m_bLanOnly = bLanOnly;
			m_bWantToBeSecure = !COM_CheckParm("-insecure") && !bLanOnly;
		}
	}
	else
	{
		m_bLoggedOn = true;
		unIP = 0;
		eSMode = eServerModeAuthenticationAndSecure;
		if (net_local_adr.type == NA_IP)
			unIP = ntohl(*(u_long *)&net_local_adr.ip[0]);

		m_bLanOnly = sv_lan.value > 0.0;
		m_bWantToBeSecure = !COM_CheckParm("-insecure") && !m_bLanOnly;
		COM_FileBase(com_gamedir, gamedir);

		if (!m_bWantToBeSecure)
			eSMode = eServerModeAuthentication;

		if (m_bLanOnly)
			eSMode = eServerModeNoAuthentication;

		gamePort = (int)iphostport.value;
		if (gamePort == 0)
			gamePort = (int)hostport.value;

		int nAppId = GetGameAppID();
		if (nAppId > 0 && g_pcls.state == ca_dedicated)
		{
			FILE* f = fopen("steam_appid.txt", "w+");
			if (f)
			{
				fprintf(f, "%d\n", nAppId);
				fclose(f);
			}
		}

		if(num_extra_games == 0) {
			if(!CRehldsPlatformHolder::get()->SteamGameServer_Init(unIP, 0, gamePort, 0xFFFFu, eSMode, gpszVersionString))
				Sys_Error("Unable to initialize Steam.");

			CRehldsPlatformHolder::get()->SteamGameServer()->SetProduct(gpszProductString);
			CRehldsPlatformHolder::get()->SteamGameServer()->SetModDir(gamedir);
			CRehldsPlatformHolder::get()->SteamGameServer()->SetDedicatedServer(g_pcls.state == ca_dedicated);
			CRehldsPlatformHolder::get()->SteamGameServer()->SetGameDescription(gEntityInterface.pfnGetGameDescription());
			Net_RequestSteamFakeIP(NS_SERVER);
			CRehldsPlatformHolder::get()->SteamGameServer()->LogOnAnonymous();
		}
		else {
			for(int iGame = 0; iGame < num_extra_games; iGame++) {
				if(!CRehldsPlatformHolder::get()->SteamGameServer_InitExtra(unIP, 0, gamePort + iGame, 0xFFFFu, eSMode, gpszVersionString, iGame))
					Sys_Error("Unable to initialize Steam.");

				CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->SetProduct(gpszProductString);
				CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->SetModDir(extra_games[iGame]);
				CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->SetDedicatedServer(g_pcls.state == ca_dedicated);
				CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->SetGameDescription(gEntityInterface.pfnGetGameDescription());
				Net_RequestSteamFakeIP(NS_EXTRA + iGame);
				CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->LogOnAnonymous();
			}
		}

		m_bLogOnResult = false;
		memset(m_bLogOnResultExtra, false, sizeof(m_bLogOnResultExtra));

		if (COM_CheckParm("-nomaster"))
		{
			Con_Printf("Master server communication disabled.\n");
			gfNoMasterServer = TRUE;
		}
		else
		{
			if (!gfNoMasterServer && g_psvs.maxclients > 1)
			{
				CRehldsPlatformHolder::get()->SteamGameServerDo([] (auto sgs) { sgs->SetAdvertiseServerActive(true); });
				Net_CheckOpenFakeUDPPorts();

				CSteam3Server::NotifyOfLevelChange(true);
			}
		}
	}
}

void CSteam3Server::Shutdown()
{
	if (m_bLoggedOn)
	{
		CRehldsPlatformHolder::get()->SteamGameServerDo([] (auto sgs) {
			sgs->SetAdvertiseServerActive(0);
			sgs->LogOff();
		});
		Net_CloseFakeUDPPorts();
		CRehldsPlatformHolder::get()->SteamGameServer_Shutdown();
		m_bLoggedOn = false;
		if (ip_sockets[NS_SERVER] != INV_SOCK)
			NET_GetLocalAddress();
	}
}

bool CSteam3Server::NotifyClientConnect(client_t *client, const void *pvSteam2Key, uint32 ucbSteam2Key)
{
	class CSteamID steamIDClient;
	bool bRet = false;

	if (client == NULL || !m_bLoggedOn)
		return false;

	client->network_userid.idtype = AUTH_IDTYPE_STEAM;
	bRet = CRehldsPlatformHolder::get()->SteamGameServerExtra(client->m_sock)->SendUserConnectAndAuthenticate_DEPRECATED(htonl(client->network_userid.clientip), pvSteam2Key, ucbSteam2Key, &steamIDClient);
	client->network_userid.m_SteamID = steamIDClient.ConvertToUint64();

	return bRet;
}

bool CSteam3Server::NotifyClientConnectExtra(client_t* client) {
	if(client == NULL || !m_bLoggedOn)
		return false;

	int clientIndex = client - g_psvs.clients;
	for(int iGame = 0; iGame < num_extra_games; iGame++) {
		if(iGame == client->m_sock - NS_EXTRA)
			gExtraSteamIDs[clientIndex][iGame] = client->network_userid.m_SteamID;
		else
			gExtraSteamIDs[clientIndex][iGame] = CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->CreateUnauthenticatedUserConnection().ConvertToUint64();
	}

	return true;
}

bool CSteam3Server::NotifyBotConnect(client_t *client)
{
	if (client == NULL || !m_bLoggedOn)
		return false;

	client->network_userid.idtype = AUTH_IDTYPE_LOCAL;
	CSteamID steamId = CRehldsPlatformHolder::get()->SteamGameServerExtra(client->m_sock)->CreateUnauthenticatedUserConnection();
	client->network_userid.m_SteamID = steamId.ConvertToUint64();

	NotifyClientConnectExtra(client);

	return true;
}

void CSteam3Server::NotifyClientDisconnect(client_t *cl)
{
	if (!cl || !m_bLoggedOn)
		return;

	if(num_extra_games == 0) {
		if(cl->network_userid.idtype == AUTH_IDTYPE_STEAM || cl->network_userid.idtype == AUTH_IDTYPE_LOCAL)
			CRehldsPlatformHolder::get()->SteamGameServer()->SendUserDisconnect_DEPRECATED(cl->network_userid.m_SteamID);

		return;
	}

	int clientIndex = cl - g_psvs.clients;
	for(int iGame = 0; iGame < num_extra_games; iGame++) {
		if(gExtraSteamIDs[clientIndex][iGame])
			CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->SendUserDisconnect_DEPRECATED(gExtraSteamIDs[clientIndex][iGame]);
	}

	for(int iGame = 0; iGame < num_extra_games; iGame++)
		gExtraSteamIDs[clientIndex][iGame] = 0;
}

void CSteam3Server::NotifyOfLevelChange(bool bForce)
{
	SendUpdatedServerDetails();
	bool iHasPW = (sv_password.string[0] && Q_stricmp(sv_password.string, "none"));
	CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) {
		sgs->SetPasswordProtected(iHasPW);
		sgs->ClearAllKeyValues();
	});
	
	for (cvar_t *var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & FCVAR_SERVER))
			continue;

		const char *szVal;
		if (var->flags & FCVAR_PROTECTED)
		{
			if (Q_strlen(var->string) > 0 && Q_stricmp(var->string, "none"))
				szVal = "1";
			else
				szVal = "0";
		}
		else
		{
			szVal = var->string;
		}

		CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) { sgs->SetKeyValue(var->name, szVal); });
	}
}

void CSteam3Server::RunFrame()
{
	bool bHasPlayers;
	char szOutBuf[4096];
	double fCurTime;

	static double s_fLastRunFragsUpdate;
	static double s_fLastRunCallback;
	static double s_fLastRunSendPackets;

	if (g_psvs.maxclients <= 1)
		return;

	fCurTime = Sys_FloatTime();
	if (fCurTime - s_fLastRunFragsUpdate > 1.0)
	{
		s_fLastRunFragsUpdate = fCurTime;
		bHasPlayers = false;
		for (int i = 0; i < g_psvs.maxclients; i++)
		{
			client_t* cl = &g_psvs.clients[i];
			if (cl->active || cl->spawned || cl->connected)
			{
				bHasPlayers = true;
				break;
			}
		}

		m_bHasActivePlayers = bHasPlayers;
		SendUpdatedServerDetails();
		bool iHasPW = (sv_password.string[0] && Q_stricmp(sv_password.string, "none"));
		CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) { sgs->SetPasswordProtected(iHasPW); });

#ifdef REHLDS_FIXES
		// Let's get it an up-to-date description of the game
		CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) { sgs->SetGameDescription(gEntityInterface.pfnGetGameDescription()); });
#endif

		for (int i = 0; i < g_psvs.maxclients; i++)
		{
			client_t* cl = &g_psvs.clients[i];
			if (!cl->active)
				continue;

#ifdef REHLDS_FIXES
			ISteamGameServer_BUpdateUserData(cl->network_userid.m_SteamID, cl->name, cl->edict->v.frags);
#else
			CRehldsPlatformHolder::get()->SteamGameServer()->BUpdateUserData(cl->network_userid.m_SteamID, cl->name, cl->edict->v.frags);
#endif
		}

		bool wasRestartRequested = false;
		CRehldsPlatformHolder::get()->SteamGameServerDo([&] (auto sgs) { wasRestartRequested |= sgs->WasRestartRequested(); });
		if (wasRestartRequested)
		{
			Con_Printf("%cMasterRequestRestart\n", 3);
			if (COM_CheckParm("-steam"))
			{
				Con_Printf("Your server needs to be restarted in order to receive the latest update.\n");
				Log_Printf("Your server needs to be restarted in order to receive the latest update.\n");
			}
			else
			{
				Con_Printf("Your server is out of date.  Please update and restart.\n");
			}
		}
	}

	if (fCurTime - s_fLastRunCallback > 0.1)
	{
		CRehldsPlatformHolder::get()->SteamGameServer_RunCallbacks();
		NET_UpdateSteamFakeIP();
		s_fLastRunCallback = fCurTime;
	}

	if (fCurTime - s_fLastRunSendPackets > 0.01)
	{
		s_fLastRunSendPackets = fCurTime;

		if(num_extra_games == 0) {
			uint16 port;
			uint32 ip;
			int iLen = CRehldsPlatformHolder::get()->SteamGameServer()->GetNextOutgoingPacket(szOutBuf, sizeof(szOutBuf), &ip, &port);
			while(iLen > 0) {
				netadr_t netAdr;
				*((uint32*)&netAdr.ip[0]) = htonl(ip);
				netAdr.port = htons(port);
				netAdr.type = NA_IP;

				NET_SendPacket(NS_SERVER, iLen, szOutBuf, netAdr);

				iLen = CRehldsPlatformHolder::get()->SteamGameServer()->GetNextOutgoingPacket(szOutBuf, sizeof(szOutBuf), &ip, &port);
			}
		}
		else {
			for(int iGame = 0; iGame < num_extra_games; iGame++) {
				uint16 port;
				uint32 ip;
				int iLen = CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->GetNextOutgoingPacket(szOutBuf, sizeof(szOutBuf), &ip, &port);
				while(iLen > 0) {
					netadr_t netAdr;
					*((uint32*)&netAdr.ip[0]) = htonl(ip);
					netAdr.port = htons(port);
					netAdr.type = NA_IP;

					NET_SendPacket((netsrc_t)(NS_EXTRA + iGame), iLen, szOutBuf, netAdr);

					iLen = CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->GetNextOutgoingPacket(szOutBuf, sizeof(szOutBuf), &ip, &port);
				}
			}
		}
	}
}

void CSteam3Server::UpdateGameTags()
{
#ifdef REHLDS_FIXES
	if (!m_GameTagsData[0] && !sv_tags.string[0])
		return;

	if (m_GameTagsData[0] && !Q_stricmp(m_GameTagsData, sv_tags.string))
		return;

	Q_strlcpy(m_GameTagsData, sv_tags.string);
	Q_strlwr(m_GameTagsData);
	CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) { sgs->SetGameTags(m_GameTagsData); });
#endif
}

void CSteam3Server::SendUpdatedServerDetails()
{
	int botCount = 0;
	if (g_psvs.maxclients > 0)
	{

		for (int i = 0; i < g_psvs.maxclients; i++)
		{
			auto cl = &g_psvs.clients[i];
			if ((cl->active || cl->spawned || cl->connected) && cl->fakeclient)
				++botCount;
		}
	}

	int maxPlayers = sv_visiblemaxplayers.value;
	if (maxPlayers < 0)
		maxPlayers = g_psvs.maxclients;

	CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) {
		sgs->SetMaxPlayerCount(maxPlayers);
		sgs->SetBotPlayerCount(botCount);
		sgs->SetServerName(Cvar_VariableString("hostname"));
		sgs->SetMapName(g_psv.name);
	});

	UpdateGameTags();
}

void CSteam3Client::Shutdown()
{
	if (m_bLoggedOn)
	{
		SteamAPI_Shutdown();
		m_bLoggedOn = false;
	}
}

int CSteam3Client::InitiateGameConnection(void *pData, int cbMaxData, uint64 steamID, uint32 unIPServer, uint16 usPortServer, bool bSecure)
{
	return SteamUser()->InitiateGameConnection_DEPRECATED(pData, cbMaxData, CSteamID(steamID), ntohl(unIPServer), ntohs(usPortServer), bSecure);
}

void CSteam3Client::TerminateConnection(uint32 unIPServer, uint16 usPortServer)
{
	SteamUser()->TerminateGameConnection_DEPRECATED(ntohl(unIPServer), ntohs(usPortServer));
}

void CSteam3Client::InitClient()
{
	if (m_bLoggedOn)
		return;

	m_bLoggedOn = true;
	_unlink("steam_appid.txt");
	if (!getenv("SteamAppId"))
	{
		int nAppID = GetGameAppID();
		if (nAppID > 0)
		{
			FILE* f = fopen("steam_appid.txt", "w+");
			if (f)
			{
				fprintf(f, "%d\n", nAppID);
				fclose(f);
			}
		}
	}

	if (!SteamAPI_Init())
		Sys_Error("Failed to initalize authentication interface. Exiting...\n");

	m_bLogOnResult = false;
	memset(m_bLogOnResultExtra, false, sizeof(m_bLogOnResultExtra));
}

void CSteam3Client::OnClientGameServerDeny(ClientGameServerDeny_t *pClientGameServerDeny)
{
	COM_ExplainDisconnection(TRUE, "Invalid server version, unable to connect.");
	CL_Disconnect();
}

void CSteam3Client::OnGameServerChangeRequested(GameServerChangeRequested_t *pGameServerChangeRequested)
{
#ifndef SWDS
	char *cmd;

	Cvar_DirectSet(&password, pGameServerChangeRequested->m_rgchPassword);
	Con_Printf("Connecting to %s\n", pGameServerChangeRequested->m_rgchServer);
	cmd = va("connect %s\n", pGameServerChangeRequested->m_rgchServer);
	Cbuf_AddText(cmd);
#endif
}

void CSteam3Client::OnGameOverlayActivated(GameOverlayActivated_t *pGameOverlayActivated)
{
#ifndef SWDS
	if (Host_IsSinglePlayerGame())
	{
		if (pGameOverlayActivated->m_bActive)
		{
			Cbuf_AddText("setpause;");
		}
		else
		{
			if (!(unsigned __int8)(*(int(**)())(*(_DWORD *)g_pGameUI007 + 44))())
			{
				Cbuf_AddText("unpause;");
			}
		}
	}
#endif
}

void CSteam3Client::RunFrame()
{
	CRehldsPlatformHolder::get()->SteamAPI_RunCallbacks();
}

uint64 ISteamGameServer_CreateUnauthenticatedUserConnection(client_t* fakeclient)
{
	if(num_extra_games == 0)
		return CRehldsPlatformHolder::get()->SteamGameServer()->CreateUnauthenticatedUserConnection().ConvertToUint64();

	int clientIndex = fakeclient - g_psvs.clients;
	for(int iGame = 0; iGame < num_extra_games; iGame++) {
		gExtraSteamIDs[clientIndex][iGame] = CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->CreateUnauthenticatedUserConnection().ConvertToUint64();
	}

	return gExtraSteamIDs[clientIndex][0];
}

bool Steam_GSBUpdateUserData(uint64 steamIDUser, const char *pchPlayerName, uint32 uScore)
{
	if(num_extra_games == 0)
		return CRehldsPlatformHolder::get()->SteamGameServer()->BUpdateUserData(steamIDUser, pchPlayerName, uScore);

	bool retval = true;
	for(int iClient = 0; iClient < g_psvs.maxclients; iClient++) {
		auto client = &g_psvs.clients[iClient];
		if(!client->active || client->network_userid.m_SteamID != steamIDUser)
			continue;

		for(int iGame = 0; iGame < num_extra_games; iGame++) {
			if(!gExtraSteamIDs[iClient][iGame]) {
				retval = false;
				continue;
			}

			if(!CRehldsPlatformHolder::get()->SteamGameServerExtra(iGame)->BUpdateUserData(gExtraSteamIDs[iClient][iGame], pchPlayerName, uScore)) {
				gExtraSteamIDs[iClient][iGame] = 0;
				retval = false;
			}
		}
	}
	return retval;
}

bool ISteamGameServer_BUpdateUserData(uint64 steamid, const char *netname, uint32 score)
{
	return g_RehldsHookchains.m_Steam_GSBUpdateUserData.callChain(Steam_GSBUpdateUserData, steamid, netname, score);
}

bool ISteamApps_BIsSubscribedApp(uint32 appid)
{
	if (CRehldsPlatformHolder::get()->SteamApps())
	{
		ISteamApps* apps = CRehldsPlatformHolder::get()->SteamApps();
		return apps->BIsSubscribedApp(appid);
	}

	return false;
}

const char *Steam_GetCommunityName()
{
	if (SteamFriends())
		return SteamFriends()->GetPersonaName();

	return NULL;
}

qboolean EXT_FUNC Steam_NotifyClientConnect_api(IGameClient *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key)
{
	return Steam_NotifyClientConnect_internal(cl->GetClient(), pvSteam2Key, ucbSteam2Key);
}

qboolean Steam_NotifyClientConnect(client_t *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key)
{
	return g_RehldsHookchains.m_Steam_NotifyClientConnect
		.callChain(Steam_NotifyClientConnect_api, GetRehldsApiClient(cl), pvSteam2Key, ucbSteam2Key);
}

qboolean Steam_NotifyClientConnect_internal(client_t *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key)
{
	if (Steam3Server())
	{
		return Steam3Server()->NotifyClientConnect(cl, pvSteam2Key, ucbSteam2Key);
	}
	return FALSE;
}

qboolean EXT_FUNC Steam_NotifyBotConnect_api(IGameClient* cl)
{
	return Steam_NotifyBotConnect_internal(cl->GetClient());
}

qboolean Steam_NotifyBotConnect(client_t *cl)
{
	return g_RehldsHookchains.m_Steam_NotifyBotConnect.callChain(Steam_NotifyBotConnect_api, GetRehldsApiClient(cl));
}

qboolean Steam_NotifyBotConnect_internal(client_t *cl)
{
	if (Steam3Server())
	{
		return Steam3Server()->NotifyBotConnect(cl);
	}
	return FALSE;
}

void EXT_FUNC Steam_NotifyClientDisconnect_api(IGameClient* cl)
{
	g_RehldsHookchains.m_Steam_NotifyClientDisconnect.callChain(Steam_NotifyClientDisconnect_internal, cl);
}

void Steam_NotifyClientDisconnect(client_t *cl)
{
	Steam_NotifyClientDisconnect_api(GetRehldsApiClient(cl));
}

void Steam_NotifyClientDisconnect_internal(IGameClient* cl)
{
	if (Steam3Server())
	{
		Steam3Server()->NotifyClientDisconnect(cl->GetClient());
	}
}

void Steam_NotifyOfLevelChange()
{
	if (Steam3Server())
	{
		Steam3Server()->NotifyOfLevelChange(false);
	}
}

void Steam_Shutdown()
{
	if (Steam3Server())
	{
		Steam3Server()->Shutdown();
		delete s_Steam3Server;
		s_Steam3Server = NULL;
	}
}

void Steam_Activate()
{
	if (!Steam3Server())
	{
		s_Steam3Server = new CSteam3Server();
		if (s_Steam3Server == NULL)
			return;
	}

	Steam3Server()->Activate();
}

void Steam_RunFrame()
{
	if (Steam3Server())
	{
		Steam3Server()->RunFrame();
	}
}

void Steam_SetCVar(const char *pchKey, const char *pchValue)
{
	if (Steam3Server())
	{
		CRehldsPlatformHolder::get()->SteamGameServerDo([=] (auto sgs) { sgs->SetKeyValue(pchKey, pchValue); });
	}
}

void Steam_ClientRunFrame()
{
	Steam3Client()->RunFrame();
}

void Steam_InitClient()
{
	Steam3Client()->InitClient();
}

int Steam_GSInitiateGameConnection(void *pData, int cbMaxData, uint64 steamID, uint32 unIPServer, uint16 usPortServer, qboolean bSecure)
{
	return Steam3Client()->InitiateGameConnection(pData, cbMaxData, steamID, unIPServer, usPortServer, bSecure != 0);
}

void Steam_GSTerminateGameConnection(uint32 unIPServer, uint16 usPortServer)
{
	Steam3Client()->TerminateConnection(unIPServer, usPortServer);
}

void Steam_ShutdownClient()
{
	Steam3Client()->Shutdown();
}

uint64 Steam_GSGetSteamID()
{
	return Steam3Server()->GetSteamID();
}

qboolean Steam_GSBSecure()
{
	//useless call
	//Steam3Server();
	bool secure = true;
	CRehldsPlatformHolder::get()->SteamGameServerDo([&] (auto sgs) { secure &= sgs->BSecure(); });
	return secure;
}

qboolean Steam_GSBLoggedOn()
{
	bool loggedOn = Steam3Server()->BLoggedOn();
	if(loggedOn)
		CRehldsPlatformHolder::get()->SteamGameServerDo([&] (auto sgs) { loggedOn &= sgs->BLoggedOn(); });

	return loggedOn;
}

qboolean Steam_GSBSecurePreference()
{
	return Steam3Server()->BWantsSecure();
}

TSteamGlobalUserID Steam_Steam3IDtoSteam2(uint64 unSteamID)
{
	class CSteamID steamID = unSteamID;
	return SteamIDToSteam2UserID(steamID);
}

static bool Steam_ParseSteam2String(const char *pchSteam2ID, TSteamGlobalUserID *pOut)
{
	pOut->m_SteamInstanceID = 0;
	pOut->m_SteamLocalUserID.Split.High32bits = 0;
	pOut->m_SteamLocalUserID.Split.Low32bits = 0;

	const char *pchTSteam2ID = pchSteam2ID;
	const char *pchOptionalLeadString = "STEAM_";
	if (Q_strnicmp(pchSteam2ID, pchOptionalLeadString, Q_strlen(pchOptionalLeadString)) == 0)
		pchTSteam2ID = pchSteam2ID + Q_strlen(pchOptionalLeadString);

	char cExtraCharCheck = 0;

	int cFieldConverted = sscanf(pchTSteam2ID, "%hu:%u:%u%c", &pOut->m_SteamInstanceID,
		&pOut->m_SteamLocalUserID.Split.High32bits, &pOut->m_SteamLocalUserID.Split.Low32bits, &cExtraCharCheck);

	if (cExtraCharCheck != 0 || cFieldConverted == EOF || cFieldConverted < 2 || (cFieldConverted < 3 && pOut->m_SteamInstanceID != 1))
		return false;

	return true;
}

uint64 Steam_StringToSteamID(const char *pStr)
{
	CSteamID steamID;
	TSteamGlobalUserID steam2ID;
	if (Steam_ParseSteam2String(pStr, &steam2ID))
	{
		EUniverse eUniverse = Steam3Server() ? CSteamID(Steam3Server()->GetSteamID()).GetEUniverse() : k_EUniversePublic;
		steamID = SteamIDFromSteam2UserID(&steam2ID, eUniverse);
	}

	return steamID.ConvertToUint64();
}

const char *Steam_GetGSUniverse()
{
	CSteamID steamID(Steam3Server()->GetSteamID());
	switch (steamID.GetEUniverse())
	{
	case k_EUniversePublic:
		return "";

	case k_EUniverseBeta:
		return "(beta)";

	case k_EUniverseInternal:
		return "(internal)";

	default:
		return "(u)";
	}
}

CSteam3Server *s_Steam3Server;
CSteam3Client s_Steam3Client;

CSteam3Server *Steam3Server()
{
	return s_Steam3Server;
}

CSteam3Client *Steam3Client()
{
	return &s_Steam3Client;
}

void Master_SetMaster_f()
{
	int i;
	const char * pszCmd;

	i = Cmd_Argc();
	if (!Steam3Server())
	{
		Con_Printf("Usage:\nSetmaster unavailable, start a server first.\n");
		return;
	}

	if (i < 2 || i > 5)
	{
		Con_Printf("Usage:\nSetmaster <enable | disable>\n");
		return;
	}

	pszCmd = Cmd_Argv(1);
	if (!pszCmd || !pszCmd[0])
		return;

	if (Q_stricmp(pszCmd, "disable") || gfNoMasterServer)
	{
		if (!Q_stricmp(pszCmd, "enable"))
		{
			if (gfNoMasterServer)
			{
				gfNoMasterServer = FALSE;
				CRehldsPlatformHolder::get()->SteamGameServer()->SetAdvertiseServerActive(gfNoMasterServer != 0);
			}
		}
	}
	else
	{
		gfNoMasterServer = TRUE;
	}
	CRehldsPlatformHolder::get()->SteamGameServer()->SetAdvertiseServerActive(gfNoMasterServer == FALSE);
}

void Steam_HandleIncomingPacket(byte *data, int len, int fromip, uint16 port)
{
	CRehldsPlatformHolder::get()->SteamGameServer()->HandleIncomingPacket(data, len, fromip, port);
}

//Steam Datagram Relay (FakeIP/FakeUDPPort) transport, backported from 25th anniversary HLDS.

uint32 g_FakeIP[NS_MAX];
static uint16 g_FakePorts[NS_MAX];
static bool g_FakeIPRequested[NS_MAX];
static ISteamNetworkingFakeUDPPort *ip_fakeudpports[NS_MAX];

static bool IsFakeIPServerSocket(int sock)
{
	if (num_extra_games > 0)
		return sock >= NS_EXTRA && sock < NS_EXTRA + num_extra_games;
	return sock == NS_SERVER;
}

static ISteamNetworkingSockets *FakeIPServerSockets(int sock)
{
	return CRehldsPlatformHolder::get()->SteamGameServerNetworkingSockets(sock == NS_SERVER ? -1 : sock - NS_EXTRA);
}

static ISteamNetworkingFakeUDPPort *FakeIPPortFor(int sock, uint32 hostIP)
{
	if (sock < 0 || sock >= NS_MAX || !ip_fakeudpports[sock])
		return NULL;

	ISteamNetworkingUtils *utils = CRehldsPlatformHolder::get()->SteamNetworkingUtils();
	return utils && utils->IsFakeIPv4(hostIP) ? ip_fakeudpports[sock] : NULL;
}

qboolean NET_ShouldUseSteamFakeIP()
{
	return sv_use_steam_networking.value > 0.0f ? TRUE : FALSE;
}

int NET_SteamFakeIPSendTo(netsrc_t sock, const char *buf, int len, const struct sockaddr *to)
{
	if (to->sa_family != AF_INET)
		return -2;

	const struct sockaddr_in *sin = (const struct sockaddr_in *)to;
	uint32 ip = ntohl(sin->sin_addr.s_addr);
	ISteamNetworkingFakeUDPPort *port = FakeIPPortFor(sock, ip);
	if (!port)
		return -2;

	SteamNetworkingIPAddr addr;
	addr.SetIPv4(ip, ntohs(sin->sin_port));

	EResult res = port->SendMessageToFakeIP(addr, buf, len, k_nSteamNetworkingSend_UnreliableNoNagle);
	if (res != k_EResultOK)
	{
		netadr_t adr;
		SockadrToNetadr(to, &adr);
		Con_Printf("SendMessageToFakeIP to %s returned %d\n", NET_AdrToString(adr), (int)res);
	}
	return len;
}

int NET_SteamFakeIPRecvFrom(netsrc_t sock, unsigned char *buf, int maxlen, netadr_t *from)
{
	ISteamNetworkingFakeUDPPort *port = (sock >= 0 && sock < NS_MAX) ? ip_fakeudpports[sock] : NULL;
	SteamNetworkingMessage_t *msg = NULL;
	if (!port || port->ReceiveMessages(&msg, 1) != 1 || !msg)
		return -1;

	int ret = -1;
	uint32 ip = msg->m_identityPeer.m_eType == k_ESteamNetworkingIdentityType_IPAddress ? msg->m_identityPeer.m_ip.GetIPv4() : 0;
	if (ip && msg->m_cbSize > maxlen)
	{
		Con_DPrintf("Ignoring oversized FakeIP message (%d bytes)\n", msg->m_cbSize);
	}
	else if (ip)
	{
		if (from)
		{
			Q_memset(from, 0, sizeof(*from));
			from->type = NA_IP;
			*(uint32 *)from->ip = htonl(ip);
			from->port = htons(msg->m_identityPeer.m_ip.m_port);
		}
		Q_memcpy(buf, msg->m_pData, msg->m_cbSize);
		ret = msg->m_cbSize;
	}
	msg->Release();
	return ret;
}

void NET_SteamFakeIPDestroySocket(netsrc_t sock)
{
	if (sock >= 0 && sock < NS_MAX && ip_fakeudpports[sock])
	{
		ip_fakeudpports[sock]->DestroyFakeUDPPort();
		ip_fakeudpports[sock] = NULL;
	}
}

void NET_CheckCleanupFakeIPConnection(netsrc_t sock, const netadr_t *adr)
{
	if (!adr || adr->type != NA_IP)
		return;

	uint32 ip = ntohl(*(uint32 *)adr->ip);
	ISteamNetworkingFakeUDPPort *port = FakeIPPortFor(sock, ip);
	if (!port)
		return;

	SteamNetworkingIPAddr saddr;
	saddr.SetIPv4(ip, ntohs(adr->port));
	port->ScheduleCleanup(saddr);
}

static void Net_RequestSteamFakeIP(int sock)
{
	if (!NET_ShouldUseSteamFakeIP() || !IsFakeIPServerSocket(sock))
		return;

	ISteamNetworkingSockets *sockets = FakeIPServerSockets(sock);
	if (sockets && sockets->BeginAsyncRequestFakeIP(1))
	{
		g_FakeIPRequested[sock] = true;
		Con_DPrintf("FakeIP enabled! Requesting a fake IP.\n");
	}
}

void NET_UpdateSteamFakeIP()
{
	if (!NET_ShouldUseSteamFakeIP())
		return;

	for (int sock = 0; sock < NS_MAX; sock++)
	{
		ISteamNetworkingSockets *sockets = (g_FakeIPRequested[sock] && !g_FakeIP[sock]) ? FakeIPServerSockets(sock) : NULL;
		if (sockets)
		{
			SteamNetworkingFakeIPResult_t result;
			result.m_eResult = k_EResultBusy;
			sockets->GetFakeIP(0, &result);
			if (result.m_eResult == k_EResultBusy)
				continue;
			if (result.m_eResult != k_EResultOK)
				Sys_Error("FakeIP allocation failed with error code %d (socket %d)", (int)result.m_eResult, sock);

			g_FakeIP[sock] = result.m_unIP;
			g_FakePorts[sock] = result.m_unPorts[0];
		}

		if (!g_FakeIP[sock])
			continue;

		netadr_t adr;
		Q_memset(&adr, 0, sizeof(adr));
		adr.type = NA_IP;
		*(uint32 *)adr.ip = htonl(g_FakeIP[sock]);
		adr.port = htons(g_FakePorts[sock]);

		netadr_t *pLocal = (sock == NS_SERVER) ? &net_local_adr : &net_local_adr_extra[sock - NS_EXTRA];
		if (NET_CompareAdr(*pLocal, adr))
			continue;

		*pLocal = adr;
		if (sock == NS_SERVER)
		{
			Con_Printf("Server IP address %s (FakeIP)\n", NET_AdrToString(adr));
			Cvar_Set("net_address", NET_AdrToString(adr));
		}
		else
		{
			Con_Printf("Server IP address %s (FakeIP, %s)\n", NET_AdrToString(adr), extra_games[sock - NS_EXTRA]);
		}
	}
}

void Net_CheckOpenFakeUDPPorts()
{
	if (!NET_ShouldUseSteamFakeIP())
		return;

	for (int sock = 0; sock < NS_MAX; sock++)
	{
		if (!IsFakeIPServerSocket(sock) || ip_fakeudpports[sock])
			continue;

		ISteamNetworkingSockets *sockets = FakeIPServerSockets(sock);
		if (sockets)
		{
			ip_fakeudpports[sock] = sockets->CreateFakeUDPPort(0);
			if (!ip_fakeudpports[sock])
				Con_Printf("Failed to create FakeUDP server port\n");
		}
	}
}

static void Net_CloseFakeUDPPorts()
{
	for (int sock = 0; sock < NS_MAX; sock++)
		NET_SteamFakeIPDestroySocket((netsrc_t)sock);

	Q_memset(g_FakeIP, 0, sizeof(g_FakeIP));
	Q_memset(g_FakePorts, 0, sizeof(g_FakePorts));
	Q_memset(g_FakeIPRequested, 0, sizeof(g_FakeIPRequested));
}
