#include "FFPlatform_private.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj.h>

#include "util/windows/unicode.h"

static void getHomeDir(FFPlatform* platform)
{
    PWSTR pPath;
    if(SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Profile, KF_FLAG_DEFAULT, NULL, &pPath)))
    {
        ffStrbufSetWS(&platform->home_dir, pPath);
        ffStrbufReplaceAllC(&platform->home_dir, '\\', '/');
        ffStrbufEnsureEndsWithC(&platform->home_dir, '/');
    }
    CoTaskMemFree(pPath);
}

static void getCacheDir(FFPlatform* platform)
{
    PWSTR pPath;
    if(SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &pPath)))
    {
        ffStrbufSetWS(&platform->cache_dir, pPath);
        ffStrbufReplaceAllC(&platform->cache_dir, '\\', '/');
        ffStrbufEnsureEndsWithC(&platform->cache_dir, '/');
    }
    else
    {
        ffStrbufAppend(&platform->cache_dir, &platform->home_dir);
        ffStrbufAppendS(&platform->cache_dir, "AppData/Local/");
    }
    CoTaskMemFree(pPath);

    ffStrbufAppendS(&platform->cache_dir, "fastfetch/");
}

static void platformPathAddKnownFolder(FFlist* dirs, REFKNOWNFOLDERID folderId)
{
    PWSTR pPath;
    if(SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &pPath)))
    {
        FFstrbuf* buffer = (FFstrbuf*) ffListAdd(dirs);
        ffStrbufInit(buffer);
        ffStrbufSetWS(buffer, pPath);
        ffStrbufReplaceAllC(buffer, '\\', '/');
        ffStrbufEnsureEndsWithC(buffer, '/');
        FF_PLATFORM_PATH_UNIQUE(dirs, buffer);
    }
    CoTaskMemFree(pPath);
}

static void platformPathAddEnvSuffix(FFlist* dirs, const char* env, const char* suffix)
{
    const char* value = getenv(env);
    if(!ffStrSet(value))
        return;

    FFstrbuf* buffer = ffListAdd(dirs);
    ffStrbufInitA(buffer, 64);
    ffStrbufAppendS(buffer, value);
    ffStrbufReplaceAllC(buffer, '\\', '/');
    ffStrbufEnsureEndsWithC(buffer, '/');
    ffStrbufAppendS(buffer, suffix);
    ffStrbufEnsureEndsWithC(buffer, '/');
    FF_PLATFORM_PATH_UNIQUE(dirs, buffer);
}

static void getConfigDirs(FFPlatform* platform)
{
    ffListInit(&platform->configDirs, sizeof(FFstrbuf));

    ffPlatformPathAddEnv(&platform->configDirs, "XDG_CONFIG_HOME");
    ffPlatformPathAddHome(&platform->configDirs, platform, "/.config/");
    platformPathAddKnownFolder(&platform->configDirs, &FOLDERID_RoamingAppData);
    platformPathAddKnownFolder(&platform->configDirs, &FOLDERID_LocalAppData);
    ffPlatformPathAddHome(&platform->configDirs, platform, "");

    if(getenv("MSYSTEM") && getenv("HOME"))
    {
        // We are in MSYS2 / Git Bash
        platformPathAddEnvSuffix(&platform->configDirs, "HOME", ".config/");
        platformPathAddEnvSuffix(&platform->configDirs, "HOME", "");
    }

    ffPlatformPathAddEnv(&platform->configDirs, "XDG_CONFIG_DIRS");
}

static void getConfigDirs(FFPlatform* platform)
{
    ffPlatformPathAddEnv(&platform->dataDirs, "XDG_DATA_HOME");
    ffPlatformPathAddHome(&platform->dataDirs, platform, "/.local/share/");
    platformPathAddKnownFolder(&platform->dataDirs, &FOLDERID_RoamingAppData);
    platformPathAddKnownFolder(&platform->dataDirs, &FOLDERID_LocalAppData);
    ffPlatformPathAddHome(&platform->dataDirs, platform, "");

    if(getenv("MSYSTEM") && getenv("HOME"))
    {
        // We are in MSYS2 / Git Bash
        platformPathAddEnvSuffix(&platform->dataDirs, "HOME", ".local/share/");
        platformPathAddEnvSuffix(&platform->dataDirs, "HOME", "");
    }

    ffPlatformPathAddEnv(&platform->dataDirs, "XDG_DATA_DIRS");
}

static void getUserName(FFPlatform* platform)
{
    ffStrbufEnsureFree(&platform->userName, 64);
    GetUserNameA(platform->userName.chars, (LPDWORD) ffStrbufGetFree(&platform->userName));
    ffStrbufRecalculateLength(&platform->userName);
}

static void getHostName(FFPlatform* platform)
{
    ffStrbufEnsureFree(&platform->hostName, 64);
    GetComputerNameA(platform->hostName.chars, (LPDWORD) ffStrbufGetFree(&platform->hostName));
    ffStrbufRecalculateLength(&platform->hostName);
}

static void getDomainName(FFPlatform* platform)
{
    ffStrbufEnsureFree(&platform->domainName, 64);
    GetComputerNameExA(ComputerNameDnsDomain, platform->domainName.chars, (LPDWORD) ffStrbufGetFree(&platform->domainName));
    ffStrbufRecalculateLength(&platform->domainName);
}

static void getSystemName(FFPlatform* platform)
{
    ffStrbufAppendS(&platform->systemName, "Windows_NT");
}

static void getSystemReleaseAndVersion(FFPlatform* platform)
{
    HKEY hKey;
    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return;

    DWORD bufSize;

    char currentVersion[32];

    {
        DWORD currentMajorVersionNumber;
        DWORD currentMinorVersionNumber;
        bufSize = sizeof(currentMajorVersionNumber);
        if(RegGetValueW(hKey, NULL, L"CurrentMajorVersionNumber", RRF_RT_REG_DWORD, NULL, &currentMajorVersionNumber, &bufSize) == ERROR_SUCCESS &&
            RegGetValueW(hKey, NULL, L"CurrentMinorVersionNumber", RRF_RT_REG_DWORD, NULL, &currentMinorVersionNumber, &bufSize) == ERROR_SUCCESS
        )
            snprintf(currentVersion, sizeof(currentVersion), "%u.%u", (unsigned)currentMajorVersionNumber, (unsigned)currentMinorVersionNumber);
        else
        {
            bufSize = sizeof(currentVersion);
            if(RegGetValueA(hKey, NULL, "CurrentVersion", RRF_RT_REG_SZ, NULL, currentVersion, &bufSize) != ERROR_SUCCESS)
                strcpy(currentVersion, "0.0");
        }
    }

    char currentBuildNumber[32];
    bufSize = sizeof(currentBuildNumber);
    if(RegGetValueA(hKey, NULL, "CurrentBuildNumber", RRF_RT_REG_SZ, NULL, currentBuildNumber, &bufSize) != ERROR_SUCCESS)
        strcpy(currentBuildNumber, "0");

    DWORD ubr;
    bufSize = sizeof(ubr);
    if(RegGetValueA(hKey, NULL, "UBR", RRF_RT_REG_DWORD, NULL, &ubr, &bufSize) != ERROR_SUCCESS || bufSize != sizeof(ubr))
        ubr = 0;

    ffStrbufAppendF(&platform->systemRelease, "%s.%s.%u", currentVersion, currentBuildNumber, (unsigned)ubr);

    ffStrbufEnsureFree(&platform->systemVersion, 256);
    RegGetValueA(hKey, NULL, "DisplayVersion", RRF_RT_REG_SZ, NULL, platform->systemVersion.chars, (LPDWORD) ffStrbufGetFree(&platform->systemVersion));
    ffStrbufRecalculateLength(&platform->systemVersion);

    RegCloseKey(hKey);
}

static void getSystemArchitecture(FFPlatform* platform)
{
    SYSTEM_INFO sysInfo = {0};
    GetNativeSystemInfo(&sysInfo);

    switch(sysInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:
            strcpy(name->machine, "x86_64");
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            strcpy(name->machine, "ia64");
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            strcpy(name->machine, "x86");
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            strcpy(name->machine, "aarch64");
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            strcpy(name->machine, "arm");
            break;
        case PROCESSOR_ARCHITECTURE_PPC:
            strcpy(name->machine, "ppc");
            break;
        case PROCESSOR_ARCHITECTURE_MIPS:
            strcpy(name->machine, "mips");
            break;
        case PROCESSOR_ARCHITECTURE_UNKNOWN:
        default:
            break;
    }
}

void ffPlatformInitImpl(FFPlatform* platform)
{
    getHomeDir(platform);
    getCacheDir(platform);
    getConfigDirs(platform);
    getDataDirs(platform);

    getUserName(platform);
    getHostName(platform);
    getDomainName(platform);

    getSystemName(platform);
    getSystemReleaseAndVersion(platform);
    getSystemArchitecture(platform);
}
