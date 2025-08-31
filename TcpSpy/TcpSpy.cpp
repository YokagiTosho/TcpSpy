// TcpSpy.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "TcpSpy.h"

#include "libTcpSpy/ConnectionTableRegistry.hpp"
#include "ListView.hpp"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ListView::pointer listView;
ConnectionsTableRegistry connectionsRegistry;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void InitCommonControls();
void InitWSA();
void HandleWM_NOTIFY(LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	InitWSA();
	InitCommonControls();
	

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_TCPSPY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);



	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCPSPY));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TCPSPY));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_TCPSPY);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	listView = std::make_unique<ListView>(hWnd);

	listView->init_list({
	   L"Process name",
	   L"PID",
	   L"Protocol",
	   L"IP version",
	   L"Local address",
	   L"Local port",
	   L"Remote address",
	   L"Remote port",
	   L"State"
		});

	connectionsRegistry.update();

	listView->insert_items(connectionsRegistry.size());

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	case WM_CREATE:

		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_NOTIFY:
		HandleWM_NOTIFY(lParam);
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_SIZE:
		listView->resize();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
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

void InitCommonControls() {
	INITCOMMONCONTROLSEX icc{};
	icc.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icc);
}



void ProcessListViewEntry(LPARAM lParam) {
	// 0 - proc name, 1 - pid, 2 - loc addr, 3 - loc port, 4 - rem addr, 5 - rem port
	// todo make it enum
#define PROC_NAME 0
#define PID       1
#define PROTOCOL  2
#define IPVERSION 3
#define LOC_ADDR  4
#define LOC_PORT  5
#define REM_ADDR  6
#define REM_PORT  7
#define STATE     8
	NMLVDISPINFO* plvdi;
	WCHAR buff[512];
	ZeroMemory(buff, sizeof(buff));

	plvdi = (NMLVDISPINFO*)lParam;

	/* It is very strange that microsoft documentation suggests to point in NMLVDISPINFO to stack memory "buff".
	But it works, maybe because its accessed right after WndProc and not yet corrupted to that moment */

	//const ConnectionRowInfo& row = g_connectionsInfo[plvdi->item.iItem];
	auto &row = connectionsRegistry.get()[plvdi->item.iItem];
#define BUF_LEN 512
	static WCHAR buf[BUF_LEN];

	switch (plvdi->item.iSubItem)
	{
	case PROC_NAME:
		lstrcpynW(buf, (LPWSTR)row->get_process_name().c_str(), BUF_LEN);
		break;
	case PID:
		lstrcpynW(buf, (LPWSTR)row->pid_str().c_str(), BUF_LEN);
		break;
	case PROTOCOL:
		switch (row->protocol()) {
		case ConnectionProtocol::PROTO_TCP:
			plvdi->item.pszText = (LPWSTR)L"TCP";
			return;
		case ConnectionProtocol::PROTO_UDP:
			plvdi->item.pszText = (LPWSTR)L"UDP";
			return;
		default:
			assert(false);
			break;
		}
		break;
	case IPVERSION:
		switch (row->address_family()) {
		case ProtocolFamily::INET:
			plvdi->item.pszText = (LPWSTR)L"IPv4";
			break;
		case ProtocolFamily::INET6:
			plvdi->item.pszText = (LPWSTR)L"IPv6";
			break;
		}
		return; // return so plvdi->item.pszText is not assigned at the end of function
		break;
	case LOC_ADDR:
		lstrcpynW(buf, (LPWSTR)row->local_addr_str().c_str(), BUF_LEN);
		break;
	case LOC_PORT:
		lstrcpynW(buf, (LPWSTR)row->local_port_str().c_str(), BUF_LEN);
		break;
	case REM_ADDR:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			lstrcpynW(buf, (LPWSTR)p->remote_addr_str().c_str(), BUF_LEN);
		}
		else {
			return;
		}
		break;
	case REM_PORT:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			lstrcpynW(buf, (LPWSTR)p->remote_port_str().c_str(), BUF_LEN);
		}
		else {
			return;
		}
		break;
	case STATE:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			lstrcpynW(buf, (LPWSTR)p->state_str().c_str(), BUF_LEN);
		}
		else {
			return;
		}
		break;
	default:
		return;
	}
	plvdi->item.pszText = buf;
}

void HandleWM_NOTIFY(LPARAM lParam) {
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_GETDISPINFO:
		ProcessListViewEntry(lParam);
		break;
	}
}

void InitWSA() {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
		exit(1);
	}
}