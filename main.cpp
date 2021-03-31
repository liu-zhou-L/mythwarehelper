#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>
#include <atomic> 
#include "HOOKAPI.h"

/*

from liu_zhou

*/

//IsStart 出问题 3/30 

//https://blog.csdn.net/yanglx2022/article/details/46582629
//https://www.52pojie.cn/thread-799791-1-1.html
//https://blog.csdn.net/Koevas/article/details/84679206?ops_request_misc=%25257B%252522request%25255Fid%252522%25253A%252522161008071416780264661008%252522%25252C%252522scm%252522%25253A%25252220140713.130102334..%252522%25257D&request_id=161008071416780264661008&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_click~default-1-84679206.first_rank_v2_pc_rank_v29&utm_term=%E6%9E%81%E5%9F%9F
//https://blog.csdn.net/u012314571/article/details/89811045?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control
//下面的回去关注 
//https://blog.csdn.net/powerful_green/article/details/85037018?utm_medium=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase&depth_1-utm_source=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase
//https://blog.csdn.net/wangjieest/article/details/7065457

const int SUSPEND_BUTTON = 3301; 
const int RESUME_BUTTON = 3302; 
const int KILL_BUTTON = 3303; 
const int BUTTON4 = 3304; 
const int JC_BUTTON = 3305; 
const int PASSWD_BUTTON = 3306; 
const int KEYBORADHOOK_BUTTON = 3307;
LPCWSTR BroadcastTitle = L"屏幕广播";
LPCSTR MythwareFilename = "StudentMain.exe";
LPCWSTR MythwareTitle = L"StudentMain.exe";

HANDLE ClassHandle = NULL, MythwareHandle = NULL, threadisstart = NULL, threadKeyboradHook = NULL, threadSetWindowName = NULL;
HWND mythwaretext = NULL, guangbotext = NULL, SuspendB = NULL, ResumeB = NULL, KillB = NULL, JcB = NULL, PassWdB = NULL, KeyboradHookB = NULL; 
HWND Class = NULL, Mythware = NULL;
DWORD pid;
std::atomic<bool> flagKeyboradHook;

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

BOOL ModuleIsAble(DWORD ProcessPid, LPCSTR Modulename) {
	if (Modulename[0] == '\0') return FALSE;
	MODULEENTRY32 me;
	me.dwSize = sizeof(MODULEENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessPid);
	if (Module32First(hSnapshot, &me)) {
		do {
			//printf("%s\n", me.szModule);
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
	DWORD IdMainThread = 0;
	PROCESSENTRY32 te;
	te.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (Process32First(hSnapshot, &te)) {
		do {
			HANDLE temphandle = OpenProcess(PROCESS_ALL_ACCESS, false, te.th32ProcessID);
			//printf("%ld\n", te.th32ProcessID);
			if (ModuleIsAble(te.th32ProcessID, Filename)) {
				IdMainThread = te.th32ProcessID;
				CloseHandle(hSnapshot);
				return IdMainThread;
			}
			CloseHandle(temphandle);
		} while (Process32Next(hSnapshot, &te));
	}
	return 0;
} 

void Buttonable(BOOL FLAG, WORD WEI) {
	switch(WEI) {
		case 1: {
			EnableWindow(SuspendB, FLAG);
			EnableWindow(ResumeB, FLAG);
			EnableWindow(KillB, FLAG);
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
		}
		else {
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

VOID CALLBACK SetWindowToTop(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	return; 
} 

VOID SetClipboard(LPCSTR str) {
	if(OpenClipboard(NULL)) {
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
	while(1) {
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
		}
		else {
			Buttonable(FALSE, 2);
			CloseHandle(ClassHandle);
			SetWindowText(guangbotext, "广播未开启");
		}
		pid = GetProcessPidFromFilename(MythwareFilename);
		if (pid != 0) {
			GetWindowThreadProcessId(Mythware, &pid);
			MythwareHandle = OpenThread(THREAD_ALL_ACCESS, false, GetMainThreadFromId(pid));
			Buttonable(TRUE, 1);
			SetWindowText(mythwaretext, "极域已开启");
		} 
		else {
			Buttonable(FALSE, 1);
			CloseHandle(MythwareHandle);
			SetWindowText(mythwaretext, "极域未开启");
		}
		printf("%u\n", pid);
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

LRESULT CALLBACK KeyboardProc(int nCode,WPARAM wParam,LPARAM lParam) {
	return CallNextHookEx(NULL,nCode,wParam,lParam);
}

unsigned int __stdcall KeyboradHook(LPVOID lParam) {
	HHOOK m_hHOOK1 = NULL, m_hHOOK2 = NULL,m_hHOOK3 = NULL,m_hHOOK4 = NULL; 
	m_hHOOK1 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	m_hHOOK2 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	m_hHOOK3 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	m_hHOOK4 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
	while(true)
	{
		LRESULT res = SendMessage(KeyboradHookB, BM_GETSTATE, 0, 0);
		if(res == BST_CHECKED)
		{
			UnhookWindowsHookEx(m_hHOOK1);
			UnhookWindowsHookEx(m_hHOOK3);
			m_hHOOK1 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
			m_hHOOK3 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
			UnhookWindowsHookEx(m_hHOOK2);
			UnhookWindowsHookEx(m_hHOOK4);
			m_hHOOK2 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
			m_hHOOK4 = (HHOOK)SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);
		}
		else {
			UnhookWindowsHookEx(m_hHOOK1);
			UnhookWindowsHookEx(m_hHOOK3);
			UnhookWindowsHookEx(m_hHOOK2);
			UnhookWindowsHookEx(m_hHOOK4);
		} 
	}
	return 0;
}

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		case WM_CLOSE: {
			 
		} 
		case WM_DESTROY: {
			CloseHandle(threadisstart);
			CloseHandle(threadKeyboradHook);
			CloseHandle(threadSetWindowName);
			PostQuitMessage(0);
			break;
		}
		case WM_CREATE: {
			mythwaretext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 10, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			guangbotext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 170, 10, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			SuspendB = CreateWindow(TEXT("button"), TEXT("挂起"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 70, 150, 50, hwnd, HMENU(SUSPEND_BUTTON), HINSTANCE(hwnd), NULL);
			ResumeB = CreateWindow(TEXT("button"), TEXT("恢复"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 130, 150, 50, hwnd, HMENU(RESUME_BUTTON), HINSTANCE(hwnd), NULL);
			KillB = CreateWindow(TEXT("button"), TEXT("杀死"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 130, 150, 50, hwnd, HMENU(KILL_BUTTON), HINSTANCE(hwnd), NULL);
			JcB = CreateWindow(TEXT("button"), TEXT("解除全屏按钮限制"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 70, 150, 50, hwnd, HMENU(JC_BUTTON), HINSTANCE(hwnd), NULL);
			PassWdB = CreateWindow(TEXT("button"), TEXT("复制万能密码"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 190, 150, 50, hwnd, HMENU(PASSWD_BUTTON), HINSTANCE(hwnd), NULL);
			KeyboradHookB = CreateWindow(TEXT("button"), TEXT(""),  WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 190, 15, 15, hwnd, HMENU(KEYBORADHOOK_BUTTON), HINSTANCE(hwnd), NULL);
			CreateWindow(TEXT("static"), TEXT("解除键盘锁"), WS_VISIBLE | WS_CHILD, 30, 190, 100, 20, hwnd, NULL, HINSTANCE(hwnd), NULL);
			break;
		} 
		/* Upon destruction, tell the main thread to stop */
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case SUSPEND_BUTTON: {
					SuspendThread(MythwareHandle);
					//MessageBox(NULL, "点击", "点击", NULL);
					break;
				}
				case RESUME_BUTTON: {
					ResumeThread(MythwareHandle);
					//MessageBox(NULL, "点击", "点击", NULL);
					break;
				}
				case KILL_BUTTON: {
					TerminateThread(MythwareHandle, 0);
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
	WNDCLASSEX wc; /* A properties struct of our window */
	HWND hwnd; /* A 'HANDLE', hence the H, or a pointer to our window */
	MSG msg; /* A temporary location for all messages */

	/* zero out the struct and set the stuff we want to modify */
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc; /* This is where we will send messages to */
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	
	/* White, COLOR_WINDOW is just a #define for a system color, try Ctrl+Clicking it */
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = TEXT("WindowClass");
	wc.hIcon		 = LoadIcon(NULL, IDC_ICON/*IDI_APPLICATION*/); /*任务栏图标*/
	wc.hIconSm		 = LoadIcon(NULL, IDC_ICON/*IDI_APPLICATION*/); /*窗口图标*/

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"WindowClass","Mythware helper",(WS_VISIBLE|WS_OVERLAPPEDWINDOW)^WS_MAXIMIZEBOX^WS_SIZEBOX,
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		350, /* width */
		280, /* height */
		NULL,NULL,hInstance,NULL);

	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	/*
		This is the heart of our program where all input is processed and 
		sent to WndProc. Note that GetMessage blocks code flow until it receives something, so
		this loop will not produce unreasonably high CPU usage
	*/
	freopen("log.txt", "w", stdout);
	threadisstart = (HANDLE)_beginthreadex(NULL, 0, IsStart, hwnd, 0, 0);
	threadKeyboradHook = (HANDLE)_beginthreadex(NULL, 0, KeyboradHook, hwnd, 0, 0);
	threadSetWindowName = (HANDLE)_beginthreadex(NULL, 0, SetWindowName, hwnd, 0, 0);
	UINT_PTR timeid = SetTimer(hwnd, 1, 1, SetWindowToTop);
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
		//EnumWindows(FindWindow, 0);
	}
	KillTimer(NULL, timeid);
	return msg.wParam;
}
