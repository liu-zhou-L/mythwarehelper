#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <psapi.h>

LPCWSTR MythwareFilename = L"C:\\Users\\NING MEI\\Desktop\\GetProcessPidFromFilename.exe";

BOOL ModuleIsAble(HANDLE Process, LPCWSTR Modulename) {
	if (Modulename[0] == '\0') return FALSE;
	MODULEENTRY32 me;
	me.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if (Module32First(hSnapshot, &me)) {
		do {
			printf("%s\n", me.szModule);
			DWORD lenMultiByteToWideChar(CP_ACP, me.szModule)
			if (!wcscmp(Modulename, )) {
				CloseHandle(hSnapshot);
				return TRUE;
			}
		} while (Module32Next(hSnapshot, &me));
	}
	CloseHandle(hSnapshot);
	return FALSE;
}

DWORD GetProcessPidFromFilename(LPCWSTR Filename) {
	if (Filename[0] == '\0') return 0;
	DWORD IdMainThread = NULL;
	PROCESSENTRY32 te;
	te.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (Process32First(hSnapshot, &te)) {
		do {
			WCHAR tempfilename[MAX_PATH];
			HANDLE temphandle = OpenProcess(PROCESS_ALL_ACCESS, false, te.th32ProcessID);
			if (ModuleIsAble(temphandle, Filename)) {
				IdMainThread = te.th32ProcessID;
				break;
			}
			CloseHandle(temphandle);
		} while (Process32Next(hSnapshot, &te));
	}
	CloseHandle(hSnapshot);
	return IdMainThread;
} 

int main() {
	DWORD pid = GetProcessPidFromFilename(MythwareFilename);
	printf("%u", pid); 
	getchar(); 
	return 0;
} 
