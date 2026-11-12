#ifndef UNICODE
#define UNICODE 1
#endif
#ifndef _UNICODE
#define _UNICODE 1
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <mmsystem.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include "wheel_logic.h"

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

enum { ID_DELAY = 101, ID_DIRECTION, ID_BUTTON, ID_STOP, ID_EXIT,
       HOT_ARM = 201, HOT_STOP, HOT_EXIT };

static HWND app, delay_edit, direction_combo, button_combo, status_label;
static HFONT font;
static HANDLE timer;
static bool timer_running, precision_requested;
static bool hot_arm, hot_stop, hot_exit;
static WheelState state;
static int hold_key = VK_MBUTTON;
static LONG wheel_delta = -WHEEL_DELTA;
static uint64_t pulse_count;
static ULONGLONG last_status;
static LARGE_INTEGER clock_frequency;

static uint64_t now_us(void) {
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return (uint64_t)(value.QuadPart / clock_frequency.QuadPart) * 1000000u
        + (uint64_t)(value.QuadPart % clock_frequency.QuadPart) * 1000000u
            / (uint64_t)clock_frequency.QuadPart;
}

static void stop_timer(void) {
    if (timer_running) {
        CancelWaitableTimer(timer);
        timer_running = false;
    }
    if (precision_requested) {
        timeEndPeriod(1);
        precision_requested = false;
    }
}

static void stop(const wchar_t *message) {
    wheel_stop(&state);
    stop_timer();
    SetWindowTextW(status_label, message);
}

static bool start_timer(void) {
    LARGE_INTEGER due;
    due.QuadPart = -10000; /* First check after 1 ms, relative time. */
    precision_requested = (timeBeginPeriod(1) == TIMERR_NOERROR);
    if (!SetWaitableTimer(timer, &due, 1, NULL, NULL, FALSE)) {
        stop(L"Không tạo được bộ định thời. Đã dừng.");
        return false;
    }
    timer_running = true;
    return true;
}

static HWND root_window(HWND window) {
    return window ? GetAncestor(window, GA_ROOT) : NULL;
}

static bool cursor_over_target(HWND target) {
    POINT point;
    return GetCursorPos(&point) && root_window(WindowFromPoint(point)) == target;
}

static void arm(void) {
    wchar_t text[16], *end = NULL;
    HWND target;
    unsigned long interval;
    int selection;
    if (state.armed) {
        stop(L"Đã tắt bằng F6. Không gửi thao tác.");
        return;
    }
    target = root_window(GetForegroundWindow());
    if (!target || target == app || !IsWindow(target) || !IsWindowVisible(target)) {
        stop(L"Chuyển sang cửa sổ cần cuộn, rồi nhấn F6 tại đó.");
        return;
    }
    GetWindowTextW(delay_edit, text, (int)(sizeof(text) / sizeof(text[0])));
    interval = wcstoul(text, &end, 10);
    if (!text[0] || !end || *end || interval < 1 || interval > 1000) {
        stop(L"Độ trễ phải là số nguyên từ 1 đến 1000 ms.");
        return;
    }
    selection = (int)SendMessageW(button_combo, CB_GETCURSEL, 0, 0);
    hold_key = selection == 1 ? VK_XBUTTON1 : selection == 2 ? VK_XBUTTON2 : VK_MBUTTON;
    selection = (int)SendMessageW(direction_combo, CB_GETCURSEL, 0, 0);
    wheel_delta = selection == 1 ? WHEEL_DELTA : -WHEEL_DELTA;
    if (!wheel_arm(&state, (uintptr_t)target, (unsigned)interval)) {
        stop(L"Không bật được. Kiểm tra cửa sổ và độ trễ.");
        return;
    }
    pulse_count = 0;
    last_status = 0;
    if (start_timer()) {
        SetWindowTextW(status_label,
            L"Đã bật. Thả rồi giữ nút chuột đã chọn để cuộn. F8: dừng.");
    }
}

static void tick(void) {
    HWND foreground;
    bool held, cursor_matches;
    ULONGLONG timestamp;
    if (!state.armed) return;
    foreground = root_window(GetForegroundWindow());
    if (!IsWindow((HWND)state.target) || foreground != (HWND)state.target) {
        stop(L"Đã dừng vì đổi/đóng cửa sổ. Nhấn F6 ở cửa sổ đích để bật lại.");
        return;
    }
    cursor_matches = cursor_over_target((HWND)state.target);
    held = (GetAsyncKeyState(hold_key) & 0x8000) != 0;
    if (wheel_tick(&state, now_us(), true, cursor_matches, held)) {
        INPUT input = {0};
        /* Recheck immediately before sending: input goes to the active desktop. */
        if (root_window(GetForegroundWindow()) != (HWND)state.target) {
            stop(L"Đã dừng vì đổi cửa sổ.");
            return;
        }
        cursor_matches = cursor_over_target((HWND)state.target);
        held = (GetAsyncKeyState(hold_key) & 0x8000) != 0;
        if (!cursor_matches || !held) {
            (void)wheel_tick(&state, now_us(), true, cursor_matches, held);
            return;
        }
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = (DWORD)wheel_delta;
        if (SendInput(1, &input, sizeof(input)) != 1) {
            stop(L"Windows không nhận SendInput. Đã dừng, không tự thử lại.");
            return;
        }
        ++pulse_count;
    }
    timestamp = GetTickCount64();
    if (timestamp - last_status >= 250) {
        wchar_t status[256];
        const wchar_t *mode = !cursor_matches ? L"Chuột ở ngoài cửa sổ: tạm dừng"
            : state.waiting_release ? L"Thả nút chuột trước khi giữ để cuộn"
            : held ? L"Đang gửi cuộn" : L"Đang chờ giữ nút chuột";
        swprintf(status, sizeof(status) / sizeof(status[0]),
            L"%ls | Đã gửi: %llu | F8: dừng",
            mode, (unsigned long long)pulse_count);
        SetWindowTextW(status_label, status);
        last_status = timestamp;
    }
}

static HWND control(const wchar_t *kind, const wchar_t *label, DWORD style,
                    int x, int y, int w, int h, int id) {
    HWND child = CreateWindowExW(!wcscmp(kind, L"EDIT") ? WS_EX_CLIENTEDGE : 0,
        kind, label, WS_CHILD | WS_VISIBLE | style, x, y, w, h, app,
        (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    if (child) SendMessageW(child, WM_SETFONT, (WPARAM)font, TRUE);
    return child;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CREATE:
        app = hwnd;
        font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        control(L"STATIC", L"AUTO CUỘN CHUỘT", 0, 20, 14, 500, 26, 0);
        control(L"STATIC",
            L"1. Trong game, gán thao tác nhảy cho cuộn xuống hoặc cuộn lên.\r\n"
            L"2. Chọn hướng, độ trễ và nút giữ ở đây.\r\n"
            L"3. Chuyển sang cửa sổ cần cuộn, nhấn F6 để bật.\r\n"
            L"4. Thả rồi giữ nút chuột đã chọn để cuộn; thả để ngừng.",
            0, 20, 48, 590, 98, 0);
        control(L"STATIC", L"Độ trễ (ms):", 0, 20, 161, 130, 24, 0);
        delay_edit = control(L"EDIT", L"1", ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP,
            160, 157, 80, 27, ID_DELAY);
        SendMessageW(delay_edit, EM_SETLIMITTEXT, 4, 0);
        control(L"STATIC", L"1–1000 ms; nhịp thực tế phụ thuộc Windows.",
            0, 255, 161, 360, 26, 0);
        control(L"STATIC", L"Hướng cuộn:", 0, 20, 204, 130, 24, 0);
        direction_combo = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP,
            160, 200, 250, 140, ID_DIRECTION);
        SendMessageW(direction_combo, CB_ADDSTRING, 0, (LPARAM)L"Cuộn xuống");
        SendMessageW(direction_combo, CB_ADDSTRING, 0, (LPARAM)L"Cuộn lên");
        SendMessageW(direction_combo, CB_SETCURSEL, 0, 0);
        control(L"STATIC", L"Giữ để cuộn:", 0, 20, 247, 130, 24, 0);
        button_combo = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP,
            160, 243, 250, 160, ID_BUTTON);
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Chuột giữa");
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Nút hông 1 (X1)");
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Nút hông 2 (X2)");
        SendMessageW(button_combo, CB_SETCURSEL, 0, 0);
        control(L"STATIC",
            L"F6: bật/tắt tại cửa sổ đích    •    F8: dừng    •    F10: thoát\r\n"
            L"Đổi cửa sổ sẽ tắt. Đưa chuột ra ngoài sẽ tạm dừng; cần thả nút.\r\n"
            L"Một nấc cuộn/lần; không chặn thao tác chuột vật lý của bạn.",
            0, 20, 292, 590, 74, 0);
        status_label = control(L"STATIC", L"Đã tắt. Không gửi thao tác.",
            SS_LEFT, 20, 377, 590, 54, 0);
        control(L"BUTTON", L"Dừng (F8)", WS_TABSTOP, 306, 442, 145, 34, ID_STOP);
        control(L"BUTTON", L"Thoát (F10)", WS_TABSTOP, 465, 442, 145, 34, ID_EXIT);
        if (!font || !delay_edit || !direction_combo || !button_combo || !status_label)
            return -1;
        return 0;
    case WM_HOTKEY:
        if (wp == HOT_ARM) arm();
        else if (wp == HOT_STOP) stop(L"Đã dừng bằng F8. Không gửi thao tác.");
        else if (wp == HOT_EXIT) DestroyWindow(hwnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == ID_STOP) stop(L"Đã dừng. Không gửi thao tác.");
        else if (LOWORD(wp) == ID_EXIT) DestroyWindow(hwnd);
        return 0;
    case WM_POWERBROADCAST:
        if (wp == PBT_APMSUSPEND) stop(L"Đã dừng do máy chuyển sang chế độ ngủ.");
        return TRUE;
    case WM_QUERYENDSESSION:
        stop(L"Đã dừng.");
        return TRUE;
    case WM_DESTROY:
        wheel_stop(&state);
        stop_timer();
        if (hot_arm) UnregisterHotKey(hwnd, HOT_ARM);
        if (hot_stop) UnregisterHotKey(hwnd, HOT_STOP);
        if (hot_exit) UnregisterHotKey(hwnd, HOT_EXIT);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous,
                    PWSTR command_line, int show) {
    WNDCLASSEXW wc = {0};
    RECT rectangle = {0, 0, 630, 495};
    MSG message;
    bool running = true;
    int result = 0;
    (void)previous; (void)command_line;
    SetProcessDPIAware();
    if (!QueryPerformanceFrequency(&clock_frequency) || clock_frequency.QuadPart <= 0)
        return 1;
    timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
        TIMER_MODIFY_STATE | SYNCHRONIZE);
    if (!timer) timer = CreateWaitableTimerW(NULL, FALSE, NULL);
    if (!timer) {
        MessageBoxW(NULL, L"Không tạo được bộ định thời.", L"Auto cuộn chuột", MB_ICONERROR);
        return 1;
    }
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"AutoWheelWindow";
    if (!RegisterClassExW(&wc)) { CloseHandle(timer); return 1; }
    AdjustWindowRectEx(&rectangle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        FALSE, 0);
    app = CreateWindowExW(0, wc.lpszClassName, L"Auto cuộn chuột — SendInput",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left,
        rectangle.bottom - rectangle.top, NULL, NULL, instance, NULL);
    if (!app) { CloseHandle(timer); if (font) DeleteObject(font); return 1; }
    hot_arm = RegisterHotKey(app, HOT_ARM, MOD_NOREPEAT, VK_F6) != 0;
    hot_stop = RegisterHotKey(app, HOT_STOP, MOD_NOREPEAT, VK_F8) != 0;
    hot_exit = RegisterHotKey(app, HOT_EXIT, MOD_NOREPEAT, VK_F10) != 0;
    if (!hot_arm || !hot_stop || !hot_exit) {
        MessageBoxW(app,
            L"Không đăng ký đủ phím F6/F8/F10. Có thể ứng dụng khác đang dùng.\r\n"
            L"Ứng dụng sẽ đóng và không gửi thao tác.", L"Trùng phím tắt", MB_ICONERROR);
        DestroyWindow(app);
        CloseHandle(timer);
        DeleteObject(font);
        return 1;
    }
    ShowWindow(app, show);
    UpdateWindow(app);
    while (running) {
        DWORD count = timer_running ? 1 : 0;
        DWORD wait = MsgWaitForMultipleObjectsEx(count, &timer, INFINITE,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait == WAIT_FAILED) { result = 1; break; }
        /* Drain hotkeys first, so a queued stop takes priority over a pulse. */
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) { running = false; break; }
            if (!IsDialogMessageW(app, &message)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        if (running && count && wait == WAIT_OBJECT_0 && timer_running) tick();
    }
    if (IsWindow(app)) DestroyWindow(app);
    CloseHandle(timer);
    if (font) DeleteObject(font);
    return result;
}
