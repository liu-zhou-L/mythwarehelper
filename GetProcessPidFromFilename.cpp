#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <psapi.h>

LPCSTR MythwareFilename = "StudentMain.exe";

BOOL ModuleIsAble(DWORD ProcessPid, LPCSTR Modulename) {
	if (Modulename[0] == '\0') return FALSE;
	MODULEENTRY32 me;
	me.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessPid);
	if (Module32First(hSnapshot, &me)) {
		do {
			printf("%s\n", me.szModule);
			if (!strcmp(Modulename, me.szModule)) {
				CloseHandle(hSnapshot);
				return TRUE;
			}
		} while (Module32Next(hSnapshot, &me));
	}
	CloseHandle(hSnapshot);
	return FALSE;
}

DWORD GetProcessPidFromFilename(LPCSTR Filename) {
	if (Filename[0] == '\0') return 0;
	DWORD IdMainThread = NULL;
	PROCESSENTRY32 te;
	te.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (Process32First(hSnapshot, &te)) {
		do {
			CHAR tempfilename[MAX_PATH];
			HANDLE temphandle = OpenProcess(PROCESS_ALL_ACCESS, false, te.th32ProcessID);
			printf("%d\n", te.th32ProcessID);
			if (ModuleIsAble(te.th32ProcessID, Filename)) {
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
	printf("%d", pid); 
	getchar(); 
	return 0;
} 
