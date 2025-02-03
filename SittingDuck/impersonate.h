#ifndef Impersonate_H
#define Impersonate_H

#include <windows.h>
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <cstring>
#include <lmaccess.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <lm.h>

BOOL CheckPermissions(const wchar_t* username);
int ImpersonateUser(const std::wstring& targetUsername);


#endif