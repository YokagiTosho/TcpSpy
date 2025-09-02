#include "framework.h"
#include "TcpSpy.h"

#include <algorithm>
#include <strsafe.h>

#include "libTcpSpy/ConnectionTableRegistry.hpp"
#include "ListView.hpp"

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ListView::pointer listView;
ConnectionsTableRegistry connectionsRegistry;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

#if 0
#define PROC_NAME 0
#define PID       1
#define PROTOCOL  2
#define IPVERSION 3
#define LOC_ADDR  4
#define LOC_PORT  5
#define REM_ADDR  6
#define REM_PORT  7
#define STATE     8
#endif

enum ListViewColumns {
	PROC_NAME,
	PID		 ,
	PROTOCOL ,
	IPVERSION,
	LOC_ADDR ,
	LOC_PORT ,
	REM_ADDR ,
	REM_PORT ,
	STATE	 ,
};

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
	
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_TCPSPY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TCPSPY));

	MSG msg;

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

	// init windows that depend on main window, initialize them. (Maybe should be moved to WM_CREATE in WndProc)
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

	listView->insert_items(connectionsRegistry);

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
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_VIEW_REFRESH:
			connectionsRegistry.update();
			listView->insert_items(connectionsRegistry);
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

	NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lParam;

	auto &row = connectionsRegistry.get()[plvdi->item.iItem];

	constexpr auto BUF_LEN = 512;
	static WCHAR buf[BUF_LEN];
	std::wstring tmp;

	switch (plvdi->item.iSubItem)
	{
	case ListViewColumns::PROC_NAME:
		tmp = row->get_process_name().c_str();
		break;
	case ListViewColumns::PID:
		tmp = row->pid_str().c_str();
		break;
	case ListViewColumns::PROTOCOL:
		switch (row->protocol()) {
		case ConnectionProtocol::PROTO_TCP:
			plvdi->item.pszText = (LPWSTR)L"TCP";
			break;
		case ConnectionProtocol::PROTO_UDP:
			plvdi->item.pszText = (LPWSTR)L"UDP";
			break;
		default:
			assert(false);
			break;
		}
		return;
	case ListViewColumns::IPVERSION:
		switch (row->address_family()) {
		case ProtocolFamily::INET:
			plvdi->item.pszText = (LPWSTR)L"IPv4";
			break;
		case ProtocolFamily::INET6:
			plvdi->item.pszText = (LPWSTR)L"IPv6";
			break;
		}
		return; // return so plvdi->item.pszText is not assigned at the end of function
	case ListViewColumns::LOC_ADDR:
		tmp = row->local_addr_str().c_str();
		break;
	case ListViewColumns::LOC_PORT:
		tmp = row->local_port_str().c_str();
		break;
	case ListViewColumns::REM_ADDR:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			tmp = p->remote_addr_str().c_str();
		}
		else {
			return;
		}
		break;
	case ListViewColumns::REM_PORT:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			tmp = p->remote_port_str().c_str();
		}
		else {
			return;
		}
		break;
	case ListViewColumns::STATE:
		if (auto p = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
			tmp = p->state_str().c_str();
		}
		else {
			return;
		}
		break;
	default:
		return;
	}
	HRESULT res = StringCchCopyW(buf, BUF_LEN, (LPWSTR)tmp.c_str());

	if (res == STRSAFE_E_INVALID_PARAMETER) {
		throw std::invalid_argument("The value in cchDest is either 0 or larger than STRSAFE_MAX_CCH");
	}

	plvdi->item.pszText = buf;
}

void SortColumn(LPARAM lParam) {
	LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

	switch (pnmv->iSubItem) {
	case ListViewColumns::PROC_NAME:
		connectionsRegistry.sort(SortBy::ProcessName);
		break;
	case ListViewColumns::PID:
		connectionsRegistry.sort(SortBy::PID);
		break;
	case ListViewColumns::PROTOCOL:
		break;
	case ListViewColumns::IPVERSION:
		break;
	case ListViewColumns::LOC_ADDR:
		break;
	case ListViewColumns::LOC_PORT:
		break;
	case ListViewColumns::REM_ADDR:
		break;
	case ListViewColumns::REM_PORT:
		break;
	case ListViewColumns::STATE:
		break;
	}

	listView->insert_items(connectionsRegistry);
}

void HandleWM_NOTIFY(LPARAM lParam) {
	switch (((NMHDR*)lParam)->code)
	{
	case LVN_GETDISPINFO:
		ProcessListViewEntry(lParam);
		break;
	case LVN_COLUMNCLICK:
		SortColumn(lParam);
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