#include <windows.h>
#include <iostream>
#include <detours.h>
#include <string> 

HANDLE hPipe;

HMODULE hModule;
wchar_t moduleName[255];

bool writeFileHooked = false;

bool writeFileMsg = false;

void WriteToPipe(const wchar_t* data) {	
	DWORD dwWritten;
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		WriteFile(hPipe, data, (wcslen(data) + 1) * sizeof(wchar_t) , &dwWritten, NULL);
		FlushFileBuffers(hPipe);
	}
}


HANDLE(WINAPI* Real_CreateFile) (LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
								 DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = CreateFile;

HANDLE WINAPI Hook_CreateFile(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
							  DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {

	WriteToPipe(L"CreateFile was called");

	return Real_CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HFILE(WINAPI* Real_OpenFile)(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle) = OpenFile;

HFILE WINAPI Hook_OpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle) {

	WriteToPipe(L"OpenFile was called");

	return Real_OpenFile(lpFileName, lpReOpenBuff, uStyle);
}

BOOL(WINAPI* Real_ReadFile)(HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) = ReadFile;

BOOL Hook_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
	
	WriteToPipe(L"ReadFile was called");

	return Real_ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
}



BOOL(WINAPI* Real_WriteFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) = WriteFile;

BOOL Hook_WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
	if (!writeFileMsg) {
		writeFileMsg = true;
		WriteToPipe(L"WriteFile was called");
	}
	writeFileMsg = false;

	return Real_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
	
}

BOOL(WINAPI* Real_MessageBox)(HWND    hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT    uType) = MessageBox;

BOOL Hook_MessageBox(HWND    hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT    uType) {

	WriteToPipe(L"MessageBox was called");

	return Real_MessageBox(hWnd, lpText, lpCaption, uType);

}

LSTATUS(WINAPI* Real_RegGetValue) (HKEY    hkey, LPCSTR  lpSubKey, LPCSTR  lpValue, DWORD   dwFlags, LPDWORD pdwType, PVOID   pvData, LPDWORD pcbData) = RegGetValueA;

LSTATUS Hook_RegGetValue (HKEY    hkey, LPCSTR  lpSubKey, LPCSTR  lpValue, DWORD   dwFlags, LPDWORD pdwType, PVOID   pvData, LPDWORD pcbData) {
	WriteToPipe(L"RegGetValue was called");

	return Real_RegGetValue(hkey, lpSubKey, lpValue, dwFlags, pdwType, pvData, pcbData);
}

LSTATUS(WINAPI* Real_RegSetValue) (HKEY    hKey, LPCSTR  lpSubKey, DWORD   dwType, LPCSTR lpData, DWORD   cbData) = RegSetValueA;

LSTATUS Hook_RegSetValue(HKEY    hKey, LPCSTR  lpSubKey, DWORD   dwType, LPCSTR lpData, DWORD   cbData) {
	WriteToPipe(L"RegSetValue was called");

	return Real_RegSetValue(hKey, lpSubKey, dwType, lpData, cbData);
}

LSTATUS(WINAPI* Real_RegOpenKey)(HKEY   hKey, LPCWSTR lpSubKey, PHKEY  phkResult) = RegOpenKey;

LSTATUS Hook_RegOpenKey(HKEY   hKey, LPCWSTR lpSubKey, PHKEY  phkResult) {
	WriteToPipe(L"RegOpenKey was called");

	return Real_RegOpenKey(hKey, lpSubKey, phkResult);
}

LSTATUS(WINAPI* Real_RegCloseKey)(HKEY   hKey) = RegCloseKey;

LSTATUS Hook_RegCloseKey(HKEY   hKey) {
	WriteToPipe(L"RegCloseKey was called");

	return Real_RegCloseKey(hKey);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)Real_CreateFile, Hook_CreateFile);
		DetourAttach(&(PVOID&)Real_OpenFile, Hook_OpenFile);
		DetourAttach(&(PVOID&)Real_ReadFile, Hook_ReadFile);
		DetourAttach(&(PVOID&)Real_WriteFile, Hook_WriteFile);
		DetourAttach(&(PVOID&)Real_MessageBox, Hook_MessageBox);
		DetourAttach(&(PVOID&)Real_RegGetValue, Hook_RegGetValue);
		DetourAttach(&(PVOID&)Real_RegSetValue, Hook_RegSetValue);
		DetourAttach(&(PVOID&)Real_RegOpenKey, Hook_RegOpenKey);
		DetourAttach(&(PVOID&)Real_RegCloseKey, Hook_RegCloseKey);

		DetourTransactionCommit();
		hModule = hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)Real_CreateFile, Hook_CreateFile);
		DetourDetach(&(PVOID&)Real_OpenFile, Hook_OpenFile);
		DetourDetach(&(PVOID&)Real_ReadFile, Hook_ReadFile);
		DetourDetach(&(PVOID&)Real_WriteFile, Hook_WriteFile);
		DetourDetach(&(PVOID&)Real_MessageBox, Hook_MessageBox);
		DetourDetach(&(PVOID&)Real_RegGetValue, Hook_RegGetValue);
		DetourDetach(&(PVOID&)Real_RegSetValue, Hook_RegSetValue);
		DetourDetach(&(PVOID&)Real_RegOpenKey, Hook_RegOpenKey);
		DetourDetach(&(PVOID&)Real_RegCloseKey, Hook_RegCloseKey);
		DetourTransactionCommit();
		CloseHandle(hPipe);
		
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void Injection() {	
	
	hPipe = CreateFile(L"\\\\.\\pipe\\InjectorPipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	GetModuleFileName(hModule, moduleName, 255);
	
	WriteToPipe(std::wstring(L"LoggerDll attached to " + std::wstring(moduleName) + L"\n").c_str());

}