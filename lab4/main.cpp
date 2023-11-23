#pragma comment(lib, "comctl32.lib")
#include <Windows.h>
#include <psapi.h>
#include "commctrl.h"
#include <TlHelp32.h>

#define IDT_TIMER1 1


WNDCLASS NewWindowClass(HINSTANCE hInst, LPCWSTR name, WNDPROC procedure);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdateProcessesInfo(HWND hWnd);
void CreateNewProcess(HWND hWnd);
void SuspendSelectedProcess(HWND hWnd);
void ResumeSelectedProcess(HWND hWnd);
void TerminateSelectedProcess(HWND hWnd);
void AddWidgets(HWND hWnd);

void CALLBACK TimeProc(UINT  uID, UINT  uMsg, DWORD dwUser, DWORD dw1, DWORD dw2, HWND hWnd);


HINSTANCE hInstance;
HWND hWndListView;
HWND hWndCreateProcessButton;
HWND hWndSuspendProcessButton;
HWND hWndResumeProcessButton;
HWND hWndTerminateProcessButton;

char szFileName[MAX_PATH];


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
		if (lParam == (LPARAM)hWndCreateProcessButton) {
			CreateNewProcess(hWnd);
		}
		else if (lParam == (LPARAM)hWndSuspendProcessButton) {
			SuspendSelectedProcess(hWnd);
		}
		else if (lParam == (LPARAM)hWndResumeProcessButton) {
			ResumeSelectedProcess(hWnd);
		}
		else if (lParam == (LPARAM)hWndTerminateProcessButton) {
			TerminateSelectedProcess(hWnd);
		}
		break;
	case WM_CREATE:
		SetTimer(hWnd, IDT_TIMER1, 1000, (TIMERPROC)UpdateProcessesInfo);
		AddWidgets(hWnd);
		UpdateProcessesInfo(hWnd);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		KillTimer(hWnd, IDT_TIMER1);
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

	hWndCreateProcessButton = CreateWindow(L"BUTTON", L"Create process",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, 414, 130, 20, hWnd, NULL, hInstance, NULL);
	hWndSuspendProcessButton = CreateWindow(L"BUTTON", L"Suspend process",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		150, 414, 130, 20, hWnd, NULL, hInstance, NULL);
	hWndResumeProcessButton = CreateWindow(L"BUTTON", L"Resume process",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		290, 414, 130, 20, hWnd, NULL, hInstance, NULL);
	hWndTerminateProcessButton = CreateWindow(L"BUTTON", L"Terminate process",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		430, 414, 140, 20, hWnd, NULL, hInstance, NULL);
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
	UINT count = 0;
	DWORD dwTotalitems = ListView_GetItemCount(hWndListView);
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

				if (count >= dwTotalitems) {
					LVITEM lvItem;
					lvItem.mask = LVIF_TEXT;
					lvItem.iItem = count;
					lvItem.iSubItem = 0;
					lvItem.pszText = new wchar_t[0];
					ListView_InsertItem(hWndListView, &lvItem);
				}

				wchar_t buffer[MAX_PATH];
				wsprintf(szInfo, TEXT("%lu"), aProcesses[i]);
				ListView_SetItemText(hWndListView, count, 0, szInfo);
				wsprintf(szInfo, TEXT("%s"), szProcessName);
				ListView_SetItemText(hWndListView, count, 1, szInfo);

				ListView_SetItemText(hWndListView, count, 2, szWorkingSet);
				count++;
			}
		}
	}
	if (count < dwTotalitems) {
		for (int i = count; i < dwTotalitems; i++) {
			ListView_DeleteItem(hWndListView, i);
		}
	}
}

void CreateNewProcess(HWND hWnd) {
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"Executable Files\0*.exe\0";
	ofn.lpstrFile = (LPWSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"exe";

	if (GetOpenFileName(&ofn)) {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		if (CreateProcess(ofn.lpstrFile, NULL,
			NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else {
			MessageBox(hWnd, L"Failed to create new process.", L"Error",
				MB_OK | MB_ICONERROR);
		}
	}
}

void SuspendSelectedProcess(HWND hWnd) {
	UINT iItem = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
	WCHAR text[MAX_PATH] = { 0 };
	ListView_GetItemText(hWndListView, iItem, 0, (LPWSTR)text, sizeof(text));
	if (iItem < 0) {
		MessageBox(hWnd, L"Please select a process to suspend.",
			L"No process selected", MB_OK | MB_ICONERROR);
		return;
	}
	DWORD dwProcessId = wcstoul(text, NULL, 10);
	HANDLE hTreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	if (Thread32First(hTreadSnapshot, &te32)) {
		do {
			if (te32.th32OwnerProcessID == dwProcessId) {
				// suspend thread
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,
					FALSE, te32.th32ThreadID);
				if (hThread == NULL) {
					MessageBox(hWnd, L"Failed to suspend process.",
						L"Error", MB_OK | MB_ICONERROR);
					break;
				}
				SuspendThread(hThread);
				CloseHandle(hThread);
			}
		} while (Thread32Next(hTreadSnapshot, &te32));
	}
	else {
		MessageBox(hWnd, L"Failed to suspend process.",
			L"Error", MB_OK | MB_ICONERROR);
	}
	CloseHandle(hTreadSnapshot);
}

void ResumeSelectedProcess(HWND hWnd) {
	UINT iItem = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
	WCHAR text[MAX_PATH] = { 0 };
	ListView_GetItemText(hWndListView, iItem, 0, (LPWSTR)text, sizeof(text));
	if (iItem < 0) {
		MessageBox(hWnd, L"Please select a process to resume.",
			L"No process selected", MB_OK | MB_ICONERROR);
		return;
	}
	DWORD dwProcessId = wcstoul(text, NULL, 10);
	HANDLE hTreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	if (Thread32First(hTreadSnapshot, &te32)) {
		do {
			if (te32.th32OwnerProcessID == dwProcessId) {
				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME,
					FALSE, te32.th32ThreadID);
				if (hThread == NULL) {
					MessageBox(hWnd, L"Failed to resume process.",
						L"Error", MB_OK | MB_ICONERROR);
					break;
				}
				ResumeThread(hThread);
				CloseHandle(hThread);
			}
		} while (Thread32Next(hTreadSnapshot, &te32));
	}
	else {
		MessageBox(hWnd, L"Failed to resume process.",
			L"Error", MB_OK | MB_ICONERROR);
	}
	CloseHandle(hTreadSnapshot);
}

void TerminateSelectedProcess(HWND hWnd) {
	UINT iItem = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
	WCHAR text[MAX_PATH] = { 0 };
	ListView_GetItemText(hWndListView, iItem, 0, (LPWSTR)text, sizeof(text));
	if (iItem < 0) {
		MessageBox(hWnd, L"Please select a process to terminate.",
			L"Process not selected", MB_OK | MB_ICONERROR);
		return;
	}
	DWORD processId = wcstoul(text, NULL, 10);
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
	if (hProcess == NULL) {
		MessageBox(hWnd, L"Failed to open process.", L"Error", MB_OK | MB_ICONERROR);
	}
	else {
		TerminateProcess(hProcess, -1);
		WaitForSingleObject(hProcess, NULL);
		CloseHandle(hProcess);
	}
}

void CALLBACK TimeProc(UINT  uID, UINT  uMsg, DWORD dwUser, DWORD dw1, DWORD dw2, HWND hWnd) {
	UpdateProcessesInfo(hWnd);
}
