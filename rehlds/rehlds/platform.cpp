#include "precompiled.h"
#include "steam/iclientengine.h"
#include "steam/iclientutils.h"

IReHLDSPlatform* CRehldsPlatformHolder::m_Platform;

IReHLDSPlatform* CRehldsPlatformHolder::get() {
	if (m_Platform == NULL) {
		m_Platform = new CSimplePlatform();
	}

	return m_Platform;
}

void CRehldsPlatformHolder::set(IReHLDSPlatform* p) {
	m_Platform = p;
}

CSimplePlatform::CSimplePlatform() {
#ifdef _WIN32
	wsock = LoadLibraryA("wsock32.dll");
	setsockopt_v11 = (setsockopt_proto)GetProcAddress(wsock, "setsockopt");
	if (setsockopt_v11 == NULL)
		rehlds_syserror("%s: setsockopt_v11 not found", __func__);
#endif
}

CSimplePlatform::~CSimplePlatform()
{
#ifdef _WIN32
	FreeLibrary(wsock);
#endif
}

uint32 CSimplePlatform::time(uint32* pTime)
{
	time_t res = ::time((time_t*)NULL);
	if (pTime != NULL) *pTime = (uint32)res;

	return (uint32) res;
}

struct tm* CSimplePlatform::localtime(uint32 time)
{
	time_t theTime = (time_t)time;
	return ::localtime(&theTime);
}

void CSimplePlatform::srand(uint32 seed)
{
	return ::srand(seed);
}

int CSimplePlatform::rand()
{
	return ::rand();
}

#ifdef _WIN32
void CSimplePlatform::Sleep(DWORD msec) {
	::Sleep(msec);
}

BOOL CSimplePlatform::QueryPerfCounter(LARGE_INTEGER* counter) {
	return ::QueryPerformanceCounter(counter);
}

BOOL CSimplePlatform::QueryPerfFreq(LARGE_INTEGER* freq) {
	return ::QueryPerformanceFrequency(freq);
}

DWORD CSimplePlatform::GetTickCount() {
	return ::GetTickCount();
}

void CSimplePlatform::GetLocalTime(LPSYSTEMTIME time) {
	return ::GetLocalTime(time);
}

void CSimplePlatform::GetSystemTime(LPSYSTEMTIME time) {
	return ::GetSystemTime(time);
}

void CSimplePlatform::GetTimeZoneInfo(LPTIME_ZONE_INFORMATION zinfo) {
	::GetTimeZoneInformation(zinfo);
}

BOOL CSimplePlatform::GetProcessTimes(HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime)
{
	return ::GetProcessTimes(hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime);
}

void CSimplePlatform::GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	::GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}
#endif //WIN32

SOCKET CSimplePlatform::socket(int af, int type, int protocol) {
	return ::socket(af, type, protocol);
}

int CSimplePlatform::setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen) {
#ifdef _WIN32
	return setsockopt_v11(s, level, optname, optval, optlen);
#else
	return ::setsockopt(s, level, optname, optval, optlen);
#endif
}

int CSimplePlatform::closesocket(SOCKET s) {
#ifdef _WIN32
	return ::closesocket(s);
#else
	return ::close(s);
#endif
}

int CSimplePlatform::recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, socklen_t *fromlen) {
	return ::recvfrom(s, buf, len, flags, from, fromlen);
}

int CSimplePlatform::sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) {
	return ::sendto(s, buf, len, flags, to, tolen);
}

int CSimplePlatform::bind(SOCKET s, const struct sockaddr* addr, int namelen) {
	return ::bind(s, addr, namelen);
}

int CSimplePlatform::getsockname(SOCKET s, struct sockaddr* name, socklen_t* namelen) {
	return ::getsockname(s, name, namelen);
}

struct hostent* CSimplePlatform::gethostbyname(const char *name) {
	return ::gethostbyname(name);
}

int CSimplePlatform::gethostname(char *name, int namelen) {
	return ::gethostname(name, namelen);
}

#ifdef _WIN32

int CSimplePlatform::ioctlsocket(SOCKET s, long cmd, u_long *argp) {
	return ::ioctlsocket(s, cmd, argp);
}

int CSimplePlatform::WSAGetLastError() {
	return ::WSAGetLastError();
}

#endif //WIN32

struct ExtraSteamServer {
	HSteamPipe pipe;
	HSteamUser user;
	ISteamGameServer *gameServer;
	ISteamNetworkingSockets *sockets;
};
static ExtraSteamServer gExtraSteamServers[MAX_EXTRA_GAMES];
static CUtlVector<CCallbackBase *> gExtraSteamCallbacks;
extern int gCurrentCallbackGame;

void Rehlds_SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback) {
	CRehldsPlatformHolder::get()->SteamAPI_RegisterCallback(pCallback, iCallback);
}

void Rehlds_SteamAPI_UnregisterCallback(CCallbackBase *pCallback) {
	CRehldsPlatformHolder::get()->SteamAPI_UnregisterCallback(pCallback);
}

void CSimplePlatform::SteamAPI_SetBreakpadAppID(uint32 unAppID) {
	::SteamAPI_SetBreakpadAppID(unAppID);
}

void CSimplePlatform::SteamAPI_UseBreakpadCrashHandler(char const* pchVersion, char const* pchDate, char const* pchTime, bool bFullMemoryDumps, void* pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback) {
	return ::SteamAPI_UseBreakpadCrashHandler(pchVersion, pchDate, pchTime, bFullMemoryDumps, pvContext, m_pfnPreMinidumpCallback);
}

void CSimplePlatform::SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback) {
	::SteamAPI_RegisterCallback(pCallback, iCallback);
	if(num_extra_games)
		gExtraSteamCallbacks.AddToTail(pCallback);
}

bool CSimplePlatform::SteamAPI_Init() {
	return ::SteamAPI_Init();
}

void CSimplePlatform::SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall) {
	return ::SteamAPI_UnregisterCallResult(pCallback, hAPICall);
}

ISteamApps* CSimplePlatform::SteamApps() {
	return ::SteamApps();
}

bool CSimplePlatform::SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString) {
	return ::SteamGameServer_Init(unIP, usGamePort, usQueryPort, eServerMode, pchVersionString);
}

static void *GetSteamClientExport(void *pSteamClientObject, const char *pszName)
{
	void *vtable = *(void **)pSteamClientObject;
#ifdef _WIN32
	HMODULE lib = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)vtable, &lib);
	return lib ? (void *)GetProcAddress(lib, pszName) : NULL;
#else
	Dl_info info;
	void *lib = dladdr(vtable, &info) ? dlopen(info.dli_fname, RTLD_NOW | RTLD_NOLOAD) : NULL;
	void *sym = lib ? dlsym(lib, pszName) : NULL;
	if (lib)
		dlclose(lib);
	return sym;
#endif
}

class ISteamGameServerInit {
public:
	virtual bool InitGameServer(uint32 unIP, uint16 usGamePort, uint16 usQueryPort, uint32 unFlags, AppId_t nGameAppId, const char *pchVersionString) = 0;
};

const uint32 k_unServerFlagSecure = 0x02;
const uint32 k_unServerFlagPrivate = 0x20;

bool CSimplePlatform::SteamGameServer_InitExtra(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char* pchVersionString, int iExtraGame) {
	(void)usSteamPort;
	if (iExtraGame == 0 && !::SteamGameServer_Init(unIP, usGamePort, usQueryPort, eServerMode, pchVersionString))
		return false;

	ISteamClient *client = SteamGameServerClient();
	HSteamPipe pipe = 0;
	HSteamUser user = client->CreateLocalUser(&pipe, k_EAccountTypeGameServer);
	ISteamGameServer *gs = user ? client->GetISteamGameServer(user, pipe, STEAMGAMESERVER_INTERFACE_VERSION) : NULL;
	if (!gs)
		return false;

	gExtraSteamServers[iExtraGame] = { pipe, user, gs, NULL };

	AppId_t appId = GetGameAppIDByName(extra_games[iExtraGame]);
	auto pfnCreateInterface = (void *(*)(const char *, int *))GetSteamClientExport(gs, "CreateInterface");
	auto engine = pfnCreateInterface ? (IClientEngine *)pfnCreateInterface(CLIENTENGINE_INTERFACE_VERSION, NULL) : NULL;
	auto utils = engine ? engine->GetIClientUtils(pipe) : NULL;
	if (utils)
		utils->SetAppIDForCurrentPipe(appId, true);

	if (!utils || utils->GetAppID() != appId)
		Con_Printf("Failed to set AppID %u for Steam pipe %d\n", appId, pipe);

	uint32 flags = 0;
	if (eServerMode == eServerModeAuthenticationAndSecure)
		flags = k_unServerFlagSecure;
	else if (eServerMode == eServerModeNoAuthentication)
		flags = k_unServerFlagPrivate;

	return ((ISteamGameServerInit *)gs)->InitGameServer(unIP, usGamePort, usQueryPort, flags, appId, pchVersionString);
}

ISteamGameServer* CSimplePlatform::SteamGameServer() {
	if(num_extra_games)
		Sys_Error("Unexpected use of SteamGameServer (should be SteamGameServerExtra)!");

	return ::SteamGameServer();
}

ISteamGameServer* CSimplePlatform::SteamGameServerExtra(int iGame) {
	if(iGame < 0 || iGame >= num_extra_games)
		Sys_Error("Invalid extra game index!");

	return gExtraSteamServers[iGame].gameServer;
}

static void RunPipeCallbacks(HSteamPipe pipe, bool bDispatch)
{
	using BGetCallback_t = bool (*)(HSteamPipe, CallbackMsg_t *, int32 *);
	using FreeLastCallback_t = void (*)(HSteamPipe);
	static BGetCallback_t pfnBGetCallback;
	static FreeLastCallback_t pfnFreeLastCallback;
	if (!pfnBGetCallback || !pfnFreeLastCallback) {
		pfnBGetCallback = (BGetCallback_t)GetSteamClientExport(SteamGameServerClient(), "Steam_BGetCallback");
		pfnFreeLastCallback = (FreeLastCallback_t)GetSteamClientExport(SteamGameServerClient(), "Steam_FreeLastCallback");
		if (!pfnBGetCallback || !pfnFreeLastCallback)
			Sys_Error("%s: steamclient callback exports not found", __func__);
	}

	CallbackMsg_t msg;
	int32 hSteamCall;
	while (pfnBGetCallback(pipe, &msg, &hSteamCall)) {
		for (int i = 0; bDispatch && i < gExtraSteamCallbacks.Count(); i++) {
			CCallbackBase *pCallback = gExtraSteamCallbacks[i];
			if (pCallback->IsGameServer() && pCallback->GetICallback() == msg.m_iCallback)
				pCallback->Run(msg.m_pubParam);
		}
		pfnFreeLastCallback(pipe);
	}
}

void CSimplePlatform::SteamGameServer_RunCallbacks() {
	if(num_extra_games == 0) {
		::SteamGameServer_RunCallbacks();
		return;
	}

	HSteamPipe bootstrapPipe = ::SteamGameServer_GetHSteamPipe();
	if(bootstrapPipe)
		RunPipeCallbacks(bootstrapPipe, false);

	for(int iGame = 0; iGame < num_extra_games; iGame++) {
		if(!gExtraSteamServers[iGame].pipe)
			continue;

		gCurrentCallbackGame = iGame;
		RunPipeCallbacks(gExtraSteamServers[iGame].pipe, true);
	}
	gCurrentCallbackGame = -1;
}

void CSimplePlatform::SteamAPI_RunCallbacks() {
	::SteamAPI_RunCallbacks();
}

void CSimplePlatform::SteamGameServer_Shutdown() {
	for(int iGame = 0; iGame < num_extra_games; iGame++) {
		auto extraSteamServer = &gExtraSteamServers[iGame];
		if(extraSteamServer->pipe) {
			SteamGameServerClient()->ReleaseUser(extraSteamServer->pipe, extraSteamServer->user);
			SteamGameServerClient()->BReleaseSteamPipe(extraSteamServer->pipe);
		}
	}

	memset(gExtraSteamServers, 0, sizeof(gExtraSteamServers));

	::SteamGameServer_Shutdown();
}

void CSimplePlatform::SteamAPI_UnregisterCallback(CCallbackBase *pCallback)
{
	::SteamAPI_UnregisterCallback(pCallback);
	gExtraSteamCallbacks.FindAndRemove(pCallback);
}

ISteamNetworkingSockets* CSimplePlatform::SteamGameServerNetworkingSockets(int iExtraGame)
{
	if (iExtraGame < 0)
		return ::SteamGameServerNetworkingSockets();

	auto extraSteamServer = &gExtraSteamServers[iExtraGame];
	if (!extraSteamServer->sockets && extraSteamServer->pipe)
		extraSteamServer->sockets = (ISteamNetworkingSockets *)SteamGameServerClient()->GetISteamGenericInterface(extraSteamServer->user, extraSteamServer->pipe, STEAMNETWORKINGSOCKETS_INTERFACE_VERSION);
	return extraSteamServer->sockets;
}

ISteamNetworkingUtils* CSimplePlatform::SteamNetworkingUtils()
{
	return ::SteamNetworkingUtils();
}

void NORETURN rehlds_syserror(const char* fmt, ...) {
	va_list			argptr;
	static char		string[8192];

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	printf("%s\n", string);

	FILE* fl = fopen("rehlds_error.txt", "w");
	fprintf(fl, "%s\n", string);
	fclose(fl);

	//TerminateProcess(GetCurrentProcess(), 1);
	volatile int *null = 0;
	*null = 0;

	while (true);
}
