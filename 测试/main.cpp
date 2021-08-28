#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>
#include <atomic> 
#include <io.h>
#include <time.h>

#pragma comment(lib, "ntdll.dll")

struct Jiyu {
	DWORD id;
	char filepath[260];
	BOOL flag;
} jiyu; 

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
	for (int i = 0; i < int(nSize); i += 1) {
		printf("%x ", retKeyVal[i]);
		if (i % 8 == 0) puts("");
	}
	int sum = 0;
	for (int i = 0; i < int(nSize); i += 1) {
		if (retKeyVal[i + 1] == 0) {
			*(str + sum) = retKeyVal[i];
			sum++;
			if (retKeyVal[i] == 0) break;
		}
	}
	return TRUE;
}

/*
函数功能:挂起进程中的所有线程
参数1:进程ID
参数2:若为TRUE时对进程中的所有线程调用SuspendThread,挂起线程
      若为FALSE时对进程中的所有线程调用ResumeThread,恢复线程
*/

VOID SuspendProcess(DWORD dwProcessID, BOOL fSuspend) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(
	                       TH32CS_SNAPTHREAD, dwProcessID);

	if (hSnapshot != INVALID_HANDLE_VALUE) {

		THREADENTRY32 te = {sizeof(te)};
		BOOL fOk = Thread32First(hSnapshot, &te);
		for (; fOk; fOk = Thread32Next(hSnapshot, &te)) {
			if (te.th32OwnerProcessID == dwProcessID) {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);

				if (hThread != NULL) {
					if (fSuspend) {
						SuspendThread(hThread);
					} else {
						ResumeThread(hThread);
					}
				}
				CloseHandle(hThread);
			}
		}
		CloseHandle(hSnapshot);
	}
}

BOOL CALLBACK EnumChildWindowsProc(HWND hwndChild, LPARAM lParam) {
	HMENU hmenu = GetMenu(hwndChild);
	printf("%u\n", LOWORD(hmenu));
	if (LOWORD(hmenu) != 0) {
		EnableWindow(hwndChild, FALSE);
		getchar();
		EnableWindow(hwndChild, TRUE);
	}
	return TRUE;
}

int main() {
//	char str[MAX_PATH * 10];
//	UINT ret = GetMythwarePasswordFromRegedit(str);
//	if (!ret) {
//		printf("ERROR");
//	}
//	else {
//		printf("%s", str);
//	}
//	getchar();




//	Jiyu tj = GetProcessPidFromFilename("StudentMain.exe");
//	HWND Mythware = FindWindow(NULL, "StudentMain.exe"); 
//	//GetWindowThreadProcessId(Mythware, &tj.id);
//	DWORD_PTR dwResult = 0;
//	LRESULT res = SendMessageTimeout(Mythware, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 500, &dwResult); 
//	if (res) {
//		printf("Hung");
//	}
//	else {
//		printf("not Hung");
//	}

	HWND Mythware = FindWindow(NULL, "StudentMain.exe"); 
	EnumChildWindows(Mythware, EnumChildWindowsProc, (LPARAM)0);
	getchar();
	return 0;
}
