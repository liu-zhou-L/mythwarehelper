// MythwareHelper.cpp : 定义应用程序的入口点。
//

#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <tlhelp32.h>
#include <process.h>
#include <psapi.h>
#include "framework.h"
#include "MythwareHelper.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

/*************************************************************/

const int SUSPEND_BUTTON = 3301;
const int RESUME_BUTTON = 3302;
const int KILL_BUTTON = 3303;
const int BUTTON4 = 3304;
const int JC_BUTTON = 3305;
const int PASSWD_BUTTON = 3306;
LPCWSTR BroadcastTitle = TEXT("屏幕广播");
LPCWSTR MythwareTitle = TEXT("C:\\Program Files (x86)\\Mythware\\极域课堂管理系统软件V6.0 2016 豪华版\\StudentMain.exe");

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

DWORD GetProcessPidFromFilename(LPCWSTR Filename) {
    if (wcslen(Filename) == '\0') return 0;
    DWORD IdMainThread = NULL;
    PROCESSENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(hSnapshot, &te)) {
        do {
            WCHAR tempfilename[MAX_PATH];
            HANDLE temphandle = OpenProcess(PROCESS_ALL_ACCESS, false, te.th32ProcessID);
            GetModuleFileNameExW(temphandle, NULL, tempfilename, MAX_PATH);
            if (wcscmp(Filename, tempfilename)) {
                IdMainThread = te.th32ProcessID;
                break;
            }
            CloseHandle(temphandle);
        } while (Process32Next(hSnapshot, &te));
    }
    CloseHandle(hSnapshot);
    return IdMainThread;
}

void Buttonable(BOOL FLAG, WORD WEI) {
    switch (WEI) {
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

/*BOOL CALLBACK FindWindow(HWND hWnd, LPARAM lparam) {
    TCHAR lpWinTitle[256] = {};
    //EnumProcessModules(hProcess, hm, 1, &count);
    GetWindowText(hWnd, lpWinTitle, sizeof(lpWinTitle));
    //GetModuleFileName(HMODULE(hWnd), lpWinTitle, sizeof(lpWinTitle));
    if(lpWinTitle != "") {
        if(_tcscmp(lpWinTitle, BroadcastTitle) == 0) {
            Class = hWnd;
            LPDWORD temp;
            GetWindowThreadProcessId(hWnd, temp);
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
            SetWindowText(JcB, TEXT("恢复全屏按钮限制"));
        }
        else {
            EnableWindow(hwndChild, FALSE);
            SetWindowText(JcB, TEXT("解除全屏按钮限制"));
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
    char buf[20] = { '\0' };
    _ultoa(LOWORD(hmenu), buf, 10);
    MessageBoxA(0, buf, buf, 0);
    return TRUE;
}

VOID CALLBACK SetWindowToTop(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
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

/*************************************************************/

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MYTHWAREHELPER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MYTHWAREHELPER));

    MSG msg;

    /*************************************************************/
    UINT_PTR timeid = SetTimer(HWND(hInst), 1, 1, SetWindowToTop);
    /*************************************************************/

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            /*************************************************************/
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
                SetWindowText(guangbotext, TEXT("广播已开启"));
            }
            else {
                Buttonable(FALSE, 2);
                CloseHandle(ClassHandle);
                SetWindowText(guangbotext, TEXT("广播未开启"));
            }
            pid = GetProcessPidFromFilename(MythwareTitle);
            if (pid != NULL) {
                GetWindowThreadProcessId(Mythware, &pid);
                MythwareHandle = OpenThread(THREAD_ALL_ACCESS, false, GetMainThreadFromId(pid));
                Buttonable(TRUE, 1);
                SetWindowText(mythwaretext, TEXT("极域已开启"));
            }
            else {
                Buttonable(FALSE, 1);
                CloseHandle(MythwareHandle);
                SetWindowText(mythwaretext, TEXT("极域未开启"));
            }
            /*************************************************************/
        }
    }
    /*************************************************************/
    KillTimer(NULL, timeid);
    /*************************************************************/

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYTHWAREHELPER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MYTHWAREHELPER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_VISIBLE | WS_OVERLAPPEDWINDOW ^ WS_MINIMIZEBOX ^ WS_MAXIMIZEBOX ^ WS_SIZEBOX,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case WM_CREATE: {
                mythwaretext = CreateWindow(TEXT("static"), TEXT(""), WS_VISIBLE | WS_CHILD, 10, 10, 150, 50, hWnd, NULL, HINSTANCE(hWnd), NULL);
                guangbotext = CreateWindow(TEXT("static"), TEXT(""), WS_VISIBLE | WS_CHILD, 10, 100, 150, 50, hWnd, NULL, HINSTANCE(hWnd), NULL);
                windowtext = CreateWindow(TEXT("static"), TEXT(""), WS_VISIBLE | WS_CHILD, 10, 350, 150, 50, hWnd, NULL, HINSTANCE(hWnd), NULL);
                SuspendB = CreateWindow(TEXT("button"), TEXT("挂起"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 190, 150, 50, hWnd, HMENU(SUSPEND_BUTTON), HINSTANCE(hWnd), NULL);
                ResumeB = CreateWindow(TEXT("button"), TEXT("恢复"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 190, 150, 50, hWnd, HMENU(RESUME_BUTTON), HINSTANCE(hWnd), NULL);
                KillB = CreateWindow(TEXT("button"), TEXT("杀死"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 260, 150, 50, hWnd, HMENU(KILL_BUTTON), HINSTANCE(hWnd), NULL);
                JcB = CreateWindow(TEXT("button"), TEXT("解除全屏按钮限制"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 100, 150, 50, hWnd, HMENU(JC_BUTTON), HINSTANCE(hWnd), NULL);
                PassWdB = CreateWindow(TEXT("button"), TEXT("复制万能密码"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 350, 150, 50, hWnd, HMENU(PASSWD_BUTTON), HINSTANCE(hWnd), NULL);
                break;
            }
                          /* Upon destruction, tell the main thread to stop */
            case WM_COMMAND: {
                switch (LOWORD(wParam)) {
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
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
