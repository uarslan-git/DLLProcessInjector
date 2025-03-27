#include <windows.h>
#include <TlHelp32.h>
#include <commctrl.h>
#include <string>
#include <algorithm>

// Dark mode colors
const COLORREF DARK_BG = RGB(30, 30, 30);
const COLORREF DARK_TEXT = RGB(255, 255, 255);
const COLORREF DARK_BUTTON = RGB(50, 50, 50);
HBRUSH hDarkBrush = CreateSolidBrush(DARK_BG);

// Global controls
HWND hProcessList, hDllPath, hInjectButton, hRefreshButton, hBrowseButton, hSearchBox;
char dllPath[MAX_PATH] = { 0 };

DWORD GetProcessId(const char* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_stricmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

void PopulateProcessList(const char* filter = "") {
    SendMessage(hProcessList, LB_RESETCONTENT, 0, 0);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                std::string processName = procEntry.szExeFile;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);

                std::string filterStr = filter;
                std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);
                if (filterStr.empty() || processName.find(filterStr) != std::string::npos) {
                    SendMessageA(hProcessList, LB_ADDSTRING, 0, (LPARAM)procEntry.szExeFile);
                }
            } while (Process32Next(hSnap, &procEntry));
        }
        CloseHandle(hSnap);
    }
}

void InjectDLL() {
    char procName[MAX_PATH] = { 0 };
    int index = SendMessage(hProcessList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR) return;

    SendMessageA(hProcessList, LB_GETTEXT, index, (LPARAM)procName);

    DWORD procId = GetProcessId(procName);
    if (!procId) return;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);
    if (hProc) {
        void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (loc) {
            WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0);
            CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
        }
        CloseHandle(hProc);
    }
}

void BrowseDLL() {
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFile = dllPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "DLL Files\0*.dll\0";
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        SetWindowTextA(hDllPath, dllPath);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Create controls with dark mode styles
        hSearchBox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 10, 250, 28, hWnd, NULL, NULL, NULL);

        hRefreshButton = CreateWindow("BUTTON", "Refresh",
            WS_CHILD | WS_VISIBLE | BS_FLAT,
            270, 10, 80, 28, hWnd, (HMENU)1, NULL, NULL);

        hProcessList = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | LBS_STANDARD | WS_VSCROLL | LBS_NOTIFY,
            10, 45, 250, 150, hWnd, NULL, NULL, NULL);

        hBrowseButton = CreateWindow("BUTTON", "Browse DLL",
            WS_CHILD | WS_VISIBLE | BS_FLAT,
            10, 200, 90, 28, hWnd, (HMENU)2, NULL, NULL);

        hDllPath = CreateWindow("STATIC", "",
            WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            110, 200, 250, 28, hWnd, NULL, NULL, NULL);

        hInjectButton = CreateWindow("BUTTON", "Inject",
            WS_CHILD | WS_VISIBLE | BS_FLAT,
            10, 235, 90, 28, hWnd, (HMENU)3, NULL, NULL);

        // Set fonts
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, "Segoe UI");

        SendMessage(hSearchBox, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hRefreshButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hProcessList, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBrowseButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hDllPath, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hInjectButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        PopulateProcessList();
        break;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, DARK_TEXT);
        SetBkColor(hdc, DARK_BG);
        return (LRESULT)hDarkBrush;
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, DARK_TEXT);
        SetBkColor(hdc, DARK_BUTTON);
        return (LRESULT)CreateSolidBrush(DARK_BUTTON);
    }

    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect((HDC)wParam, &rc, hDarkBrush);
        return 1;
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == hSearchBox) {
            char filter[256] = { 0 };
            GetWindowTextA(hSearchBox, filter, 256);
            PopulateProcessList(filter);
        }
        else if (LOWORD(wParam) == 1) { // Refresh
            SetWindowTextA(hSearchBox, "");
            PopulateProcessList();
        }
        else if (LOWORD(wParam) == 2) { // Browse
            BrowseDLL();
        }
        else if (LOWORD(wParam) == 3) { // Inject
            InjectDLL();
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        DeleteObject(hDarkBrush);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "DarkInjector";
    wc.hbrBackground = hDarkBrush;

    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(0, "DarkInjector", "DLL Injector",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 375, 310,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}