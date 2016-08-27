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

#include <iostream>
#ifdef _WIN32
#include "halley/support/exception.h"

#pragma warning(disable: 6387)
#include "os_win32.h"
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
		hr = pLoc->ConnectServer( L"root\\cimv2", nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &pSvc);
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
	return "\"" + String(LPCSTR(tmp)) + "\", code 0x"+ String::integerToString(hr, 16);
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

void Halley::OSWin32::createLogConsole(String winTitle)
{
	AllocConsole();
	SetConsoleTitle(winTitle.c_str());

	// Position console
	MonitorInfo info;
	info.n = 0;
	EnumDisplayMonitors(nullptr, nullptr, onMonitorInfo, LPARAM(&info));
	if (info.n > 1) {
		HWND con = GetConsoleWindow();
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


#endif
