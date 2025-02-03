#include "impersonate.h"
#include <windows.h>
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <cstring>
#include <lmaccess.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <lm.h>
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "Netapi32.lib")


DWORD GetExplorerPidByUser(const std::wstring& targetUsername) {
    DWORD explorerPid = 0;
    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to create process snapshot!" << std::endl;
        return 0;
    }

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, L"notepad.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                std::wcout << L"Found Process" << std::endl;
                if (hProcess) {
                    HANDLE hToken;
                    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
                        std::wcout << L"Could open token!" << std::endl;
                        DWORD length = 0;
                        GetTokenInformation(hToken, TokenUser, NULL, 0, &length);
                        if (length > 0) {
                            std::wcout << L"Token Valid!" << std::endl;
                            PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(length);
                            if (pTokenUser) {
                                if (GetTokenInformation(hToken, TokenUser, pTokenUser, length, &length)) {
                                    std::wcout << L"Got Token Info" << std::endl;
                                    WCHAR userName[256], domainName[256];
                                    DWORD userSize = 256, domainSize = 256;
                                    SID_NAME_USE sidType;
                                    if (LookupAccountSidW(NULL, pTokenUser->User.Sid, userName, &userSize, domainName, &domainSize, &sidType)) {
                                        std::wstring fullUsername = std::wstring(domainName) + L"\\" + std::wstring(userName);
                                        std::wcout << L"Got User of the Process:" << fullUsername << std::endl;
                                        if (_wcsicmp(userName, targetUsername.c_str()) == 0) {
                                            std::wcout << L"Username is target!" << std::endl;
                                            explorerPid = pe32.th32ProcessID;
                                            free(pTokenUser);
                                            CloseHandle(hToken);
                                            CloseHandle(hProcess);
                                            break;
                                        }
                                    }
                                }
                                free(pTokenUser);
                            }
                        }
                        CloseHandle(hToken);
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return explorerPid;
}

int ImpersonateUser(const std::wstring& targetUsername) {
    DWORD pid = GetExplorerPidByUser(targetUsername);
		STARTUPINFOW si = { 0 }; // Initialize all fields to zero
		si.cb = sizeof(STARTUPINFOW); // Set the size field explicitly
		PROCESS_INFORMATION pi;
		wchar_t desktop[] = L"winsta0\\default";
		si.lpDesktop = desktop;

		if (pid) {
			std::wcout << L"Process ID of " << targetUsername << L": " << pid << std::endl;
		}
		else {
			std::wcout << L"Process not found." << std::endl;
		}

		// Impresonate Token

		HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		HANDLE process_token;


		if (OpenProcessToken(process_handle, MAXIMUM_ALLOWED, &process_token)) {
			std::wcout << L"Got access to the token" << std::endl;
		}
		else
		{
			std::wcout << L"Could not access Token!" << std::endl;
		}
		HANDLE duplicate_token;


		if (DuplicateTokenEx(process_token, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &duplicate_token)) {

			std::wcout << L"Successfuly duplicated the token!" << std::endl;

		}
		else
		{
			std::wcout << L"Could not duplicate the token!" << std::endl;
		}

		if (ImpersonateLoggedOnUser(duplicate_token)) {

			std::wcout << L"Successfuly impersonated the token!" << std::endl;

		}
		else
		{
			std::wcout << L"Could not impersonate the token!" << std::endl;
		}

		wchar_t cmdLine[] = L"/c whoami > c:\\ProgramData\\whoami.txt";

		if (CreateProcessAsUser(duplicate_token, L"c:\\Windows\\System32\\cmd.exe", cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
			std::wcout << L"Created Process with duplicated token!" << std::endl;

		}
		else
		{
			DWORD dwError = GetLastError();
			std::wcout << L"Failed to create process! Error Code: " << dwError << std::endl;
		}
		return 0;

	}

BOOL CheckPermissions(const wchar_t* username) {
	LPBYTE groups = NULL;
	DWORD groupcount = 0, totalentries = 0;
	LPCWSTR search = L"DAdmin";  // Search for this local group

	// Use NetUserLocalGroups to get local group memberships
	NET_API_STATUS status = NetUserGetLocalGroups(
		NULL,          // Local machine
		username,      // Target username
		0,             // Level 0 (returns group names)
		LG_INCLUDE_INDIRECT,  // Include both direct and indirect memberships
		&groups,       // Output buffer
		MAX_PREFERRED_LENGTH,
		&groupcount,
		&totalentries
	);

	if (status != NERR_Success) {
		std::wcerr << L"NetUserLocalGroups failed with error: " << status << std::endl;
		return FALSE;
	}

	// Cast the buffer to an array of LOCALGROUP_USERS_INFO_0 structures
	PLOCALGROUP_USERS_INFO_0 pGroupInfo = (PLOCALGROUP_USERS_INFO_0)groups;

	BOOL found = FALSE;
	for (DWORD i = 0; i < groupcount; i++) {
		if (wcsstr(pGroupInfo[i].lgrui0_name, search)) {  // Use wide string search
			std::wcout << L"User is in group: " << pGroupInfo[i].lgrui0_name << std::endl;
			found = TRUE;
			break;
		}
	}

	// Free allocated memory
	NetApiBufferFree(groups);

	return found;
}


