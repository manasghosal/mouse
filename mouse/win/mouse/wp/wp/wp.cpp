// wp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "wp.h"
#include <stdio.h>      /* printf */
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <math.h>       /* sqrt */
#include <windows.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
extern BOOL facing_me();

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
extern int run_networkers();
// Variable to store the hook handle
HHOOK miHook;
int letsIgnoreThisClick = false;
POINT l_click_pt;
WPARAM ms_evt = 0;


// This is the hook procedure
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    extern DWORD Evt, X, Y;
    extern DWORD Evt_bk, X_bk, Y_bk;

    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT& msll = *(reinterpret_cast<MSLLHOOKSTRUCT*>(lParam)); // In there is more context if you need it
        ms_evt = wParam;
        X = msll.pt.x;
        Y = msll.pt.y;
        Evt = ms_evt;
        if (wParam == WM_LBUTTONDOWN) {
            l_click_pt = msll.pt;
            // Do your logic here
            if (letsIgnoreThisClick) {

                return -1; // This will make the click be ignored
            }
        }
    }

    return CallNextHookEx(miHook, nCode, wParam, lParam); // Important! Otherwise other mouse hooks may misbehave
}


DWORD WINAPI ThreadFunc(void* data) {
    // Do stuff.  This will be the first function called on the new thread.
    // When this function returns, the thread goes away.  See MSDN for more details.
    //const clock_t begin_time = clock();
    //// do something
    //float t_diff = 0;

    //while (t_diff < 100.0)
    //{
    //    letsIgnoreThisClick = true;
    //    t_diff = (float(clock() - begin_time) / CLOCKS_PER_SEC);
    //    if (ms_evt == WM_LBUTTONDOWN)
    //    {
    //        int x = l_click_pt.x;
    //        int y = l_click_pt.y;
    //    }
    //}
    //letsIgnoreThisClick = false;
    while (true)
    {
        if (ms_evt == WM_LBUTTONDOWN)
        {
            if (!facing_me())
                letsIgnoreThisClick = true;
            else
                letsIgnoreThisClick = false;
        }
        Sleep(200);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    //// Initialize global strings
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_WP, szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    //// Perform application initialization:
    //if (!InitInstance (hInstance, nCmdShow))
    //{
    //    return FALSE;
    //}

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WP));

    miHook = SetWindowsHookEx(WH_MOUSE_LL, reinterpret_cast<HOOKPROC>(&LowLevelMouseProc), hInstance, 0);

    HANDLE thread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
    run_networkers();
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
    UnhookWindowsHookEx(miHook);

    return 0;// (int)msg.wParam;
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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
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
