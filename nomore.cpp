#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shellapi.h>

#include <tlhelp32.h>
#include <winhttp.h>

#include <json\json.h>

#include "resource.h"

#pragma comment(lib, "Winhttp.lib")

#define NOMORE_CLASSNAME        L"NoMore"
#define	NOMORE_WM_SHELLICON     WM_USER + 1
#define NOMORE_WM_TIMER         10000

#define NOMORE_MENU_ABOUT       10000
#define NOMORE_MENU_EXIT        10001

#define NOMORE_TIP              L"NoMore"
#define NOMORE_INFOTITLE        L"NoMore"
#define NOMORE_INFO             L"NoMore is running."
#define NOMORE_MUTEX            L"Global\\NoMore_Mutex"

#define NOMORE_ELAPSE            3 * 1000
#define NOMORE_ELAPSE_LONG      60 * 1000

#define NOMORE_KEY              VK_LCONTROL

NOTIFYICONDATAW nid = { 0, };
HMENU hMenu;
UINT_PTR hTimer = NULL;

bool checkLatestRelease();
LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
inline void NoMoreInsertMenu(HMENU hMenu, UINT uMenuIndex, UINT uidCmd, LPCWSTR dwTypedata);

void CALLBACK noMore(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    if (nIDEvent == NOMORE_WM_TIMER)
    {
        if (hTimer != NULL)
        {
            KillTimer(hWnd, hTimer);
            SetTimer(hWnd, NOMORE_WM_TIMER, NOMORE_ELAPSE_LONG, noMore);
            hTimer = NULL;
        }

        HWND hwnd_foreground = GetForegroundWindow();

        HWND hwnd = NULL;

        LASTINPUTINFO lii = { 0 , };
        lii.cbSize = sizeof(LASTINPUTINFO);

        while (true)
        {
            hwnd = FindWindowExW(NULL, hwnd, L"FFXIVGAME", NULL);
            if (hwnd == NULL)
                break;

            lii.dwTime = 0;
            if (
                hwnd != hwnd_foreground ||
                (
                    GetLastInputInfo(&lii) != 0 &&
                    (GetTickCount64() - lii.dwTime) > NOMORE_ELAPSE_LONG
                )
            )
            {
                PostMessageW(hwnd, WM_KEYDOWN, NOMORE_KEY, 0);
                Sleep(100);
                PostMessageW(hwnd, WM_KEYUP, NOMORE_KEY, 0);
            }
        };
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int cmdShow)
{
    if (CreateMutexW(NULL, FALSE, NOMORE_MUTEX) == NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return 0;
    }

#ifndef _DEBUG
    if (!checkLatestRelease())
    {
        ShellExecuteW(0, 0, L"https://github.com/RyuaNerin/NoMore/releases/latest", 0, 0, SW_SHOW);
        return 0;
    }
#endif

#pragma region RegisterClassExW
    WNDCLASSEXW wcex = { 0, };
    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = NOMORE_CLASSNAME;
    if (RegisterClassExW(&wcex) == NULL)
        return 1;
#pragma endregion

#pragma region CreateWindowExW
    auto hWnd = CreateWindowExW(0, NOMORE_CLASSNAME, NULL, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return 1;
#pragma endregion

#pragma region Shell_NotifyIconW
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = (HWND)hWnd;
    nid.uID              = IDI_ICON;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.hIcon            = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON));
    nid.uCallbackMessage = NOMORE_WM_SHELLICON;
    wcscpy_s(nid.szTip,       ARRAYSIZE(nid.szTip      ), NOMORE_TIP);
    wcscpy_s(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), NOMORE_INFOTITLE);
    wcscpy_s(nid.szInfo,      ARRAYSIZE(nid.szInfo     ), NOMORE_INFO);

    Shell_NotifyIconW(NIM_ADD, &nid);
#pragma endregion

    hMenu = CreatePopupMenu();
    NoMoreInsertMenu(hMenu, 0, NOMORE_MENU_ABOUT, L"By RyuaNerin");
    NoMoreInsertMenu(hMenu, 1, NOMORE_MENU_EXIT, L"Exit");
    
    hTimer = SetTimer(hWnd, NOMORE_WM_TIMER, NOMORE_ELAPSE, noMore);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

inline void NoMoreInsertMenu(HMENU hMenu, UINT uMenuIndex, UINT uidCmd, LPCWSTR dwTypedata)
{
    MENUITEMINFOW mii = { 0, };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
    mii.wID = uidCmd;
    mii.fType = MFT_STRING;
    mii.dwTypeData = (LPWSTR)dwTypedata;
    mii.fState = MFS_ENABLED;

    InsertMenuItemW(hMenu, uMenuIndex, TRUE, &mii);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case NOMORE_WM_SHELLICON:
        {
            auto l = LOWORD(lParam);
            if (l == WM_RBUTTONDOWN ||
                l == WM_LBUTTONDOWN)
            {
                SetForegroundWindow(hWnd);

                POINT p;
                GetCursorPos(&p);
                TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, p.x, p.y, 0, hWnd, NULL);

                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case NOMORE_MENU_ABOUT:
                    ShellExecuteW(0, 0, L"https://github.com/RyuaNerin/NoMore", 0, 0, SW_SHOW);
                    break;

                case NOMORE_MENU_EXIT:
                    Shell_NotifyIconW(NIM_DELETE, &nid);
                    DestroyWindow(hWnd);
                    break;
            }
            break;
        }
    }

    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

bool getHttp(LPCWSTR host, LPCWSTR path, std::string &body)
{
    bool res = false;

    BOOL      bResults = FALSE;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    DWORD dwStatusCode;
    DWORD dwSize;
    DWORD dwRead;

    hSession = WinHttpOpen(NOMORE_CLASSNAME, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession)
        bResults = WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);
    if (bResults)
        hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest)
        bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL);
    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (bResults)
    {
        dwSize = sizeof(dwStatusCode);
        bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
    }
    if (bResults)
        bResults = dwStatusCode == 200;
    if (bResults)
    {
        size_t dwOffset;
        do
        {
            dwSize = 0;
            bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (!bResults || dwSize == 0)
                break;

            while (dwSize > 0)
            {
                dwOffset = body.size();
                body.resize(dwOffset + dwSize);

                bResults = WinHttpReadData(hRequest, &body[dwOffset], dwSize, &dwRead);
                if (!bResults || dwRead == 0)
                    break;

                body.resize(dwOffset + dwRead);

                dwSize -= dwRead;
            }
        } while (true);

        res = true;
    }
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return res;
}

bool checkLatestRelease()
{
    std::string body;
    if (getHttp(L"api.github.com", L"/repos/RyuaNerin/NoMore/releases/latest", body))
    {
        Json::CharReaderBuilder                 jo_r_builder;
        const std::unique_ptr<Json::CharReader> jo_r(jo_r_builder.newCharReader());
        Json::Value                             jo;

        if (jo_r->parse(body.c_str(), body.c_str() + body.size(), &jo, nullptr))
        {
            std::string tag_name = jo["tag_name"].asString();
            return (tag_name.compare(NOMORE_VERSION_STR) == 0);
        }
    }

    return false;
}
