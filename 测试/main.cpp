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

UINT GetMythwarePathFromRegedit(char *str) {
	HKEY retKey;
	char tstr[200] = "SOFTWARE\\WOW6432Node\\TopDomain\\e-Learning Class Standard\\1.00";
	DWORD dwDisposition = REG_OPENED_EXISTING_KEY;
	LONG ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, tstr, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &retKey, &dwDisposition);
	if (ret != ERROR_SUCCESS) {
		return 1;
	}
	BYTE tByte[MAX_PATH * 2 + 1]; 
	DWORD nSize;
	int sum = 0;
	ret = RegQueryValueEx(retKey, "TargetDirectory", NULL, NULL, tByte, &nSize);
	if (ret == ERROR_SUCCESS) {
		for (int i = 0; i < int(nSize); i += 1) {
			*(str + sum) = tByte[i];
			if (*(str + sum++) == '\\') {
				*(str + sum++) = '\\';
			}		
		}
		printf("%d\n", sum); 
		return 2;
	}
	return 3;
}

BOOL GetMythwarePasswordFromRegedit(char *str) {
	HKEY retKey;
	BYTE retKeyVal[MAX_PATH * 10] = { 0 };
	DWORD nSize = MAX_PATH * 10;
	LONG ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\TopDomain\\e-Learning Class\\Student", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &retKey);
	if (ret != ERROR_SUCCESS) {
		return FALSE;
	}
	ret = RegQueryValueExA(retKey, "knock1", NULL, NULL, (LPBYTE)retKeyVal, &nSize);
	RegCloseKey(retKey);
	if (ret != ERROR_SUCCESS) {
		return FALSE;
	}
	for (int i = 0; i < int(nSize); i += 4) {
		retKeyVal[i + 0] = (retKeyVal[i + 0] ^ 0x50 ^ 0x45);
		retKeyVal[i + 1] = (retKeyVal[i + 1] ^ 0x43 ^ 0x4c);
		retKeyVal[i + 2] = (retKeyVal[i + 2] ^ 0x4c ^ 0x43);
		retKeyVal[i + 3] = (retKeyVal[i + 3] ^ 0x45 ^ 0x50);
	}
	for (int i = 0; i < int(nSize); i += 2) {
		printf("%x ", retKeyVal[i]);
		if (i % 16 == 0) puts("");
	}
	int sum = 0;
	for (int i = 0; i < int(nSize); i += 1) {
		if (retKeyVal[i + 1] == 0) {
			*(str + sum) = retKeyVal[i];
			sum++;
			if (retKeyVal[i + 2] == 0 && retKeyVal[i + 3] == 0) break;
		}
	}
	return TRUE;
}

int main() {
	char str[MAX_PATH * 10];
	UINT ret = GetMythwarePasswordFromRegedit(str);
	if (!ret) {
		printf("ERROR");
	}
	else {
		printf("%s", str);
	}
	getchar();
	return 0;
}
