#include <iostream>
#include <windows.h>

//L"LoggerDll.dll"
//L"AppToInject.exe"

//arg list: argv[0] - injector executable path, argv[1] - process path, argv[2] - dll path
//dll must have function called Injection
int wmain(int argc, wchar_t* argv[]) {
	const wchar_t* errNoArgs = L"Error: Application and dll paths were not specified";
	const wchar_t* errNoDllPath = L"Error: Dll path was not specified";
	const wchar_t* errRoutineName = L"Error: Routine name was not specified";
	const wchar_t* errWrongFilePath = L"Error: Failed to run application with specified path";
	const wchar_t* errMemAlloc = L"Error: Failed to allocate memory inside target process";
	const wchar_t* errMemWrite = L"Error: Failed to write to memory of target process";
	const wchar_t* errKrnlNotFound = L"Error: kernel32 not found";
	const wchar_t* errLdLibNotFound = L"Error: LoadLibraryW not found";
	const wchar_t* errRemThread = L"Error: Cannot create remote thread in target process";

	const wchar_t* warTooManyArgs = L"Warning: Too many arguments were passed, processing only first and second argument";
	const wchar_t* warNoInjection = L"Warning: Dll has no Injection function";
	const wchar_t* warNoComms = L"Warning: Communication channel with dll was not created";

	const wchar_t* msgProcessStartSuccess = L"Message: Suspended process was created successfully!";
	const wchar_t* msgThreadStartWait = L"Message: Waiting for thread to start";
	const wchar_t* msgInjectionDone = L"Injection successfull";
	const wchar_t* msgThreadTerm = L"Target thread was terminated";

	STARTUPINFO startupInfo = { 0 };
	PROCESS_INFORMATION processInformation = { 0 };

	if (argc == 1) {
		std::wcout << errNoArgs << std::endl;
	}
	else if (argc == 2) {
		std::wcout << errNoDllPath << std::endl;
	}
	else if (argc == 2) {
		std::wcout << errNoDllPath << std::endl;
	}
	else if (argc > 3) {
		std::wcout << warTooManyArgs << std::endl;
	}
	else {
		LPVOID lpInjection = NULL;

		HMODULE hLib = LoadLibrary(argv[2]);
		if (hLib) {
			lpInjection = GetProcAddress(hLib, "Injection");
			FreeLibrary(hLib);
		}
		if (lpInjection) {
			wchar_t buffer[1024];
			HANDLE hPipe = CreateNamedPipe(L"\\\\.\\pipe\\InjectorPipe", PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				1, 1024 * 16, 1024 * 16, NMPWAIT_USE_DEFAULT_WAIT, NULL);
			if (hPipe) {
				if (CreateProcess(argv[1], NULL, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &startupInfo, &processInformation)) {


					std::wcout << msgProcessStartSuccess << std::endl;

					size_t dllPathLength = wcslen(argv[2]) * sizeof(wchar_t);
					LPVOID lpDllPath = VirtualAllocEx(processInformation.hProcess, NULL, dllPathLength + 1, MEM_COMMIT, PAGE_READWRITE);
					if (lpDllPath) {
						if (WriteProcessMemory(processInformation.hProcess, lpDllPath, argv[2], dllPathLength, NULL)) {
							HMODULE hModule = GetModuleHandle(L"KERNEL32.DLL");
							if (hModule) {
								LPVOID lpLoadLibraryW = GetProcAddress(hModule, "LoadLibraryW");
								if (lpLoadLibraryW) {
									HANDLE hThread = CreateRemoteThread(processInformation.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)lpLoadLibraryW, lpDllPath, NULL, NULL);
									if (hThread) {
										std::wcout << msgThreadStartWait << std::endl;

										WaitForSingleObject(hThread, INFINITE);
										CloseHandle(hThread);

										hThread = CreateRemoteThread(processInformation.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)lpInjection, NULL, NULL, NULL);

										if (hThread) {
											std::wcout << msgInjectionDone << std::endl;

											ResumeThread(processInformation.hThread);

											DWORD dwRead;
											while (1) {

												int ret = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL);
												if (ret) {
													std::wcout << buffer << std::endl;
												}
												else if (ret == 0 && GetLastError() == ERROR_BROKEN_PIPE) {
													break;
												}
											}
											std::wcout << msgThreadTerm << std::endl;
											CloseHandle(hThread);
										}
									}
									else {
										std::wcout << errRemThread << std::endl;
									}
								}
								else {
									std::wcout << errLdLibNotFound << std::endl;
								}
							}
							else {
								std::wcout << errKrnlNotFound << std::endl;
							}

						}
						else {
							std::wcout << errMemWrite << std::endl;
						}
						VirtualFreeEx(processInformation.hProcess, lpDllPath, 0, MEM_RELEASE);
					}
					else {
						std::wcout << errMemAlloc << std::endl;
					}
					CloseHandle(processInformation.hThread);
					CloseHandle(processInformation.hProcess);
				}
				else {
					std::wcout << errWrongFilePath << std::endl;
				}
			}
			else {
				std::wcout << warNoComms << std::endl;
			}
		}
		else {
			std::wcout << warNoInjection << std::endl;
		}
	}
	system("pause");
}