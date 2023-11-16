#include <Windows.h>
#include <WinUser.h>

#define OnMenuOpen 1
#define OnMenuSave 2
#define OnMenuExit 3
#define OnChangeFont 4
#define OnChangeBkColor 5

WNDCLASS NewWindowClass(HINSTANCE hInst, LPCWSTR name, WNDPROC procedure);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AddMenus(HWND hWnd);
void AddWidgets(HWND hWnd);
void InitOpenParams(HWND hWnd);
BOOL LoadTextToEdit(HWND hEdit, LPCWSTR pszFileName);
BOOL SaveFileFromEdit(HWND hEdit, LPCTSTR pszFileName);
HGDIOBJ SetFont(HWND hWnd);
HGDIOBJ SetBkColorEdit(HWND hWnd);

HWND hEditControl;
OPENFILENAME ofn;
char szFileName[MAX_PATH];
HGDIOBJ g_hfCurrentFont = GetStockObject(DEFAULT_GUI_FONT);
COLORREF crTextColor = RGB(0, 0, 0);
COLORREF crBkColor = RGB(255, 255, 255);
COLORREF crCustomBkColors[16] = { 0 };


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	WNDCLASS MainClass = NewWindowClass(hInst, L"Main Window Class", WndProc);
	if (!RegisterClass(&MainClass)) { return -1; }

	MSG MainMessage = { 0 };

	CreateWindow(L"Main Window Class", L"Text Editor: New Document",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 50, 800, 600, NULL, NULL, hInst, NULL);

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
		switch (wParam)
		{
		case OnMenuOpen:
			if (GetOpenFileName(&ofn)) {
				if (!LoadTextToEdit(hEditControl, ofn.lpstrFile)) {
					MessageBox(hWnd, L"Failed to open file", L"Error", MB_OK);
				}
				else {
					WCHAR pszNewTitle[MAX_PATH + 14] = { 0 };
					wcscpy_s(pszNewTitle, L"Text Editor: ");
					SetWindowText(hWnd, lstrcat(pszNewTitle, ofn.lpstrFile));
				}
			}
			break;
		case OnMenuSave:
			if (GetSaveFileName(&ofn)) {
				if (!SaveFileFromEdit(hEditControl, (LPCTSTR)szFileName)) {
					MessageBox(hWnd, L"Failed to save file", L"Error", MB_OK);
				}
				else {
					WCHAR pszNewTitle[MAX_PATH + 14] = { 0 };
					wcscpy_s(pszNewTitle, L"Text Editor: ");
					SetWindowText(hWnd, lstrcat(pszNewTitle, ofn.lpstrFile));
				}
			}
			break;
		case OnMenuExit:
			DestroyWindow(hWnd);
			break;
		case OnChangeFont:
			SendMessage(hEditControl, WM_SETFONT, (WPARAM)SetFont(hWnd), TRUE);
			break;
		case OnChangeBkColor:
			SendMessage(hEditControl, WM_SETFONT, (WPARAM)SetBkColorEdit(hWnd), TRUE);
			SendMessage(hEditControl, WM_SETFONT, (WPARAM)g_hfCurrentFont, TRUE);
			break;
		default:
			break;
		}
		break;
	case WM_CREATE:
		AddMenus(hWnd);
		AddWidgets(hWnd);
		InitOpenParams(hWnd);
		break;
	case WM_CTLCOLOREDIT:
		SetTextColor((HDC)wParam, crTextColor);
		SetBkColor((HDC)wParam, crBkColor);
		return (LRESULT)GetStockObject(DC_BRUSH);
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void AddMenus(HWND hWnd) {
	HMENU RootMenu = CreateMenu();
	HMENU FileMenu = CreateMenu();

	AppendMenu(RootMenu, MF_POPUP, (UINT_PTR)FileMenu, L"File");

	AppendMenu(FileMenu, MF_STRING, OnMenuOpen, L"Open");
	AppendMenu(FileMenu, MF_STRING, OnMenuSave, L"Save");

	AppendMenu(RootMenu, MF_STRING, OnChangeFont, L"Font");
	AppendMenu(RootMenu, MF_STRING, OnChangeBkColor, L"Background Color");

	AppendMenu(RootMenu, MF_STRING, OnMenuExit, L"Exit");

	SetMenu(hWnd, RootMenu);
}


void AddWidgets(HWND hWnd) {
	hEditControl = CreateWindowA("edit", NULL,
		WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL, 
		5, 5, 780, 530, hWnd, NULL, NULL, NULL);
}


void InitOpenParams(HWND hWnd) {
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"*.txt\0";
	ofn.lpstrFile = (LPWSTR)szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"txt";
}


BOOL LoadTextToEdit(HWND hEdit, LPCWSTR pszFileName) {
	HANDLE hFile;
	BOOL bSuccess = false;
	
	hFile = CreateFile(pszFileName, GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize != 0xFFFFFFFF) {
			LPSTR pszFileText = (LPSTR)GlobalAlloc(GPTR, dwFileSize + 1);
			
			if (pszFileText != NULL) {
				DWORD dwRead;

				if (ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL)) {
					pszFileText[dwFileSize] = { 0 };
					if (SetWindowTextA(hEdit, pszFileText)) {
						bSuccess = true;
					}
				}
				GlobalFree(pszFileText);
			}
		}
		CloseHandle(hFile);
	}
	return bSuccess;
}


BOOL SaveFileFromEdit(HWND hEdit, LPCTSTR pszFileName) {
	HANDLE hFile;
	BOOL bSuccess = false;

	hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwTextLength = GetWindowTextLengthA(hEdit);

		DWORD dwBufferSize = dwTextLength + 1;
		LPSTR pszText = (LPSTR)GlobalAlloc(GPTR, dwBufferSize);

		if (pszText != NULL) {
			if (GetWindowTextA(hEdit, pszText, dwBufferSize) == dwTextLength) {
				DWORD dwWritten;
				if (WriteFile(hFile, pszText, dwTextLength, &dwWritten, NULL)) {
					bSuccess = true;
				}
			}
			GlobalFree(pszText);
		}
		CloseHandle(hFile);
	}
	return bSuccess;
}


HGDIOBJ SetFont(HWND hWnd) {
	CHOOSEFONT cf = { sizeof(CHOOSEFONT) };
	LOGFONT lf;
	GetObject(g_hfCurrentFont, sizeof(LOGFONT), &lf);

	cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &lf;
	cf.rgbColors = crTextColor;

	if (ChooseFont(&cf)) {
		HFONT hf = CreateFontIndirect(&lf);
		if (hf) {
			g_hfCurrentFont = hf;
		}
		// set current text color
		crTextColor = cf.rgbColors;
	}
	else
	{
		MessageBox(hWnd, L"Font creation failed!", L"Error", MB_OK | MB_ICONEXCLAMATION);
	}
	
	return g_hfCurrentFont;
}

HGDIOBJ SetBkColorEdit(HWND hWnd) {
	CHOOSECOLOR cc = { sizeof(CHOOSECOLOR) };

	cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
	cc.hwndOwner = hWnd;
	cc.rgbResult = crBkColor;
	cc.lpCustColors = crCustomBkColors;

	if (ChooseColor(&cc))
	{
		crBkColor = cc.rgbResult;
	}
	return (HGDIOBJ)crBkColor;
}
