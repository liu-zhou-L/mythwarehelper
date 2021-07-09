#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>
#include <atomic>
#include <io.h>
#include <time.h>
#include <versionhelpers.h>
#include <shellscalingapi.h>
#include "ziyuan.h"

#define ThreadSafe __declspec(thread)

/*

from liu_zhou

*/

//https://blog.csdn.net/yanglx2022/article/details/46582629
//https://www.52pojie.cn/thread-799791-1-1.html
//https://blog.csdn.net/Koevas/article/details/84679206?ops_request_misc=%25257B%252522request%25255Fid%252522%25253A%252522161008071416780264661008%252522%25252C%252522scm%252522%25253A%25252220140713.130102334..%252522%25257D&request_id=161008071416780264661008&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_click~default-1-84679206.first_rank_v2_pc_rank_v29&utm_term=%E6%9E%81%E5%9F%9F
//https://blog.csdn.net/u012314571/article/details/89811045?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control
//https://blog.csdn.net/m0_46544479/article/details/105195666	文件操作
//下面的回去关注
//https://blog.csdn.net/powerful_green/article/details/85037018?utm_medium=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase&depth_1-utm_source=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase
//https://blog.csdn.net/wangjieest/article/details/7065457

const int SUSPEND_BUTTON = 3301;
const int REBOOT_BUTTON = 3302;
const int KILL_BUTTON = 3303;
const int SETCANNOTSHUTDOWN_BUTTON = 3304;
const int JC_BUTTON = 3305;
const int PASSWD_BUTTON = 3306;
const int KEYBORADHOOK_BUTTON = 3307;
const int SETFIRST_BUTTON = 3308;
const int SETTOUM_BUTTON = 3309;
const int MOVESHUTDOWN_BUTTON = 3310;
const int YINC_BUTTON = 3311;
const int GETREGEDITPASSWD_BUTTON = 3312;
const int ID_ACCR_SHOW = 1001;
const int ID_ACCR_HIDE = 1002;
const int WM_MSG =  RegisterWindowMessage("postmessage");
CRITICAL_SECTION CSPID;
LPCWSTR BroadcastTitle = L"屏幕广播";
LPCSTR MythwareFilename = "StudentMain.exe";
LPCWSTR MythwareTitle = L"StudentMain.exe";
LPCSTR TISHIYU = "请您认真阅读并充分理解以下内容\n本软件仅以学习交流为目的，请勿用于任何非法用途.本软件的所有功能均只可以用于课堂辅助，禁止用于干坏事.\n事先声明，因使用本软件造成的一切后果与作者无关，由直接使用者承担责任，如做不到请自行删除本软件.\n一旦您选择是即表示您已经阅读并且同意与此工具开发者达成上述协议";

char TempWorkPath[MAX_PATH + 1];

HANDLE ClassHandle = NULL, MythwareHandle = NULL, threadisstart = NULL, threadKeyboradHook = NULL, threadSetWindowName = NULL;
HWND mythwaretext = NULL, guangbotext = NULL, mythwareversiontext = NULL;
HWND SuspendB = NULL, RebootB = NULL, KillB = NULL, JcB = NULL, PassWdB = NULL, KeyboradHookB = NULL, SetFirstB = NULL, MoveShutdownB = NULL, SetCannotShutdownB = NULL, YinCB = NULL, GetRegeditPassWdB = NULL;
HWND Class = NULL, Mythware = NULL;
DWORD pid;
NOTIFYICONDATA m_nid;

DWORD GetMainThreadFromId(const DWORD IdProcess) {
	if (IdProcess <= 0) return 0;
	DWORD IdMainThread = 0;
	THREADENTRY32 te;
	te.dwSize = sizeof(THREADENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (Thread32First(hSnapshot, &te)) {
		do {
			if (IdProcess == te.th32OwnerProcessID) {
				IdMainThread = te.th32ThreadID;
				break;
			}
		} while (Thread32Next(hSnapshot, &te));
	}
	CloseHandle(hSnapshot);
	return IdMainThread;
}

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
			//printf("thread - %s\n", me.szModule);
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

BOOL IsRegeditHasSet(char *str, char *ValueName) {
	HKEY retKey;
	char tstr[200] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
	strcat(tstr, str);
	DWORD dwDisposition = REG_OPENED_EXISTING_KEY;
	LONG ret = RegCreateKeyEx(HKEY_LOCAL_MACHINE, tstr, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &retKey, &dwDisposition);
	if (ret != ERROR_SUCCESS) {
		return FALSE;
	}
	ret = RegQueryValueEx(retKey, ValueName, NULL, NULL, NULL, NULL);
	if (ret == ERROR_SUCCESS) {
		return TRUE;
	}
	return FALSE;
}

bool SetupTrayIcon(HWND m_hWnd) {
	m_nid.cbSize = sizeof(NOTIFYICONDATA); // 结构大小
	m_nid.hWnd = m_hWnd; // 接收 托盘通知消息 的窗口句柄
	m_nid.uID = 1231;
	m_nid.uFlags = NIF_MESSAGE; //表示uCallbackMessage 有效
	m_nid.uCallbackMessage = WM_MSG; // 消息被发送到此窗口过程
	m_nid.hIcon = ::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(1231));
	strcpy(m_nid.szTip, "我的任务栏程序");             // 提示文本
	return 0 != Shell_NotifyIcon(NIM_ADD, &m_nid);
}

BOOL ShowBalloonTip(LPCTSTR szMsg, LPCTSTR szTitle, DWORD dwInfoFlags = NIIF_INFO, UINT uTimeout = 1)  {
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.uFlags = NIF_INFO;
	m_nid.uVersion = NOTIFYICON_VERSION;
	m_nid.uTimeout = uTimeout;
	m_nid.dwInfoFlags = dwInfoFlags;
	strcpy(m_nid.szInfo, szMsg ? szMsg : _T(""));
	strcpy(m_nid.szInfoTitle, szTitle ? szTitle : _T(""));

	return 0 != Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}
//————————————————
//版权声明：本文为CSDN博主「江南-一苇渡江」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
//原文链接：https://blog.csdn.net/wangshubo1989/article/details/49533051

void Buttonable(BOOL FLAG, WORD WEI) {
	switch (WEI) {
		case 1: {
			EnableWindow(SuspendB, FLAG);
			break;
		}
		case 2: {
			EnableWindow(JcB, FLAG);
			//EnableWindow(KeyboradHookB, FLAG);
			break;
		}
	}
	return;
}

/*BOOL CALLBACK FindWindow(HWND hwnd, LPARAM lparam) {
	TCHAR lpWinTitle[256] = {};
	//EnumProcessModules(hProcess, hm, 1, &count);
	GetWindowText(hwnd, lpWinTitle, sizeof(lpWinTitle));
	//GetModuleFileName(HMODULE(hwnd), lpWinTitle, sizeof(lpWinTitle));
	if(lpWinTitle != "") {
		if(_tcscmp(lpWinTitle, BroadcastTitle) == 0) {
			Class = hwnd;
			LPDWORD temp;
			GetWindowThreadProcessId(hwnd, temp);
			wprintf(pid, "%u", temp);
			flag = true;
		}
	}
	return TRUE;
}*/

BOOL CALLBACK EnumChildWindowsProc(HWND hwndChild, LPARAM lParam) {
	HMENU hmenu = GetMenu(hwndChild);
	if (LOWORD(hmenu) == 1004) {
		if (!IsWindowEnabled(hwndChild)) {
			EnableWindow(hwndChild, TRUE);
			SetWindowText(JcB, "恢复全屏按钮限制");
		} else {
			EnableWindow(hwndChild, FALSE);
			SetWindowText(JcB, "解除全屏按钮限制");
		}
		return FALSE;
	}
//	wchar_t str[100], strben[1000];
//	GetWindowTextW(hwndChild, str, 100);
//
//	MessageBoxW(0, str, str, 0);
	return TRUE;
}

BOOL CALLBACK EnumChildWindowsTest(HWND hwndChild, LPARAM lParam) {
	EnableWindow(hwndChild, FALSE);
	HMENU hmenu = GetMenu(hwndChild);
	char buf[20] = {'\0'};
	_ultoa(LOWORD(hmenu), buf, 10);
	MessageBoxA(0, buf, buf, 0);
	return TRUE;
}

VOID CALLBACK SetWindowToTopT(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	LRESULT res = SendMessage(SetFirstB, BM_GETSTATE, 0, 0);
	if (res == BST_CHECKED) {
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	} else {
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return;
}

VOID CALLBACK SetHideorShowT(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	POINT tm;
	GetCursorPos(&tm);
	int tsx = GetSystemMetrics(SM_CXSCREEN);
	int tsy = GetSystemMetrics(SM_CYSCREEN);
	if (tm.x == tsx - 1 && tm.y == tsy - 1) {
		ShowWindow(hwnd, SW_HIDE);
		SetCursorPos(tsx / 2, tsy / 2);
	} else if (tm.x == tsx - 1 && tm.y == 0) {
		ShowWindow(hwnd, SW_RESTORE);
		SetCursorPos(tsx / 2, tsy / 2);
	}
	//printf("MOUSE(%d, %d) SCREEN(%d, %d)\n", tm.x, tm.y, tsx, tsy);
	return;
}

VOID SetClipboard(LPCSTR str) {
	if (OpenClipboard(NULL)) {
		HGLOBAL hmem = GlobalAlloc(GHND, strlen(str) + 1);
		LPVOID pmem = GlobalLock(hmem);
		EmptyClipboard();
		memcpy(pmem, str, strlen(str) + 1);
		SetClipboardData(CF_TEXT, hmem);
		CloseClipboard();
		GlobalFree(hmem);
	}
	return;
}


unsigned int __stdcall IsStart(LPVOID lParam) {
	bool FlagSetToum = false, FlagMoveShutdown = false;
	while (1) {
		Class = FindWindowW(NULL, BroadcastTitle);
		if (Class != NULL) {
			GetWindowThreadProcessId(Class, &pid);
			//ClassHandle = OpenProcess(PROCESS_SUSPEND_RESUME, false, pid);
			ClassHandle = OpenThread(THREAD_ALL_ACCESS, false, GetMainThreadFromId(pid));
			/*wchar_t pidtemp[15];
			_itow(pid, pidtemp, 10);
			SetWindowTextW(mythwaretext, temptext);
			*/
			Buttonable(TRUE, 2);
			SetWindowText(guangbotext, "广播已开启");
		} else {
			Buttonable(FALSE, 2);
			CloseHandle(ClassHandle);
			SetWindowText(guangbotext, "广播未开启");
		}
//		clock_t tt, tt_;
//		tt = clock();
		Jiyu tj = GetProcessPidFromFilename(MythwareFilename);
//		tt_ = clock();
//		printf("spend - %lf\n", (double)(tt_ - tt) / CLOCKS_PER_SEC);
		if (tj.flag == TRUE) {

			GetWindowThreadProcessId(Mythware, &tj.id);
			MythwareHandle = OpenProcess(THREAD_ALL_ACCESS, false, tj.id);
			Buttonable(TRUE, 1);
			SetWindowText(mythwaretext, "极域已开启");
			SetWindowText(KillB, "杀死极域");
		} else {
			Buttonable(FALSE, 1);
			CloseHandle(MythwareHandle);
			SetWindowText(mythwaretext, "极域未开启");

			SetWindowText(KillB, "重启极域");
		}

		if (tj.flag == TRUE) {
			EnterCriticalSection(&CSPID);
			jiyu.id = tj.id;
			jiyu.flag = tj.flag;
			strcpy_s(jiyu.filepath, rsize_t(260), tj.filepath);
			LeaveCriticalSection(&CSPID);
			printf("jiyu - %s\n", tj.filepath);
			for (char i = strlen(tj.filepath) - 1; i >= 0; i--) {
				if (tj.filepath[i] == '\\') {
					tj.filepath[i] = '\0';
					break;
				}
			}
			printf("jiyu查询后 - %s\n", tj.filepath);
			SetWindowText(mythwareversiontext, tj.filepath);
//			char tp1[260] = {0}, tp2[260] = {0};
//			strcpy_s(tp1, rsize_t(260), tj.filepath);
//			strcat_s(tp1, rsize_t(260), "\\Shutdown.exe");
//			strcpy_s(tp2, rsize_t(260), tj.filepath);
//			strcat_s(tp2, rsize_t(260), "\\hhh\\Shutdown.exe");
//			int res1 = _taccess(tp1, F_OK);
//			int res2 = _taccess(tp2, F_OK);
//			printf("tp1 - %s\n", tp1);
//			printf("tp2 - %s\n", tp2);
//			if (res1 == -1 && res2 == -1 && FlagMoveShutdown == true) {
//				FlagMoveShutdown == false;
//				EnableWindow(MoveShutdownB, FALSE);
//				SetWindowText(MoveShutdownB, "未检测到Shutdown文件");
//			}
//			else {
//				if (res1 == 0 && FlagMoveShutdown == false) {
//					//printf("_taccess返回 0\n");
//					FlagMoveShutdown = true;
//					EnableWindow(MoveShutdownB, TRUE);
//					SetWindowText(MoveShutdownB, "开启防教师端关机");
//				}
//				else if (res2 == 0 && FlagMoveShutdown == true) {
//					FlagMoveShutdown = false;
//					EnableWindow(MoveShutdownB, TRUE);
//					SetWindowText(MoveShutdownB, "关闭防教师端关机");
//				}
//			}
		} else {
			SetWindowText(mythwareversiontext, "极域未开启！！！");
		}
		//printf("%u\n", pid);
	}
	return 0;
}

unsigned int _stdcall SetWindowName(LPVOID lParam) {
	while (1) {
		HWND hwndsec = GetNextWindow(HWND(lParam), GW_HWNDNEXT);
		char tempstr[100];
		GetWindowText(hwndsec, tempstr, 100);
		SetWindowText(HWND(lParam), tempstr);
		Sleep(500);
	}
	return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

unsigned int __stdcall KeyboradHook(LPVOID lParam) {
	HHOOK m_hHOOK1 = NULL, m_hHOOK2 = NULL, m_hHOOK3 = NULL, m_hHOOK4 = NULL;
	m_hHOOK1 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	m_hHOOK2 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	m_hHOOK3 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	m_hHOOK4 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	while (true) {
		LRESULT res = SendMessage(KeyboradHookB, BM_GETSTATE, 0, 0);
		if (res == BST_CHECKED) {
			UnhookWindowsHookEx(m_hHOOK1);
			UnhookWindowsHookEx(m_hHOOK3);
			m_hHOOK1 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
			m_hHOOK3 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
			UnhookWindowsHookEx(m_hHOOK2);
			UnhookWindowsHookEx(m_hHOOK4);
			m_hHOOK2 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
			m_hHOOK4 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
		} else {
			UnhookWindowsHookEx(m_hHOOK1);
			UnhookWindowsHookEx(m_hHOOK3);
			UnhookWindowsHookEx(m_hHOOK2);
			UnhookWindowsHookEx(m_hHOOK4);
		}
	}
	return 0;
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

char TempNtsdPath[MAX_PATH + 1];

VOID UseNtsd() {
	if (!(fopen("ntsd.exe", "r") == NULL)) {
		return;
	}
	HRSRC hRsrc = FindResource(NULL, MAKEINTRESOURCEA(NTSDEXE), TEXT("EXETYPE"));
	if (hRsrc == NULL) {
//		puts("资源文件查找失败");
//		printf("错误代码%u\n", GetLastError());
		return;
	}
	DWORD dwSize = SizeofResource(NULL, hRsrc);
	HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
	LPVOID pBuffer = LockResource(hGlobal);
	FILE* fp = fopen("ntsd.exe", "wb");
	if (fp != NULL) {
		fwrite(pBuffer, sizeof(char), dwSize, fp);
//		printf("写ntsd成功");
	}
	fclose(fp);
	return;
}

//int GetMythwarePortFromPid(int pid) {
//	popen("ls", "r");
//	return port;
//}

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
		case WM_CLOSE: {
			remove("ntsd.exe");
		}
		case WM_DESTROY: {
			Shell_NotifyIcon(NIM_DELETE, &m_nid);
			CloseHandle(threadisstart);
			CloseHandle(threadKeyboradHook);
			CloseHandle(threadSetWindowName);
			DeleteCriticalSection(&CSPID);
			UnregisterHotKey(hwnd, ID_ACCR_SHOW);
			UnregisterHotKey(hwnd, ID_ACCR_HIDE);
			PostQuitMessage(0);
			break;
		}
		case WM_QUERYENDSESSION: {
			LRESULT res = SendMessage(SetCannotShutdownB, BM_GETSTATE, 0, 0);
			if (res == BST_CHECKED) {
				return 0;
			}
			return 1;
		}
		case WM_CTLCOLORSTATIC: {
			HDC hdcStatic = (HDC)wParam;
			SetBkMode(hdcStatic, TRANSPARENT); //透明背景
			return (INT_PTR)GetStockObject(NULL_BRUSH); //无颜色画刷
		}
		case WM_CREATE: {
			
			SetupTrayIcon(hwnd);
			RegisterHotKey(hwnd, ID_ACCR_SHOW, MOD_WIN | MOD_CONTROL | MOD_ALT, 83);
			RegisterHotKey(hwnd, ID_ACCR_HIDE, MOD_WIN | MOD_CONTROL | MOD_ALT, 72);
			SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//设置为后台程序

			//SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_LAYERED);
			//SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

			mythwaretext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD | WS_DLGFRAME, 10, 10, 150, 30, hwnd, NULL, HINSTANCE(hwnd), NULL);
			guangbotext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD | WS_DLGFRAME, 170, 10, 150, 30, hwnd, NULL, HINSTANCE(hwnd), NULL);
			mythwareversiontext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD | WS_DLGFRAME, 10, 240, 300, 60, hwnd, NULL, HINSTANCE(hwnd), NULL);
			CreateWindow(TEXT("static"), TEXT("from liu_zhou"),  WS_VISIBLE | WS_CHILD, 10, 310, 300, 30, hwnd, NULL, HINSTANCE(hwnd), NULL);
			SuspendB = CreateWindow(TEXT("button"), TEXT("挂起极域"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 50, 150, 30, hwnd, HMENU(SUSPEND_BUTTON), HINSTANCE(hwnd), NULL);
			KillB = CreateWindow(TEXT("button"), TEXT("杀死极域"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 90, 150, 30, hwnd, HMENU(KILL_BUTTON), HINSTANCE(hwnd), NULL);
			JcB = CreateWindow(TEXT("button"), TEXT("解除全屏按钮限制"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 50, 150, 30, hwnd, HMENU(JC_BUTTON), HINSTANCE(hwnd), NULL);
			GetRegeditPassWdB = CreateWindow(TEXT("button"), TEXT("复制密码（注册表）"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 130, 150, 30, hwnd, HMENU(GETREGEDITPASSWD_BUTTON), HINSTANCE(hwnd), NULL);
			PassWdB = CreateWindow(TEXT("button"), TEXT("复制万能密码"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 130, 150, 30, hwnd, HMENU(PASSWD_BUTTON), HINSTANCE(hwnd), NULL);
			YinCB = CreateWindow(TEXT("button"), TEXT("隐藏窗体"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 90, 150, 30, hwnd, HMENU(YINC_BUTTON), HINSTANCE(hwnd), NULL);
			EnableWindow(MoveShutdownB, FALSE);

			KeyboradHookB = CreateWindow(TEXT("button"), TEXT(""),  WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 170, 15, 15, hwnd, HMENU(KEYBORADHOOK_BUTTON), HINSTANCE(hwnd), NULL);
			SendMessage(KeyboradHookB, BM_SETCHECK, BST_CHECKED, 0);
			CreateWindow(TEXT("static"), TEXT("解除键盘锁"), WS_VISIBLE | WS_CHILD, 30, 170, 100, 20, hwnd, NULL, HINSTANCE(hwnd), NULL);

			SetFirstB = CreateWindow(TEXT("button"), TEXT(""),  WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 190, 15, 15, hwnd, HMENU(SETFIRST_BUTTON), HINSTANCE(hwnd), NULL);
			SendMessage(SetFirstB, BM_SETCHECK, BST_CHECKED, 0);
			CreateWindow(TEXT("static"), TEXT("置顶窗口"), WS_VISIBLE | WS_CHILD, 30, 190, 100, 20, hwnd, NULL, HINSTANCE(hwnd), NULL);

			SetCannotShutdownB = CreateWindow(TEXT("button"), TEXT(""),  WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 210, 15, 15, hwnd, HMENU(SETCANNOTSHUTDOWN_BUTTON), HINSTANCE(hwnd), NULL);
			SendMessage(SetCannotShutdownB, BM_SETCHECK, BST_CHECKED, 0);
			CreateWindow(TEXT("static"), TEXT("拦截关机（失败率高）"), WS_VISIBLE | WS_CHILD, 30, 210, 180, 20, hwnd, NULL, HINSTANCE(hwnd), NULL);

			break;
		}
		/* Upon destruction, tell the main thread to stop */
		case WM_ENDSESSION: {
			return 0;
		}
		case WM_HOTKEY: {
			switch (LOWORD(wParam)) {
				case ID_ACCR_SHOW: {
					ShowWindow(hwnd, SW_RESTORE);
					break;
				}
				case ID_ACCR_HIDE: {
					ShowWindow(hwnd, SW_HIDE);
					break;
				}
				default: {
					break;
				}
			}
			break;
		}
		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case SUSPEND_BUTTON: {
					//SuspendThread(MythwareHandle);
					char tstr[50];
					GetWindowText(SuspendB, tstr, 50);
					if (strcmp(tstr, "挂起极域") == 0) {
						SuspendProcess(jiyu.id, TRUE);
						SetWindowText(SuspendB, "恢复极域");
					} else if (strcmp(tstr, "恢复极域") == 0) {
						SuspendProcess(jiyu.id, FALSE);
						SetWindowText(SuspendB, "挂起极域");
					}
					//MessageBox(NULL, "点击", "点击", NULL);
					break;
				}
				case KILL_BUTTON: {
					//TerminateThread(MythwareHandle, 0);
					char tstr[50];
					GetWindowText(KillB, tstr, 50);
					if (strcmp(tstr, "杀死极域") == 0) {
						UseNtsd();
						WinExec("./ntsd -c q -pn studentmain.exe", SW_HIDE);
						
						SetWindowText(KillB, "重启极域");
					} else if (strcmp(tstr, "重启极域") == 0) {
						EnterCriticalSection(&CSPID);
						UINT res = WinExec(jiyu.filepath, SW_HIDE);
						LeaveCriticalSection(&CSPID);
						if (res <= 32) {
							//MessageBox(NULL, "重启极域失败!无法获取极域路径", "提示", MB_ICONWARNING | MB_OK);
							ShowBalloonTip("重启极域失败!无法获取极域路径", "提示");
						} else {
							SetWindowText(KillB, "杀死极域");
						}
					}
					break;
				}
				case JC_BUTTON: {
					EnumChildWindows(Class, EnumChildWindowsProc, (LPARAM)0);
//					const int JMP =  0x73EB;
//					const LPVOID address = LPVOID(0x00431c14);
//					PSIZE_T pWritten = new SIZE_T;
//					WriteProcessMemory(Class, address, &JMP, 1, pWritten);
					//EnumChildWindows(Class, EnumChildWindowsTest, NULL);
					break;
				}
				case PASSWD_BUTTON: {
					SetClipboard("mythware_super_password");
					break;
				}
				case KEYBORADHOOK_BUTTON: {
					//MessageBox(NULL, "点击", "点击", 0);
					printf("%u\n", Message);
					/*if (flagKeyboradHook.load()) {
						flagKeyboradHook.store(false);
						SetWindowText(KeyboradHookB, "解除键盘锁");
					}
					else {
						flagKeyboradHook.store(true);
						SetWindowText(KeyboradHookB, "恢复键盘锁");
					}*/
					break;
				}
				case REBOOT_BUTTON: {
					break;
				}
				case YINC_BUTTON: {
					ShowWindow(hwnd, SW_HIDE);
					break;
				}
				case GETREGEDITPASSWD_BUTTON: {
					char str[MAX_PATH * 10] = {0};
					if (GetMythwarePasswordFromRegedit(str) != FALSE) {
						ShowBalloonTip("复制密码成功", "提示");
						SetClipboard(str);
					} else {
						ShowBalloonTip("复制密码失败", "提示");
					}
					break;
				}
			}
			break;
		}
		/* All other messages (a lot of them) are processed using default procedures */
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

/* The 'main' function of Win32 GUI programs: this is where execution starts */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	SetProcessDPIAware();
	if (!IsRegeditHasSet("MythwareHelper", "agree")) {
		if (MessageBox(NULL, TISHIYU, "提示", MB_YESNO | MB_ICONWARNING) != IDYES) {
			return 0;
		}
		if (!SetRegedit("MythwareHelper", "agree", "yes")) {
			MessageBox(NULL, "写注册表失败，请尝试以管理员权限重新运行本程序，除此以外本程序不需要以管理员权限运行", "提示", MB_OK);
			return 0; 
		}
	}
	
//	原文链接：https://blog.csdn.net/awlp1990/article/details/51564379
	WNDCLASSEX wc; /* A properties struct of our window */
	HWND hwnd; /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg; /* A temporary location for all messages */

	/* zero out the struct and set the stuff we want to modify */
	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; /* This is where we will send messages to */
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);

	/* White, COLOR_WINDOW is just a #define for a system color, try Ctrl+Clicking it */
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = TEXT("WindowClass");
	wc.hIcon		 = LoadIcon(NULL, IDC_ICON/*IDI_APPLICATION*/); /*任务栏图标*/
	wc.hIconSm		 = LoadIcon(NULL, IDC_ICON/*IDI_APPLICATION*/); /*窗口图标*/

	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "WindowClass", "Mythware helper", (WS_VISIBLE | WS_OVERLAPPEDWINDOW)^WS_MAXIMIZEBOX ^ WS_SIZEBOX,
	                      CW_USEDEFAULT, /* x */
	                      CW_USEDEFAULT, /* y */
	                      350, /* width */
	                      365, /* height */
	                      NULL, NULL, hInstance, NULL);

	if (hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	/*
		This is the heart of our program where all input is processed and
		sent to WndProc. Note that GetMessage blocks code flow until it receives something, so
		this loop will not produce unreasonably high CPU usage
	*/
	GetTempPath(MAX_PATH, TempWorkPath);
	SetCurrentDirectory(TempWorkPath); 
	freopen("log.txt", "w", stdout);
	InitializeCriticalSection(&CSPID);
	threadisstart = (HANDLE)_beginthreadex(NULL, 0, IsStart, hwnd, 0, 0);
	threadKeyboradHook = (HANDLE)_beginthreadex(NULL, 0, KeyboradHook, hwnd, 0, 0);
	threadSetWindowName = (HANDLE)_beginthreadex(NULL, 0, SetWindowName, hwnd, 0, 0);
	UINT_PTR timeid1 = SetTimer(hwnd, 1, 1, SetWindowToTopT);
	UINT_PTR timeid2 = SetTimer(hwnd, 2, 1, SetHideorShowT);
	while (GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
		//EnumWindows(FindWindow, 0);
	}
	KillTimer(NULL, timeid1);
	KillTimer(NULL, timeid2);
	return msg.wParam;
}
