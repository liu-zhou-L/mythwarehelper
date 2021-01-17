#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>

/*

from liu_zhou

*/

//https://blog.csdn.net/yanglx2022/article/details/46582629
//https://www.52pojie.cn/thread-799791-1-1.html
//https://blog.csdn.net/Koevas/article/details/84679206?ops_request_misc=%25257B%252522request%25255Fid%252522%25253A%252522161008071416780264661008%252522%25252C%252522scm%252522%25253A%25252220140713.130102334..%252522%25257D&request_id=161008071416780264661008&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_click~default-1-84679206.first_rank_v2_pc_rank_v29&utm_term=%E6%9E%81%E5%9F%9F
//https://blog.csdn.net/u012314571/article/details/89811045?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.control
//下面的回去关注 
//https://blog.csdn.net/powerful_green/article/details/85037018?utm_medium=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase&depth_1-utm_source=distribute.pc_relevant_download.none-task-blog-baidujs-1.nonecase

const int BUTTON1 = 3301; 
const int BUTTON2 = 3302; 
const int BUTTON3 = 3303; 
const int BUTTON4 = 3304; 
const int BUTTON5 = 3305; 
const int BUTTON6 = 3306; 
TCHAR lpTitle[256] = TEXT("屏幕广播");

HANDLE ClassHandle = NULL, threadtotop = NULL;
HWND windowtext = NULL, mytext = NULL, pidtext = NULL, nametext = NULL, SuspendB = NULL, ResumeB = NULL, KillB = NULL, YesB = NULL, JcB = NULL, PassWdB = NULL; 
HWND Class = NULL;
bool flag;
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

void Buttonable(BOOL FLAG) {
	if (FLAG == NULL) {
		EnableWindow(SuspendB, FALSE);
		EnableWindow(ResumeB, FALSE);
		EnableWindow(KillB, FALSE);
		EnableWindow(JcB, FALSE);
	}
	else {
		EnableWindow(SuspendB, TRUE);
		EnableWindow(ResumeB, TRUE);
		EnableWindow(KillB, TRUE);
		EnableWindow(JcB, TRUE);
	}
	return;
}

/*BOOL CALLBACK FindWindow(HWND hwnd, LPARAM lparam) {
	TCHAR lpWinTitle[256] = {};
	//EnumProcessModules(hProcess, hm, 1, &count);
	GetWindowText(hwnd, lpWinTitle, sizeof(lpWinTitle)); 
	//GetModuleFileName(HMODULE(hwnd), lpWinTitle, sizeof(lpWinTitle));
	if(lpWinTitle != "") {
		if(_tcscmp(lpWinTitle, lpTitle) == 0) {
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
		EnableWindow(hwndChild, TRUE);
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
			mytext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 10, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			pidtext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 100, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			nametext = CreateWindow(TEXT("edit"), lpTitle,  WS_VISIBLE | WS_CHILD | WS_BORDER, 170, 10, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			windowtext = CreateWindow(TEXT("static"), TEXT(""),  WS_VISIBLE | WS_CHILD, 10, 350, 150, 50, hwnd, NULL, HINSTANCE(hwnd), NULL);
			SuspendB = CreateWindow(TEXT("button"), TEXT("挂起"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 190, 150, 50, hwnd, HMENU(BUTTON1), HINSTANCE(hwnd), NULL);
			ResumeB = CreateWindow(TEXT("button"), TEXT("恢复"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 190, 150, 50, hwnd, HMENU(BUTTON2), HINSTANCE(hwnd), NULL);
			KillB = CreateWindow(TEXT("button"), TEXT("杀死"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 260, 150, 50, hwnd, HMENU(BUTTON3), HINSTANCE(hwnd), NULL);
			YesB = CreateWindow(TEXT("button"), TEXT("确认更改"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 260, 150, 50, hwnd, HMENU(BUTTON4), HINSTANCE(hwnd), NULL);
			JcB = CreateWindow(TEXT("button"), TEXT("开放所有子控件"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 100, 150, 50, hwnd, HMENU(BUTTON5), HINSTANCE(hwnd), NULL);
			PassWdB = CreateWindow(TEXT("button"), TEXT("复制万能密码"),  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 350, 150, 50, hwnd, HMENU(BUTTON6), HINSTANCE(hwnd), NULL);
			break;
		} 
		/* Upon destruction, tell the main thread to stop */
		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case BUTTON1: {
					SuspendThread(ClassHandle);
					//MessageBox(NULL, "点击", "点击", NULL);
					break;
				}
				case BUTTON2: {
					ResumeThread(ClassHandle);
					//MessageBox(NULL, "点击", "点击", NULL);
					break;
				}
				case BUTTON3: {
					TerminateThread(ClassHandle, NULL);
					break;
				}
				case BUTTON4: {
					TCHAR temp[128] = TEXT("");
					GetWindowText(nametext, temp, 128);;
					if (_tcscmp(lpTitle, temp) != 0) _tcscpy(lpTitle, temp);
					break;
				}
				case BUTTON5: {
					EnumChildWindows(Class, EnumChildWindowsProc, NULL);
//					const int JMP =  0x73EB;
//					const LPVOID address = LPVOID(0x00431c14);
//					PSIZE_T pWritten = new SIZE_T;
//					WriteProcessMemory(Class, address, &JMP, 1, pWritten);
					//EnumChildWindows(Class, EnumChildWindowsTest, NULL);
					break;
				}
				case BUTTON6: {
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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
	flag = false;
	UINT_PTR timeid = SetTimer(hwnd, 1, 1, SetWindowToTop);
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
		//EnumWindows(FindWindow, 0);
		TCHAR temp[128];
		GetWindowText(nametext, temp, 127);
		Class = FindWindow(NULL, lpTitle);
		if (Class != NULL) {
			SetWindowText(mytext, "已开启");
			GetWindowThreadProcessId(Class, &pid);
			//ClassHandle = OpenProcess(PROCESS_SUSPEND_RESUME, false, pid);
			ClassHandle = OpenThread(THREAD_ALL_ACCESS, false, GetMainThreadFromId(pid));
			wchar_t temp[15] = {};
			_itow(pid, temp, 10);
			//swprintf(temp, TEXT("%u"), pid);
			SetWindowTextW(pidtext, temp);
			flag = true;
			Buttonable(TRUE);
		}
		else {
			flag = false;
			Buttonable(FALSE);
			CloseHandle(ClassHandle);
			SetWindowText(mytext, "未开启");
			SetWindowText(pidtext, "未开启");
		}
	}
	KillTimer(NULL, timeid);
	return msg.wParam;
}
