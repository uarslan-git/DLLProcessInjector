
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>

DWORD GetProcessId(const char* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);

        if (Process32First(hSnap, &pe32))
        {
            do {
                if (!strcmp((char*)pe32.szExeFile, procName)) {
                    procId = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &pe32));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

int main()
{
    const char* dllPath = "./dll.dll";
    const char* procName = "ac_client.exe";
    DWORD procId = 0;

    while (!procId) {
        procId = GetProcessId(procName);
        Sleep(30);
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

    if (hProc && hProc != INVALID_HANDLE_VALUE) {
        void* loc = VirtualAllocEx(hProc, 0, sizeof(dllPath), (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE);

        WriteProcessMemory(hProc, loc, dllPath, sizeof(dllPath) + 1, 0);

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);

        if (hThread) {
            CloseHandle(hThread);
        }

        if (hProc) {
            CloseHandle(hProc);
        }
    }
}