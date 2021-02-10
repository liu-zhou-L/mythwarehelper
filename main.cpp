#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>

/*

from liu_zhou

*/

//https://blog.csdn.net/yanglx2022/article/details/46582629
//https://www.52pojie.cn/thread-799791-1-1.html
//https://blog.csdn.net/Koevas/article/details/84679206?ops_request_misc=%25257B%252522request%25255Fid%252522%25253A%252522161008071416780264661008%252522%25252C%252522scm%252522%25253A%25252220140713.130102334..%252522%25257D&request_id=161008071416780264661008&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_click~default-1-84679206.first_rank_v2_pc_rank_v29&utm_term=%E6%9E%81%E5%9F%9F
//https://blog.csdn.net/u012314571/article/details/89811045?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control
//下面的回去关注 
//https://blog.csdn.net/powerful_green/article/details/85037018?utm_medium=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase&depth_1-utm_source=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase

const int SUSPEND_BUTTON = 3301; 
const int RESUME_BUTTON = 3302; 
const int KILL_BUTTON = 3303; 
const int BUTTON4 = 3304; 
const int JC_BUTTON = 3305; 
const int PASSWD_BUTTON = 3306; 
LPCTSTR BroadcastTitle = TEXT("屏幕广播");
LPCTSTR MythwareTitle = TEXT("C:\\Program Files (x86)\\Mythware\\极域课堂管理系统软件V6.0 2016 豪华版\\StudentMain.exe");

HANDLE ClassHandle = NULL, MythwareHandle = NULL, threadtotop = NULL;
HWND windowtext = NULL, mythwaretext = NULL, guangbotext = NULL, SuspendB = NULL, ResumeB = NULL, KillB = NULL, JcB = NULL, PassWdB = NULL; 
HWND Class = NULL, Mythware = NULL;
DWORD pid;

DWORD GetMainThreadFromId(const DWORD IdProcess) {
	if (IdProcess <= 0) return 0;
	DWORD IdMainThread = NULL;
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

DWORD GetProcessPidFromFilename(LPCTSTR Filename) {
	if (Filename == '\0') return 0;
	DWORD IdMainThread = NULL;
	PROCESSENTRY32 te;
	te.dwSize = sizeof(THREADENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (Process32First(hSnapshot, &te)) {
		do {
			LPTSTR tempfilename;
			GetModuleFileNameEx(OpenProcess(PROCESS_ALL_ACCESS, false, te.th32ProcessID), NULL, tempfilename, 100);
			if (_tcscmp(Filename, tempfilename)) {
				IdMainThread = te.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &te));
	}
	CloseHandle(hSnapshot);
	return IdMainThread;
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


//unsigned int __stdcall SetWindowToTop(LPVOID lParam) {
//	while(exitflag) {
//		SetWindowPos(HWND(lParam), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//	}
//	_endthreadex(0);
//	return TRUE;
//}

/* This is where all the input to the window goes to */
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		case WM_DESTROY: {
			CloseHandle(threadtotop);
			PostQuitMessage(0);
			break;
		}
		case WM_CREATE: {
			mythwaretext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 10, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			guangbotext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 100, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			windowtext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 350, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			SuspendB = CreateWindow(TEXT("button"), TEXT("挂起"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 190, 150, 50, hwnd, HMENU(SUSPEND_BUTTON), HINSTANCE(hwnd), NULL);
			ResumeB = CreateWindow(TEXT("button"), TEXT("恢复"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 190, 150, 50, hwnd, HMENU(RESUME_BUTTON), HINSTANCE(hwnd), NULL);
			KillB = CreateWindow(TEXT("button"), TEXT("杀死"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 260, 150, 50, hwnd, HMENU(KILL_BUTTON), HINSTANCE(hwnd), NULL);
			JcB = CreateWindow(TEXT("button"), TEXT("解除全屏按钮限制"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 100, 150, 50, hwnd, HMENU(JC_BUTTON), HINSTANCE(hwnd), NULL);
			PassWdB = CreateWindow(TEXT("button"), TEXT("复制万能密码"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 350, 150, 50, hwnd, HMENU(PASSWD_BUTTON), HINSTANCE(hwnd), NULL);
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
					TerminateThread(MythwareHandle, NULL);
					break;
				}
				case JC_BUTTON: {
					EnumChildWindows(Class, EnumChildWindowsProc, NULL);
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
	wc.lpszClassName = "WindowClass";
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION); /*任务栏图标*/
	wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION); /*窗口图标*/

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"WindowClass","Mythware helper",WS_VISIBLE|WS_OVERLAPPEDWINDOW^WS_MINIMIZEBOX^WS_MAXIMIZEBOX^WS_SIZEBOX,
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		350, /* width */
		450, /* height */
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
	//threadtotop = (HANDLE)_beginthreadex(NULL, 0, SetWindowToTop, hwnd, NULL, NULL);
	UINT_PTR timeid = SetTimer(hwnd, 1, 1, SetWindowToTop);
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
		//EnumWindows(FindWindow, 0);
		Class = FindWindow(NULL, BroadcastTitle);
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
		pid = GetProcessPidFromFilename(MythwareTitle);
		if (pid != NULL) {
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
	}
	KillTimer(NULL, timeid);
	return msg.wParam;
}
