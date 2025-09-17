#
# CMakeGDKXbox.cmake : CMake definitions for Microsoft GDK
#
# This version does not include the XboxOne or Scarlett include/lib paths required for Direct3D graphics,
# and uses /SUBSYSTEM:CONSOLE.
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER_WORKS TRUE CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_WORKS TRUE CACHE INTERNAL "")

if(_GDK_XBOX_TOOLCHAIN_)
  return()
endif()

#--- Microsoft Game Development Kit
set(XdkEditionTarget "240602" CACHE STRING "Microsoft GDK Edition")

message("XdkEditionTarget = ${XdkEditionTarget}")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XdkEditionTarget BUILD_USING_BWOI)

#--- Windows SDK
set(SDKVersion "10.0.26100.0" CACHE STRING "Windows SDK Version")

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${SDKVersion})

#--- Locate Visual Studio (needed for VC Runtime DLLs)

if (NOT DEFINED VCInstallDir AND DEFINED ENV{VCINSTALLDIR})
    cmake_path(SET VCInstallDir "$ENV{VCINSTALLDIR}")
endif()

if (NOT DEFINED VCInstallDir)
    set(GDK_VS_EDITIONS "Community" "Professional" "Enterprise" "Preview" "BuildTools")

    if (MSVC_TOOLSET_VERSION MATCHES "143")
        foreach(vsedition IN LISTS GDK_VS_EDITIONS)
            cmake_path(SET VCInstallDir "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/${vsedition}/VC")
            if(EXISTS ${VCInstallDir})
                break()
            endif()
        endforeach()
    else()
        foreach(vsedition IN LISTS GDK_VS_EDITIONS)
            cmake_path(SET VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/${vsedition}/VC")
            if(EXISTS ${VCInstallDir})
                break()
            endif()
        endforeach()

        if (NOT EXISTS ${VCInstallDir})
            foreach(vsedition IN LISTS GDK_VS_EDITIONS)
                cmake_path(SET VCInstallDir "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/${vsedition}/VC")
                if(EXISTS ${VCInstallDir})
                    break()
                endif()
            endforeach()
        endif()
    endif()
endif()

if(EXISTS ${VCInstallDir})
    message("VCInstallDir = ${VCInstallDir}")
else()
    message(FATAL_ERROR "ERROR: Failed to locate Visual Studio 2019 or 2022 install")
endif()

# Find VC toolset/runtime versions
file(STRINGS "${VCInstallDir}/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" VCToolsVersion)
message("VCToolsVersion = ${VCToolsVersion}")

file(STRINGS "${VCInstallDir}/Auxiliary/Build/Microsoft.VCRedistVersion.default.txt" VCToolsRedistVersion)
message("VCToolsRedistVersion = ${VCToolsRedistVersion}")

#--- GameRuntime and Extension Libraries
set(_GDK_XBOX_ ON)
#include(${CMAKE_CURRENT_LIST_DIR}/GDK-targets.cmake)

set(DurangoXdkInstallPath "${Console_SdkRoot}/${XdkEditionTarget}")

message("Microsoft GDK = ${DurangoXdkInstallPath}")

#--- Windows SDK (BWOI or installed)
if(BUILD_USING_BWOI)
    if (DEFINED ENV{ExtractedFolder})
        cmake_path(SET ExtractedFolder $ENV{ExtractedFolder})
    else()
        set(ExtractedFolder "d:/xtrctd.sdks/BWOIExample/")
    endif()

    if(NOT EXISTS ${ExtractedFolder})
        message(FATAL_ERROR "ERROR: BWOI requires a valid ExtractedFolder (${ExtractedFolder})")
    endif()

    set(WindowsSdkDir "${ExtractedFolder}/Windows Kits/10")
    if (NOT EXISTS ${WindowsSdkDir})
        cmake_path(SET WindowsSdkDir "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
    endif()
else()
    cmake_path(SET WindowsSdkDir "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
endif()

if(EXISTS "${WindowsSdkDir}/Include/${SDKVersion}" )
    message("Windows SDK = v${SDKVersion} in ${WindowsSdkDir}")
else()
    message(FATAL_ERROR "ERROR: Cannot locate Windows SDK (${SDKVersion})")
endif()

#--- Headers
set(Console_EndpointIncludeRoot
    "${DurangoXdkInstallPath}/GXDK/gameKit/Include"
    "${DurangoXdkInstallPath}/GRDK/gameKit/Include")
set(Console_WindowsIncludeRoot ${WindowsSdkDir}/Include/${SDKVersion})
set(Console_SdkIncludeRoot
    "${Console_EndpointIncludeRoot}"
    "${Console_WindowsIncludeRoot}/um"
    "${Console_WindowsIncludeRoot}/shared"
    "${Console_WindowsIncludeRoot}/winrt"
    "${Console_WindowsIncludeRoot}/cppwinrt"
    "${Console_WindowsIncludeRoot}/ucrt")

#--- Libraries
# Don't link with onecore.lib, kernel32.lib, etc.
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")

# Need to link with "onecore" versions of Visual C++ libraries ("msvc_x64_x64" environment uses desktop libpath)
set(VC_OneCore_LibPath "${VCInstallDir}/Tools/MSVC/${VCToolsVersion}/lib/onecore/x64")
if(NOT EXISTS ${VC_OneCore_LibPath}/msvcrt.lib)
    message(FATAL_ERROR "ERROR: Cannot locate msvcrt.lib for the Visual C++ toolset (${VCToolsVersion})")
endif()

set(Console_LibRoot ${WindowsSdkDir}/Lib/${SDKVersion})
set(Console_EndpointLibRoot
    "${DurangoXdkInstallPath}/GXDK/gameKit/Lib/amd64"
    "${DurangoXdkInstallPath}/GRDK/gameKit/Lib/amd64")
set(Console_SdkLibPath
    "${Console_EndpointLibRoot}"
    "${Console_LibRoot}/ucrt/x64"
    "${Console_LibRoot}/um/x64")

set(Console_Libs xgameplatform.lib xgameruntime.lib)

#--- Binaries
set(Console_UCRTRedistDebug ${WindowsSdkDir}/bin/${SDKVersion}/x64/ucrt)
if(NOT EXISTS ${Console_UCRTRedistDebug}/ucrtbased.dll)
    message(FATAL_ERROR "ERROR: Cannot locate ucrtbased.dll in the Windows SDK (${SDKVersion})")
endif()

set(CRTPlatformToolset 143)
if (NOT EXISTS "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/x64/Microsoft.VC${CRTPlatformToolset}.CRT")
    set(CRTPlatformToolset 142)
endif()

message("CRT Platform Toolset = ${CRTPlatformToolset}")

set(CppRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/x64/Microsoft.VC${CRTPlatformToolset}.CRT")
set(OpenMPRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/x64/Microsoft.VC${CRTPlatformToolset}.OpenMP")
set(DebugCppRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/Debug_NonRedist/x64/Microsoft.VC${CRTPlatformToolset}.DebugCRT")
set(DebugOpenMPRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/Debug_NonRedist/x64/Microsoft.VC${CRTPlatformToolset}.DebugOpenMP")

#--- Build options
# Required preprocessor defines
# WIN32
# _WINDOWS

# Traditional indicator of a 'Win32 console' application
set(Console_Defines ${Console_Defines} _CONSOLE)

# Standard Debug vs. Release preprocessor definitions
# (automatically defined by MSVC and MSVC-like compilers)
# _DEBUG (Debug)
# NDEBUG (Release without asserts)

# Build as Unicode (see UTF-8 Everywhere article's Win32 recommendations)
set(Console_Defines ${Console_Defines} _UNICODE UNICODE)

# Game Core on Xbox preprocessor definitions
set(Console_Defines ${Console_Defines} WIN32_LEAN_AND_MEAN _GAMING_XBOX WINAPI_FAMILY=WINAPI_FAMILY_GAMES)

# Additional recommended preprocessor defines
set(Console_Defines ${Console_Defines} _CRT_USE_WINAPI_PARTITION_APP _UITHREADCTXT_SUPPORT=0 __WRL_CLASSIC_COM_STRICT__)

# Default library controls
set(Console_Defines ${Console_Defines} _ATL_NO_DEFAULT_LIBS __WRL_NO_DEFAULT_LIB__)

set(UnsupportedLibs advapi32.lib comctl32.lib comsupp.lib dbghelp.lib gdi32.lib gdiplus.lib guardcfw.lib kernel32.lib mmc.lib msimg32.lib msvcole.lib msvcoled.lib mswsock.lib ntstrsafe.lib ole2.lib ole2autd.lib ole2auto.lib ole2d.lib ole2ui.lib ole2uid.lib ole32.lib oleacc.lib oleaut32.lib oledlg.lib oledlgd.lib oldnames.lib runtimeobject.lib shell32.lib shlwapi.lib strsafe.lib urlmon.lib user32.lib userenv.lib wlmole.lib wlmoled.lib onecore.lib)

# Required compiler switches:
# /MD or /MDd (VC Runtime DLL)
# /O? or /Od (Optimize code)

# Required linker switches:
# /MACHINE:X64 /SUBSYSTEM:CONSOLE
# /DYNAMICBASE
# /NXCOMPAT
set(Console_LinkOptions "/SUBSYSTEM:CONSOLE,10.0" "/DYNAMICBASE" "/NXCOMPAT")

# Prevent accidental use of libraries that are not supported by Game Core on Xbox
foreach(arg ${UnsupportedLibs})
    list(APPEND Console_LinkOptions "/NODEFAULTLIB:${arg}")
endforeach()

if (OPTIMIZE_FOR_SCARLETT)
    message("Optimizing code for Xbox Series X|S (won't run on Xbox One)")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   set(Console_ArchOptions /favor:AMD64 $<IF:$<BOOL:${OPTIMIZE_FOR_SCARLETT}>,/arch:AVX2,/arch:AVX>)

   # Titles should use this switch to optimize the vzeroupper codegen based on console target
   if(OPTIMIZE_FOR_SCARLETT)
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.23)
            set(Console_ArchOptions ${Console_ArchOptions} /d2vzeroupper)
            set(Console_ArchOptions_LTCG /d2:-vzeroupper)
        endif()
   elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.30)
        set(Console_ArchOptions ${Console_ArchOptions} /d2vzeroupper-)
        set(Console_ArchOptions_LTCG /d2:-vzeroupper-)
   endif()

endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # -march=btver2 to target AMD Jaguar CPU; -march=znver2 to target AMD Hercules CPU (requires clang v9; otherwise use znver1)
   set(Console_ArchOptions -march=$<IF:$<BOOL:${OPTIMIZE_FOR_SCARLETT}>,$<IF:$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,9.0>,znver2,znver1>,btver2>)
endif()

set(_GDK_XBOX_TOOLCHAIN_ ON)
