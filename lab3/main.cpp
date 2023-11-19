#pragma comment(lib, "comctl32.lib")
#include <Windows.h>
#include <psapi.h>
#include "commctrl.h"


WNDCLASS NewWindowClass(HINSTANCE hInst, LPCWSTR name, WNDPROC procedure);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdateProcessesInfo(HWND hWnd);
LPWSTR GetProcessInfo(DWORD processId);
void AddWidgets(HWND hWnd);

HINSTANCE hInstance;
HWND hWndListView;
HWND hWndUpdateButton;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	hInstance = hInst;
	WNDCLASS MainClass = NewWindowClass(hInst, L"Main Window Class", WndProc);
	if (!RegisterClass(&MainClass)) { return -1; }

	MSG MainMessage = { 0 };

	CreateWindow(L"Main Window Class", L"Memory Usage",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 50, 640, 480, NULL, NULL, hInst, NULL);

	while (GetMessage(&MainMessage, NULL, NULL, NULL)) {
		TranslateMessage(&MainMessage);
		DispatchMessage(&MainMessage);
	}
}


WNDCLASS NewWindowClass(HINSTANCE hInst, LPCWSTR name, WNDPROC procedure) {
	WNDCLASS windowClass = {};
	windowClass.hInstance = hInst;
	windowClass.lpszClassName = name;
	windowClass.lpfnWndProc = procedure;

	return windowClass;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (lParam == (LPARAM)hWndUpdateButton) {
			UpdateProcessesInfo(hWnd);
		}
		break;
	case WM_CREATE:
		AddWidgets(hWnd);
		UpdateProcessesInfo(hWnd);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void AddWidgets(HWND hWnd)
{
	InitCommonControls();
	hWndListView = CreateWindow(WC_LISTVIEW, L"",
		WS_VISIBLE | WS_BORDER | WS_CHILD | LVS_REPORT,
		0, 0, 625, 406, hWnd, NULL, hInstance, 0);

	ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);

	LVCOLUMN lvcolId;
	lvcolId.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvcolId.pszText = LPWCH(L"ID");
	lvcolId.cx = 90;
	ListView_InsertColumn(hWndListView, 0, &lvcolId);

	LVCOLUMN lvcolName;
	lvcolId.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvcolName.pszText = LPWCH(L"Name");
	ListView_InsertColumn(hWndListView, 1, &lvcolName);
	ListView_SetColumnWidth(hWndListView, 1, 320);

	LVCOLUMN lvcolMemory;
	lvcolId.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvcolMemory.pszText = LPWCH(L"Memory");
	ListView_InsertColumn(hWndListView, 2, &lvcolMemory);
	ListView_SetColumnWidth(hWndListView, 2, 195);

	hWndUpdateButton = CreateWindow(L"BUTTON", L"Update",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, 414, 80, 20, hWnd, NULL, hInstance, NULL);
}

void UpdateProcessesInfo(HWND hWnd) {
	// array of process ids
	DWORD aProcesses[1024];
	// number of bytes returned in process ids array
	DWORD cbNeeded;
	// number of process ids returned
	DWORD cProcesses;
	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
		MessageBox(hWnd, L"Failed to get process ids!", L"Error", MB_OK);
		return;
	}
	cProcesses = cbNeeded / sizeof(DWORD);
	ListView_DeleteAllItems(hWndListView);
	UINT count = 0;
	for (UINT i = 0; i < cProcesses; i++) {
		if (aProcesses[i] != 0) {
			TCHAR szProcessName[MAX_PATH] = L"<unknown>";
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE, aProcesses[i]);
			PROCESS_MEMORY_COUNTERS pmc;
			WCHAR szInfo[MAX_PATH]; 
			WCHAR szWorkingSet[100];

			if (hProcess != NULL) {
				HMODULE hModule;
				DWORD cbNeeded;
				if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
					GetModuleBaseName(hProcess, hModule, szProcessName,
						sizeof(szProcessName) / sizeof(TCHAR));
					GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
					wsprintf(szWorkingSet, TEXT("%lu bytes"), pmc.WorkingSetSize);
				}
				CloseHandle(hProcess);

				LVITEM lvItem;
				lvItem.mask = LVIF_TEXT;
				lvItem.iItem = count;
				lvItem.iSubItem = 0;
				lvItem.pszText = new wchar_t[0];
				ListView_InsertItem(hWndListView, &lvItem);

				wchar_t buffer[MAX_PATH];
				wsprintf(szInfo, TEXT("%lu"), aProcesses[i]);
				ListView_SetItemText(hWndListView, count, 0, szInfo)

				wsprintf(szInfo, TEXT("%s"), szProcessName);
				ListView_SetItemText(hWndListView, count, 1, szInfo);
				
				ListView_SetItemText(hWndListView, count, 2, szWorkingSet);
				count++;
			}
		}
	}
}
