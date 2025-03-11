#include "stdafx.h"
#include "helper.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <inipp/inipp.h>
#include <safetyhook.hpp>

#define spdlog_confparse(var) spdlog::info("Config Parse: {}: {}", #var, var)

HMODULE exeModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Fix details
std::string sFixName = "RiseOfTheRoninFix";
std::string sFixVersion = "0.0.1";
std::filesystem::path sFixPath;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::string sLogFile = sFixName + ".log";
std::filesystem::path sExePath;
std::string sExeName;

// Aspect ratio / FOV / HUD
std::pair DesktopDimensions = { 0,0 };
const float fPi = 3.1415926535f;
const float fNativeAspect = (16.00f / 9.00f);
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDWidthOffset;
float fHUDHeight;
float fHUDHeightOffset;

// Ini variables
bool bCustomRes;
int iCustomResX;
int iCustomResY;
bool bFixMovies;

// Variables
int iCurrentResX;
int iCurrentResY;

void CalculateAspectRatio(bool bLog)
{
    if (iCurrentResX <= 0 || iCurrentResY <= 0)
        return;

    // Calculate aspect ratio
    fAspectRatio = (float)iCurrentResX / (float)iCurrentResY;
    fAspectMultiplier = fAspectRatio / fNativeAspect;

    // HUD 
    fHUDWidth = (float)iCurrentResY * fNativeAspect;
    fHUDHeight = (float)iCurrentResY;
    fHUDWidthOffset = (float)(iCurrentResX - fHUDWidth) / 2.00f;
    fHUDHeightOffset = 0.00f;
    if (fAspectRatio < fNativeAspect) {
        fHUDWidth = (float)iCurrentResX;
        fHUDHeight = (float)iCurrentResX / fNativeAspect;
        fHUDWidthOffset = 0.00f;
        fHUDHeightOffset = (float)(iCurrentResY - fHUDHeight) / 2.00f;
    }

    // Log details about current resolution
    if (bLog) {
        spdlog::info("----------");
        spdlog::info("Current Resolution: Resolution: {:d}x{:d}", iCurrentResX, iCurrentResY);
        spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
        spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
        spdlog::info("Current Resolution: fHUDWidth: {}", fHUDWidth);
        spdlog::info("Current Resolution: fHUDHeight: {}", fHUDHeight);
        spdlog::info("Current Resolution: fHUDWidthOffset: {}", fHUDWidthOffset);
        spdlog::info("Current Resolution: fHUDHeightOffset: {}", fHUDHeightOffset);
        spdlog::info("----------");
    }
}

void Logging()
{
    // Get path to DLL
    WCHAR dllPath[_MAX_PATH] = {0};
    GetModuleFileNameW(thisModule, dllPath, MAX_PATH);
    sFixPath = dllPath;
    sFixPath = sFixPath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = {0};
    GetModuleFileNameW(exeModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // Spdlog initialisation
    try
    {
        logger = spdlog::basic_logger_st(sFixName, sExePath.string() + sLogFile, true);
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::debug);

        spdlog::info("----------");
        spdlog::info("{:s} v{:s} loaded.", sFixName, sFixVersion);
        spdlog::info("----------");
        spdlog::info("Log file: {}", sFixPath.string() + sLogFile);
        spdlog::info("----------");
        spdlog::info("Module Name: {:s}", sExeName);
        spdlog::info("Module Path: {:s}", sExePath.string());
        spdlog::info("Module Address: 0x{:x}", (uintptr_t)exeModule);
        spdlog::info("Module Timestamp: {:d}", Memory::ModuleTimestamp(exeModule));
        spdlog::info("----------");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        FreeLibraryAndExitThread(thisModule, 1);
    }
}

void Configuration()
{
    // Inipp initialisation
    std::ifstream iniFile(sFixPath / sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVersion.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sFixPath.string().c_str() << std::endl;
        spdlog::error("ERROR: Could not locate config file {}", sConfigFile);
        spdlog::shutdown();
        FreeLibraryAndExitThread(thisModule, 1);
    }
    else
    {
        spdlog::info("Config file: {}", sFixPath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    spdlog::info("----------");

    // Load settings from ini
    inipp::get_value(ini.sections["Custom Resolution"], "Enabled", bCustomRes);
    inipp::get_value(ini.sections["Custom Resolution"], "Width", iCustomResX);
    inipp::get_value(ini.sections["Custom Resolution"], "Height", iCustomResY);
    inipp::get_value(ini.sections["Fix Movies"], "Enabled", bFixMovies);

    // Log ini parse
    spdlog_confparse(bCustomRes);
    spdlog_confparse(iCustomResX);
    spdlog_confparse(iCustomResY);
    spdlog_confparse(bFixMovies);

    spdlog::info("----------");
}

void CustomResolution()
{
    if (bCustomRes) 
    {
        // Grab desktop resolution
        DesktopDimensions = Util::GetPhysicalDesktopDimensions();

        // Use desktop resolution if custom resolution is set automatic/invalid
        if (iCustomResX <= 0 || iCustomResY <= 0) {
            iCustomResX = DesktopDimensions.first;
            iCustomResY = DesktopDimensions.second;
        }

        // Resolution list
        std::uint8_t* ResolutionListScanResult = Memory::PatternScan(exeModule, "00 1E 00 00 E0 10 00 00 00 14 00 00 70 08 00 00");
        if (ResolutionListScanResult) {
            spdlog::info("Resolution List: Address is {:s}+{:x}", sExeName.c_str(), ResolutionListScanResult - (std::uint8_t*)exeModule);

            // Overwrite 7680x4320
            Memory::Write(ResolutionListScanResult, iCustomResX);
            Memory::Write(ResolutionListScanResult + 0x4, iCustomResY);
            spdlog::info("Resolution List: Replaced 7680x4320 with {}x{}", iCustomResX, iCustomResY);
        }
        else {
            spdlog::error("Resolution List: Pattern scan failed.");
        }

        // Resolution string
        std::uint8_t* ResolutionStringScanResult = Memory::PatternScan(exeModule, "83 ?? 0D 0F 87 ?? ?? ?? ?? 48 8D ?? ?? ?? ?? ?? 48 ?? 8B ?? ?? ?? ?? ?? ?? 48 03 ?? FF ?? 4C 8B ?? ?? ?? ?? ??");
        if (ResolutionStringScanResult) {
            spdlog::info("Resolution String: Address is {:s}+{:x}", sExeName.c_str(), ResolutionStringScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid ResolutionStringMidHook{};
            ResolutionStringMidHook = safetyhook::create_mid(ResolutionStringScanResult + 0x126, // Huge offset, maybe find an alternate way of getting here
                [](SafetyHookContext& ctx) {
                    if (ctx.r13) {
                        const std::wstring oldRes = L"7680 x 4320";
                        std::wstring newRes = std::to_wstring(iCustomResX) + L" x " + std::to_wstring(iCustomResY);
                        
                        wchar_t* currentString = (wchar_t*)ctx.r13;
                        if (wcsncmp(currentString, oldRes.c_str(), oldRes.size()) == 0) {
                            if (newRes.size() <= oldRes.size()) {
                                std::wmemcpy(currentString, newRes.c_str(), newRes.size() + 1);
                                spdlog::info("Resolution String: Replaced {} with {}", Util::wstring_to_string(oldRes), Util::wstring_to_string(newRes));
                            }
                        }
                    }
                });
        }
        else {
            spdlog::error("Resolution String: Pattern scan failed.");
        }
    }
}

void CurrentResolution()
{
    // Current resolution
    std::uint8_t* CurrentResolutionScanResult = Memory::PatternScan(exeModule, "49 89 ?? ?? ?? ?? ?? 41 8B ?? ?? ?? ?? ?? ?? 41 89 ?? ?? ?? ?? ?? 4B ?? ?? ?? 49 89 ?? ?? ?? ?? ??");
    if (CurrentResolutionScanResult) {
        spdlog::info("Current Resolution: Address is {:s}+{:x}", sExeName.c_str(), CurrentResolutionScanResult - (std::uint8_t*)exeModule);
        static SafetyHookMid CurrentResolutionMidHook{};
        CurrentResolutionMidHook = safetyhook::create_mid(CurrentResolutionScanResult,
            [](SafetyHookContext& ctx) {
                  // Get current resolution
                  int iResX = static_cast<int>(ctx.rcx & 0xFFFFFFFF);
                  int iResY = static_cast<int>((ctx.rcx >> 32) & 0xFFFFFFFF);
  
                  // Log resolution
                  if (iResX != iCurrentResX || iResY != iCurrentResY) {
                      iCurrentResX = iResX;
                      iCurrentResY = iResY;
                      CalculateAspectRatio(true);
                  }
            });
    }
    else {
        spdlog::error("Current Resolution: Pattern scan failed.");
    }
}

void Movies()
{
    if (bFixMovies) 
    {
        // Movies
        std::uint8_t* MovieSizeScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? 48 8D ?? ?? ?? 48 89 ?? ?? ?? 48 8D ?? ?? ?? C7 ?? ?? ?? 00 00 80 3F");
        std::uint8_t* MovieAspectScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F ?? ?? ?? 4D ?? ??");
        if (MovieSizeScanResult && MovieAspectScanResult) {
            spdlog::info("Movies: Size: Address is {:s}+{:x}", sExeName.c_str(), MovieSizeScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MovieSizeMidHook{};
            MovieSizeMidHook = safetyhook::create_mid(MovieSizeScanResult,
            [](SafetyHookContext& ctx) {
                 if (fAspectRatio > fNativeAspect)
                    ctx.xmm1.f32[0] = 1.00f;
            });

            spdlog::info("Movies: Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), MovieAspectScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MovieAspectMidHook{};
            MovieAspectMidHook = safetyhook::create_mid(MovieAspectScanResult + 0x8,
            [](SafetyHookContext& ctx) {
                  if (fAspectRatio > fNativeAspect)
                    ctx.xmm0.f32[0] = 0.00f;
            });

        }
        else {
            spdlog::error("Movies: Size: Pattern scan(s) failed.");
        }
    }  
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    CustomResolution();
    CurrentResolution();
    Movies();

    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        thisModule = hModule;

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST);
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}