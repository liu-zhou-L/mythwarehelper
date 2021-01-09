#include <tlhelp32.h>

DWORD GetProcessIDFromName(LPCSTR szName)
{
    DWORD id = 0;       // 进程ID
    PROCESSENTRY32 pe;  // 进程信息
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // 获取系统进程列表
    if(Process32First(hSnapshot, &pe))      // 返回系统中第一个进程的信息
    {
        do
        {
            if(0 == _stricmp(pe.szExeFile, szName)) // 不区分大小写比较
            {
                id = pe.th32ProcessID;
                break;
            }
        }while(Process32Next(hSnapshot, &pe));      // 下一个进程
    }
    CloseHandle(hSnapshot);     // 删除快照
    return id;
}

DWORD GetMainThreadIdFromName(LPCSTR szName)
{
    DWORD idThread = 0;         // 进程ID
    DWORD idProcess = 0;        // 主线程ID

    // 获取进程ID
    PROCESSENTRY32 pe;      // 进程信息
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // 获取系统进程列表
    if(Process32First(hSnapshot, &pe))      // 返回系统中第一个进程的信息
    {
        do
        {
            if(0 == _stricmp(pe.szExeFile, szName)) // 不区分大小写比较
            {
                idProcess = pe.th32ProcessID;
                break;
            }
        }while(Process32Next(hSnapshot, &pe));      // 下一个进程
    }
    CloseHandle(hSnapshot); // 删除快照
    if (idProcess == 0)
    {
        return 0;
    }

    // 获取进程的主线程ID
    THREADENTRY32 te;       // 线程信息
    te.dwSize = sizeof(THREADENTRY32);
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); // 系统所有线程快照
    if(Thread32First(hSnapshot, &te))       // 第一个线程
    {
        do
        {
            if(idProcess == te.th32OwnerProcessID)      // 认为找到的第一个该进程的线程为主线程
            {
                idThread = te.th32ThreadID;
                break;
            }
        }while(Thread32Next(hSnapshot, &te));           // 下一个线程
    }
    CloseHandle(hSnapshot); // 删除快照
    return idThread;
}
