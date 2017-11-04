// CRCleaner.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "CRCleaner.h"
#include <tchar.h>
#include <process.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <string>
#include <ShlObj.h>

#define MAX_LOADSTRING 100

//Thanks, Stack Overflow!

BOOL TerminateProcess(DWORD dwProcessId, UINT uExitCode)
{
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	BOOL  bInheritHandle = FALSE;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
	if (hProcess == NULL)
		return FALSE;
	BOOL result = TerminateProcess(hProcess, uExitCode);
	CloseHandle(hProcess);
	return result;
}

BOOL IsElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}

// Global Variables:
HINSTANCE hInst;                                // current instance

// Forward declarations of functions included in this code module:
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	if (!IsElevated()) {
		MessageBoxA(NULL, "Please run this as administrator!", "Oh no!", MB_OK);
		return BOOL(FALSE);
	}
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		MessageBoxA(NULL, "Failed to grab process snap!", "Oh no!", MB_OK);
		return(FALSE);
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		MessageBoxA(NULL, "Failed to grab good process snap!", "Oh no!", MB_OK);
		CloseHandle(hProcessSnap);  // clean the snapshot object
		return(FALSE);
	}

	// Now walk the snapshot of processes 
	do
	{
		std::wstring str(pe32.szExeFile);
		std::string pname(str.begin(), str.end());
		if (pname == "svchost.exe") // put the name of your process you want to kill
		{
			HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ParentProcessID);
			if (h) {
				TCHAR name[MAX_PATH];
				GetProcessImageFileName(h, name, MAX_PATH);
				std::wstring parentwide(name);
				std::string parent(parentwide.begin(), parentwide.end());
				if (parent.find("services.exe") == std::string::npos) {
					//Blow it away!
					TerminateProcess(pe32.th32ProcessID, 1);
				}
				CloseHandle(h);
			}
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	WCHAR path[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path);
	std::wstring pathw(path);
	std::string filename(pathw.begin(), pathw.end());
	filename += "\\AppData\\Roaming\\svchost.exe";
	std::wstring w(filename.begin(), filename.end());
	DeleteFile(w.c_str());
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0L, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
		RegDeleteValueW(hkey, _T("Java"));
		RegCloseKey(hkey);
	}
	else {
		int e = GetLastError();
		MessageBoxA(NULL, std::to_string(e).c_str(), "aa", MB_OK);
	}
	MessageBoxA(NULL, "You should probably reboot.", "Oh yes!", MB_OK);
	return(TRUE);

}
