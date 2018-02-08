/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "halley/text/string_converter.h"
#if defined(_WIN32) && !defined(WINDOWS_STORE)
#include "halley/support/exception.h"

#pragma warning(disable: 6387)
#include "os_win32.h"

#include <iostream>
#include <winuser.h>
#include <Lmcons.h>
#include <Shlobj.h>
#include <fcntl.h>
#include <io.h>
#include <comutil.h>
#include <objbase.h>
#include <atlbase.h>

#pragma comment(lib, "wbemuuid.lib")
//#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "comsuppw.lib")


using namespace Halley;

Halley::OSWin32::OSWin32()
	: pLoc(nullptr)
	, pSvc(nullptr)
{
	// From http://msdn.microsoft.com/en-us/library/aa389762(v=VS.85).aspx
	// "Creating a WMI Application Using C++"

	try {
		// Initialize COM
		HRESULT hr;
		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) throw Exception("Unable to initialize COM.");
		hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
		if (FAILED(hr)) throw Exception("Unable to initialize COM security.");

		// Initialize WMI
		hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&pLoc));
		if (FAILED(hr)) throw Exception("Unable to obtain locator");
		//hr = pLoc->ConnectServer(BSTR(L"ROOT\\DEFAULT"), nullptr, nullptr, 0, 0, 0, 0, &pSvc);
		hr = pLoc->ConnectServer(BSTR(L"root\\cimv2"), nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &pSvc);
		if (FAILED(hr)) throw Exception("Unable to connect to WMI service");

		// Set security on WMI connection
		hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
		if (FAILED(hr)) throw Exception("Unable to set WMI security");
	} catch (std::exception& e) {
		std::cout << "Exception initializing COM/WMI: " << e.what() << std::endl;
		pSvc = nullptr;
		pLoc = nullptr;
	} catch (...) {
		std::cout << "Unknown exception initializing COM/WMI.";
		pSvc = nullptr;
		pLoc = nullptr;
	}

	// Load icon
	HINSTANCE handle = ::GetModuleHandle(nullptr);
	icon = ::LoadIcon(handle, "IDI_MAIN_ICON");
}

Halley::OSWin32::~OSWin32()
{
	if (pSvc) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}
}

static String getCOMError(int hr)
{
	IErrorInfo* info;
	HRESULT hr2 = GetErrorInfo(0, &info);
	if (FAILED(hr2)) return "Failed getting COM error.";
	BSTR str;
	info->GetDescription(&str);
	_bstr_t tmp(str);
	return "\"" + String(LPCSTR(tmp)) + "\", code 0x"+ toString(hr, 16);
}

Halley::String Halley::OSWin32::runWMIQuery(String query, String parameter) const
{
	// See:
	// http://www.codeproject.com/KB/system/UsingWMI.aspx
	// http://www.codeproject.com/KB/system/Using_WMI_in_Visual_C__.aspx

	if (pSvc) {
		try {
			HRESULT hr;
			CComPtr<IEnumWbemClassObject> enumerator;
			BSTR lang = CComBSTR(L"WQL");
			BSTR q = CComBSTR(query.c_str());
			hr = pSvc->ExecQuery(lang, q, WBEM_FLAG_FORWARD_ONLY, nullptr, &enumerator);
			if (FAILED(hr)) throw Exception("Error running WMI query: "+getCOMError(hr));

			ULONG retcnt;
			CComPtr<IWbemClassObject> result;
			hr = enumerator->Next(WBEM_INFINITE, 1L, &result, &retcnt);
			if (retcnt == 0) return "Unknown";
			if (FAILED(hr)) throw Exception("Error obtaining WMI enumeration");

			_variant_t var_val;
			hr = result->Get(parameter.getUTF16().c_str(), 0, &var_val, nullptr, nullptr);
			if (FAILED(hr)) throw Exception("Error retrieving name from WMI query result");

			if (var_val.vt == VT_NULL) {
				return "";
			} else {
				return String(static_cast<const char*>(_bstr_t(var_val)));
			}
		} catch (std::exception& e) {
			std::cout << "Exception running WMI query: " << e.what() << std::endl;
		} catch (...) {
			std::cout << "Unknown exception running WMI query." << std::endl;
		}
	}
	return "Unknown";
}



struct MonitorInfo {
public:
	int n;
	int x, y, w, h;
};

BOOL CALLBACK onMonitorInfo(HMONITOR /*hMonitor*/, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM dwData)
{
	MonitorInfo* info = reinterpret_cast<MonitorInfo*>(dwData);
	info->n++;
	info->x = lprcMonitor->left;
	info->y = lprcMonitor->top;
	info->w = lprcMonitor->right - lprcMonitor->left;
	info->h = lprcMonitor->bottom - lprcMonitor->top;
	return (info->n != 2);
}

void OSWin32::loadWindowIcon(HWND hwnd)
{
	if (icon != nullptr) {
		::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
		::SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
	}
}

void OSWin32::onWindowCreated(void* window)
{
	loadWindowIcon(reinterpret_cast<HWND>(window));
}

void Halley::OSWin32::createLogConsole(String winTitle)
{
	AllocConsole();
	SetConsoleTitle(winTitle.c_str());

	// Icon
	HWND con = GetConsoleWindow();
	loadWindowIcon(con);

	// Position console
	MonitorInfo info;
	info.n = 0;
	EnumDisplayMonitors(nullptr, nullptr, onMonitorInfo, LPARAM(&info));
	if (info.n > 1) {
		RECT rect;
		GetWindowRect(con, &rect);
		int w = rect.right - rect.left;
		int h = info.h;
		//MoveWindow(con, (info.w - w)/2 + info.x, (info.h - h)/2 + info.h, w, h, true);
		SetWindowPos(con, HWND_TOP, (info.w - w)/2 + info.x, (info.h - h)/2 + info.y, w, h, 0);
	}
}

void OSWin32::initializeConsole()
{
	// From http://stackoverflow.com/a/25927081/546712
	// Redirect the CRT standard input, output, and error handles to the console
#pragma warning(push)
#pragma warning(disable: 4996)
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#pragma warning(pop)

	//Clear the error state for each of the C++ standard stream objects. We need to do this, as
	//attempts to access the standard streams before they refer to a valid target will cause the
	//iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
	//to always occur during startup regardless of whether anything has been read from or written to
	//the console or not.
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
}

Halley::ComputerData Halley::OSWin32::getComputerData()
{
	ComputerData data;
	return data;

	TCHAR chrComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerName(chrComputerName, &dwBufferSize)) data.computerName = String(chrComputerName);

	TCHAR name[UNLEN + 1];
	DWORD dwBufferSize2 = UNLEN + 1;
	if (GetUserName(name, &dwBufferSize2)) data.userName = String(name);

	String os = runWMIQuery("SELECT * FROM Win32_OperatingSystem", "Caption");
	String servPack = runWMIQuery("SELECT * FROM Win32_OperatingSystem", "CSDVersion");
	String osArch = "Unknown";
	if (!os.contains("Windows XP") && !os.contains("2003") && !os.contains("2000")) osArch = runWMIQuery("SELECT * FROM Win32_OperatingSystem", "OSArchitecture");
	data.osName = os.trimBoth();
	if (osArch != "Unknown") data.osName += " " + osArch.trimBoth();
	if (servPack != "Unknown") data.osName += " " + servPack.trimBoth();
	data.cpuName = runWMIQuery("SELECT * FROM Win32_Processor", "Name");
	data.gpuName = runWMIQuery("SELECT * FROM Win32_DisplayConfiguration", "DeviceName");
	data.RAM = runWMIQuery("SELECT * FROM Win32_OperatingSystem", "TotalVisibleMemorySize").toInteger64() * 1024;

	return data;
}

Halley::String Halley::OSWin32::getUserDataDir()
{
	TCHAR path[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, path);
	return String(path) + "\\";
}

String OSWin32::getEnvironmentVariable(const String& name)
{
	auto bufferSize = GetEnvironmentVariable(name.c_str(), nullptr, 0);
	std::string buffer(bufferSize - 1, 0);
	GetEnvironmentVariable(name.c_str(), &buffer[0], bufferSize);
	return buffer;
}

Path OSWin32::parseProgramPath(const String&)
{
	HMODULE hModule = GetModuleHandleW(nullptr);
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(hModule, path, MAX_PATH);
	String programPath(path);
	return Path(programPath).parentPath() / ".";
}

void Halley::OSWin32::setConsoleColor(int foreground, int background)
{
	if (foreground == -1) {
		foreground = 7;
	}
	if (background == -1) {
		background = 0;
	}

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, WORD(foreground | (background << 4)));
}

static bool hasDirectory(const Path& directory) {
    DWORD res = GetFileAttributesW(directory.getString().getUTF16().c_str());
    return res != INVALID_FILE_ATTRIBUTES && (res & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

void OSWin32::createDirectories(const Path& path)
{
	size_t n = path.getNumberPaths();
	for (size_t i = 1; i < n; ++i) {
		Path curPath = path.getFront(i);
		if (!hasDirectory(curPath)) {
			if (!CreateDirectoryW(curPath.getString().getUTF16().c_str(), 0)) {
				throw Exception("Unable to create directory: " + curPath + " (trying to make " + path + ")");
			}
		}
	}
}

std::vector<Path> OSWin32::enumerateDirectory(const Path& path)
{
	std::vector<Path> result;

	WIN32_FIND_DATAW ffd;
	auto pathStr = (path.getString() + "/*").replaceAll("/", "\\");
	auto handle = FindFirstFileW(pathStr.getUTF16().c_str(), &ffd);
	if (handle != INVALID_HANDLE_VALUE) {
		do {
			auto filePath = Path(String(ffd.cFileName));
			result.push_back(filePath);
		} while (FindNextFileW(handle, &ffd) != 0);
	}

	return result;
}

int OSWin32::runCommand(String rawCommand)
{
	auto command = rawCommand.getUTF16();
	if (command.length() >= 1024) {
		throw Exception("Command is too long!");
	}
	wchar_t buffer[1024];
	memcpy(buffer, command.c_str(), command.size() * sizeof(wchar_t));
	buffer[command.size()] = 0;

	STARTUPINFOW si;
    PROCESS_INFORMATION pi;
	DWORD exitCode = -2;

	memset(&si, 0, sizeof(STARTUPINFO));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

	if (!CreateProcessW(nullptr, buffer, nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
		return -1;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return int(exitCode);
}

#endif
