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
       HOT_ARM = 201 };

static HWND app, delay_edit, direction_combo, button_combo, status_label;
static HFONT font;
static HANDLE timer;
static bool timer_running, precision_requested;
static bool hot_arm;
static WheelState state;
static uint32_t selected_buttons = 7u;
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

static uint32_t pressed_buttons(void) {
    uint32_t buttons = 0;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) buttons |= 1u;
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) buttons |= 2u;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) buttons |= 4u;
    return buttons & selected_buttons;
}

static void arm(void) {
    wchar_t text[16], *end = NULL;
    HWND target;
    unsigned long interval;
    int selection;
    target = root_window(GetForegroundWindow());
    if (!target || target == app || !IsWindow(target) || !IsWindowVisible(target)) {
        stop(L"Chuyển sang cửa sổ cần cuộn, rồi nhấn Home tại đó.");
        return;
    }
    GetWindowTextW(delay_edit, text, (int)(sizeof(text) / sizeof(text[0])));
    interval = wcstoul(text, &end, 10);
    if (!text[0] || !end || *end || interval < 1 || interval > 1000) {
        stop(L"Độ trễ phải là số nguyên từ 1 đến 1000 ms.");
        return;
    }
    selection = (int)SendMessageW(button_combo, CB_GETCURSEL, 0, 0);
    selected_buttons = selection == 1 ? 1u : selection == 2 ? 2u : selection == 3 ? 4u : 7u;
    selection = (int)SendMessageW(direction_combo, CB_GETCURSEL, 0, 0);
    wheel_delta = selection == 1 ? WHEEL_DELTA : -WHEEL_DELTA;
    stop_timer();
    if (!wheel_arm(&state, (uintptr_t)target, (unsigned)interval)) {
        stop(L"Không bật được. Kiểm tra cửa sổ và độ trễ.");
        return;
    }
    pulse_count = 0;
    last_status = 0;
    if (start_timer()) {
        SetWindowTextW(status_label,
            L"Đã kích hoạt. Thả nút nếu đang giữ; bấm chuột giữa/X1/X2 để bật/tắt.");
    }
}

static void tick(void) {
    HWND foreground;
    bool cursor_matches;
    uint32_t buttons;
    ULONGLONG timestamp;
    if (!state.armed) return;
    foreground = root_window(GetForegroundWindow());
    if (!IsWindow((HWND)state.target) || foreground != (HWND)state.target) {
        stop(L"Đã dừng vì đổi/đóng cửa sổ. Nhấn Home ở cửa sổ đích để kích hoạt lại.");
        return;
    }
    cursor_matches = cursor_over_target((HWND)state.target);
    buttons = pressed_buttons();
    if (wheel_tick(&state, now_us(), true, cursor_matches, buttons)) {
        INPUT input = {0};
        uint32_t latest_buttons;
        /* Recheck immediately before sending: input goes to the active desktop. */
        if (root_window(GetForegroundWindow()) != (HWND)state.target) {
            stop(L"Đã dừng vì đổi cửa sổ.");
            return;
        }
        cursor_matches = cursor_over_target((HWND)state.target);
        latest_buttons = pressed_buttons();
        if (!cursor_matches || latest_buttons != buttons) {
            (void)wheel_tick(&state, now_us(), true, cursor_matches, latest_buttons);
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
        const wchar_t *mode = !cursor_matches ? L"Chuột ở ngoài cửa sổ: đã ngừng cuộn"
            : state.waiting_release ? L"Thả các nút chuột, rồi bấm để bắt đầu"
            : state.scrolling ? L"Đang cuộn — bấm nút chuột để dừng"
            : L"Đã kích hoạt — bấm nút chuột để bắt đầu";
        swprintf(status, sizeof(status) / sizeof(status[0]),
            L"%ls\r\nĐã gửi: %llu nấc cuộn",
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
        control(L"STATIC", L"AUTO CUỘN CHUỘT 1.1 — HOME + CHUỘT", 0, 20, 14, 590, 26, 0);
        control(L"STATIC",
            L"1. Trong game, gán thao tác nhảy cho cuộn xuống hoặc cuộn lên.\r\n"
            L"2. Chọn hướng, độ trễ và nút bật/tắt ở đây.\r\n"
            L"3. Chuyển vào game, nhấn Home để kích hoạt (chưa cuộn).\r\n"
            L"4. Bấm chuột giữa/X1/X2: bắt đầu; bấm lần nữa: dừng.",
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
        control(L"STATIC", L"Nút bật/tắt:", 0, 20, 247, 130, 24, 0);
        button_combo = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP,
            160, 243, 350, 180, ID_BUTTON);
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Chuột giữa hoặc nút hông 1/2 (mặc định)");
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Chỉ chuột giữa");
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Chỉ nút hông 1 (X1)");
        SendMessageW(button_combo, CB_ADDSTRING, 0, (LPARAM)L"Chỉ nút hông 2 (X2)");
        SendMessageW(button_combo, CB_SETCURSEL, 0, 0);
        control(L"STATIC",
            L"Chỉ dùng Home trên bàn phím; bấm chuột để bật/tắt, không cần giữ.\r\n"
            L"Đổi cửa sổ sẽ tắt. Chuột ra ngoài sẽ ngừng, cần bấm lại để chạy.\r\n"
            L"Không chặn nút chuột vật lý. Đóng cửa sổ ứng dụng để thoát.",
            0, 20, 292, 590, 74, 0);
        status_label = control(L"STATIC", L"Chưa kích hoạt. Nhấn Home khi ở cửa sổ đích.",
            SS_LEFT, 20, 377, 590, 54, 0);
        control(L"BUTTON", L"Hủy kích hoạt", WS_TABSTOP, 306, 442, 145, 34, ID_STOP);
        control(L"BUTTON", L"Thoát", WS_TABSTOP, 465, 442, 145, 34, ID_EXIT);
        if (!font || !delay_edit || !direction_combo || !button_combo || !status_label)
            return -1;
        return 0;
    case WM_HOTKEY:
        if (wp == HOT_ARM) arm();
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
    app = CreateWindowExW(0, wc.lpszClassName, L"AutoWheel 1.1 — Home + Chuột",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left,
        rectangle.bottom - rectangle.top, NULL, NULL, instance, NULL);
    if (!app) { CloseHandle(timer); if (font) DeleteObject(font); return 1; }
    hot_arm = RegisterHotKey(app, HOT_ARM, MOD_NOREPEAT, VK_HOME) != 0;
    if (!hot_arm) {
        MessageBoxW(app,
            L"Không đăng ký được phím Home. Có thể ứng dụng khác đang dùng.\r\n"
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
