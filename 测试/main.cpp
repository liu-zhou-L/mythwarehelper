#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>
#include <atomic> 
#include <io.h>
#include <time.h>

struct Jiyu {
	DWORD id;
	char filepath[260];
	BOOL flag;
}; 

Jiyu ModuleIsAble(DWORD ProcessPid, LPCSTR Modulename) {
	Jiyu tj;
	if (Modulename[0] == '\0') {
		tj.flag = FALSE;
		return tj;
	}
	MODULEENTRY32 me;
	me.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessPid);
	if (Module32First(hSnapshot, &me)) {
		do {
			//printf("%s\n", me.szModule);
			if (!strcmp(Modulename, me.szModule)) {
				CloseHandle(hSnapshot);
				strcpy(tj.filepath, me.szExePath);
				tj.id = ProcessPid;
				tj.flag = TRUE;
				return tj;
			}
		} while (Module32Next(hSnapshot, &me));
	}
	CloseHandle(hSnapshot);
	tj.flag = FALSE;
	return tj;
}

Jiyu GetProcessPidFromFilename(LPCSTR Filename) {
	Jiyu tj;
	if (Filename[0] == '\0') {
		tj.flag = FALSE;
		return tj;
	}
	PROCESSENTRY32 te;
	te.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (Process32First(hSnapshot, &te)) {
		do {
			HANDLE temphandle = OpenProcess(PROCESS_ALL_ACCESS, false, te.th32ProcessID);
			//printf("%ld\n", te.th32ProcessID);
			tj = ModuleIsAble(te.th32ProcessID, Filename);
			if (tj.flag == TRUE) {
				CloseHandle(hSnapshot);
				return tj;
			}
			CloseHandle(temphandle);
		} while (Process32Next(hSnapshot, &te));
	}
	tj.flag = FALSE;
	return tj;
} 

BOOL SetRegedit(char *str, char *ValueName, char *Value) {
	HKEY retKey;
	char tstr[200] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
	strcat(tstr, str);
	LONG ret = RegCreateKey(HKEY_LOCAL_MACHINE, tstr, &retKey);
	if (ret != ERROR_SUCCESS) {
		return FALSE;
	}
	ret = RegSetValueEx(retKey,             // sub key handle 
	            ValueName,       // value name 
	            0,                        // must be zero 
	            REG_EXPAND_SZ,            // value type 
	            (LPBYTE)Value,           // pointer to value data 
	            strlen(Value) + 1);       // length of value
	if (ret != ERROR_SUCCESS) {
	   	return FALSE;
	}         
	RegCloseKey(retKey);
	return TRUE;
}

int main() {
	puts(SetRegedit("MythwareHelper", "agree", "yes") ? "succes" : "fail");
	getchar();
	return 0;
}
