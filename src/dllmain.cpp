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
std::string sFixVersion = "0.0.2";
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
const float fNativeAspect = 16.00f / 9.00f;
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
float fGameplayFOVMulti;
bool bFixFOV;
bool bFixHUD;
bool bFixMovies;
bool bAdjustFramerate;
int iFramerateTarget;

// Variables
const float fLetterboxAspect = 2.35f;
int iCurrentResX;
int iCurrentResY;
std::string sMovieName;
bool bLetterboxedMovie = false;

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
    inipp::get_value(ini.sections["Gameplay FOV"], "Multiplier", fGameplayFOVMulti);
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFixFOV);
    inipp::get_value(ini.sections["Fix HUD"], "Enabled", bFixHUD);
    inipp::get_value(ini.sections["Fix Movies"], "Enabled", bFixMovies);
    inipp::get_value(ini.sections["Framerate"], "Enabled", bAdjustFramerate);
    inipp::get_value(ini.sections["Framerate"], "FramerateTarget", iFramerateTarget);

    // Log ini parse
    spdlog_confparse(bCustomRes);
    spdlog_confparse(iCustomResX);
    spdlog_confparse(iCustomResY);
    spdlog_confparse(fGameplayFOVMulti);
    spdlog_confparse(bFixFOV);
    spdlog_confparse(bFixHUD);
    spdlog_confparse(bFixMovies);
    spdlog_confparse(bAdjustFramerate);
    spdlog_confparse(iFramerateTarget);

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

            Memory::Write(ResolutionListScanResult + 0x70, iCustomResX); // Not exactly sure what this second set is used for but may as well change it.
            Memory::Write(ResolutionListScanResult + 0x74, iCustomResY);

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

void FOV()
{
    if (fGameplayFOVMulti != 1.00f) 
    {
        // Gameplay FOV
        std::uint8_t* GameplayFOVScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? 48 8B ?? E8 ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 45 ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ??");
        if (GameplayFOVScanResult) {
            spdlog::info("Gameplay FOV: Address is {:s}+{:x}", sExeName.c_str(), GameplayFOVScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid GameplayFOVMidHook{};
            GameplayFOVMidHook = safetyhook::create_mid(GameplayFOVScanResult,
                [](SafetyHookContext& ctx) {
                    ctx.xmm0.f32[0] *= fGameplayFOVMulti;
                });
        }
        else {
            spdlog::error("Gameplay FOV: Pattern scan failed.");
        }
    }

    if (bFixFOV) 
    {
        // Cutscene camera
        std::uint8_t* CutsceneFOVScanResult = Memory::PatternScan(exeModule, "E8 ?? ?? ?? ?? 83 ?? 01 75 ?? F3 0F ?? ?? ?? ?? ?? ?? EB ??");
        std::uint8_t* CutsceneCameraPositionScanResult = Memory::PatternScan(exeModule, "74 ?? 83 ?? FF E8 ?? ?? ?? ?? EB ?? E8 ?? ?? ?? ?? 84 ?? 74 ?? E8 ?? ?? ?? ??");
        if (CutsceneFOVScanResult && CutsceneCameraPositionScanResult) {
            spdlog::info("Cutscene Camera: FOV: Address is {:s}+{:x}", sExeName.c_str(), CutsceneFOVScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(CutsceneFOVScanResult + 0x8, "\x90\x90", 2);
            spdlog::info("Cutscene Camera: FOV: Patched instruction.");

            spdlog::info("Cutscene Camera: Position: Address is {:s}+{:x}", sExeName.c_str(), CutsceneCameraPositionScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(CutsceneCameraPositionScanResult, "\xEB\x53", 2);
            spdlog::info("Cutscene Camera: Position: Patched instruction.");
        }
        else {
            spdlog::error("Cutscene Camera: Pattern scan(s) failed.");
        }
    }
   
}

void Movies()
{
    if (bFixMovies) 
    {
        // Very cool naming scheme
        static std::vector<std::string> sNonLetterboxedMovies = {
            "02D0FA5FDB4FA4FEA5D0C1CAAEF8D6CE260EADE39CC753AB7D0C376B55012958",
            "388CDBB2F103BE34D9ECF75D5038A7AF5406F9B40250C3B569D167D851C18189",
            "68C0955A422BF998BB1647D704DBFDAF02C1EAEB5F21CA568BC9B43EB53EBF66",
            "F11D39AF90D578381099BAE062CB35BE9BDBB339B67AAAF5CBC48FBFE5F59A37",
        };

        // Movie name
        std::uint8_t* MovieNameScanResult = Memory::PatternScan(exeModule, "48 8D ?? ?? ?? E8 ?? ?? ?? ?? B8 01 00 00 00 8B ?? 87 ?? ?? 8B ??");
        if (MovieNameScanResult) {
            spdlog::info("Movies: Name: Address is {:s}+{:x}", sExeName.c_str(), MovieNameScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MovieNameMidHook{};
            MovieNameMidHook = safetyhook::create_mid(MovieNameScanResult,
            [](SafetyHookContext& ctx) {
                if (ctx.rcx) {
                    // Grab movie filename
                    sMovieName = *(char**)ctx.rcx;

                    // Check if it is in the non-letterboxed list
                    bLetterboxedMovie = std::ranges::none_of(sNonLetterboxedMovies, [&](const std::string& movie) {
                        return sMovieName.find(movie) != std::string::npos;
                    });
                }
            });
        }

        // Movies
        std::uint8_t* MovieSizeScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? 48 8D ?? ?? ?? 48 89 ?? ?? ?? 48 8D ?? ?? ?? C7 ?? ?? ?? 00 00 80 3F");
        std::uint8_t* MovieAspectScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? F3 0F ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F ?? ?? ?? 4D ?? ??");
        if (MovieSizeScanResult && MovieAspectScanResult) {
            spdlog::info("Movies: Size: Address is {:s}+{:x}", sExeName.c_str(), MovieSizeScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MovieSizeMidHook{};
            MovieSizeMidHook = safetyhook::create_mid(MovieSizeScanResult,
            [](SafetyHookContext& ctx) { 
                if (fAspectRatio != fNativeAspect) {
                    if (bLetterboxedMovie)
                        ctx.xmm1.f32[0] = fLetterboxAspect / fNativeAspect;
                    else
                        ctx.xmm1.f32[0] = 1.00f;
                }
            });

            spdlog::info("Movies: Aspect Ratio: Address is {:s}+{:x}", sExeName.c_str(), MovieAspectScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MovieAspectMidHook{};
            MovieAspectMidHook = safetyhook::create_mid(MovieAspectScanResult - 0x8,
            [](SafetyHookContext& ctx) {
                if (fAspectRatio != fNativeAspect) {
                    if (bLetterboxedMovie)
                        ctx.xmm1.f32[0] = fLetterboxAspect;
                    else
                        ctx.xmm1.f32[0] = fNativeAspect;
                }  
            });
        }
        else {
            spdlog::error("Movies: Size: Pattern scan(s) failed.");
        }
    }  
}

void HUD()
{
    if (bFixHUD) 
    {
        // Cutscene letterboxing
        std::uint8_t* CutsceneLetterboxingScanResult = Memory::PatternScan(exeModule, "34 01 48 8D ?? ?? ?? 44 ?? ?? 48 8D ?? ?? ?? E8 ?? ?? ?? ?? 4C ?? ?? ?? ??");
        if (CutsceneLetterboxingScanResult) {
            spdlog::info("HUD: Cutscene Letterboxing: Address is {:s}+{:x}", sExeName.c_str(), CutsceneLetterboxingScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid CutsceneLetterboxingMidHook{};
            CutsceneLetterboxingMidHook = safetyhook::create_mid(CutsceneLetterboxingScanResult,
            [](SafetyHookContext& ctx) {
                // Disable letterboxing at <16:9
                if (fAspectRatio < fNativeAspect)
                    ctx.rax = (ctx.rax & ~0xFF) | 0x01;
            });
        }
        else {
            spdlog::error("HUD: Cutscene Letterboxing: Pattern scan failed.");
        }  
        
        // HUD height
        std::uint8_t* HUDHeightScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? 48 8B ?? ?? ?? 48 83 ?? ?? 5F E9 ?? ?? ?? ?? CC 48 83 ?? ??");
        std::uint8_t* MenuHeightScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 0F ?? ?? 77 ?? 0F ?? ?? 73 ?? 0F ?? ?? 77 ??");
        std::uint8_t* MarkersHeightScanResult = Memory::PatternScan(exeModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 48 83 ?? ?? C3");
        if (HUDHeightScanResult && MenuHeightScanResult && MarkersHeightScanResult) {
            static std::uint8_t* HUDHeight = Memory::GetAbsolute(MenuHeightScanResult - 0x4);

            spdlog::info("HUD: Height: Address is {:s}+{:x}", sExeName.c_str(), HUDHeightScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid HUDHeightMidHook{};
            HUDHeightMidHook = safetyhook::create_mid(HUDHeightScanResult,
            [](SafetyHookContext& ctx) {
                if (fAspectRatio < fNativeAspect)
                    ctx.xmm0.f32[0] = (float)iCurrentResY / (1920.00f / fAspectRatio);
                
                if (HUDHeight) {
                    if (fAspectRatio < fNativeAspect)
                        Memory::Write(HUDHeight, 1920.00f / fAspectRatio);
                    else if (HUDHeight)
                        Memory::Write(HUDHeight, 1080.00f);
                }
            });

            spdlog::info("HUD: Menu Height: Address is {:s}+{:x}", sExeName.c_str(), MenuHeightScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MenuHeight1MidHook{};
            MenuHeight1MidHook = safetyhook::create_mid(MenuHeightScanResult,
            [](SafetyHookContext& ctx) {
                if (fAspectRatio < fNativeAspect)
                    ctx.xmm0.f32[0] = ctx.xmm13.f32[0] / 1080.00f;
            });          

            static SafetyHookMid MenuHeight2MidHook{};
            MenuHeight2MidHook = safetyhook::create_mid(MenuHeightScanResult - 0xA8,
            [](SafetyHookContext& ctx) {
                if (fAspectRatio < fNativeAspect)
                    ctx.xmm6.f32[0] = ctx.xmm11.f32[0] / fNativeAspect;
            }); 
            
            spdlog::info("HUD: Markers Height: Address is {:s}+{:x}", sExeName.c_str(), MarkersHeightScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid MarkersHeightMidHook{};
            MarkersHeightMidHook = safetyhook::create_mid(MarkersHeightScanResult + 0x18,
            [](SafetyHookContext& ctx) {
                if (fAspectRatio < fNativeAspect) {
                    ctx.xmm3.f32[0] += 540.00f;
                    ctx.xmm3.f32[0] -= (1920.00f / fAspectRatio) / 2.00f;
                }
            }); 
        }
        else {
            spdlog::error("HUD: Height: Pattern scan(s) failed.");
        }

        // HUD Objects
        std::uint8_t* HUDObjectsScanResult = Memory::PatternScan(exeModule, "4D ?? ?? 74 ?? 41 ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 41 ?? 01 00 00 00");
        if (HUDObjectsScanResult) {
            static std::string sHUDObjectName;
            static short iHUDObjectX;
            static short iHUDObjectY;

            spdlog::info("HUD: Objects: Address is {:s}+{:x}", sExeName.c_str(), HUDObjectsScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid HUDObjectsMidHook{};
            HUDObjectsMidHook = safetyhook::create_mid(HUDObjectsScanResult,
                [](SafetyHookContext& ctx) {
                    if (ctx.rdx) {
                        sHUDObjectName = (char*)(ctx.rdx - 0x10);
                        iHUDObjectX = *reinterpret_cast<short*>(ctx.rdx + 0xF0);
                        iHUDObjectY = *reinterpret_cast<short*>(ctx.rdx + 0xF2);

                        // Stealth vignette
                        if (sHUDObjectName.contains("Null_item_list") && iHUDObjectX == 5000 && iHUDObjectY == 2400) {
                            if (fAspectRatio > 3.55f)
                                ctx.rax = (static_cast<uintptr_t>(iHUDObjectY) << 16) | (short)ceilf(iHUDObjectY * fAspectRatio);
                            else if (fAspectRatio < fNativeAspect)
                                ctx.rax = (static_cast<uintptr_t>((short)ceilf(iHUDObjectX / fAspectRatio)) << 16) | iHUDObjectX;
                        }

                        // Stealth vignette inner
                        if (sHUDObjectName.contains("Blur") && iHUDObjectX == 4000 && iHUDObjectY == 2200) {
                            if (fAspectRatio > 3.55f)
                                ctx.rax = (static_cast<uintptr_t>(iHUDObjectY) << 16) | (short)ceilf(iHUDObjectY * fAspectRatio);
                            else if (fAspectRatio < fNativeAspect)
                                ctx.rax = (static_cast<uintptr_t>((short)ceilf(iHUDObjectX / fAspectRatio)) << 16) | iHUDObjectX;
                        }
                    }
                });
        }
        else {
            spdlog::error("HUD Objects: Pattern scan failed.");
        }
    } 
}

void Framerate()
{
    if (bAdjustFramerate) 
    {
        // Framerate target
        std::uint8_t* FramerateTargetScanResult = Memory::PatternScan(exeModule, "48 83 ?? 03 73 ?? 8B ?? ?? EB ?? 8B ?? 48 8B ?? ?? ?? 48 33 ?? E8 ?? ?? ?? ?? 48 83 ?? ?? C3");
        if (FramerateTargetScanResult) {
            spdlog::info("Framerate: Target: Address is {:s}+{:x}", sExeName.c_str(), FramerateTargetScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid FramerateTargetMidHook{};
            FramerateTargetMidHook = safetyhook::create_mid(FramerateTargetScanResult + 0xD,
            [](SafetyHookContext& ctx) {
                ctx.rax = iFramerateTarget;
            });       
        }
        else {
            spdlog::error("Framerate: Target: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    CustomResolution();
    CurrentResolution();
    FOV();
    Movies();
    HUD();
    Framerate();

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
