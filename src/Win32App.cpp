#include "Win32App.h"
#include <windowsx.h>
#include <string>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

enum ControlId {
    ID_COMBO_RENDER = 101,
    ID_COMBO_CAPTURE,
    ID_BTN_START,
    ID_BTN_STOP,
    ID_BTN_CALIB,
    ID_BTN_MEASURE,
    ID_SLIDER_INTENSITY,
    ID_SLIDER_CUTOFF,
    ID_CHK_HP,
    ID_CHK_COMP
};

static Win32App* g_App = nullptr;

int Win32App::Run(HINSTANCE hInstance, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = Win32App::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OpenANC_MainWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(
        0, L"OpenANC_MainWindowClass", L"OpenANC - Real-Time Active Noise Cancellation",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 580, 500,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void Win32App::CreateControls(HWND hWnd) {
    hWnd_ = hWnd;
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    auto CreateLabel = [&](const wchar_t* text, int x, int y, int w, int h) {
        HWND lbl = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, hWnd, nullptr, nullptr, nullptr);
        SendMessage(lbl, WM_SETFONT, (WPARAM)hFont, TRUE);
        return lbl;
    };

    CreateLabel(L"Output / Headphone Device:", 20, 15, 240, 18);
    hComboRender_ = CreateWindowW(L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 35, 520, 200, hWnd, (HMENU)ID_COMBO_RENDER, nullptr, nullptr);
    SendMessage(hComboRender_, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateLabel(L"Microphone (Reference Noise Capture):", 20, 75, 260, 18);
    hComboCapture_ = CreateWindowW(L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 20, 95, 520, 200, hWnd, (HMENU)ID_COMBO_CAPTURE, nullptr, nullptr);
    SendMessage(hComboCapture_, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Buttons
    hBtnStart_ = CreateWindowW(L"BUTTON", L"Start ANC", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 140, 115, 32, hWnd, (HMENU)ID_BTN_START, nullptr, nullptr);
    SendMessage(hBtnStart_, WM_SETFONT, (WPARAM)hFont, TRUE);

    hBtnStop_ = CreateWindowW(L"BUTTON", L"Stop ANC", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED, 145, 140, 115, 32, hWnd, (HMENU)ID_BTN_STOP, nullptr, nullptr);
    SendMessage(hBtnStop_, WM_SETFONT, (WPARAM)hFont, TRUE);

    hBtnCalib_ = CreateWindowW(L"BUTTON", L"Calibrate Latency", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 270, 140, 130, 32, hWnd, (HMENU)ID_BTN_CALIB, nullptr, nullptr);
    SendMessage(hBtnCalib_, WM_SETFONT, (WPARAM)hFont, TRUE);

    hBtnMeasure_ = CreateWindowW(L"BUTTON", L"Measure Response", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 410, 140, 130, 32, hWnd, (HMENU)ID_BTN_MEASURE, nullptr, nullptr);
    SendMessage(hBtnMeasure_, WM_SETFONT, (WPARAM)hFont, TRUE);

    // ANC Intensity
    CreateLabel(L"ANC Anti-Noise Intensity:", 20, 195, 200, 18);
    hLblIntensityVal_ = CreateLabel(L"100%", 230, 195, 60, 18);
    hSliderIntensity_ = CreateWindowW(TRACKBAR_CLASSW, nullptr, WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS, 20, 215, 520, 30, hWnd, (HMENU)ID_SLIDER_INTENSITY, nullptr, nullptr);
    SendMessage(hSliderIntensity_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(hSliderIntensity_, TBM_SETPOS, TRUE, 100);

    // High pass filter
    hChkHP_ = CreateWindowW(L"BUTTON", L"Enable High-Pass Pre-Filter", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 260, 220, 20, hWnd, (HMENU)ID_CHK_HP, nullptr, nullptr);
    SendMessage(hChkHP_, WM_SETFONT, (WPARAM)hFont, TRUE);
    Button_SetCheck(hChkHP_, BST_CHECKED);

    CreateLabel(L"Cutoff Frequency:", 20, 290, 140, 18);
    hLblCutoffVal_ = CreateLabel(L"200 Hz", 170, 290, 80, 18);
    hSliderCutoff_ = CreateWindowW(TRACKBAR_CLASSW, nullptr, WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS, 20, 310, 520, 30, hWnd, (HMENU)ID_SLIDER_CUTOFF, nullptr, nullptr);
    SendMessage(hSliderCutoff_, TBM_SETRANGE, TRUE, MAKELPARAM(20, 600));
    SendMessage(hSliderCutoff_, TBM_SETPOS, TRUE, 200);

    // Speaker compensation
    hChkComp_ = CreateWindowW(L"BUTTON", L"Enable Speaker Acoustic Response Compensation (FIR)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 355, 400, 20, hWnd, (HMENU)ID_CHK_COMP, nullptr, nullptr);
    SendMessage(hChkComp_, WM_SETFONT, (WPARAM)hFont, TRUE);
    Button_SetCheck(hChkComp_, BST_CHECKED);

    // Status
    CreateLabel(L"Status:", 20, 395, 60, 18);
    hLblStatus_ = CreateLabel(L"Ready. Select devices and click Start ANC.", 80, 395, 460, 40);

    PopulateDeviceLists();
}

void Win32App::PopulateDeviceLists() {
    deviceManager_.Init();
    renderDevices_ = deviceManager_.GetRenderDevices();
    captureDevices_ = deviceManager_.GetCaptureDevices();

    SendMessage(hComboRender_, CB_RESETCONTENT, 0, 0);
    for (const auto& dev : renderDevices_) {
        std::wstring label = (dev.isBluetooth ? L"[BT] " : L"[WIRED] ") + dev.name;
        SendMessage(hComboRender_, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    if (!renderDevices_.empty()) {
        SendMessage(hComboRender_, CB_SETCURSEL, 0, 0);
    }

    SendMessage(hComboCapture_, CB_RESETCONTENT, 0, 0);
    for (const auto& dev : captureDevices_) {
        std::wstring label = (dev.isBluetooth ? L"[BT] " : L"[WIRED] ") + dev.name;
        SendMessage(hComboCapture_, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    if (!captureDevices_.empty()) {
        SendMessage(hComboCapture_, CB_SETCURSEL, 0, 0);
    }
}

void Win32App::UpdateStatus(const std::wstring& status) {
    SetWindowTextW(hLblStatus_, status.c_str());
}

LRESULT CALLBACK Win32App::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        g_App = new Win32App();
        g_App->CreateControls(hWnd);
        return 0;
    }

    if (!g_App) return DefWindowProcW(hWnd, msg, wParam, lParam);

    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == ID_BTN_START) {
            int rIdx = (int)SendMessage(g_App->hComboRender_, CB_GETCURSEL, 0, 0);
            int cIdx = (int)SendMessage(g_App->hComboCapture_, CB_GETCURSEL, 0, 0);

            std::wstring rId = (rIdx >= 0 && rIdx < (int)g_App->renderDevices_.size()) ? g_App->renderDevices_[rIdx].id : L"";
            std::wstring cId = (cIdx >= 0 && cIdx < (int)g_App->captureDevices_.size()) ? g_App->captureDevices_[cIdx].id : L"";

            g_App->audioEngine_.SetPreferredDeviceIds(rId, cId);
            HRESULT hr = g_App->audioEngine_.Init();
            if (FAILED(hr)) {
                g_App->UpdateStatus(L"AudioEngine Init failed. Code: " + std::to_wstring(hr));
                return 0;
            }

            hr = g_App->audioEngine_.Start();
            if (FAILED(hr)) {
                g_App->UpdateStatus(L"AudioEngine Start failed. Code: " + std::to_wstring(hr));
                return 0;
            }

            EnableWindow(g_App->hBtnStart_, FALSE);
            EnableWindow(g_App->hBtnStop_, TRUE);
            g_App->UpdateStatus(L"Active Noise Cancellation ACTIVE. Processing real-time anti-noise.");
        }
        else if (id == ID_BTN_STOP) {
            g_App->audioEngine_.Stop();
            EnableWindow(g_App->hBtnStart_, TRUE);
            EnableWindow(g_App->hBtnStop_, FALSE);
            g_App->UpdateStatus(L"ANC Stopped. Normal audio pass-through.");
        }
        else if (id == ID_BTN_CALIB) {
            g_App->UpdateStatus(L"Calibrating round-trip acoustic latency...");
            HRESULT hr = g_App->audioEngine_.CalibrateLatency();
            if (SUCCEEDED(hr)) {
                size_t samples = g_App->audioEngine_.GetLatencySamples();
                g_App->UpdateStatus(L"Latency calibrated: " + std::to_wstring(samples) + L" samples delay compensated.");
            } else {
                g_App->UpdateStatus(L"Latency calibration encountered error.");
            }
        }
        else if (id == ID_BTN_MEASURE) {
            g_App->UpdateStatus(L"Measuring speaker-to-ear acoustic impulse response...");
            HRESULT hr = g_App->audioEngine_.MeasureAcousticResponse();
            if (SUCCEEDED(hr)) {
                g_App->UpdateStatus(L"Acoustic measurement complete. FIR profile updated and saved.");
            } else {
                g_App->UpdateStatus(L"Acoustic measurement failed.");
            }
        }
        else if (id == ID_CHK_HP) {
            BOOL checked = Button_GetCheck(g_App->hChkHP_) == BST_CHECKED;
            g_App->audioEngine_.SetHighPassEnabled(checked);
        }
        else if (id == ID_CHK_COMP) {
            BOOL checked = Button_GetCheck(g_App->hChkComp_) == BST_CHECKED;
            g_App->audioEngine_.EnableSpeakerCompensation(checked);
        }
        break;
    }

    case WM_HSCROLL: {
        HWND slider = (HWND)lParam;
        if (slider == g_App->hSliderIntensity_) {
            int pos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
            g_App->audioEngine_.SetIntensity(static_cast<float>(pos) / 100.0f);
            SetWindowTextW(g_App->hLblIntensityVal_, (std::to_wstring(pos) + L"%").c_str());
        } else if (slider == g_App->hSliderCutoff_) {
            int pos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
            g_App->audioEngine_.SetHighPassCutoff(static_cast<float>(pos));
            SetWindowTextW(g_App->hLblCutoffVal_, (std::to_wstring(pos) + L" Hz").c_str());
        }
        break;
    }

    case WM_DESTROY:
        if (g_App) {
            g_App->audioEngine_.Stop();
            delete g_App;
            g_App = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

