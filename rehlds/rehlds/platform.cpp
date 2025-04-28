#include "precompiled.h"

#ifdef WIN32
using dllhandle_t = HMODULE;
#define DLL_FORMAT ".dll"
#else
using dllhandle_t = void*;
#define DLL_FORMAT ".so"
#endif

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
void CSimplePlatform::SteamAPI_SetBreakpadAppID(uint32 unAppID) {
	return ::SteamAPI_SetBreakpadAppID(unAppID);
}

void CSimplePlatform::SteamAPI_UseBreakpadCrashHandler(char const *pchVersion, char const *pchDate, char const *pchTime, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback) {
	::SteamAPI_UseBreakpadCrashHandler(pchVersion, pchDate, pchTime, bFullMemoryDumps, pvContext, m_pfnPreMinidumpCallback);
}


static dllhandle_t getSteamApiExtra(int iExtraGame)
{
	char libName[32];

#ifdef _WIN32
	snprintf(libName, sizeof(libName), "steam_api%d.dll", iExtraGame + 1);
#else
	snprintf(libName, sizeof(libName), "libsteam_api%d.so", iExtraGame + 1);
#endif

	auto lib = FS_LoadLibrary(libName);

	if (lib)
		return (dllhandle_t)lib;

#ifdef _WIN32
	FILE* sf = fopen("steam_api.dll", "rb");
#else
	FILE* sf = fopen("libsteam_api.so", "rb");
#endif
	if (!sf)
		Sys_Error("Couldn't open steam_api for reading!");

	FILE* tf = fopen(libName, "wb");
	if (!tf)
		Sys_Error("Couldn't open %s for writing!", libName);

	static char buffer[8192];
	size_t read = 0;
	do {
		read = fread(buffer, 1, sizeof(buffer), sf);
		fwrite(buffer, 1, read, tf);
	} while (read);

	fclose(sf);
	fclose(tf);

	lib = FS_LoadLibrary(libName);

	if (!lib)
		Sys_Error("Couldn't load %s!", libName);

	return (dllhandle_t)lib;
}

void CSimplePlatform::SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback) {
	if(num_extra_games == 0)
		::SteamAPI_RegisterCallback(pCallback, iCallback);
	else for(int iGame = 0; iGame < num_extra_games; iGame++) {
		auto pfnSteamAPI_RegisterCallback = (void (*)(CCallbackBase * pCallback, int iCallback))Sys_GetProcAddress(getSteamApiExtra(iGame), "SteamAPI_RegisterCallback");
		pfnSteamAPI_RegisterCallback(pCallback, iCallback);
	}
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
	return ::SteamGameServer_Init(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString);
}

static int gExtraGame;
static uint32 getAppIdExtra(ISteamUtils* that, const void* edx) {
	if(gExtraGame == -1)
		return -1;

	uint32 appId = GetGameAppIDByName(extra_games[gExtraGame]);
	gExtraGame = -1;
	return appId;
}

bool CSimplePlatform::SteamGameServer_InitExtra(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char* pchVersionString, int iExtraGame) {
	static bool gsuHooked = false;
	if(!gsuHooked) {
		// do a bogus init to get the GSU instance
		::SteamGameServer_Init(0, 0, 0, 0, EServerMode::eServerModeInvalid, 0);
		auto gsu = SteamGameServerUtils();

#ifdef _WIN32
		auto vft = *(void***)(gsu);
		auto pGetAppId = &vft[9];
		DWORD oldProtect = 0;
		VirtualProtect(pGetAppId, sizeof(*pGetAppId), PAGE_READWRITE, &oldProtect);
		*pGetAppId = (void*)&getAppIdExtra;
		VirtualProtect(pGetAppId, sizeof(*pGetAppId), oldProtect, &oldProtect);
#else // LINUX
		auto vft = *(void***)(gsu);
		auto pGetAppId = &vft[9];

		size_t pagesize = sysconf(_SC_PAGE_SIZE);
		uintptr_t start = ((uintptr_t)pGetAppId) & ~(pagesize - 1);

		mprotect((void*)start, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);
		*pGetAppId = (void*)&getAppIdExtra;
		mprotect((void*)start, pagesize, PROT_READ | PROT_EXEC);
#endif
		gsuHooked = true;
	}

	auto pfnSteamGameServer_Init = (bool(*)(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char* pchVersionString))Sys_GetProcAddress(getSteamApiExtra(iExtraGame), "SteamGameServer_Init");

	gExtraGame = iExtraGame;

	return pfnSteamGameServer_Init(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, gpszVersionString);
}

ISteamGameServer* CSimplePlatform::SteamGameServer() {
	if(num_extra_games)
		Sys_Error("Unexpected use of SteamGameServer (should be SteamGameServerExtra)!");

	return ::SteamGameServer();
}

ISteamGameServer* CSimplePlatform::SteamGameServerExtra(int iGame) {
	if(iGame < 0 || iGame >= num_extra_games)
		Sys_Error("Invalid extra game index!");

	static ISteamGameServer* (*pfnSteamGameServer[MAX_EXTRA_GAMES])() = {};
	if(!pfnSteamGameServer[iGame])
		pfnSteamGameServer[iGame] = (ISteamGameServer*(*)())Sys_GetProcAddress(getSteamApiExtra(iGame), "SteamGameServer");
	return pfnSteamGameServer[iGame]();
}

extern int gCurrentCallbackGame;

void CSimplePlatform::SteamGameServer_RunCallbacks() {
	if(num_extra_games == 0) {
		::SteamGameServer_RunCallbacks();
	}
	else {
		static void (*pfnSteamGameServer_RunCallbacks[MAX_EXTRA_GAMES])() = {};
		for(int iGame = 0; iGame < num_extra_games; iGame++) {
			if(!pfnSteamGameServer_RunCallbacks[iGame])
				pfnSteamGameServer_RunCallbacks[iGame] = (void (*)())Sys_GetProcAddress(getSteamApiExtra(iGame), "SteamGameServer_RunCallbacks");

			gCurrentCallbackGame = iGame;
			pfnSteamGameServer_RunCallbacks[iGame]();
		}
		gCurrentCallbackGame = -1;
	}
}

void CSimplePlatform::SteamAPI_RunCallbacks() {
	if(num_extra_games == 0) {
		::SteamAPI_RunCallbacks();
	}
	else {
		static void (*pfnSteamAPI_RunCallbacks[MAX_EXTRA_GAMES])() = {};
		for(int iGame = 0; iGame < num_extra_games; iGame++) {
			if(!pfnSteamAPI_RunCallbacks[iGame])
				pfnSteamAPI_RunCallbacks[iGame] = (void (*)())Sys_GetProcAddress(getSteamApiExtra(iGame), "SteamAPI_RunCallbacks");

			gCurrentCallbackGame = iGame;
			pfnSteamAPI_RunCallbacks[iGame]();
		}
		gCurrentCallbackGame = -1;
	}
}

void CSimplePlatform::SteamGameServer_Shutdown() {
	if(num_extra_games == 0)
		::SteamGameServer_Shutdown();
	else for(int iGame = 0; iGame < num_extra_games; iGame++) {
		auto pfnSteamGameServer_Shutdown = (void (*)())Sys_GetProcAddress(getSteamApiExtra(iGame), "SteamGameServer_Shutdown");
		pfnSteamGameServer_Shutdown();
	}
}

void CSimplePlatform::SteamAPI_UnregisterCallback(CCallbackBase *pCallback)
{
	if(num_extra_games == 0)
		::SteamAPI_UnregisterCallback(pCallback);
	else for(int iGame = 0; iGame < num_extra_games; iGame++) {
		auto pfnSteamAPI_UnregisterCallback = (void (*)(CCallbackBase * pCallback))Sys_GetProcAddress(getSteamApiExtra(iGame), "SteamAPI_UnregisterCallback");
		pfnSteamAPI_UnregisterCallback(pCallback);
	}
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
