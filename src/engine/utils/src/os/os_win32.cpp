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
#include "halley/support/logger.h"
#if defined(_WIN32) && !defined(WINDOWS_STORE)
#include "halley/support/exception.h"

#pragma warning(disable: 6387)
#include "os_win32.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <winuser.h>
#include <Lmcons.h>
#include <Shlobj.h>
#include <fcntl.h>
#include <io.h>
#include <comutil.h>
#include <objbase.h>
#include <fstream>
#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <atlbase.h>
#endif

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
#ifdef __MINGW32__
		// Fix symbol undefined bug of MINGW
		static const IID CLSID_WbemAdministrativeLocator = {0xcb8555cc, 0x9128, 0x11d1, {0xad,0x9b,0x00,0xc0,0x4f,0xd8,0xfd,0xff}};
#endif

		// Initialize COM
		HRESULT hr;
		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) throw Exception("Unable to initialize COM.", HalleyExceptions::OS);
		hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
		if (FAILED(hr)) throw Exception("Unable to initialize COM security.", HalleyExceptions::OS);

		// Initialize WMI
		hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<void**>(&pLoc));
		if (FAILED(hr)) throw Exception("Unable to obtain locator", HalleyExceptions::OS);
		//hr = pLoc->ConnectServer(BSTR(L"ROOT\\DEFAULT"), nullptr, nullptr, 0, 0, 0, 0, &pSvc);
		hr = pLoc->ConnectServer(BSTR(L"root\\cimv2"), nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &pSvc);
		if (FAILED(hr)) throw Exception("Unable to connect to WMI service", HalleyExceptions::OS);

		// Set security on WMI connection
		hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
		if (FAILED(hr)) throw Exception("Unable to set WMI security", HalleyExceptions::OS);
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
#ifdef __MINGW32__
	String tmp(str);
    return "\"" + tmp + "\", code 0x"+ toString(hr, 16);
#else
	_bstr_t tmp(str);
	return "\"" + String(LPCSTR(tmp)) + "\", code 0x"+ toString(hr, 16);
#endif
}

Halley::String Halley::OSWin32::runWMIQuery(String query, String parameter) const
{
	// See:
	// http://www.codeproject.com/KB/system/UsingWMI.aspx
	// http://www.codeproject.com/KB/system/Using_WMI_in_Visual_C__.aspx

	if (pSvc) {
		try {
#ifdef __MINGW32__
			HRESULT hr;
			IEnumWbemClassObject* enumerator;
			BSTR lang = const_cast<BSTR>(L"WQL");
			auto query_wchar = query.getUTF16();
			BSTR q = const_cast<BSTR>(query_wchar.c_str());
			hr = pSvc->ExecQuery(lang, q, WBEM_FLAG_FORWARD_ONLY, nullptr, &enumerator);
			if (FAILED(hr)) {
				if (enumerator) enumerator->Release(); enumerator = nullptr;
				throw Exception("Error running WMI query: "+getCOMError(hr), HalleyExceptions::OS);
			}

			ULONG retcnt;
			IWbemClassObject* result;
			hr = enumerator->Next(WBEM_INFINITE, 1L, &result, &retcnt);
			if (retcnt == 0) {
				if (result) result->Release(); result = nullptr;
				if (enumerator) enumerator->Release(); enumerator = nullptr;
				return "Unknown";
			}
			if (FAILED(hr)) {
				if (result) result->Release(); result = nullptr;
				if (enumerator) enumerator->Release(); enumerator = nullptr;
				throw Exception("Error obtaining WMI enumeration", HalleyExceptions::OS);
			}

			_variant_t var_val;
			hr = result->Get(parameter.getUTF16().c_str(), 0, &var_val, nullptr, nullptr);

			if (result) result->Release(); result = nullptr;
			if (enumerator) enumerator->Release(); enumerator = nullptr;

			if (FAILED(hr)) throw Exception("Error retrieving name from WMI query result", HalleyExceptions::OS);

			if (var_val.vt == VT_NULL) {
				return "";
			} else {
				return String(var_val.bstrVal);
			}
#else
			HRESULT hr;
			CComPtr<IEnumWbemClassObject> enumerator;
			BSTR lang = CComBSTR(L"WQL");
			BSTR q = CComBSTR(query.c_str());
			hr = pSvc->ExecQuery(lang, q, WBEM_FLAG_FORWARD_ONLY, nullptr, &enumerator);
			if (FAILED(hr)) throw Exception("Error running WMI query: "+getCOMError(hr), HalleyExceptions::OS);

			ULONG retcnt;
			CComPtr<IWbemClassObject> result;
			hr = enumerator->Next(WBEM_INFINITE, 1L, &result, &retcnt);
			if (retcnt == 0) return "Unknown";
			if (FAILED(hr)) throw Exception("Error obtaining WMI enumeration", HalleyExceptions::OS);

			_variant_t var_val;
			hr = result->Get(parameter.getUTF16().c_str(), 0, &var_val, nullptr, nullptr);
			if (FAILED(hr)) throw Exception("Error retrieving name from WMI query result", HalleyExceptions::OS);

			if (var_val.vt == VT_NULL) {
				return "";
			} else {
				return String(static_cast<const char*>(_bstr_t(var_val)));
			}
#endif
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
	PWSTR path;
	auto hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);
	if (SUCCEEDED(hr)) {
		auto result = String(path) + "\\";
		CoTaskMemFree(path);
		return result;
	}

	TCHAR path2[MAX_PATH];
	SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, path2);
	return String(path2) + "\\";
}

String OSWin32::getCurrentWorkingDir()
{
	DWORD bufferSize = GetCurrentDirectory(0, nullptr);
	std::string buffer(bufferSize - 1, 0);
	GetCurrentDirectory(bufferSize, &buffer[0]);
	return buffer;
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
				throw Exception("Unable to create directory: " + curPath + " (trying to make " + path + ")", HalleyExceptions::OS);
			}
		}
	}
}

static void writeFile(const wchar_t* str, const Bytes& data)
{
	//std::ofstream fp(str, std::ios::binary | std::ios::out);
	//fp.write(reinterpret_cast<const char*>(data.data()), data.size());
	//fp.close();

	for (int i = 3; --i >= 0;) {
		{
			auto file = CreateFileW(str, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE) {
				if (i > 0) {
					continue;
				}
				throw Exception("Unable to write file " + String(str), HalleyExceptions::File);
			}
			DWORD written = 0;
			WriteFile(file, data.data(), DWORD(data.size()), &written, nullptr);
			FlushFileBuffers(file);
			CloseHandle(file);
		}

		{
			// Read back to ensure it's OK
			auto file = CreateFileW(str, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE) {
				if (i > 0) {
					continue;
				}
				throw Exception("Unable to read file " + String(str), HalleyExceptions::File);
			}
			DWORD read = 0;
			DWORD highSize;
			DWORD size = GetFileSize(file, &highSize);
			Bytes readBack;
			readBack.resize(size_t(size));

			ReadFile(file, readBack.data(), size, &read, nullptr);
			CloseHandle(file);

			if (readBack != data) {
				if (i > 0) {
					continue;
				}
				throw Exception("Unable to correctly write file " + String(str), HalleyExceptions::File);
			}

			// All good
			return;
		}
	}
}

void OSWin32::atomicWriteFile(const Path& path, const Bytes& data, Maybe<Path> backupOldVersionPath)
{
	auto dstPath = path.getString().replaceAll("/", "\\").getUTF16();
	if (PathFileExistsW(dstPath.c_str())) {
		auto temp = path.replaceExtension(path.getExtension() + ".tmp");
		auto tempPath = temp.getString().replaceAll("/", "\\").getUTF16();
		auto backupPath = backupOldVersionPath ? backupOldVersionPath.get().getString().replaceAll("/", "\\").getUTF16() : StringUTF16();
		writeFile(tempPath.c_str(), data);

		const int result = ReplaceFileW(dstPath.c_str(), tempPath.c_str(), backupOldVersionPath ? backupPath.c_str() : nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
		if (result == 0) {
			Logger::logWarning("Unable to safely overwrite file " + path.getString());
			writeFile(dstPath.c_str(), data);
		}
	} else {
		writeFile(dstPath.c_str(), data);
	}
}

std::vector<Path> OSWin32::enumerateDirectory(const Path& rootPath)
{
	std::list<String> dirsToList;
	dirsToList.push_back(".");

	std::vector<Path> result;

	WIN32_FIND_DATAW ffd;
	while (!dirsToList.empty()) {
		const auto curDir = dirsToList.front();
		const auto curPath = (rootPath / curDir).getString();
		const auto pathStr = (curPath + "/*").replaceAll("/", "\\");
		dirsToList.pop_front();

		const auto handle = FindFirstFileW(pathStr.getUTF16().c_str(), &ffd);
		if (handle != INVALID_HANDLE_VALUE) {
			do {
				String curFile = String(ffd.cFileName);

				const DWORD attrib = GetFileAttributesW((rootPath / curDir / curFile).getString().getUTF16().c_str());
				if ((attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
					if (curFile != "." && curFile != "..") {
						dirsToList.emplace_back(curDir + "/" + curFile);
					}
				} else {
					auto res = (Path(curDir) / curFile).dropFront(1);
					result.emplace_back(res);
				}
			} while (FindNextFileW(handle, &ffd) != 0);
		}
	}

	return result;
}

void OSWin32::displayError(const std::string& cs)
{
	std::string error = "Halley has aborted with an unhandled exception: \n\n" + cs;
	int n = 0;
	const size_t sz = error.size();
	for (size_t i = 0; i < sz; ++i) {
		if (error[i] == '\n') {
			++n;
			if (n == 10) {
				error[i] = 0;
				break;
			}
		}
	}
	MessageBox(nullptr, error.c_str(), "Error", MB_ICONEXCLAMATION | MB_TASKMODAL);
}

static String readPipeToString(HANDLE pipe)
{
	DWORD bytesRead;
	DWORD bytesAvailable;

	char buffer[4096];
	String result;

	while (PeekNamedPipe(pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable) {
		ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
		buffer[bytesRead] = 0;
		result += buffer;
	}

	return result;
}

int OSWin32::runCommand(String rawCommand)
{
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(100ms);

	// Create the commandline
	auto command = rawCommand.getUTF16();
	if (command.length() >= 1024) {
		throw Exception("Command is too long!", HalleyExceptions::OS);
	}
	wchar_t buffer[1024];
	memcpy(buffer, command.c_str(), command.size() * sizeof(wchar_t));
	buffer[command.size()] = 0;


	// Create pipes for reading process output
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = nullptr;
	HANDLE inRead;
	HANDLE inWrite;
	HANDLE outRead;
	HANDLE outWrite;
	HANDLE errorRead;
	HANDLE errorWrite;
	if (!CreatePipe(&outRead, &outWrite, &saAttr, 0)) {
		throw Exception("Unable to create stdout pipe", HalleyExceptions::OS);
	}
	SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
	if (!CreatePipe(&errorRead, &errorWrite, &saAttr, 0)) {
		throw Exception("Unable to create stderr pipe", HalleyExceptions::OS);
	}
	SetHandleInformation(errorRead, HANDLE_FLAG_INHERIT, 0);
	if (!CreatePipe(&inRead, &inWrite, &saAttr, 0)) {
		throw Exception("Unable to create stdin pipe", HalleyExceptions::OS);
	}
	SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0);

	// Startup info
	STARTUPINFOW si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = outWrite;
	si.hStdError = errorWrite;
	si.hStdInput = inRead;
	si.dwFlags |= STARTF_USESTDHANDLES;

	// Create the process
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	if (!CreateProcessW(nullptr, buffer, nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
		return -1;
	}

	// Wait for process to end
	CloseHandle(inWrite);
	HANDLE waitHandles[] = { pi.hProcess, outRead, errorRead };
	WaitForMultipleObjects(3, waitHandles, TRUE, INFINITE);

	// Read output
	{
		String out = readPipeToString(outRead);
		if (!out.isEmpty()) {
			Logger::logInfo(out);
		}
		String error = readPipeToString(errorRead);
		if (!error.isEmpty()) {
			Logger::logError(error);
		}
	}

	// Get exit code
	DWORD exitCode = -2;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	// Cleanup
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(inRead);
	CloseHandle(outRead);
	CloseHandle(outWrite);
	CloseHandle(errorRead);
	CloseHandle(errorWrite);

	return int(exitCode);
}

class Win32Clipboard : public IClipboard {
public:
	void setData(const String& stringData) override
	{
		if (!OpenClipboard(nullptr)) {
			throw Exception("Unable to open clipboard.", HalleyExceptions::OS);
		}
		
		if (!EmptyClipboard()) {
			CloseClipboard();
			throw Exception("Unable to empty clipboard", HalleyExceptions::OS);
		}
		auto utf16str = stringData.getUTF16();
		auto data = GlobalAlloc(GMEM_MOVEABLE, (utf16str.length() + 1) * sizeof(wchar_t));
		auto dst = GlobalLock(data);
		memcpy(dst, utf16str.data(), (utf16str.size() + 1) * sizeof(wchar_t));
		GlobalUnlock(data);

		if (!SetClipboardData(CF_UNICODETEXT, data)) {
			CloseClipboard();
			throw Exception("Unable to set clipboard data", HalleyExceptions::OS);
		}

		CloseClipboard();
	}

	Maybe<String> getStringData() override
	{
		if (!OpenClipboard(nullptr)) {
			throw Exception("Unable to open clipboard.", HalleyExceptions::OS);
		}

		if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			CloseClipboard();
			return {};
		}

		HANDLE data = GetClipboardData(CF_UNICODETEXT);
		if (data == nullptr) {
			CloseClipboard();
			throw Exception("Unable to read clipboard data.", HalleyExceptions::OS);
		}

		auto src = reinterpret_cast<const wchar_t*>(GlobalLock(data));
		auto result = String(src);
		GlobalUnlock(data);

		CloseClipboard();

		return result;
	}
};

std::shared_ptr<IClipboard> OSWin32::getClipboard()
{
	try {
		return std::make_shared<Win32Clipboard>();
	} catch (...) {
		return {};
	}
}

void OSWin32::openURL(const String& url)
{
	if (url.startsWith("http://") || url.startsWith("https://")) {
		ShellExecute(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}
}

#endif
