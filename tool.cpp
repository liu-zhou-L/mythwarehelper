#include <tlhelp32.h>

DWORD GetProcessIDFromName(LPCSTR szName)
{
    DWORD id = 0;       // ����ID
    PROCESSENTRY32 pe;  // ������Ϣ
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // ��ȡϵͳ�����б�
    if(Process32First(hSnapshot, &pe))      // ����ϵͳ�е�һ�����̵���Ϣ
    {
        do
        {
            if(0 == _stricmp(pe.szExeFile, szName)) // �����ִ�Сд�Ƚ�
            {
                id = pe.th32ProcessID;
                break;
            }
        }while(Process32Next(hSnapshot, &pe));      // ��һ������
    }
    CloseHandle(hSnapshot);     // ɾ������
    return id;
}

DWORD GetMainThreadIdFromName(LPCSTR szName)
{
    DWORD idThread = 0;         // ����ID
    DWORD idProcess = 0;        // ���߳�ID

    // ��ȡ����ID
    PROCESSENTRY32 pe;      // ������Ϣ
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // ��ȡϵͳ�����б�
    if(Process32First(hSnapshot, &pe))      // ����ϵͳ�е�һ�����̵���Ϣ
    {
        do
        {
            if(0 == _stricmp(pe.szExeFile, szName)) // �����ִ�Сд�Ƚ�
            {
                idProcess = pe.th32ProcessID;
                break;
            }
        }while(Process32Next(hSnapshot, &pe));      // ��һ������
    }
    CloseHandle(hSnapshot); // ɾ������
    if (idProcess == 0)
    {
        return 0;
    }

    // ��ȡ���̵����߳�ID
    THREADENTRY32 te;       // �߳���Ϣ
    te.dwSize = sizeof(THREADENTRY32);
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); // ϵͳ�����߳̿���
    if(Thread32First(hSnapshot, &te))       // ��һ���߳�
    {
        do
        {
            if(idProcess == te.th32OwnerProcessID)      // ��Ϊ�ҵ��ĵ�һ���ý��̵��߳�Ϊ���߳�
            {
                idThread = te.th32ThreadID;
                break;
            }
        }while(Thread32Next(hSnapshot, &te));           // ��һ���߳�
    }
    CloseHandle(hSnapshot); // ɾ������
    return idThread;
}
